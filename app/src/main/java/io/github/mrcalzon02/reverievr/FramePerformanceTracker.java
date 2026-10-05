package io.github.mrcalzon02.reverievr;

import java.util.Arrays;
import java.util.Locale;

final class FramePerformanceTracker {
    static final long TARGET_FRAME_NANOS = 16666667L;
    private static final long MAX_VALID_INTERVAL_NANOS = 1000000000L;
    private static final int MAX_SAMPLES = 600;

    static final class Snapshot {
        final int sampleCount;
        final double averageMillis;
        final double p95Millis;
        final double maximumMillis;
        final int slowFrames;
        final int severeFrames;

        Snapshot(int sampleCount, double averageMillis, double p95Millis,
                 double maximumMillis, int slowFrames, int severeFrames) {
            this.sampleCount = sampleCount;
            this.averageMillis = averageMillis;
            this.p95Millis = p95Millis;
            this.maximumMillis = maximumMillis;
            this.slowFrames = slowFrames;
            this.severeFrames = severeFrames;
        }

        String toLogString() {
            return String.format(Locale.US,
                "samples=%d avgMs=%.2f p95Ms=%.2f maxMs=%.2f slow=%d severe=%d",
                sampleCount, averageMillis, p95Millis, maximumMillis,
                slowFrames, severeFrames);
        }
    }

    private final long[] intervals = new long[MAX_SAMPLES];
    private int count;
    private int nextIndex;
    private long previousFrameNanos;

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
        if (count == 0) return new Snapshot(0, 0, 0, 0, 0, 0);
        long[] ordered = new long[count];
        long total = 0L;
        long maximum = 0L;
        int slow = 0;
        int severe = 0;
        for (int i = 0; i < count; i++) {
            long value = intervals[i];
            ordered[i] = value;
            total += value;
            maximum = Math.max(maximum, value);
            if (value > TARGET_FRAME_NANOS * 3L / 2L) slow++;
            if (value > TARGET_FRAME_NANOS * 2L) severe++;
        }
        Arrays.sort(ordered);
        int p95Index = Math.max(0, Math.min(count - 1,
            (int) Math.ceil(count * 0.95) - 1));
        return new Snapshot(count, millis((double) total / count),
            millis(ordered[p95Index]), millis(maximum), slow, severe);
    }

    private static double millis(double nanos) {
        return nanos / 1000000.0;
    }
}
