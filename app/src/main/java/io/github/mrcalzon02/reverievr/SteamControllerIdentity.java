package io.github.mrcalzon02.reverievr;

import java.util.Locale;

/** Explicit 2015 Valve Steam Controller identity, never the Steam platform. */
final class SteamControllerIdentity {
    static final int VALVE_VENDOR_ID = 0x28de;
    // Known USB and BLE receiver identities vary by firmware and transport.
    // Require Valve vendor and a recognizable controller product label;
    // never claim all Valve HID hardware is a Steam Controller.
    static boolean matches(int vendorId, String productName) {
        if (vendorId != VALVE_VENDOR_ID || productName == null) return false;
        String normalized = productName.trim().toLowerCase(Locale.ROOT);
        return normalized.contains("steam controller")
            && !normalized.contains("steam deck");
    }

    // BLE HID often omits vendor identity. This conservative name is heuristic.
    static boolean matchesAndroidHid(int vendorId, String productName) {
        if (productName == null || (vendorId != 0 && vendorId != VALVE_VENDOR_ID)) return false;
        String n = productName.trim().toLowerCase(Locale.ROOT);
        return n.equals("steam controller") || n.equals("valve steam controller");
    }

    private SteamControllerIdentity() {}
}
