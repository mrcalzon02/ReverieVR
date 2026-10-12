package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.assertEquals;

public class VrUiTargetStabilizerTest {
    @Test public void firstAcquisitionIsImmediateButCrossKeyJitterDoesNotFlap() {
        VrUiTargetStabilizer latch = new VrUiTargetStabilizer();
        long t = 1_000_000_000L;
        assertEquals(4, latch.update(4, t, false));
        assertEquals(4, latch.update(5, t + 5_000_000L, false));
        assertEquals(4, latch.update(4, t + 10_000_000L, false));
        assertEquals(4, latch.update(5, t + 20_000_000L, false));
        assertEquals(4, latch.update(5, t + 60_000_000L, false));
        assertEquals(5, latch.update(5, t + 66_000_000L, false));
    }

    @Test public void oneFrameMissDoesNotClearHighlightButLongMissDoes() {
        VrUiTargetStabilizer latch = new VrUiTargetStabilizer();
        long t = 1_000_000_000L;
        assertEquals(7, latch.update(7, t, false));
        assertEquals(7, latch.update(-1, t + 15_000_000L, false));
        assertEquals(7, latch.update(-1, t + 64_000_000L, false));
        assertEquals(-1, latch.update(-1, t + 65_000_000L, false));
    }

    @Test public void selectAlwaysUsesCurrentRayAndNeverStaleHighlight() {
        VrUiTargetStabilizer latch = new VrUiTargetStabilizer();
        long t = 1_000_000_000L;
        assertEquals(3, latch.update(3, t, false));
        assertEquals(3, latch.update(-1, t + 10_000_000L, false));
        assertEquals(-1, latch.update(-1, t + 12_000_000L, true));
        assertEquals(8, latch.update(8, t + 20_000_000L, true));
        assertEquals(8, latch.update(9, t + 21_000_000L, false));
        assertEquals(9, latch.update(9, t + 22_000_000L, true));
    }

    @Test public void modeResetFrameStallAndBackwardClockDropOldFocus() {
        VrUiTargetStabilizer latch = new VrUiTargetStabilizer();
        long t = 1_000_000_000L;
        assertEquals(2, latch.update(2, t, false));
        latch.reset();
        assertEquals(-1, latch.update(-1, t + 1_000_000L, false));
        assertEquals(6, latch.update(6, t + 2_000_000L, false));
        assertEquals(-1, latch.update(-1, t + 300_000_000L, false));
        assertEquals(1, latch.update(1, t + 301_000_000L, false));
        assertEquals(-1, latch.update(-1, t + 50_000_000L, false));
        assertEquals(-1, latch.update(1, -1L, true));
    }

    @Test public void targetMissesCannotBeTreatedAsAnotherValidKey() {
        VrUiTargetStabilizer latch = new VrUiTargetStabilizer();
        assertEquals(-1, latch.update(-2, 1L, false));
        assertEquals(0, latch.update(0, 2L, false));
        assertEquals(-1, latch.update(-7, 3L, true));
    }
}
