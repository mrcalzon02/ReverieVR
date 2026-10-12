package io.github.mrcalzon02.reverievr;

/** Desk keyboard quad and key hit testing in the same 1024x768 UV space. */
final class HomeKeyboardGeometry {
    static final float LEFT = -1.73f;
    static final float RIGHT = 1.73f;
    static final float FRONT_Z = -1.36f;
    static final float BACK_Z = -4.08f;
    static final float TOP_Y = -1.065f;
    private HomeKeyboardGeometry() {}

    static float hitDistance(float[] origin, float[] ray) {
        if (origin == null || ray == null || origin.length < 3 || ray.length < 3
            || !Float.isFinite(ray[1]) || Math.abs(ray[1]) < 0.00001f)
            return -1f;
        float t = (TOP_Y - origin[1]) / ray[1];
        if (!Float.isFinite(t) || t <= 0f) return -1f;
        float x = origin[0] + t * ray[0];
        float z = origin[2] + t * ray[2];
        return x >= LEFT && x <= RIGHT && z >= BACK_Z && z <= FRONT_Z
            ? t : -1f;
    }

    static int keyAtRay(float[] origin, float[] ray, int[][] buttons) {
        float t = hitDistance(origin, ray);
        if (t <= 0f) return -1;
        float x = origin[0] + t * ray[0];
        float z = origin[2] + t * ray[2];
        float px = (x - LEFT) / (RIGHT - LEFT) * 1024f;
        float py = (z - BACK_Z) / (FRONT_Z - BACK_Z) * 768f;
        for (int i = 0; i < buttons.length; i++) {
            int[] rect = buttons[i];
            if (px >= rect[0] && px <= rect[2]
                && py >= rect[1] && py <= rect[3])
                return i;
        }
        return -1;
    }
}
