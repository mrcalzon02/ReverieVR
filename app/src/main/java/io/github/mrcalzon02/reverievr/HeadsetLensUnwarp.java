package io.github.mrcalzon02.reverievr;

/**
 * Bounded radial remapping for prewarped headset-eye recordings.
 *
 * The coefficient maps a destination eye pixel to a location in the source
 * eye image. It is NOT a calibration for any particular headset. The zero
 * setting is an exact no-op and preserves already-undistorted recordings.
 *
 * The radius is normalized so the eye-image corners have squared radius 1.
 * Each eye is independently remapped before the MediaPlayer surface transform.
 */
final class HeadsetLensUnwarp {
    static final int MIN_STEP = -6;
    static final int MAX_STEP = 6;
    static final float COEFFICIENT_PER_STEP = 0.05f;

    private HeadsetLensUnwarp() { }

    static int clampStep(int step) {
        return Math.max(MIN_STEP, Math.min(MAX_STEP, step));
    }

    static int adjust(int step, int direction) {
        return clampStep(clampStep(step) + Integer.signum(direction));
    }

    static float coefficient(int step) {
        return clampStep(step) * COEFFICIENT_PER_STEP;
    }

    /** Mirrors the fragment shader radial mapping for isolated JVM tests. */
    static float sourceRadiusScale(float eyeU, float eyeV, int step) {
        float x = eyeU * 2.0f - 1.0f;
        float y = eyeV * 2.0f - 1.0f;
        float radiusSquared = 0.5f * (x * x + y * y);
        return 1.0f + coefficient(step) * radiusSquared;
    }

    static String displayName(int step) {
        int normalized = clampStep(step);
        if (normalized == 0) {
            return "OFF";
        }
        return (normalized > 0 ? "+" : "")
            + (normalized * 5) + "%";
    }
}
