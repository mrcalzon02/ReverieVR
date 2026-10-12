package io.github.mrcalzon02.reverievr;

/**
 * One authoritative world-space layout shared by the phone housing meshes
 * and the controller/gaze raycast. Each panel control is a true 3D target.
 */
final class PhonePanelControlGeometry {
    static final int COUNT = 6;
    static final int CLOSE_INDEX = 5;
    static final float LEFT_X = 0.72f;
    static final float RIGHT_X = 1.12f;
    static final float FIRST_TOP_Y = 1.02f;
    static final float PITCH_Y = 0.165f;
    static final float KEY_HEIGHT = 0.145f;
    static final float RAY_Z = -2.245f;

    private PhonePanelControlGeometry() {}

    static float top(int index) {
        if (index < 0 || index >= COUNT)
            throw new IllegalArgumentException("Unknown panel control");
        return FIRST_TOP_Y - PITCH_Y * index;
    }

    static int indexAtLocal(float x, float y) {
        if (!Float.isFinite(x) || !Float.isFinite(y)
            || x < LEFT_X || x > RIGHT_X) return -1;
        for (int i = 0; i < COUNT; i++) {
            float top = top(i);
            if (y >= top - KEY_HEIGHT && y <= top) return i;
        }
        return -1;
    }
}
