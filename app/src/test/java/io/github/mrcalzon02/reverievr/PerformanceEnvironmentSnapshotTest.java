package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import org.junit.Test;

public final class PerformanceEnvironmentSnapshotTest {
    @Test
    public void formatsBatteryTemperatureAndThermalStatus() {
        PerformanceEnvironmentSnapshot snapshot =
            new PerformanceEnvironmentSnapshot(347, 2);

        assertEquals(
            "batteryTempC=34.7 thermalStatus=moderate",
            snapshot.toLogString()
        );
    }

    @Test
    public void unavailableStateIsExplicit() {
        assertEquals(
            "batteryTempC=unavailable thermalStatus=unavailable",
            PerformanceEnvironmentSnapshot
                .unavailable()
                .toLogString()
        );
    }

    @Test
    public void unexpectedThermalStatusIsNotInvented() {
        assertEquals(
            "unknown(91)",
            PerformanceEnvironmentSnapshot
                .thermalStatusLabel(91)
        );
    }
}
