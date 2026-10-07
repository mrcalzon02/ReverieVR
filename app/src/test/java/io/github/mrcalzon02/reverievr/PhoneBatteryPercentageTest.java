package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import org.junit.Test;

public final class PhoneBatteryPercentageTest {
    @Test public void convertsNonHundredScaleAndEndpoints() {
        assertEquals(50, PhoneBatteryPercentage.fromLevelAndScale(1, 2));
        assertEquals(33, PhoneBatteryPercentage.fromLevelAndScale(1, 3));
        assertEquals(100, PhoneBatteryPercentage.fromLevelAndScale(4200, 4200));
        assertEquals(0, PhoneBatteryPercentage.fromLevelAndScale(0, 100));
    }
    @Test public void rejectsInvalidBroadcastValues() {
        assertEquals(-1, PhoneBatteryPercentage.fromLevelAndScale(-1, 100));
        assertEquals(-1, PhoneBatteryPercentage.fromLevelAndScale(50, 0));
        assertEquals(-1, PhoneBatteryPercentage.fromLevelAndScale(101, 100));
        assertEquals(-1, PhoneBatteryPercentage.fromLevelAndScale(50, -1));
    }
}
