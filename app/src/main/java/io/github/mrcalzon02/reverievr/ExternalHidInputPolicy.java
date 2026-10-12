package io.github.mrcalzon02.reverievr;

/** Independently testable VR policy; Android event classification stays in VrActivity. */
final class ExternalHidInputPolicy {
    private ExternalHidInputPolicy() {}

    /**
     * Controller ownership has priority over an optional keyboard/mouse gate:
     * original Steam hardware reports as keyboard/mouse; selected scrcpy HID
     * mouse input can be the actual VR pointer, not merely guest passthrough.
     */
    static boolean controllerOwnsHid(boolean gamepad, boolean originalSteam,
                                     boolean mouse, boolean selectedVrPointer) {
        return gamepad || originalSteam || mouse && selectedVrPointer;
    }

    static boolean shouldConsume(boolean enabled, boolean keyboard, boolean mouse,
                                 boolean controllerInput, boolean recovery) {
        return !enabled && !recovery && !controllerInput && (keyboard || mouse);
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
