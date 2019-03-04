const guid = "ded73175-c489-4f7e-acdc-3dbdde784468";
const id = 1;

catchErrors(() => {
    const tray = require('.');

    const icon = tray.Icon.load(tray.Icon.ids.warning, "small");
    // const icon = tray.Icon.loadFileSmall("test-2.ico");
    // const icon = tray.Icon.load("test-2.ico", "small");
    // const notificationIcon = tray.Icon.loadBuiltin(tray.icons.app, tray.Icon.largeWidth, tray.Icon.largeHeight);
    const notificationIcon = tray.Icon.loadFileLarge("test-1.ico");

    const altMenu = tray.createMenu([
        { id: 123, text: "Item 123", checked: false },
        { id: 10, text: "Totally different item!" },
        { separator: true },
        { text: "Empty menu", items: [] },
        {
            text: "Submenu", items: [
                { id: 456, text: "Subitem 456" },
                { id: 789, text: "Subitem 789" },
            ]
        },
    ]);

    tray.add(id, guid, {
        icon,
        tooltip: "Example Tooltip Text",

        contextMenu: tray.createMenu([
            { id: 123, text: "Item 123", checked: true },
            {
                text: "Submenu", items: [
                    { id: 456, text: "Subitem 456" },
                    { id: 789, text: "Subitem 789" },
                ]
            },
        ]),

        onSelect: catchErrors((itemId) => {
            console.log("selected %O", itemId);

            tray.update(id, {
                contextMenu: itemId === 123 ? altMenu : undefined,
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