package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.assertEquals;

public class SteamControllerBatteryLevelTest {
    @Test public void recognizesOnlyValidStandardBatteryLevels() {
        assertEquals(-1, SteamControllerBatteryLevel.decode(null));
        assertEquals(-1, SteamControllerBatteryLevel.decode(new byte[0]));
        assertEquals(-1, SteamControllerBatteryLevel.decode(new byte[]{50,0}));
        assertEquals(-1, SteamControllerBatteryLevel.decode(new byte[]{101}));
        assertEquals(-1, SteamControllerBatteryLevel.decode(new byte[]{(byte)255}));
        assertEquals(0, SteamControllerBatteryLevel.decode(new byte[]{0}));
        assertEquals(50, SteamControllerBatteryLevel.decode(new byte[]{50}));
        assertEquals(100, SteamControllerBatteryLevel.decode(new byte[]{100}));
    }
}
