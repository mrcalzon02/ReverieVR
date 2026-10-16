package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class FrameRateMeterTest {
    private static FrameRateMeter sample(int fps) {
        FrameRateMeter meter = new FrameRateMeter();
        long time = 1_000_000_000L;
        meter.recordFrame(time);
        long step = Math.round(1_000_000_000.0 / fps);
        for (int i = 0; i < fps * 2; i++) {
            meter.recordFrame(time += step);
        }
        return meter;
    }

    @Test public void countsStereoFramesAtSixtyThirtyAndTwenty() {
        assertEquals(60, sample(60).getFps());
        assertEquals(30, sample(30).getFps());
        assertEquals(20, sample(20).getFps());
    }

    @Test public void bandThresholdsMatchTheHeadsetContract() {
        assertEquals(FrameRateMeter.Band.UNKNOWN, FrameRateMeter.bandFor(-1));
        assertEquals(FrameRateMeter.Band.RED, FrameRateMeter.bandFor(0));
        assertEquals(FrameRateMeter.Band.RED, FrameRateMeter.bandFor(20));
        assertEquals(FrameRateMeter.Band.AMBER, FrameRateMeter.bandFor(21));
        assertEquals(FrameRateMeter.Band.AMBER, FrameRateMeter.bandFor(29));
        assertEquals(FrameRateMeter.Band.GREEN, FrameRateMeter.bandFor(30));
        assertEquals(FrameRateMeter.Band.GREEN, FrameRateMeter.bandFor(59));
        assertEquals(FrameRateMeter.Band.BLUE, FrameRateMeter.bandFor(60));
        assertEquals(FrameRateMeter.Band.BLUE, FrameRateMeter.bandFor(144));
    }

    @Test public void paintsOnlyForChangedReadoutAndNotEveryFrame() {
        FrameRateMeter meter = new FrameRateMeter();
        long time = 1_000_000_000L;
        assertFalse(meter.recordFrame(time));
        int changed = 0;
        for (int i = 0; i < 120; i++) {
            if (meter.recordFrame(time += 16_666_667L)) changed++;
        }
        assertEquals(60, meter.getFps());
        assertEquals(1, changed);
    }

    @Test public void suspendingClearsStaleValueThenReacquires() {
        FrameRateMeter meter = sample(60);
        assertEquals(60, meter.getFps());
        assertTrue(meter.recordFrame(10_000_000_000L));
        assertEquals(-1, meter.getFps());
        long time = 10_000_000_000L;
        for (int i = 0; i < 16; i++)
            meter.recordFrame(time += 33_333_333L);
        assertEquals(30, meter.getFps());
    }
}
