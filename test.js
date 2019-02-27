const guid = "ded73175-c489-4f7e-acdc-3dbdde784468";
const id = 1;

catchErrors(() => {
    const tray = require('./build/Debug/tray.node');

    tray.add(id, guid, {
        // icon: tray.icons.warning,
        icon: "test-2.ico",
        // notificationIcon: tray.icons.app,
        notificationIcon: "test-1.ico",
        tooltip: "Example Tooltip Text",
        onSelect: catchErrors((itemId) => {
            console.log("selected %O", itemId);

            tray.update(id, {
                notification: {
                    sound: false,
                    title: "Annoying Message",
                    text: `The time is: ${new Date().toLocaleTimeString()}`
                },
            });
        }),
    });

    let hidden = false;
    setInterval(catchErrors(() => {
        hidden = !hidden;
        tray.update(id, {
            hidden,
            // sound: false,
            // balloonTitle: "Annoying Message",
            // balloonText: `The time is: ${new Date().toLocaleTimeString()}`
        });
    }), 5000);

    if (process.stdin.setRawMode)
        process.stdin.setRawMode(true);
    process.stdout.write('Press any key to exit... ');
    process.stdin.on('data', catchErrors(() => {
        process.stdout.write('\n');
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