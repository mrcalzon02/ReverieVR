package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import org.junit.Test;

public final class DeviceWarningLatchTest {
    @Test public void escalatesImmediatelyAndHoldsTemporaryRecovery() {
        DeviceWarningLatch latch = new DeviceWarningLatch();
        long t = 1_000_000_000L;
        assertEquals(DeviceWarningPolicy.BATTERY_WARM,
            latch.update(DeviceWarningPolicy.BATTERY_WARM, t));
        assertEquals(DeviceWarningPolicy.THERMAL_CRITICAL,
            latch.update(DeviceWarningPolicy.THERMAL_CRITICAL, t + 1L));
        assertEquals(DeviceWarningPolicy.THERMAL_CRITICAL,
            latch.update(DeviceWarningPolicy.NONE, t + 2_000_000_000L));
        assertEquals(DeviceWarningPolicy.THERMAL_CRITICAL,
            latch.update(DeviceWarningPolicy.NONE, t + 11_000_000_000L));
        assertEquals(DeviceWarningPolicy.NONE,
            latch.update(DeviceWarningPolicy.NONE, t + 12_000_000_000L));
    }

    @Test public void bouncingReadingsCancelPendingClear() {
        DeviceWarningLatch latch = new DeviceWarningLatch();
        long t = 2_000_000_000L;
        latch.update(DeviceWarningPolicy.BATTERY_HOT, t);
        latch.update(DeviceWarningPolicy.NONE, t + 1_000_000_000L);
        assertEquals(DeviceWarningPolicy.BATTERY_HOT,
            latch.update(DeviceWarningPolicy.BATTERY_HOT, t + 8_000_000_000L));
        assertEquals(DeviceWarningPolicy.BATTERY_HOT,
            latch.update(DeviceWarningPolicy.NONE, t + 9_000_000_000L));
        assertEquals(DeviceWarningPolicy.NONE,
            latch.update(DeviceWarningPolicy.NONE, t + 19_000_000_000L));
    }

    @Test public void resetAndClockRollbackAreSafe() {
        DeviceWarningLatch latch = new DeviceWarningLatch();
        latch.update(DeviceWarningPolicy.THERMAL_SEVERE, 100L);
        latch.update(DeviceWarningPolicy.NONE, 200L);
        assertEquals(DeviceWarningPolicy.THERMAL_SEVERE,
            latch.update(DeviceWarningPolicy.NONE, 150L));
        latch.reset();
        assertEquals(DeviceWarningPolicy.NONE,
            latch.update(DeviceWarningPolicy.NONE, 150L));
        assertEquals(DeviceWarningPolicy.NONE,
            latch.update(-50, 10L));
    }
}
