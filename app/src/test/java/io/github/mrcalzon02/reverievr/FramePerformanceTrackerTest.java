package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class FramePerformanceTrackerTest {
    @Test
    public void snapshotReportsCadenceAndSlowFrames() {
        FramePerformanceTracker tracker = new FramePerformanceTracker();
        long now = 1000000000L;
        tracker.recordFrame(now);
        tracker.recordFrame(now += 16000000L);
        tracker.recordFrame(now += 17000000L);
        tracker.recordFrame(now += 26000000L);
        tracker.recordFrame(now += 40000000L);
        FramePerformanceTracker.Snapshot snapshot = tracker.snapshot();
        assertEquals(4, snapshot.sampleCount);
        assertEquals(24.75, snapshot.averageMillis, 0.001);
        assertEquals(40.0, snapshot.p95Millis, 0.001);
        assertEquals(2, snapshot.slowFrames);
        assertEquals(1, snapshot.severeFrames);
    }

    @Test
    public void p99JitterAndObservedLongGaps() {
        FramePerformanceTracker t = new FramePerformanceTracker();
        long n = 1_000_000_000L;
        t.recordFrame(n);
        for (long gap : new long[]{
            16_000_000L, 17_000_000L, 33_333_333L, 42_000_000L, 55_000_000L
        }) t.recordFrame(n += gap);
        FramePerformanceTracker.Snapshot s = t.snapshot();
        assertEquals(5, s.sampleCount);
        assertEquals(55.0, s.p99Millis, 0.0001);
        assertEquals(2, s.gapsOver33Millis);
        assertEquals(1, s.gapsOver50Millis);
        assertTrue(s.jitterMillis > 14.0);
        t.reset();
        assertEquals(0, t.snapshot().sampleCount);
    }

    @Test
    public void perfectThirtyHzCadenceHasZeroJitter() {
        FramePerformanceTracker t = new FramePerformanceTracker();
        long n = 1_000_000_000L;
        t.recordFrame(n);
        for (int i = 0; i < 90; i++) t.recordFrame(n += 33_333_333L);
        FramePerformanceTracker.Snapshot s = t.snapshot();
        assertEquals(0.0, s.jitterMillis, 0.00001);
        assertEquals(0, s.gapsOver33Millis);
        assertEquals(0, s.gapsOver50Millis);
    }

    @Test
    public void lifecycleSizedGapIsNotCountedAsAFrame() {
        FramePerformanceTracker tracker = new FramePerformanceTracker();
        tracker.recordFrame(1000000000L);
        tracker.recordFrame(1016000000L);
        tracker.recordFrame(3000000000L);
        tracker.recordFrame(3016000000L);
        FramePerformanceTracker.Snapshot snapshot = tracker.snapshot();
        assertEquals(2, snapshot.sampleCount);
        assertEquals(16.0, snapshot.maximumMillis, 0.001);
    }

    @Test
    public void sampleWindowRemainsBounded() {
        FramePerformanceTracker tracker = new FramePerformanceTracker();
        long now = 1000000000L;
        tracker.recordFrame(now);
        for (int i = 0; i < 900; i++) {
            tracker.recordFrame(now += 16666667L);
        }
        FramePerformanceTracker.Snapshot snapshot = tracker.snapshot();
        assertEquals(600, snapshot.sampleCount);
        assertTrue(snapshot.p95Millis > 16.6);
        assertTrue(snapshot.p95Millis < 16.8);
    }
}
