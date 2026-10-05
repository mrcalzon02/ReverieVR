package io.github.mrcalzon02.reverievr;

import java.util.Locale;

final class PerformanceEnvironmentSnapshot {
    static final int BATTERY_TEMPERATURE_UNAVAILABLE =
        Integer.MIN_VALUE;
    static final int THERMAL_STATUS_UNAVAILABLE = -1;

    final int batteryTemperatureTenthsC;
    final int thermalStatus;

    PerformanceEnvironmentSnapshot(
        int batteryTemperatureTenthsC,
        int thermalStatus
    ) {
        this.batteryTemperatureTenthsC =
            batteryTemperatureTenthsC;
        this.thermalStatus = thermalStatus;
    }

    static PerformanceEnvironmentSnapshot unavailable() {
        return new PerformanceEnvironmentSnapshot(
            BATTERY_TEMPERATURE_UNAVAILABLE,
            THERMAL_STATUS_UNAVAILABLE
        );
    }

    String toLogString() {
        String batteryTemperature =
            batteryTemperatureTenthsC
                    == BATTERY_TEMPERATURE_UNAVAILABLE
                ? "unavailable"
                : String.format(
                    Locale.US,
                    "%.1f",
                    batteryTemperatureTenthsC / 10.0
                );

        return "batteryTempC="
            + batteryTemperature
            + " thermalStatus="
            + thermalStatusLabel(thermalStatus);
    }

    static String thermalStatusLabel(int status) {
        switch (status) {
            case THERMAL_STATUS_UNAVAILABLE:
                return "unavailable";
            case 0:
                return "none";
            case 1:
                return "light";
            case 2:
                return "moderate";
            case 3:
                return "severe";
            case 4:
                return "critical";
            case 5:
                return "emergency";
            case 6:
                return "shutdown";
            default:
                return "unknown(" + status + ")";
        }
    }
}
