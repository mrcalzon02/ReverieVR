package io.github.mrcalzon02.reverievr;

/**
 * Converts clipped mirror-plane line segments to visible GLES2 triangles.
 * GLES2 wide GL_LINES are not portable; this uses fixed 14mm world-space
 * ribbons, no per-frame allocations, and clips the final thickness to the
 * existing mirror rectangle. The caller owns both scratch arrays.
 */
final class MirrorLineRibbons {
    static final float HALF_THICKNESS = 0.007f;

    private MirrorLineRibbons() {}

    static int write(float[] lines, int vertexCount, float[] out) {
        if (lines == null || out == null || vertexCount < 0
            || vertexCount % 2 != 0
            || lines.length < vertexCount * 3
            || out.length < (vertexCount / 2) * 18) return 0;
        int written = 0;
        for (int i = 0; i < vertexCount * 3; i += 6) {
            float ay = lines[i + 1], az = lines[i + 2];
            float by = lines[i + 4], bz = lines[i + 5];
            if (!Float.isFinite(ay) || !Float.isFinite(az)
                || !Float.isFinite(by) || !Float.isFinite(bz)) continue;
            float dy = by - ay, dz = bz - az;
            float len = (float) Math.sqrt((double) dy * dy + (double) dz * dz);
            if (!Float.isFinite(len) || len < 0.00001f) continue;
            float oy = -dz / len * HALF_THICKNESS;
            float oz = dy / len * HALF_THICKNESS;
            written = vertex(out, written, ay + oy, az + oz);
            written = vertex(out, written, ay - oy, az - oz);
            written = vertex(out, written, by + oy, bz + oz);
            written = vertex(out, written, by + oy, bz + oz);
            written = vertex(out, written, ay - oy, az - oz);
            written = vertex(out, written, by - oy, bz - oz);
        }
        return written / 3;
    }

    private static int vertex(float[] out, int i, float y, float z) {
        out[i++] = HeadsetMirrorProjection.LINE_X;
        out[i++] = Math.max(-HeadsetMirrorProjection.HALF_HEIGHT,
            Math.min(HeadsetMirrorProjection.HALF_HEIGHT, y));
        out[i++] = Math.max(-HeadsetMirrorProjection.HALF_WIDTH,
            Math.min(HeadsetMirrorProjection.HALF_WIDTH, z));
        return i;
    }
}
