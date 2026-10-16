package io.github.mrcalzon02.reverievr;

import java.util.Arrays;
import java.util.Locale;

/** CPU-side intervals between Cardboard frame callbacks, NOT display present. */
final class FramePerformanceTracker {
    static final long TARGET_FRAME_NANOS = 16_666_667L;
    private static final long MAX_VALID_INTERVAL_NANOS = 1_000_000_000L;
    private static final int MAX_SAMPLES = 600;

    static final class Snapshot {
        final int sampleCount;
        final double averageMillis;
        final double p95Millis;
        final double p99Millis;
        final double maximumMillis;
        final double jitterMillis;
        final int slowFrames;
        final int severeFrames;
        final int gapsOver33Millis;
        final int gapsOver50Millis;

        Snapshot(int n, double average, double p95, double p99,
                 double maximum, double jitter, int slow, int severe,
                 int over33, int over50) {
            sampleCount = n;
            averageMillis = average;
            p95Millis = p95;
            p99Millis = p99;
            maximumMillis = maximum;
            jitterMillis = jitter;
            slowFrames = slow;
            severeFrames = severe;
            gapsOver33Millis = over33;
            gapsOver50Millis = over50;
        }

        String toLogString() {
            return String.format(Locale.US,
                "samples=%d avgMs=%.2f p95Ms=%.2f p99Ms=%.2f maxMs=%.2f jitterMs=%.2f slow=%d severe=%d gapsOver33ms=%d gapsOver50ms=%d",
                sampleCount, averageMillis, p95Millis, p99Millis,
                maximumMillis, jitterMillis, slowFrames, severeFrames,
                gapsOver33Millis, gapsOver50Millis);
        }
    }

    private final long[] intervals = new long[MAX_SAMPLES];
    private int count;
    private int nextIndex;
    private long previousFrameNanos;

    void reset() {
        count = 0;
        nextIndex = 0;
        previousFrameNanos = 0L;
    }

    void recordFrame(long frameNanos) {
        if (frameNanos <= 0L) return;
        if (previousFrameNanos > 0L && frameNanos > previousFrameNanos) {
            long interval = frameNanos - previousFrameNanos;
            if (interval <= MAX_VALID_INTERVAL_NANOS) {
                intervals[nextIndex] = interval;
                nextIndex = (nextIndex + 1) % intervals.length;
                if (count < intervals.length) count++;
            }
        }
        previousFrameNanos = frameNanos;
    }

    Snapshot snapshot() {
        if (count == 0) return new Snapshot(0,0,0,0,0,0,0,0,0,0);
        long[] ordered = new long[count];
        double sum = 0.0;
        int slow = 0, severe = 0, over33 = 0, over50 = 0;
        for (int i = 0; i < count; i++) {
            long value = intervals[i];
            ordered[i] = value;
            sum += value;
            if (value > TARGET_FRAME_NANOS * 3L / 2L) slow++;
            if (value > TARGET_FRAME_NANOS * 2L) severe++;
            if (value > 33_333_334L) over33++;
            if (value > 50_000_000L) over50++;
        }
        double mean = sum / count;
        double variance = 0.0;
        for (int i = 0; i < count; i++) {
            double diff = intervals[i] - mean;
            variance += diff * diff;
        }
        Arrays.sort(ordered);
        int p95 = Math.max(0,(int)Math.ceil(count * 0.95) - 1);
        int p99 = Math.max(0,(int)Math.ceil(count * 0.99) - 1);
        return new Snapshot(count, mean / 1_000_000.0,
            ordered[p95] / 1_000_000.0, ordered[p99] / 1_000_000.0,
            ordered[count-1] / 1_000_000.0,
            Math.sqrt(variance / count) / 1_000_000.0,
            slow, severe, over33, over50);
    }
}
