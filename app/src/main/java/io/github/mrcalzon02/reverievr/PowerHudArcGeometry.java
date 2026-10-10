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
    private PowerHudArcGeometry() {}
    static float progress(int pct) {
        return pct < 0 || pct > 100 ? 0f : SWEEP * pct / 100f;
    }
    static float revealY(boolean lowered) { return lowered ? DROP : 0f; }
}
