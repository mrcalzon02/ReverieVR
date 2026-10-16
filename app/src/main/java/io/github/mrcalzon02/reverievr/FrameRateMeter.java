package io.github.mrcalzon02.reverievr;

/** A bounded, allocation-free estimate of submitted stereo frame pairs per second.
 * Feed once from Cardboard's onNewFrame callback, never once per eye. */
final class FrameRateMeter {
    static final long UPDATE_NANOS = 500_000_000L;
    static final long SUSPEND_GAP_NANOS = 2_000_000_000L;
    enum Band { UNKNOWN, RED, AMBER, GREEN, BLUE }

    private long lastFrameNanos;
    private long windowStartNanos;
    private int intervalCount;
    private int displayedFps = -1;

    int getFps() { return displayedFps; }

    static Band bandFor(int fps) {
        if (fps < 0) return Band.UNKNOWN;
        if (fps <= 20) return Band.RED;
        if (fps < 30) return Band.AMBER;
        if (fps < 60) return Band.GREEN;
        return Band.BLUE;
    }

    /** True only when the displayed number needs repainting. */
    boolean recordFrame(long frameNanos) {
        if (frameNanos <= 0L) return false;
        if (lastFrameNanos == 0L
            || frameNanos <= lastFrameNanos
            || frameNanos - lastFrameNanos > SUSPEND_GAP_NANOS) {
            lastFrameNanos = frameNanos;
            windowStartNanos = frameNanos;
            intervalCount = 0;
            boolean changed = displayedFps != -1;
            displayedFps = -1;
            return changed;
        }
        lastFrameNanos = frameNanos;
        intervalCount++;
        long elapsed = frameNanos - windowStartNanos;
        if (elapsed < UPDATE_NANOS) return false;
        int next = (int) Math.min(999L,
            Math.round(intervalCount * 1_000_000_000.0 / elapsed));
        intervalCount = 0;
        windowStartNanos = frameNanos;
        boolean changed = displayedFps != next;
        displayedFps = next;
        return changed;
    }
}
