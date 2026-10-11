package io.github.mrcalzon02.reverievr;

/** The standard BLE Battery Level (0x2A19) is exactly one unsigned percent. */
final class SteamControllerBatteryLevel {
    static int decode(byte[] value) {
        if (value == null || value.length != 1) return -1;
        int percent = value[0] & 0xff;
        return percent <= 100 ? percent : -1;
    }
    private SteamControllerBatteryLevel() {}
}
