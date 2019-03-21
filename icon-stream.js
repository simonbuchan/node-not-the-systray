let native;
try {
    native = require('./build/Release/notify_icon.node');
} catch (e) {
    native = require('./build/Debug/notify_icon.node');
}

module.exports = { printIconStream };

// For debugging icon registration, see
// https://github.com/electron/electron/issues/2468#issuecomment-142684129
function printIconStream() {
    /** @type Buffer */
    const buffer = native.icon_stream;
    // const headerSize = buffer.readUInt32LE(0);
    // buffer.readUInt32LE(4); = 7
    // buffer.readUInt16LE(8); = 1
    // buffer.readUInt16LE(10); = 1
    const itemCount = buffer.readUInt32LE(12);
    const itemListOffset = buffer.readUInt32LE(16);

    const itemSize = 1640;

    const pathStart = 0;
    const pathEnd = pathStart + 2 * 260;
    const tipStart = pathEnd + 16;
    const tipEnd = tipStart + 2 * 260; // 260 seems to be closer than the NOTIFYITEMDATA size of 128
    const infoStart = tipEnd + 60;
    const infoEnd = infoStart + 2 * 260;

    console.log("header data = %s", buffer.slice(0, itemListOffset).toString("hex"));

    for (let itemIndex = 0; itemIndex < itemCount; itemIndex++) {
        const itemOffset = itemListOffset + itemIndex * itemSize;
        const itemData = buffer.slice(itemOffset, itemOffset + itemSize);
        console.group('Item %d/%d at 0x%s', itemIndex, itemCount, itemOffset.toString(16));

        const [path, pathTrailing] = readUTF16(itemData, pathStart, pathEnd);
        console.log("path = %O", stringRot(path, 13));
        if (pathTrailing.some(b => b != 0)) {
            console.log("  trailing = %s", pathTrailing.toString("hex"));
        }

        console.group("? = %s", itemData.slice(pathEnd, pathEnd + 12).toString("hex"));
        console.log("? = %O", itemData.readInt32LE(pathEnd));
        console.log("? = %O", itemData.readInt32LE(pathEnd + 4));
        console.log("? = %O", itemData.readInt32LE(pathEnd + 8));
        console.groupEnd();

        console.log("last seen = %d-%d", itemData.readUInt16LE(pathEnd + 12), itemData.readUInt16LE(pathEnd + 14));

        const [tip, tipTrailing] = readUTF16(itemData, tipStart, tipEnd);
        console.log("tooltip = %O", stringRot(tip, 13));
        if (tipTrailing.some(b => b != 0)) {
            console.log("  trailing = %s", tipTrailing.toString("hex"));
        }
        console.log("? = %O", itemData.readInt32LE(tipEnd));
        console.log("? = %O", itemData.readInt32LE(tipEnd + 4));
        console.log("order? = %O", itemData.readInt32LE(tipEnd + 8));
        console.log("? = %s", itemData.slice(tipEnd + 12, tipEnd + 40).toString("hex"));
        console.log("? = %s", itemData.slice(tipEnd + 40, infoStart).toString("hex"));

        const [info, infoTrailing] = readUTF16(itemData, infoStart, infoEnd);
        console.log("info? = %O", stringRot(info, 13));
        console.log("? = %s", infoTrailing.slice(0, 4).toString("hex"));
        const [info2, info2Trailing] = readUTF16(infoTrailing, 4);
        console.log("? = %O", stringRot(info2, 13));
        if (info2Trailing.some(b => b != 0)) {
            console.log("  trailing = %s", info2Trailing.toString("hex"));
        }

        console.log("? = %O", itemData.readUInt32LE(infoEnd));
        console.groupEnd();
    }

    console.log("trailer data = %s", buffer.slice(itemListOffset + itemCount * itemSize).toString("hex"));
}

function stringRot(value, rot) {
    const codes = Array(value.length);
    for (let i = 0; i != value.length; i++) {
        const code = value.charCodeAt(i);
        if (65 <= code && code <= 90) {
            codes[i] = (code - 65 + rot) % 26 + 65;
        } else if (97 <= code && code <= 122) {
            codes[i] = (code - 97 + rot) % 26 + 97;
        } else {
            codes[i] = code;
        }
    }
    return String.fromCharCode(...codes);
}

function readUTF16(buffer, offset = 0, bufferEnd = buffer.length) {
    let end = offset;
    while (end < bufferEnd && buffer.readUInt16LE(end)) {
        end += 2;
    }
    const value = buffer.slice(offset, end).toString('utf16le');
    const trailing = buffer.slice(end + 2, bufferEnd);
    return [value, trailing];
}