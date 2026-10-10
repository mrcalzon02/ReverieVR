package io.github.mrcalzon02.reverievr;

/** Rejects corrupt or stale controller rays before rendering stereo geometry. */
final class PointerRayValidation {
    private static final float MIN_DIRECTION_SQUARED = 0.00000001f;
    private PointerRayValidation() {}

    static boolean valid(float ox, float oy, float oz,
                         float dx, float dy, float dz, float distance) {
        if (!Float.isFinite(ox) || !Float.isFinite(oy) || !Float.isFinite(oz)
                || !Float.isFinite(dx) || !Float.isFinite(dy)
                || !Float.isFinite(dz) || !Float.isFinite(distance)) {
            return false;
        }
        float squared = dx * dx + dy * dy + dz * dz;
        return Float.isFinite(squared) && squared > MIN_DIRECTION_SQUARED;
    }

    static float clampDistance(float distance) {
        return Math.max(0.25f, Math.min(distance, 12.0f));
    }
}
