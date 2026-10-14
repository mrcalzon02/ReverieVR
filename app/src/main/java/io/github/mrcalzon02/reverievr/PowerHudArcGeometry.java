package io.github.mrcalzon02.reverievr;

final class PowerHudArcGeometry {
    static final float CENTER = 256.0f;
    static final float START = -77.0f;
    static final float SWEEP = 45.0f;
    static final float OUTER = 177.0f;
    static final float INNER = 157.0f;
    static final float TRACK_WIDTH = 13.0f;
    static final float PROGRESS_WIDTH = 11.0f;
    static final float DROP = 52.0f;
    /** 70% smaller than the previous version, anchored at eye top-right. */
    static final float OUTPUT_SCALE = 0.30f;
    // Eye NDC spans -1..+1: a 0.20 downward shift equals 10% of the
    // visible eye height. Translate the entire cropped quad, not its UVs.
    static final float VIEWPORT_DOWN_NDC = 0.20f;
    // User adjustment: move the whole overlay 15% of the eye UP/LEFT.
    // Each eye spans two NDC units, so the translation is 0.30 in X/Y.
    // Retain the original 10%-down basis, reduce additional down to 5%.
    static final float EXTRA_DOWN_NDC = 0.10f;
    static final float EXTRA_INWARD_NDC = 0.30f;
    static float lowerInViewport(float eyeY) {
        return eyeY - VIEWPORT_DOWN_NDC - EXTRA_DOWN_NDC;
    }
    static float inwardInViewport(float eyeX) {
        return eyeX - EXTRA_INWARD_NDC;
    }
    static float scaledNearTopRight(float value, float anchor) {
        return anchor + (value - anchor) * OUTPUT_SCALE;
    }
    static float labelCenterX(float radius) {
        return CENTER + radius * (float)Math.cos(Math.toRadians(START + SWEEP / 2f));
    }
    static float labelTopBaselineY(float radius) {
        return CENTER + radius * (float)Math.sin(Math.toRadians(START + SWEEP / 2f))
            - 24f;
    }
    private PowerHudArcGeometry() {}
    static float progress(int pct) {
        return pct < 0 || pct > 100 ? 0f : SWEEP * pct / 100f;
    }
    static float revealY(boolean lowered) { return lowered ? DROP : 0f; }
}
