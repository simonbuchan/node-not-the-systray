export interface Icon {
    readonly width: number;
    readonly height: number;
}

export namespace Icon {
    export interface Size {
        width: number;
        height: number;
    }

    /** Windows built-in icon IDs. */
    export type BuiltinId = 32512 | 32513 | 32514 | 32515 | 32516 | 32516 | 32517 | 32518;
    export const ids: {
        readonly app: BuiltinId;// = 32512;
        readonly error: BuiltinId;// = 32513;
        readonly question: BuiltinId;// = 32514;
        readonly warning: BuiltinId;// = 32515;
        readonly info: BuiltinId;// = 32516;
        readonly winLogo: BuiltinId;// = 32517;
        readonly shield: BuiltinId;// = 32518;
    };

    export const small: Readonly<Size>;
    export const large: Readonly<Size>;

    export function load(pathOrId: string | BuiltinId, size: Readonly<Size>): Icon;

    export function loadResource(size: Readonly<Size>, id?: number, path?: string): Icon;
    /** Native API to load a built-in icon at a specific size. */
    export function loadBuiltin(id: BuiltinId, size: Readonly<Size>): Icon;
    export function loadFile(path: string, size: Readonly<Size>): Icon;
}

export namespace Menu {
    export interface ItemInput {
        readonly id?: number;
        readonly text?: string;
        readonly separator?: boolean;
        readonly disabled?: boolean;
        readonly checked?: boolean;
        readonly items?: ReadonlyArray<ItemInput>;
    }

    export interface Item {
        id: number;
        text: string | null;
        separator: boolean;
        disabled: boolean;
        checked: boolean;
    }
}

export class Menu {
    /**
     * Create a resource template in MENUEX binary format, that
     * can be persisted and used in a `Menu` constructor.
     */
    static createTemplate(items: ReadonlyArray<Menu.Item>): Buffer;

    /**
     * Create a context menu from a template resource.
     * This resource should be in a Windows resource binary format
     * of either MENU or MENUEX type, with a single top-level item
     * containing as sub-items the context menu items (required for
     * internal Windows reasons.)
     * This format is created as the output of `Menu.createTemplate()`;
     * @param template A Buffer containing the resource data.
     */
    constructor(template: Buffer);

    /**
     * Create a context menu from description items.
     * Equivalent to `new Menu(Menu.createTemplate(items))`.
     * @param items A list of menu item descriptions to create.
     */
    constructor(items: ReadonlyArray<Menu.ItemInput>);

    /**
     * Open the menu and return a promise resolved when a selection is made by the user.
     * Should be called in response to `NotifyIcon#onSelect()`, which
     * will give the `mouseX`, `mouseY` values to use for the arguments.
     * If called at other times, you will likely not have a foreground
     * window, which will cause the menu to misbehave, not correctly closing
     * on the first selection.
     * @param x Desktop x coordinate to open menu near.
     * @param y Desktop y coordinate to open menu near.
     * @returns Item id if selected or `null` if the menu was dismissed.
     */
    show(x: number, y: number): Promise<number | null>;

    /**
     * Open the menu and block until a selection is made by the user.
     * Should be called in response to `NotifyIcon#onSelect()`, which
     * will give the `mouseX`, `mouseY` values to use for the arguments.
     * If called at other times, you will likely not have a foreground
     * window, which will cause the menu to misbehave, not correctly closing
     * on the first selection.
     * @param x Desktop x coordinate to open menu near.
     * @param y Desktop y coordinate to open menu near.
     * @returns Item id if selected or `null` if the menu was dismissed.
     */
    showSync(x: number, y: number): number | null;

    /**
     * Return a summary of the menu item by index.
     * Can only select top-level items (currently).
     * @param index Top level index of the item to describe.
     */
    getAt(index: number): Menu.Item;

    /**
     * Return a summary of the menu item by item id.
     * Can select any nested item.
     * @param itemId the `id` property of the item to describe.
     */
    get(itemId: number): Menu.Item;

    /**
     * Update a menu item by index.
     * Can only select top-level items (currently).
     * @param index Top level index of the item to update.
     */
    updateAt(index: number, updates: Menu.ItemInput): void;

    /**
     * Update a menu item by item id.
     * Can select any nested item.
     * @param itemId the `id` property of the item to update.
     */
    update(itemId: number, updates: Menu.ItemInput): void;
}

/**
 * Properties for a notification, also called "toasts", or
 * "balloons" for earlier windows versions.
 */
interface NotificationOptions {
    /**
     * Icon displayed in the notification itself.
     * If not provided, Windows will re-use the value for `icon`.
     * Should be created with the size `Icon.large`.
     */
    icon?: Icon;
    /**
     * Title of the notification.
     * At least one of `title` and `text` is required (present and non-empty).
     */
    title?: string;
    /**
     * Body text of the notification.
     * At least one of `title` and `text` is required (present and non-empty).
     */
    text?: string;
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
}

export namespace NotifyIcon {
    /**
     * Properties for the clickable icon in the notification area (aka. system tray).
     */
    export interface Options {
        /**
         * Display icon to use for the clickable area.
         * If not provided, Windows will simply not draw any icon in the clickable area.
         * Should be created with the size `Icon.small`.
         */
        icon?: Icon;
        /** Tooltip displayed when hovered or keyboard navigated. */
        tooltip?: string;
        /**
         * Hide the icon completely.
         * Note that you cannot show notifications while hidden.
         */
        hidden?: boolean;

        /**
         * Notification to display, replacing any current notification,
         * or remove the existing notification if `null`.
         */
        notification?: NotificationOptions | null;

        /**
         * Callback fired when a user selects the notify icon.
         * A standard response is to invoke `Menu#showSync(event.mouseX, event.mouseY)`
         * for some menu.
         */
        onSelect?: (this: NotifyIcon, event: SelectEvent) => void;
    }

    /**
     * Properties of the icon selection.
     */
    export interface SelectEvent {
        target: NotifyIcon;
        rightButton: boolean;
        mouseX: number;
        mouseY: number;
    }

    /**
     * Initial properties for the clickable icon in the notification area (system tray).
     */
    export interface NewOptions extends Options {
        /**
         * Persistent identifier for this icon.
         * Must be either a 16-byte `Buffer` or a string in the standard UUID/GUID format, matching
         * `/^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$/`,
         * e.g. `"01234567-89ab-cdef-0123-456789abcdef"`.
         *
         * `guid` is a unique id used by Windows to preserve user preferences
         * for this icon. As it is a persistent identifier, do not generate values
         * for this at runtime using, e.g. the `uuid` package, generate them once
         * and save it as a constant in your code.
         *
         * Only one icon can use the same `guid` at a time, even between processes.
         * 
         * Be cautious about using this feature, as it reserves the guid for the
         * executable path and has weird reliablility issues, but it can avoid
         * duplicate settings entries for the notification icon.
         * 
         * https://github.com/electron/electron/issues/2468
         */
        guid?: string;
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
}

export class NotifyIcon {
    /**
     * Add a notification icon to the notification area (system tray).
     * 
     * @param options
     *      Options controlling the creation, display of the icon, and optionally
     *      the notification ("toast" or "balloon").
     */
    constructor(options?: NotifyIcon.NewOptions);

    /**
     * Update the options for a notification icon, and optionally a notification.
     * Only the provided options (exists and not `undefined`) will be updated.
     *
     * @param options 
     *      Options to be updated controlling the display of the icon, and optionally
     *      the notification ("toast" or "balloon").
     */
    update(options?: NotifyIcon.Options): void;

    /**
     * Remove this notification icon and any notification it is showing.
     */
    remove(): void;
}
