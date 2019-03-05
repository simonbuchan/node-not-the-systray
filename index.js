let bindings;
try {
    bindings = require('./build/Release/tray.node');
} catch (e) {
    bindings = require('./build/Debug/tray.node');
}

module.exports = bindings;

const { Icon, Menu } = bindings;

Object.defineProperties(Menu, {
    createTemplate: { value: createMenuTemplate, enumerable: true },
    // create: { value: createMenu, enumerable: true },
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
            switch (size) {
                case "small":
                    size = { width: Icon.smallWidth, height: Icon.smallHeight };
                    break;
                case "large":
                    size = { width: Icon.largeWidth, height: Icon.largeHeight };
                    break;
            }
            if (!size || typeof size.width !== "number" || typeof size.height !== "number") {
                throw new Error("'size' should be either 'small', 'large', or { width, height }.");
            }
            switch (typeof pathOrId) {
                default:
                    throw new Error("'pathOrId' should be either a file path or a property of Icon.ids.");
                case "number":
                    return Icon.loadBuiltin(pathOrId, size.width, size.height);
                case "string":
                    return Icon.loadFile(pathOrId, size.width, size.height);
            }
        },
    },
    loadFileSmall: {
        enumerable: true,
        value: function Icon_loadFileSmall(path) {
            const { loadFile, smallWidth, smallHeight } = Icon;
            return loadFile(path, smallWidth, smallHeight);
        },
    },
    loadFileLarge: {
        enumerable: true,
        value: function Icon_loadFileLarge(path) {
            const { loadFile, largeWidth, largeHeight } = Icon;
            return loadFile(path, largeWidth, largeHeight);
        },
    },
});

function createMenu(items) {
    return bindings.Menu.createFromTemplate(createMenuTemplate(items));
}

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
