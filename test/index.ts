import { NotifyIcon, Icon, Menu } from "../";

class TestError extends Error {
    name = 'TestError';
}

process.on('uncaughtException', (error) => {
    console.error("Uncaught exception:\n%O", error);
});

catchErrors(() => {
    const guid = "ded73175-c489-4f7e-acdc-3dbdde784468";
    const guid2 = "ded73175-c489-4f7e-acdc-3dbdde784469";

    const icon = Icon.load(`${__dirname}/stop.ico`, Icon.small);
    const altIcon = Icon.load(Icon.ids.warning, Icon.small);
    const notificationIcon = Icon.load(`${__dirname}/lightbulb.ico`, Icon.large);

    let count = 0;

    const contextMenu = new Menu([
        { id: 123, text: "Checkable", checked: true },
        { separator: true },
        { id: 103, text: "Time", disabled: true },
        { id: 124, text: "Counter" },
        { text: "Notification" },
        {
            text: "Submenu", items: [
                { id: 456, text: "Use warning" },
                { id: 789, text: "Subitem Notification" },
            ]
        },

        { separator: true },
        { id: 2, text: "Throw error" },
        { id: 3, text: "Add icon" },
        { id: 4, text: "Remove icon" },
        { separator: true },
        { id: 1, text: "Quit" },
    ]);
    console.log("menu item at 0 details %O", contextMenu.getAt(0));
    console.log("menu item id 123 details %O", contextMenu.get(123));
    console.log("menu item at 1 details %O", contextMenu.getAt(1));
    // Can't support this one yet.
    // console.log("menu item at 3,1 details %O", contextMenu.getAt(3, 1));
    console.log("menu item id 456 details %O", contextMenu.get(456));

    // new NotifyIcon(guid2, {
    //     icon: Icon.load(Icon.ids.error, Icon.small),
    //     tooltip: "Will get garbage collected",
    // });

    const onSelect = catchErrors(function (this: NotifyIcon, event: NotifyIcon.SelectEvent) {
        console.log("tray icon selected %O %O", this, event);

        const itemId = contextMenu.showSync(event.mouseX, event.mouseY);
        console.log("menu item selected %O", itemId);
        if (!itemId) {
            return;
        }
        const item = contextMenu.get(itemId);
        console.log("menu item details %O", item);

        switch (itemId) {
            case 1:
                process.exit(0);
                return;
            case 2:
                throw new TestError("Should bubble out to uncaughtException listener.");
            case 3:
                new NotifyIcon({
                    icon: Icon.load(Icon.ids.info, Icon.small),
                    tooltip: "Dynamically added",
                    onSelect,
                });
                return;
            case 4:
                this.remove();
                return;

            case 123:
                contextMenu.updateAt(0, {
                    checked: !item.checked,
                });
                return;
            case 124:
                contextMenu.update(124, {
                    text: `Counter: ${++count}`,
                });
                return;
            case 456:
                const useWarning = !item.checked;
                contextMenu.update(itemId, {
                    checked: useWarning,
                });
                notifyIcon.update({
                    icon: useWarning ? altIcon : icon,
                });
                return;
        }

        notifyIcon.update({
            notification: {
                icon: notificationIcon,
                sound: false,
                title: "Annoying Message",
                text: `You selected: "${item.text}"\nThe time is: ${new Date().toLocaleTimeString()}`
            },
        });
    });

    const notifyIcon = new NotifyIcon({
        guid,
        icon,
        tooltip: "Example Tooltip Text",

        onSelect,
    });

    if (process.stdin.setRawMode)
        process.stdin.setRawMode(true);
    process.stdout.write('Press any key to exit...\n');
    process.stdin.on('data', catchErrors(() => {
        notifyIcon.remove();

        process.exit();
    }));
})();

// Catches and prints the error properties, including the `syscall` and `errno` for `Win32Error`s
function catchErrors<Args extends any[], R, This = void>(body: (this: This, ...args: Args) => R): (this: This, ...args: Args) => (R | undefined) {
    return function (...args) {
        try {
            return body.apply(this, args);
        } catch (e) {
            console.error(e);
            // This will not work if transpile target is less than es2015
            if (e instanceof TestError) {
                throw e;
            }
        }
    };
}
