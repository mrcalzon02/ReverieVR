package io.github.mrcalzon02.reverievr;
import java.util.Locale;

/** Recognition of scrcpy-labelled UHID, not arbitrary keyboard/mouse sources. */
final class ScrcpyHidIdentity {
    static boolean matchesName(String name) {
        if (name == null) return false;
        String value = name.trim().toLowerCase(Locale.ROOT);
        return value.equals("scrcpy") || value.startsWith("scrcpy ")
            || value.startsWith("scrcpy-") || value.startsWith("scrcpy_");
    }
    /**
     * scrcpy's UHID gamepad intentionally emulates the Xbox 360 USB HID
     * identity, NOT the scrcpy name. A real Xbox 360 pad is indistinguishable
     * from this signature through Android's public InputDevice properties.
     * Only use this alias after the user EXPLICITLY selects scrcpy input;
     * automatic controller identification must not claim it is scrcpy.
     */
    static boolean matchesCompatibleGamepad(int vendorId, int productId, String name) {
        return vendorId == 0x045e && productId == 0x028e
            && name != null && name.trim().equalsIgnoreCase("Microsoft X-Box 360 Pad");
    }

    private ScrcpyHidIdentity() {}
}
