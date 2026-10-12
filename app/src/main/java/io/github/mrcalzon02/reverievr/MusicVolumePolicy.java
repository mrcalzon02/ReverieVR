package io.github.mrcalzon02.reverievr;

/** Bounded, transparent volume control for only ReverieVR's own
 * shell music. Does not change Android system/media stream volume. */
final class MusicVolumePolicy {
    static final int DEFAULT_PERCENT = 44; // preserves legacy 0.22 gain
    static final int STEP_PERCENT = 10;
    private static final float MAX_GAIN = 0.50f;

    private MusicVolumePolicy() {}

    static int clamp(int percent) {
        return Math.max(0, Math.min(100, percent));
    }

    static int adjust(int current, int direction) {
        int safe=clamp(current);
        if (direction < 0) return clamp(safe - STEP_PERCENT);
        if (direction > 0) return clamp(safe + STEP_PERCENT);
        return safe;
    }

    static float gain(int percent, boolean muted) {
        return muted ? 0f : clamp(percent) * MAX_GAIN / 100f;
    }

    static String display(int percent, boolean muted) {
        return "MUSIC " + clamp(percent) + "% "
            + (muted ? "MUTED" : "ON");
    }
}
