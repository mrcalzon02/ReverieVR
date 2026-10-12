package io.github.mrcalzon02.reverievr;

/**
 * Authoritative world-space control positions for panel meshes and raycasts.
 * Right rail retains the original six controls including Close; left rail
 * adds size and distance adjustments without repurposing existing buttons.
 */
final class PhonePanelControlGeometry {
    static final int COUNT = 10;
    static final int CLOSE_INDEX = 5;
    static final int SIZE_SMALLER_INDEX = 6;
    static final int SIZE_LARGER_INDEX = 7;
    static final int DISTANCE_NEARER_INDEX = 8;
    static final int DISTANCE_FARTHER_INDEX = 9;
    static final float LEFT_X = 0.72f;
    static final float RIGHT_X = 1.12f;
    static final float AUX_LEFT_X = -1.12f;
    static final float AUX_RIGHT_X = -0.72f;
    static final float FIRST_TOP_Y = 1.02f;
    static final float PITCH_Y = 0.165f;
    static final float KEY_HEIGHT = 0.145f;
    static final float PIVOT_Z = -2.27f;
    static final float RAY_Z = -2.245f;
    static final float MIN_SCALE = 0.75f;
    static final float MAX_SCALE = 1.15f;
    static final float SCALE_STEP = 0.10f;
    static final float MIN_DISTANCE_OFFSET = -0.30f;
    static final float MAX_DISTANCE_OFFSET = 0.45f;
    static final float DISTANCE_STEP = 0.15f;

    private PhonePanelControlGeometry() {}

    private static void requireIndex(int index) {
        if (index < 0 || index >= COUNT)
            throw new IllegalArgumentException("Unknown panel control");
    }

    static float left(int index) {
        requireIndex(index);
        return index < SIZE_SMALLER_INDEX ? LEFT_X : AUX_LEFT_X;
    }

    static float right(int index) {
        requireIndex(index);
        return index < SIZE_SMALLER_INDEX ? RIGHT_X : AUX_RIGHT_X;
    }

    static float top(int index) {
        requireIndex(index);
        int row = index < SIZE_SMALLER_INDEX ? index
            : index - SIZE_SMALLER_INDEX;
        return FIRST_TOP_Y - PITCH_Y * row;
    }

    static int indexAtLocal(float x, float y) {
        if (!Float.isFinite(x) || !Float.isFinite(y)) return -1;
        for (int i = 0; i < COUNT; i++) {
            float top = top(i);
            if (x >= left(i) && x <= right(i)
                && y >= top - KEY_HEIGHT && y <= top) return i;
        }
        return -1;
    }

    static float adjustedScale(float current, int direction) {
        if (!Float.isFinite(current)) current = 1.0f;
        return Math.max(MIN_SCALE, Math.min(MAX_SCALE,
            current + (direction < 0 ? -SCALE_STEP : SCALE_STEP)));
    }

    /** Positive offset moves the whole assembly farther from the user. */
    static float adjustedDistance(float current, int direction) {
        if (!Float.isFinite(current)) current = 0.0f;
        return Math.max(MIN_DISTANCE_OFFSET, Math.min(MAX_DISTANCE_OFFSET,
            current + (direction < 0 ? -DISTANCE_STEP : DISTANCE_STEP)));
    }
}
