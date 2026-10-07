package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class EyeRenderPerformanceTrackerTest {
    @Test public void independentEyesAndPercentiles() {
        EyeRenderPerformanceTracker t = new EyeRenderPerformanceTracker();
        t.recordEye(5, 0, 1_000_000L);
        t.recordEye(5, 1, 2_000_000L);
        t.recordEye(5, 0, 3_000_000L);
        t.recordEye(5, 1, 4_000_000L);
        EyeRenderPerformanceTracker.Snapshot s = t.snapshot();
        assertEquals(5, s.mode);
        assertEquals(2, s.left.count);
        assertEquals(2, s.right.count);
        assertEquals(2.0, s.left.averageMillis, 0.0001);
        assertEquals(3.0, s.left.p95Millis, 0.0001);
        assertEquals(3.0, s.left.p99Millis, 0.0001);
        assertEquals(4.0, s.right.maximumMillis, 0.0001);
        assertTrue(s.toLogString().contains("cpuSubmissionOnly=true"));
    }

    @Test public void invalidSamplesCannotResetMode() {
        EyeRenderPerformanceTracker t = new EyeRenderPerformanceTracker();
        t.recordEye(5, 0, 1_000_000L);
        t.recordEye(6, -1, 1_000_000L);
        t.recordEye(6, 2, 1_000_000L);
        t.recordEye(6, 1, 0L);
        t.recordEye(6, 1, 1_000_000_001L);
        assertEquals(5, t.snapshot().mode);
        assertEquals(1, t.snapshot().left.count);
        assertEquals(0, t.snapshot().right.count);
    }

    @Test public void modeTransitionAndResetDiscardOldSamples() {
        EyeRenderPerformanceTracker t = new EyeRenderPerformanceTracker();
        t.recordEye(5, 0, 2_000_000L);
        t.recordEye(5, 1, 3_000_000L);
        t.recordEye(1, 1, 4_000_000L);
        assertEquals(1, t.snapshot().mode);
        assertEquals(0, t.snapshot().left.count);
        assertEquals(1, t.snapshot().right.count);
        t.reset();
        assertEquals(-1, t.snapshot().mode);
        assertEquals(0, t.snapshot().right.count);
    }

    @Test public void ringDropsOldestAndBoundsMemory() {
        EyeRenderPerformanceTracker t = new EyeRenderPerformanceTracker();
        t.recordEye(5, 0, 900_000_000L);
        for (int i = 0; i < 600; i++) {
            t.recordEye(5, 0, 5_000_000L);
        }
        EyeRenderPerformanceTracker.EyeStats s = t.snapshot().left;
        assertEquals(600, s.count);
        assertEquals(5.0, s.averageMillis, 0.0001);
        assertEquals(5.0, s.maximumMillis, 0.0001);
    }
}
