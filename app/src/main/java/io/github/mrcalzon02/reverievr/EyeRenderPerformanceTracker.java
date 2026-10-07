package io.github.mrcalzon02.reverievr;

import java.util.Arrays;
import java.util.Locale;

/** Development-only CPU eye submission timings, not GPU/compositor timings.
 * Recording is allocation-free; mode changes discard mixed samples. */
final class EyeRenderPerformanceTracker {
    static final int MAX_SAMPLES_PER_EYE = 600;
    private static final long MAX_SAMPLE_NANOS = 1_000_000_000L;
    private final long[][] samples = new long[2][MAX_SAMPLES_PER_EYE];
    private final int[] counts = new int[2];
    private final int[] nextIndices = new int[2];
    private int mode = -1;

    static final class EyeStats {
        final int count;
        final double averageMillis;
        final double p95Millis;
        final double p99Millis;
        final double maximumMillis;

        EyeStats(int count, double averageMillis, double p95Millis,
                 double p99Millis, double maximumMillis) {
            this.count = count;
            this.averageMillis = averageMillis;
            this.p95Millis = p95Millis;
            this.p99Millis = p99Millis;
            this.maximumMillis = maximumMillis;
        }

        String toLogString() {
            return String.format(Locale.US,
                "n=%d avgMs=%.2f p95Ms=%.2f p99Ms=%.2f maxMs=%.2f",
                count, averageMillis, p95Millis, p99Millis, maximumMillis);
        }
    }

    static final class Snapshot {
        final int mode;
        final EyeStats left;
        final EyeStats right;

        Snapshot(int mode, EyeStats left, EyeStats right) {
            this.mode = mode;
            this.left = left;
            this.right = right;
        }

        String toLogString() {
            return "cpuSubmissionOnly=true mode=" + mode
                + " left=[" + left.toLogString() + "]"
                + " right=[" + right.toLogString() + "]";
        }
    }

    void recordEye(int currentMode, int eyeIndex, long elapsedNanos) {
        if (eyeIndex < 0 || eyeIndex > 1
            || elapsedNanos <= 0L || elapsedNanos > MAX_SAMPLE_NANOS) {
            return;
        }
        if (currentMode != mode) {
            reset();
            mode = currentMode;
        }
        samples[eyeIndex][nextIndices[eyeIndex]] = elapsedNanos;
        nextIndices[eyeIndex] =
            (nextIndices[eyeIndex] + 1) % MAX_SAMPLES_PER_EYE;
        if (counts[eyeIndex] < MAX_SAMPLES_PER_EYE) {
            counts[eyeIndex]++;
        }
    }

    void reset() {
        Arrays.fill(counts, 0);
        Arrays.fill(nextIndices, 0);
        mode = -1;
    }

    Snapshot snapshot() {
        return new Snapshot(mode, eyeStats(0), eyeStats(1));
    }

    private EyeStats eyeStats(int eye) {
        int count = counts[eye];
        if (count == 0) return new EyeStats(0, 0, 0, 0, 0);
        long[] ordered = Arrays.copyOf(samples[eye], count);
        Arrays.sort(ordered);
        double total = 0.0;
        for (long value : ordered) total += value;
        return new EyeStats(count, millis(total / count),
            millis(ordered[percentileIndex(count, 0.95)]),
            millis(ordered[percentileIndex(count, 0.99)]),
            millis(ordered[count - 1]));
    }

    private static int percentileIndex(int count, double percentile) {
        return Math.max(0, Math.min(count - 1,
            (int) Math.ceil(count * percentile) - 1));
    }

    private static double millis(double nanos) {
        return nanos / 1_000_000.0;
    }
}
