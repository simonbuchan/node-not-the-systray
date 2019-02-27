/** Windows built-in icons. */
declare const icons: {
    /** Generic application icon (representation of a window) */
    readonly app: number;
    /** Generic information icon (blue circle with a white "i") */
    readonly info: number;
    /** Generic warning icon (yellow triangle with a black "!") */
    readonly warning: number;
    /** Generic error icon (red circle with a white "x") */
    readonly error: number;
};

/** Properties for a notification ("toasts" or "balloons" for earlier windows versions) */
interface NotificationOptions {
    /**
     * Discard the notification if it will not be immediately displayed, otherwise
     * display when convienient to the user (e.g. after dismissing the last notification,
     * log-in, leaving a presentation, ...).
     * Default is `false`.
     */
    realtime?: boolean;
    /**
     * Do not display within the first hour of setting up the computer.
     * Default is `true`.
     */
    respectQuietTime?: boolean;
    /**
     * Play a system-defined sound when the notification is displayed.
     * Default is `true`.
     */
    sound?: boolean;
    /**
     * Title of the notification.
     */
    title?: string;
    /**
     * Body text of the notification.
     */
    text?: string;
}

/**
 * Properties for the clickable icon in the notification area (system tray).
 */
interface NotifyIconOptions {
    /**
     * Display icon to use for the clickable area.
     * Can be either a property of `icons` for a built-in icon, or the
     * path to an `.ico` file on disk.
     * If not provided, Windows will simply not draw any icon in the clickable area.
     * Displayed at 16x16 pixels at 100% DPI.
     */
    icon?: number | string;
    /** Tooltip displayed when hovered or keyboard navigated. */
    tooltip?: string;
    /**
     * Hide the icon completely.
     * Note that you cannot show notifications while hidden.
     */
    hidden?: boolean;
    /**
     * Icon displayed in the notification itself.
     * This value is persisted between notifications.
     * If not provided, Windows will re-use the value for `icon`, at the
     * same resolution (scaling up from 16x16).
     * If `largeNotificationIcon` is not provided or `true`, then this is
     * displayed at 32x32 pixels at 100% DPI, otherwise, if `largeNotificationIcon`
     * is explicictly `false`, then this is displayed at 16x16 pixels at 100% DPI.
     */
    notificationIcon?: number | string;

    /**
     * Size to load and display `notificationIcon` at, either the system
     * small icon size (defaults to 16x16) or standard icon size (32x32).
     * Defaults to `true`.
     */
    largeNotificationIcon?: boolean;

    /**
     * Notification to display, or remove the existing notification
     * if provided as `null` or `undefined`.
     */
    notification?: NotificationOptions | null | undefined;

    /**
     * Callback fired when 
     */
    onSelect?: (this: void, menuItemId: number) => void;
}

/**
 * Initial properties for the clickable icon in the notification area (system tray).
 */
interface AddNotifyIconOptions extends NotifyIconOptions {
    /**
     * Automatically delete any previous icon added with the same `guid`.
     * Windows only allows one instance of the icon to be added at a time,
     * and will fail with `Win32Error: Unspecified error.` if it already
     * exists. To make things worse, if the program exits without removing
     * the icon, it remains in the notification area until the user tries
     * to interact with it (e.g. mouse over).
     * Default is `true`.
     */
    replace?: boolean;
}

/**
 * Add a notification icon to the notification area (system tray).
 *
 * `guid` is a unique id used by Windows to preserve user preferences
 * for this icon. As it is a persistent identifier, do not generate values
 * for this at runtime using, e.g. the `uuid` package, generate them once
 * and save it as a constant in your code.
 *
 * Only one icon can use the same `guid` at a time, even between processes.
 *
 * The icon will automatically be removed on normal exit of the process.
 * 
 * @param id 
 *      Application-specific identifier for this icon.
 * @param guid
 *      Persistent identifier for this icon.
 *      Must be either a 16-byte `Buffer` or a string in the standard UUID/GUID format, matching
 *      `/^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$/`,
 *      e.g. `"01234567-89ab-cdef-0123-456789abcdef"`.
 * @param options
 *      Options controlling the creation, display of the icon, and optionally
 *      the notification ("toast" or "balloon").
 */
export function add(id: number, guid: string, options?: AddNotifyIconOptions): void;

/**
 * Update the options for a notification icon, and optionally a notification.
 * To an extent, only the provided options will be updated, and the previous values
 * will be left with their previous values.
 *
 * @param id 
 *      The `id` of the icon to update.
 * @param options 
 *      Options to be updated controlling the display of the icon, and optionally
 *      the notification ("toast" or "balloon").
 */
export function update(id: number, options?: NotifyIconOptions): void;

/**
 * Remove a previously added notification icon and any current notification.
 *
 * @param id 
 *      The `id` of the icon to remove.
 */
export function remove(id: number): void;
