package io.github.mrcalzon02.reverievr;

import java.nio.FloatBuffer;

/** GLES2-safe pointer geometry: a thin beam and two crossing target bars. */
final class PointerRibbonGeometry {
    private static final float MIN_LENGTH = 0.000001f;
    private static final float BEAM_HALF_WIDTH = 0.003f;

    private PointerRibbonGeometry() {}

    /** Writes exactly 18 vertices (54 floats), without allocating or touching GL state. */
    static void fill(FloatBuffer out, float ox, float oy, float oz,
                     float ex, float ey, float ez, float markerSize) {
        if (out == null || out.remaining() < 54) {
            throw new IllegalArgumentException("Pointer ribbon requires 54 floats");
        }
        float dx = ex - ox;
        float dy = ey - oy;
        float xyLength = (float) Math.sqrt(dx * dx + dy * dy);
        float perpendicularX = xyLength > MIN_LENGTH ? -dy / xyLength : 1f;
        float perpendicularY = xyLength > MIN_LENGTH ? dx / xyLength : 0f;
        quad(out, ox + perpendicularX * BEAM_HALF_WIDTH, oy + perpendicularY * BEAM_HALF_WIDTH, oz,
             ox - perpendicularX * BEAM_HALF_WIDTH, oy - perpendicularY * BEAM_HALF_WIDTH, oz,
             ex + perpendicularX * BEAM_HALF_WIDTH, ey + perpendicularY * BEAM_HALF_WIDTH, ez,
             ex - perpendicularX * BEAM_HALF_WIDTH, ey - perpendicularY * BEAM_HALF_WIDTH, ez);
        float halfBar = markerSize;
        float halfWidth = markerSize * 0.12f;
        quad(out, ex - halfBar, ey + halfWidth, ez, ex - halfBar, ey - halfWidth, ez,
             ex + halfBar, ey + halfWidth, ez, ex + halfBar, ey - halfWidth, ez);
        quad(out, ex - halfWidth, ey + halfBar, ez, ex - halfWidth, ey - halfBar, ez,
             ex + halfWidth, ey + halfBar, ez, ex + halfWidth, ey - halfBar, ez);
    }

    private static void quad(FloatBuffer out,
                             float ax, float ay, float az, float bx, float by, float bz,
                             float cx, float cy, float cz, float dx, float dy, float dz) {
        vertex(out, ax, ay, az); vertex(out, bx, by, bz); vertex(out, cx, cy, cz);
        vertex(out, cx, cy, cz); vertex(out, bx, by, bz); vertex(out, dx, dy, dz);
    }

    private static void vertex(FloatBuffer out, float x, float y, float z) {
        out.put(x); out.put(y); out.put(z);
    }
}
