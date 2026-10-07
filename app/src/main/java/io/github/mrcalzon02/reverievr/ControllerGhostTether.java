package io.github.mrcalzon02.reverievr;

/** Allocation-free coil geometry shared between both stereo eye draws. */
final class ControllerGhostTether {
    static final int VERTEX_COUNT = 65;

    private ControllerGhostTether() {}

    static float opacity(float distance) {
        if (!Float.isFinite(distance) || distance <= 0.0f) return 0.0f;
        return 0.27f * Math.min(1.0f, distance / 0.040f);
    }

    static int write(float[] vertices,
        float sx, float sy, float sz, float ex, float ey, float ez) {
        if (vertices == null || vertices.length < VERTEX_COUNT * 3
            || !Float.isFinite(sx) || !Float.isFinite(sy)
            || !Float.isFinite(sz) || !Float.isFinite(ex)
            || !Float.isFinite(ey) || !Float.isFinite(ez)) return 0;
        float dx = ex - sx, dy = ey - sy, dz = ez - sz;
        float distance = (float) Math.sqrt(dx * dx + dy * dy + dz * dz);
        if (!Float.isFinite(distance) || distance < 0.010f) return 0;
        float ux = dx / distance, uy = dy / distance, uz = dz / distance;
        float rx = Math.abs(uy) < 0.85f ? 0.0f : 1.0f;
        float ry = Math.abs(uy) < 0.85f ? 1.0f : 0.0f;
        float vx = -uz * ry, vy = uz * rx, vz = ux * ry - uy * rx;
        float length = (float) Math.sqrt(vx * vx + vy * vy + vz * vz);
        vx /= length;
        vy /= length;
        vz /= length;
        float wx = uy * vz - uz * vy;
        float wy = uz * vx - ux * vz;
        float wz = ux * vy - uy * vx;
        float radius = Math.min(0.009f, distance * 0.085f);
        for (int i = 0; i < VERTEX_COUNT; i++) {
            float t = i / (float) (VERTEX_COUNT - 1);
            float taper = (float) Math.sin(Math.PI * t);
            float angle = (float) (Math.PI * 12.0 * t);
            float a = radius * taper * (float) Math.cos(angle);
            float b = radius * taper * (float) Math.sin(angle);
            int j = i * 3;
            vertices[j] = sx + dx * t + vx * a + wx * b;
            vertices[j + 1] = sy + dy * t + vy * a + wy * b;
            vertices[j + 2] = sz + dz * t + vz * a + wz * b;
        }
        return VERTEX_COUNT;
    }
}
