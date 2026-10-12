package io.github.mrcalzon02.reverievr;

/** One authoritative controller ownership policy for the VR input router. */
final class ControllerSourcePolicy {
    private ControllerSourcePolicy() {}

    static final String[] CHOICES = {
        ReveriePreferences.CONTROLLER_AUTO,
        ReveriePreferences.CONTROLLER_DAYDREAM,
        ReveriePreferences.CONTROLLER_STEAM,
        ReveriePreferences.CONTROLLER_SCRCPY,
        ReveriePreferences.CONTROLLER_ANDROID,
        ReveriePreferences.CONTROLLER_PHONE
    };

    static String label(String choice) {
        if (ReveriePreferences.CONTROLLER_DAYDREAM.equals(choice)) return "DAYDREAM";
        if (ReveriePreferences.CONTROLLER_STEAM.equals(choice)) return "STEAM CONTROLLER";
        if (ReveriePreferences.CONTROLLER_SCRCPY.equals(choice)) return "SCRCPY / LAPTOP";
        if (ReveriePreferences.CONTROLLER_ANDROID.equals(choice)) return "ANDROID USB / BT";
        if (ReveriePreferences.CONTROLLER_PHONE.equals(choice)) return "PHONE CONTROLLER";
        return "AUTO";
    }

    static boolean acceptsProvider(String choice, String providerId) {
        if ("daydream_ble".equals(providerId)) {
            return ReveriePreferences.CONTROLLER_AUTO.equals(choice)
                || ReveriePreferences.CONTROLLER_DAYDREAM.equals(choice);
        }
        if ("daydream_phone_emulator".equals(providerId)) {
            return ReveriePreferences.CONTROLLER_AUTO.equals(choice)
                || ReveriePreferences.CONTROLLER_PHONE.equals(choice);
        }
        return false;
    }

    static boolean acceptsGamepad(String choice, boolean providerReady,
                                  boolean originalSteam, boolean scrcpyLabelled,
                                  boolean scrcpyCompatible, boolean isGamepad,
                                  boolean steamPresent) {
        if (ReveriePreferences.CONTROLLER_DAYDREAM.equals(choice)
            || ReveriePreferences.CONTROLLER_PHONE.equals(choice)) return false;
        if (ReveriePreferences.CONTROLLER_STEAM.equals(choice))
            return originalSteam;
        if (ReveriePreferences.CONTROLLER_SCRCPY.equals(choice))
            return scrcpyLabelled || scrcpyCompatible;
        if (ReveriePreferences.CONTROLLER_ANDROID.equals(choice))
            return isGamepad;
        // Auto keeps an established BLE controller as primary, then Steam,
        // then a connected Android gamepad. No two sources control the shell.
        if (providerReady) return false;
        if (steamPresent) return originalSteam;
        return isGamepad || scrcpyLabelled;
    }
}
