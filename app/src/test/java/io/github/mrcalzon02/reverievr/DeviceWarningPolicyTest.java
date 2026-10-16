package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public final class DeviceWarningPolicyTest {
    @Test public void unavailableTelemetryDoesNotInventHeat() {
        assertEquals(DeviceWarningPolicy.NONE,
            DeviceWarningPolicy.level(-1,
                PerformanceEnvironmentSnapshot.BATTERY_TEMPERATURE_UNAVAILABLE,
                PerformanceEnvironmentSnapshot.THERMAL_STATUS_UNAVAILABLE));
    }

    @Test public void thermalSeverityTakesPriorityOverBatteryLevel() {
        assertEquals(DeviceWarningPolicy.THERMAL_CRITICAL,
            DeviceWarningPolicy.level(3, 460, 4));
        assertEquals(DeviceWarningPolicy.THERMAL_SEVERE,
            DeviceWarningPolicy.level(3, 350, 3));
        assertEquals(DeviceWarningPolicy.THERMAL_MODERATE,
            DeviceWarningPolicy.level(80, 400, 2));
    }

    @Test public void batteryTemperatureIsNotCalledCpuTemperature() {
        assertEquals(DeviceWarningPolicy.BATTERY_WARM,
            DeviceWarningPolicy.level(85, 420, -1));
        assertEquals(DeviceWarningPolicy.BATTERY_HOT,
            DeviceWarningPolicy.level(85, 450, -1));
        assertTrue(DeviceWarningPolicy.text(DeviceWarningPolicy.BATTERY_HOT)
            .contains("BATTERY TEMP"));
    }

    @Test public void lowPowerWarningHasKnownBatteryGate() {
        assertEquals(DeviceWarningPolicy.BATTERY_LOW,
            DeviceWarningPolicy.level(15, 300, -1));
        assertEquals(DeviceWarningPolicy.NONE,
            DeviceWarningPolicy.level(16, 300, -1));
        assertEquals("", DeviceWarningPolicy.text(DeviceWarningPolicy.NONE));
    }
}
