package io.github.mrcalzon02.reverievr;

/** Independently testable VR policy; Android event classification stays in VrActivity. */
final class ExternalHidInputPolicy {
    private ExternalHidInputPolicy() {}

    static boolean shouldConsume(boolean enabled, boolean keyboard, boolean mouse,
                                 boolean gamepad, boolean recovery) {
        return !enabled && !recovery && !gamepad && (keyboard || mouse);
    }

    /** Ordinary external Bluetooth/USB keyboards and scrcpy UHID use one
     * Home editor path. Exclude Android virtual keys and hybrid gamepad/HID
     * controllers, which have their own input-action ownership. */
    static boolean shouldEditHomeKeyboard(boolean enabled, boolean homeEditor,
                                          boolean keyboardSource, boolean physicalDevice,
                                          boolean controllerDevice, boolean shortcutModifier) {
        return enabled && homeEditor && keyboardSource && physicalDevice
            && !controllerDevice && !shortcutModifier;
    }
}
