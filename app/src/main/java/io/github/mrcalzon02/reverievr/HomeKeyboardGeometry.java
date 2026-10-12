package io.github.mrcalzon02.reverievr;

/** Desk keyboard quad and key hit testing in the same 1024x768 UV space. */
final class HomeKeyboardGeometry {
    static final float LEFT = -1.73f;
    static final float RIGHT = 1.73f;
    static final float FRONT_Z = -1.36f;
    static final float BACK_Z = -4.08f;
    static final float TOP_Y = -1.039f;
    // Dais is at -1.08. A real keycap must rise above it, while the
    // transparent glyph/legend texture floats just above the keycap tops.
    static final float KEYCAP_BASE_Y = -1.069f;
    static final float KEYCAP_TOP_Y = -1.043f;
    static final float LEGEND_Y = TOP_Y;
    static final int WIDTH_PIXELS = 1024;
    static final int HEIGHT_PIXELS = 768;

    private HomeKeyboardGeometry() {}

    /** One authoritative 51-key layout for renderer, labels and ray hits. */
    static int[][] keyRects() {
        int[][] buttons = new int[51][4];
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 10; col++) {
                int left = 72 + col * 88;
                int top = 238 + row * 84;
                buttons[row * 10 + col] =
                    new int[] {left, top, left + 82, top + 74};
            }
        }
        buttons[50] = new int[] {340, 684, 684, 736};
        return buttons;
    }

    static float worldX(float pixel) {
        return LEFT + pixel / WIDTH_PIXELS * (RIGHT - LEFT);
    }

    static float worldZ(float pixel) {
        return BACK_Z + pixel / HEIGHT_PIXELS * (FRONT_Z - BACK_Z);
    }

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
        float px = (x - LEFT) / (RIGHT - LEFT) * WIDTH_PIXELS;
        float py = (z - BACK_Z) / (FRONT_Z - BACK_Z) * HEIGHT_PIXELS;
        for (int i = 0; i < buttons.length; i++) {
            int[] rect = buttons[i];
            if (px >= rect[0] && px <= rect[2]
                && py >= rect[1] && py <= rect[3])
                return i;
        }
        return -1;
    }
}
