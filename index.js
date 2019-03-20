let native;
try {
    native = require('./build/Release/tray.node');
} catch (e) {
    native = require('./build/Debug/tray.node');
}

const { NotifyIcon, Icon, Menu } = native;

module.exports = { NotifyIcon, Icon, Menu, printIconStream };

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

Object.defineProperties(Menu, {
    createTemplate: { value: createMenuTemplate, enumerable: true },
});

Object.defineProperties(Icon, {
    ids: {
        enumerable: true,
        value: Object.create(null, {
            app: { value: 32512, enumerable: true },
            error: { value: 32513, enumerable: true },
            question: { value: 32514, enumerable: true },
            warning: { value: 32515, enumerable: true },
            info: { value: 32516, enumerable: true },
            winLogo: { value: 32517, enumerable: true },
            shield: { value: 32518, enumerable: true },
        }),
    },
    load: {
        enumerable: true,
        value: function Icon_load(pathOrId, size) {
            switch (typeof pathOrId) {
                default:
                    throw new Error("'pathOrId' should be either a file path or a property of Icon.ids.");
                case "number":
                    return Icon.loadBuiltin(pathOrId, size);
                case "string":
                    return Icon.loadFile(pathOrId, size);
            }
        },
    },
});

function createMenuTemplate(items) {
    // Generates a MENUEX binary resource structure to be
    // loaded by LoadMenuIndirectW().
    // Docs are pretty garbarge here, I found the wine resource
    // compiler source helpful for some of the edge cases.
    // https://github.com/wine-mirror/wine/blob/master/tools/wrc/genres.c

    // struct MENUEX_TEMPLATE_HEADER {
    //   0 uint16 version = 1;
    //   2 uint16 offset = 4;
    //   4 uint32 helpId = 0;
    // };
    const header = Buffer.alloc(8);
    header.writeUInt16LE(1, 0); // version
    header.writeUInt16LE(4, 2); // offset
    const buffers = [header];

    // Wrap everything in a menu item so the contents are a
    // valid popup menu, otherwise it doesn't display right.
    // No idea why it matters.
    addItem({ text: "root", items }, true);

    return Buffer.concat(buffers);

    function addList(items) {
        if (items.length < 1) {
            addItem({ text: "Empty", disabled: true }, true);
            return;
        }
        const startItems = items.slice();
        const lastItem = startItems.pop();
        for (const item of startItems) {
            addItem(item);
        }
        addItem(lastItem, true);
    }

    function addItem({
        id = 0,
        text = "",
        separator = false,
        disabled = false,
        checked = false,
        items,
    }, isLast = false) {
        // A variable-length structure, that must be aligned to 4-bytes.
        // struct MENUEX_TEMPLATE_ITEM {
        //    0 uint32 type;
        //    4 uint32 state;
        //    8 uint32 id;
        //   12 uint16 flags;
        //   14 utf16[...] text; // '\0' terminated
        //   ?? padding to 4-byte boundary
        //   if (items) {
        //       ?? uint32 helpid;
        //       ?? MENUEX_TEMPLATE_ITEM[...] items;
        //   }
        // }
        let size = 14 + text.length * 2 + 2;
        if (items) {
            size += 4;
        }
        if (size % 4) size += 4 - (size % 4);

        let type = 0;
        if (separator) {
            type |= 0x800;
        }
        let state = 0;
        if (disabled) {
            state |= 3;
        }
        if (checked) {
            state |= 8;
        }
        let flags = 0;
        if (isLast) {
            flags |= 0x80;
        }
        if (items) {
            flags |= 0x01;
        }

        const buffer = Buffer.alloc(size);
        buffer.writeUInt32LE(type, 0);
        buffer.writeUInt32LE(state, 4);
        buffer.writeUInt32LE(id, 8);
        buffer.writeUInt16LE(flags, 12);
        buffer.write(text, 14, 'utf16le');
        buffers.push(buffer);

        if (items) {
            addList(items);
        }
    }
}
