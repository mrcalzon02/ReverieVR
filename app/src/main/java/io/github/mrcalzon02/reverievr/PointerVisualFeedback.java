package io.github.mrcalzon02.reverievr;

/**
 * Allocation-free visual-state policy shared by the controller emitter and
 * interaction ray. Press intentionally overrides hover so routed activation is
 * visible even during the short frame in which the shell consumes Select.
 */
final class PointerVisualFeedback {
    static final int IDLE = 0;
    static final int HOVER = 1;
    static final int PRESSED = 2;

    private PointerVisualFeedback() {}

    static int resolve(boolean hitting, boolean pressed) {
        if (pressed) return PRESSED;
        return hitting ? HOVER : IDLE;
    }

    static float markerSize(int state) {
        switch (sanitize(state)) {
            case PRESSED:
                return 0.030f;
            case HOVER:
                return 0.024f;
            default:
                return 0.016f;
        }
    }

    static void writeColor(int state, float[] rgba) {
        if (rgba == null || rgba.length < 4) return;
        switch (sanitize(state)) {
            case PRESSED:
                rgba[0] = 1.00f;
                rgba[1] = 0.72f;
                rgba[2] = 0.18f;
                rgba[3] = 1.00f;
                return;
            case HOVER:
                rgba[0] = 0.18f;
                rgba[1] = 0.92f;
                rgba[2] = 1.00f;
                rgba[3] = 0.95f;
                return;
            default:
                rgba[0] = 0.46f;
                rgba[1] = 0.68f;
                rgba[2] = 0.78f;
                rgba[3] = 0.72f;
        }
    }

    private static int sanitize(int state) {
        return state == HOVER || state == PRESSED ? state : IDLE;
    }
}
