package io.github.mrcalzon02.reverievr;

import java.nio.FloatBuffer;

/**
 * Exact per-eye planar projection of the shell headset and handset proxies.
 * The vanity panel is perpendicular to +X in the white home room. Geometry
 * is clipped to the panel, generated into caller-owned arrays, and never
 * written into the actual headset near-eye view.
 */
final class HeadsetMirrorProjection {
    static final float PLANE_X = 2.14f;
    // Different depths: the cage ALWAYS sits in front of the projected body.
    // Equal mirror-plane depths previously made the flat portrait occlude it.
    static final float LINE_X = 2.074f;
    static final float AVATAR_X = 2.105f;
    static final float HALF_HEIGHT = 0.85f;
    static final float HALF_WIDTH = 0.95f;
    static final int MAX_VERTICES = PlayerHeadRig.WIREFRAME_VERTICES;
    static final int CONTROLLER_VERTICES = 12;

    private final float[] headset = new float[PlayerHeadRig.WIREFRAME_FLOATS];
    private final float[] eyes = new float[6];
    private final float[] projected = new float[4];
    private final float[] clip = new float[2];
    // Reused polygon workspaces for projected avatar triangles (no per-eye GC).
    private final float[] avatarClipA = new float[20];
    private final float[] avatarClipB = new float[20];
    private float eyeX, eyeY, eyeZ;
    private final float[] forward = new float[2];
    private boolean ready;

    boolean prepare(PlayerHeadRig rig, int eyeIndex) {
        ready = rig != null && rig.isValid()
            && (eyeIndex == 0 || eyeIndex == 1)
            && rig.copyEyeCenters(eyes)
            && rig.writeWireframe(headset) == MAX_VERTICES;
        if (!ready) return false;
        eyeX = eyes[eyeIndex * 3];
        eyeY = eyes[eyeIndex * 3 + 1];
        eyeZ = eyes[eyeIndex * 3 + 2];
        // Keep the avatar upright, but rotate it with the user's actual
        // headset heading. A forward-facing constant-X billboard cannot do
        // this, and shows the same profile regardless of head motion.
        if (!rig.copyHorizontalForward(forward)) {
            forward[0] = 1f;
            forward[1] = 0f;
        }
        // A mirror is visible only from its front, with a nonzero eye gap.
        ready = Float.isFinite(eyeX)
            && eyeX < PLANE_X - 0.05f;
        return ready;
    }

    /** Front-only features are hidden when the avatar is side-on or turned
     * away from the mirror; otherwise its face looks painted on its back.
     */
    boolean avatarFaceVisible() {
        return ready && forward[0] > 0.10f;
    }

    int writeHeadset(float[] out) {
        if (!ready || out == null || out.length < MAX_VERTICES * 3) return 0;
        int written = 0;
        for (int i = 0; i < headset.length; i += 6) {
            written = segment(out, written,
                headset[i], headset[i + 1], headset[i + 2],
                headset[i + 3], headset[i + 4], headset[i + 5]);
        }
        return written / 3;
    }

    /**
     * A headset-relative handset position marker plus a world-space aiming
     * arrow. The arrow follows the same normalized ray used for interaction,
     * whether the source is a tracked or virtual controller. No raw
     * quaternion is inferred here and invalid rays never draw stale aim.
     */
    int writeController(float x, float y, float z,
        float dx, float dy, float dz, float[] out) {
        if (!ready || out == null || out.length < CONTROLLER_VERTICES * 3
            || !Float.isFinite(x) || !Float.isFinite(y)
            || !Float.isFinite(z) || !Float.isFinite(dx)
            || !Float.isFinite(dy) || !Float.isFinite(dz)) return 0;
        double length = Math.sqrt((double) dx * dx
            + (double) dy * dy + (double) dz * dz);
        if (!Double.isFinite(length) || length < 0.00001) return 0;
        float nx = (float) (dx / length);
        float ny = (float) (dy / length);
        float nz = (float) (dz / length);

        // Stable perpendicular arrowhead basis even when aiming vertically.
        float tx = Math.abs(ny) > 0.90f ? 0.0f : -nz;
        float ty = Math.abs(ny) > 0.90f ? nz : 0.0f;
        float tz = Math.abs(ny) > 0.90f ? -ny : nx;
        float tangentLength = (float) Math.sqrt(tx * tx + ty * ty + tz * tz);
        if (tangentLength < 0.00001f) return 0;
        tx /= tangentLength;
        ty /= tangentLength;
        tz /= tangentLength;

        int written = 0;
        written = segment(out, written,
            x - 0.08f, y, z, x + 0.08f, y, z);
        written = segment(out, written,
            x, y - 0.06f, z, x, y + 0.06f, z);
        written = segment(out, written,
            x, y, z - 0.06f, x, y, z + 0.06f);

        float tipX = x + nx * 0.32f;
        float tipY = y + ny * 0.32f;
        float tipZ = z + nz * 0.32f;
        written = segment(out, written, x, y, z, tipX, tipY, tipZ);
        written = segment(out, written,
            tipX, tipY, tipZ,
            tipX - nx * 0.07f + tx * 0.04f,
            tipY - ny * 0.07f + ty * 0.04f,
            tipZ - nz * 0.07f + tz * 0.04f);
        written = segment(out, written,
            tipX, tipY, tipZ,
            tipX - nx * 0.07f - tx * 0.04f,
            tipY - ny * 0.07f - ty * 0.04f,
            tipZ - nz * 0.07f - tz * 0.04f);
        return written / 3;
    }


    /**
     * Convert the authored avatar's local portrait triangles into the
     * physically located reflection of the player's body. The authored
     * mesh X coordinates are offsets from the legacy MIRROR_X origin;
     * they are NOT themselves mirror coordinates. Rotate actual low-poly
     * X-depth volumes with the horizontal headset heading, then reflect
     * their WORLD coordinates into each eye's mirror pane.
     *
     * Projects into this eye's mirror plane and clips every triangle to
     * the four edges of the glass. The destination contains GL_TRIANGLES
     * (three floats per vertex); at most five triangles per source triangle.
     * The source buffer's position/limit are never changed.
     */
    int writeAvatarTriangles(FloatBuffer source, int vertexCount,
        float bodyX, float headY, float bodyZ, float scale, float[] out) {
        if (!ready || source == null || out == null
            || vertexCount < 0 || vertexCount % 3 != 0
            || vertexCount > source.limit() / 3
            || out.length / 15 < vertexCount
            || !Float.isFinite(bodyX) || !Float.isFinite(headY)
            || !Float.isFinite(bodyZ) || !Float.isFinite(scale)
            || scale <= 0f) return 0;
        int written = 0;
        for (int i = 0; i < vertexCount; i += 3) {
            boolean valid = true;
            for (int v = 0; v < 3; v++) {
                int at = (i + v) * 3;
                float localX = (source.get(at)
                    - HomeAvatarWardrobeGeometry.MIRROR_X) * scale;
                float localZ = source.get(at + 2) * scale;
                float x = bodyX + forward[0] * localX
                    - forward[1] * localZ;
                float y = headY + (source.get(at + 1) - 0.65f) * scale;
                float z = bodyZ + forward[1] * localX
                    + forward[0] * localZ;
                if (!project(x, y, z, v == 0 ? 0 : v == 1 ? 2 : 0)) {
                    // The ordinary line projection scratch has four floats.
                    // For the third vertex, use a separate pair below.
                    valid = false;
                    break;
                }
                if (v == 0 || v == 1) {
                    avatarClipA[v * 2] = projected[v * 2];
                    avatarClipA[v * 2 + 1] = projected[v * 2 + 1];
                } else {
                    avatarClipA[4] = projected[0];
                    avatarClipA[5] = projected[1];
                }
            }
            if (!valid) continue;

            float[] input = avatarClipA;
            float[] output = avatarClipB;
            int count = 3;
            for (int edge = 0; edge < 4 && count > 0; edge++) {
                int next = 0;
                int previous = (count - 1) * 2;
                float py = input[previous], pz = input[previous + 1];
                float pd = edgeDistance(edge, py, pz);
                for (int v = 0; v < count; v++) {
                    float y = input[v * 2], z = input[v * 2 + 1];
                    float d = edgeDistance(edge, y, z);
                    if ((pd >= 0f) != (d >= 0f)) {
                        float t = pd / (pd - d);
                        output[next++] = py + t * (y - py);
                        output[next++] = pz + t * (z - pz);
                    }
                    if (d >= 0f) {
                        output[next++] = y;
                        output[next++] = z;
                    }
                    py = y; pz = z; pd = d;
                }
                count = next / 2;
                float[] swap = input; input = output; output = swap;
            }
            for (int v = 1; v + 1 < count; v++) {
                for (int fan = 0; fan < 3; fan++) {
                    int k = (fan == 0 ? 0 : fan == 1 ? v : v + 1) * 2;
                    out[written++] = AVATAR_X;
                    out[written++] = input[k];
                    out[written++] = input[k + 1];
                }
            }
        }
        return written / 3;
    }

    private static float edgeDistance(int edge, float y, float z) {
        switch (edge) {
            case 0: return y + HALF_HEIGHT;
            case 1: return HALF_HEIGHT - y;
            case 2: return z + HALF_WIDTH;
            default: return HALF_WIDTH - z;
        }
    }

    private int segment(float[] out, int offset,
        float ax, float ay, float az, float bx, float by, float bz) {
        if (!project(ax, ay, az, 0) || !project(bx, by, bz, 2)) {
            return offset;
        }
        float y0=projected[0], z0=projected[1];
        float dy=projected[2]-y0, dz=projected[3]-z0;
        clip[0]=0.0f; clip[1]=1.0f;
        if (!clipEdge(-dy, y0 + HALF_HEIGHT)
            || !clipEdge(dy, HALF_HEIGHT - y0)
            || !clipEdge(-dz, z0 + HALF_WIDTH)
            || !clipEdge(dz, HALF_WIDTH - z0)) {
            return offset;
        }
        float t0=clip[0], t1=clip[1];
        if (t0 > t1) return offset;
        out[offset++]=LINE_X;
        out[offset++]=y0 + dy*t0;
        out[offset++]=z0 + dz*t0;
        out[offset++]=LINE_X;
        out[offset++]=y0 + dy*t1;
        out[offset++]=z0 + dz*t1;
        return offset;
    }

    private boolean project(float x, float y, float z, int index) {
        // Reflect geometry across the mirror and intersect the sightline
        // from the physical eye with the panel. Each eye differs naturally.
        float reflectedX=2.0f*PLANE_X - x;
        float denominator=reflectedX-eyeX;
        if (!Float.isFinite(denominator) || denominator <= 0.0001f) {
            return false;
        }
        float t=(PLANE_X-eyeX)/denominator;
        if (!Float.isFinite(t) || t <= 0.0f || t >= 1.0f) return false;
        projected[index]=eyeY + (y-eyeY)*t;
        projected[index+1]=eyeZ + (z-eyeZ)*t;
        return Float.isFinite(projected[index])
            && Float.isFinite(projected[index+1]);
    }

    private boolean clipEdge(float p, float q) {
        if (Math.abs(p) < 0.0000001f) return q >= 0.0f;
        float r=q/p;
        if (p < 0.0f) {
            if (r > clip[1]) return false;
            if (r > clip[0]) clip[0]=r;
        } else {
            if (r < clip[0]) return false;
            if (r < clip[1]) clip[1]=r;
        }
        return true;
    }
}
