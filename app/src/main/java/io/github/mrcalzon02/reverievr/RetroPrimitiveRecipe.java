package io.github.mrcalzon02.reverievr;

/**
 * PS1-inspired primitive material recipe. Runs only during asset construction:
 * no texture files, random devices, per-frame synthesis, or floating dithering.
 */
final class RetroPrimitiveRecipe {
    static final float POSITION_GRID = 128f; // intentionally coarse 1/128 metre
    private RetroPrimitiveRecipe() {}

    static float snapPosition(float coordinate) {
        if (!Float.isFinite(coordinate)) {
            throw new IllegalArgumentException("non-finite primitive vertex");
        }
        return Math.round(coordinate * POSITION_GRID) / POSITION_GRID;
    }

    /** RGB555 palette: classic small fixed palette instead of full 8-bit tints. */
    static float[] color555(float red, float green, float blue) {
        return new float[] {quantizeChannel(red), quantizeChannel(green),
            quantizeChannel(blue)};
    }

    private static float quantizeChannel(float value) {
        if (!Float.isFinite(value)) {
            throw new IllegalArgumentException("non-finite material color");
        }
        return Math.round(Math.max(0f, Math.min(1f, value)) * 31f) / 31f;
    }

    /** Stable explicit-seed variation; only called while generating geometry. */
    static float offset(int seed, int index) {
        int x = seed ^ (index * 0x9e3779b9);
        x ^= x >>> 16;
        x *= 0x7feb352d;
        x ^= x >>> 15;
        x *= 0x846ca68b;
        x ^= x >>> 16;
        return ((x & 255) - 127.5f) / 127.5f * .075f;
    }
}
