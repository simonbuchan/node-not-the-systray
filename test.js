const util = require("util");
const guid = "ded73175-c489-4f7e-acdc-3dbdde784468";
const id = 1;

catchErrors(() => {
    const tray = require('.');

    const icon = tray.Icon.load(tray.Icon.ids.warning, "small");
    // const icon = tray.Icon.loadFileSmall("test-2.ico");
    // const icon = tray.Icon.load("test-2.ico", "small");
    // const notificationIcon = tray.Icon.loadBuiltin(tray.icons.app, tray.Icon.largeWidth, tray.Icon.largeHeight);
    const notificationIcon = tray.Icon.loadFileLarge("test-1.ico");

    let count = 0;

    const contextMenu = tray.Menu.create([
        { id: 123, text: "Checkable", checked: true },
        { separator: true },
        { id: 124, text: "Counter" },
        {
            text: "Submenu", items: [
                { id: 456, text: "Subitem 456" },
                { id: 789, text: "Subitem 789" },
            ]
        },
    ]);
    console.log("menu item at 0 details %O", contextMenu.getByIndex(0));
    console.log("menu item id 123 details %O", contextMenu.getById(123));
    console.log("menu item at 1 details %O", contextMenu.getByIndex(1));

    tray.add(id, guid, {
        icon,
        tooltip: "Example Tooltip Text",

        onSelect: catchErrors((event) => {
            console.log("tray icon selected %O", event);
            const { iconId, right, mouseX, mouseY } = event;

            const itemId = contextMenu.show(mouseX, mouseY);
            console.log("menu item selected %O", itemId);
            if (!itemId) {
                return;
            }
            const item = contextMenu.getById(itemId);
            console.log("menu item details %O", item);

            switch (itemId) {
                case 123:
                    contextMenu.update(0, {
                        checked: !item.checked,
                    });
                    return;
                case 124:
                    contextMenu.update(1, {
                        text: `Counter: ${++count}`,
                    });
                    return;
            }

            tray.update(id, {
                notification: {
                    icon: notificationIcon,
                    sound: false,
                    title: "Annoying Message",
                    text: `The time is: ${new Date().toLocaleTimeString()}`
                },
            });
        }),
    });

    if (process.stdin.setRawMode)
        process.stdin.setRawMode(true);
    process.stdout.write('Press any key to exit...\n');
    process.stdin.on('data', catchErrors(() => {
        tray.remove(id);

        process.exit();
    }));
})();

// Shows the error properties, including the syscall and errno for Win32Error
function catchErrors(body) {
    return (...args) => {
        try {
            return body(...args);
        } catch (e) {
            console.error(e);
        }
    };
}