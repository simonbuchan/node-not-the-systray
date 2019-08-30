import { NotifyIcon, Icon, Menu } from "not-the-systray";

class TestError extends Error {
    name = 'TestError';
}

process.on('unhandledRejection', (error) => {
    console.error("Unhandled rejection:\n%O", error);
});

process.on('uncaughtException', (error) => {
    console.error("Uncaught exception:\n%O", error);
});

const notificationId = 125;
const useWarningId = 456;
const subitemNotificationId = 789;

main().catch((error) => {
    console.error(error);
    process.exit(1);
});

async function main() {
    const guid = "ded73175-c489-4f7e-acdc-3dbdde784468";
    const guid2 = "ded73175-c489-4f7e-acdc-3dbdde784469";

    console.log("Large icon size: %o", Icon.large);
    // const icon = Icon.load(`${__dirname}/stop.ico`, Icon.small);
    const icon = Icon.loadResource(Icon.small, undefined, "cmd.exe");
    const altIcon = Icon.load(Icon.ids.warning, Icon.small);
    const notificationIcon = Icon.loadResource(Icon.large, undefined, "cmd.exe");
    // const notificationIcon = Icon.load(`${__dirname}/lightbulb.ico`, Icon.large);

    let count = 0;

    const contextMenu = new Menu([
        { id: 123, text: "Checkable", checked: true },
        { separator: true },
        { id: 103, text: "Time", disabled: true },
        { id: 124, text: "Counter" },
        { id: notificationId, text: "Notification" },
        {
            text: "Submenu", items: [
                { id: useWarningId, text: "Use warning" },
                { id: subitemNotificationId, text: "Subitem Notification" },
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
            case notificationId:
                notifyIcon.update({
                    notification: {
                        icon: notificationIcon,
                        sound: false,
                        title: "Some notification",
                        text: `You selected: "${item.text}"\nThe time is: ${new Date().toLocaleTimeString()}`
                    },
                });
                return;
            case useWarningId:
                toggleIcon(item);
                return;
        }
    });

    const notifyIcon = new NotifyIcon({
        // guid,
        icon,
        tooltip: "Example Tooltip Text",

        onSelect,
    });

    if (process.stdin.setRawMode)
        process.stdin.setRawMode(true);
    help();
    for await (const chunk of process.stdin) {
        switch (chunk.toString()) {
            default:
                help();
                break;
            case 'i':
                toggleIcon(contextMenu.get(useWarningId));
                break;
            case 'q':
                notifyIcon.remove();
                process.exit();
                break;
        }
    }

    function help() {
        process.stdout.write('h: help | i: toggle icon | q: quit\n');
    }

    function toggleIcon(item: Menu.Item) {
        const useWarning = !item.checked;
        contextMenu.update(item.id, {
            checked: useWarning,
        });
        notifyIcon.update({
            icon: useWarning ? altIcon : icon,
        });
    }
}

function catchErrors<Args extends any[], R, This = void>(body: (this: This, ...args: Args) => R) {
    return function (this: This, ...args: Args): R | void {
        try {
            return body.apply(this, args);
        } catch (e) {
            handleError(e);
        }
    };
}

// Catches and prints the error properties, including the `syscall` and `errno` for `Win32Error`s
function catchErrorsAsync<Args extends any[], R, This = void>(body: (this: This, ...args: Args) => Promise<R>) {
    return async function (this: This, ...args: Args): Promise<R | void> {
        try {
            return await body.apply(this, args);
        } catch (e) {
            handleError(e);
        }
    };
}

function handleError(e: any) {
    console.error(e);
    // This will not work if transpile target is less than es2015
    if (e instanceof TestError) {
        throw e;
    }
}
