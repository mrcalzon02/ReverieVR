package io.github.mrcalzon02.reverievr;

import java.util.Locale;

/** Explicit 2015 Valve Steam Controller identity, never the Steam platform. */
final class SteamControllerIdentity {
    static final int VALVE_VENDOR_ID = 0x28de;
    // Valve's 2015 original BLE controller advertises "SteamController" on
    // some firmware/Android stacks, including the user's Galaxy S9. The
    // friendly product name "Steam Controller" is NOT its only HID identity.
    // Be explicit about the limited aliases: never match generic Steam
    // services, Steam Deck, or arbitrary Valve input hardware by substring.
    private static boolean originalControllerName(String productName) {
        if (productName == null) return false;
        String name = productName.trim().toLowerCase(Locale.ROOT);
        return name.equals("steamcontroller")
            || name.equals("steam controller")
            || name.equals("valve steamcontroller")
            || name.equals("valve steam controller")
            || name.equals("wireless steam controller");
    }

    /** Strongly identified Valve HID (when the OS exposes vendor ID). */
    static boolean matches(int vendorId, String productName) {
        return vendorId == VALVE_VENDOR_ID && originalControllerName(productName);
    }

    /**
     * Android BLE HID sometimes reports VID=0 for a genuine paired controller.
     * This is a conservative display-name heuristic, not proof of pairing or
     * connectivity. Runtime input device + source enumeration is still needed.
     */
    static boolean matchesAndroidHid(int vendorId, String productName) {
        return (vendorId == 0 || vendorId == VALVE_VENDOR_ID)
            && originalControllerName(productName);
    }

    private SteamControllerIdentity() {}
}
