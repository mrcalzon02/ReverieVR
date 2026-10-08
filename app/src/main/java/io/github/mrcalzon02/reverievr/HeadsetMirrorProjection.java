package io.github.mrcalzon02.reverievr;

/**
 * Exact per-eye planar projection of the shell headset and handset proxies.
 * The vanity panel is perpendicular to +X in the white home room. Geometry
 * is clipped to the panel, generated into caller-owned arrays, and never
 * written into the actual headset near-eye view.
 */
final class HeadsetMirrorProjection {
    static final float PLANE_X = 2.14f;
    static final float LINE_X = 2.11f;
    static final float HALF_HEIGHT = 0.85f;
    static final float HALF_WIDTH = 0.95f;
    static final int MAX_VERTICES = PlayerHeadRig.WIREFRAME_VERTICES;
    static final int CONTROLLER_VERTICES = 6;

    private final float[] headset = new float[PlayerHeadRig.WIREFRAME_FLOATS];
    private final float[] eyes = new float[6];
    private final float[] projected = new float[4];
    private final float[] clip = new float[2];
    private float eyeX, eyeY, eyeZ;
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
        // A mirror is visible only from its front, with a nonzero eye gap.
        ready = Float.isFinite(eyeX)
            && eyeX < PLANE_X - 0.05f;
        return ready;
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

    int writeController(float x, float y, float z, float[] out) {
        if (!ready || out == null || out.length < CONTROLLER_VERTICES * 3
            || !Float.isFinite(x) || !Float.isFinite(y)
            || !Float.isFinite(z)) return 0;
        int written = 0;
        written = segment(out, written,
            x - 0.08f, y, z, x + 0.08f, y, z);
        written = segment(out, written,
            x, y - 0.06f, z, x, y + 0.06f, z);
        written = segment(out, written,
            x, y, z - 0.06f, x, y, z + 0.06f);
        return written / 3;
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
