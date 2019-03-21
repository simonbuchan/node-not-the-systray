# Windows Notification Icons

Because calling it the system tray [would be wrong](https://devblogs.microsoft.com/oldnewthing/20030910-00/?p=42583).

## Description

Provides fairly direct access to the Win32
[`Shell_NotifyIcon`](https://docs.microsoft.com/en-nz/windows/desktop/api/shellapi/nf-shellapi-shell_notifyiconw)
API, which allows adding an icon to the notification area, and
support for the related windows APIs to load icons and use context
(e.g. right-click) menus.

Prebuilt as N-API (64-bit only for now, sorry), meaning you
don't need Visual Studio or build tools installed on your
development machine or CI server.

Note that this is using the Win32 desktop APIs, so you don't
need to be running under the UWP (Windows Store) model,
and thus has odd limitations like requiring an icon in the
notification area to show a notification message, and not
supporting notification actions (e.g. buttons).

## Usage

The [type definitions](index.d.ts) have fuller jsdoc descriptions,
but as a basic overview:

```js
import { NotifyIcon, Icon, Menu } from "not-the-systray";

// Creates an empty (blank) icon in the notification area
// that does nothing.
// Unfortunately the Windows terminology I'm exposing here is
// pretty confusing, they have "Notification icons" that have
// icons and can show notifications that also have icons.
const emptyIcon = new NotifyIcon();
// Remove it!
emptyIcon.remove();

// Creates and adds an initialized icon, with a select callback.
const appIcon = new NotifyIcon({
    icon: Icon.load("my-icon.ico", Icon.small), // Notify icons should use 
    tooltip: "My icon from nodejs",
    onSelect({ target, rightButton, mouseX, mouseY }) {
        // `this` is `target` is `appIcon`.
        // `rightButton` is which mouse button it was selected with,
        // `mouseX and `mouseX` are the screen co-ordinates of the click.
        // If selected with the keyboard, these values will be simulated.

        if (rightButton) {
            handleMenu(mouseX, mouseY);
        } else {
            // some default action, or just show the same menu.
        }
    },
});

// Notifications should use the size `Icon.large`.
// Icons can be loaded ahead of time to catch errors earlier,
// and save a bit of time and memory.
const notificationIcon = Icon.load("notification-icon.ico", Icon.large);
// You can also use some built-in icons, for example:
const errorIcon = Icon.load(Icon.ids.error, Icon.large);

const toggleId = 1;
const notificationId = 2;
const submenuId = 3;

const menu = new Menu([
    { id: toggleId, text: "Toggle item", checked: true },
    { id: notificationId, text: "Show notification" },
    { separator: true },
    { text: "Sub-menu", items: [
        { id: submenuId, text: "Sub-menu item" },
    ] }
]);

function handleMenu(x, y) {
    const id = menu.showSync(x, y);
    switch (id) {
        case null:
            // user cancelled selection.
            break;
        case toggleId: {
            // You can read the immediate properties (ie. not counting `items`)
            // of any item by id.
            const { checked } = menu.get(toggleId);
            // You can update any properties of an item whenever you want, including
            // `items`.
            menu.update(toggleId, { checked: !checked });
            break;
        }
        case notificationId:
            // The win32 (non-UWP) model of notifications are as
            // a property of a notification area icon, and can't
            // be displayed without one (remember the old balloons?)
            // Updating with a new notification will remove the
            // previous one.
            appIcon.update({
                notification: {
                    icon: notificationIcon,
                    title: "Hello, world",
                    text: "A notification from nodejs",
                },
            });
            break;
        case submenuId:
            // Ids work in submenus too.
            console.log("Selected the submenu item");
            break;
    }
}
```
