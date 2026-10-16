package io.github.mrcalzon02.reverievr;

/** Prioritized, explicitly sourced battery/Android thermal warning policy. */
final class DeviceWarningPolicy {
    private DeviceWarningPolicy() {}

    static final int NONE = 0;
    static final int BATTERY_LOW = 1;
    static final int BATTERY_WARM = 2;
    static final int BATTERY_HOT = 3;
    static final int THERMAL_MODERATE = 4;
    static final int THERMAL_SEVERE = 5;
    static final int THERMAL_CRITICAL = 6;

    static int level(int phoneBatteryPercent,
                     int batteryTemperatureTenthsC, int thermalStatus) {
        if (thermalStatus >= 4 && thermalStatus <= 6)
            return THERMAL_CRITICAL;
        if (thermalStatus == 3)
            return THERMAL_SEVERE;
        if (batteryTemperatureTenthsC !=
            PerformanceEnvironmentSnapshot.BATTERY_TEMPERATURE_UNAVAILABLE
            && batteryTemperatureTenthsC >= 450)
            return BATTERY_HOT;
        if (thermalStatus == 2)
            return THERMAL_MODERATE;
        if (batteryTemperatureTenthsC !=
            PerformanceEnvironmentSnapshot.BATTERY_TEMPERATURE_UNAVAILABLE
            && batteryTemperatureTenthsC >= 420)
            return BATTERY_WARM;
        if (phoneBatteryPercent >= 0 && phoneBatteryPercent <= 15)
            return BATTERY_LOW;
        return NONE;
    }

    static String text(int level) {
        switch (level) {
            case BATTERY_LOW: return "PHONE BATTERY LOW - SAVE SOON";
            case BATTERY_WARM: return "BATTERY TEMP WARM - CHECK VENTILATION";
            case BATTERY_HOT: return "BATTERY TEMP HIGH - PAUSE VR";
            case THERMAL_MODERATE: return "ANDROID THERMAL MODERATE - COOL DOWN";
            case THERMAL_SEVERE: return "ANDROID THERMAL SEVERE - STOP VR";
            case THERMAL_CRITICAL: return "ANDROID THERMAL CRITICAL - EXIT VR";
            default: return "";
        }
    }
}
