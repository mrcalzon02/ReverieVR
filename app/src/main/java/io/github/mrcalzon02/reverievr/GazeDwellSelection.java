package io.github.mrcalzon02.reverievr;

/**
 * Headset-only emergency selection, independent of Android input devices.
 * A single eligible target must stay under the live gaze ray for five seconds.
 * The same target never repeats until gaze leaves it. All timing is monotonic.
 */
final class GazeDwellSelection {
    static final long DWELL_NANOS = 5_000_000_000L;
    private int context = Integer.MIN_VALUE;
    private int target = -1;
    private long startedAt;
    private boolean fired;
    private float progress;

    void reset() {
        context = Integer.MIN_VALUE;
        target = -1;
        startedAt = 0L;
        fired = false;
        progress = 0.0f;
    }

    boolean update(boolean available, int newContext, int liveTarget, long nowNanos) {
        if (!available || liveTarget < 0 || nowNanos <= 0L) {
            reset();
            return false;
        }
        if (context != newContext || target != liveTarget
            || nowNanos < startedAt || startedAt <= 0L) {
            context = newContext;
            target = liveTarget;
            startedAt = nowNanos;
            fired = false;
            progress = 0.0f;
            return false;
        }
        long elapsed = nowNanos - startedAt;
        progress = Math.min(1.0f,
            Math.max(0.0f, (float) ((double) elapsed / DWELL_NANOS)));
        if (elapsed < DWELL_NANOS || fired) return false;
        fired = true;
        return true;
    }

    float progress() { return progress; }
    int target() { return target; }
    boolean completed() { return fired; }
}
