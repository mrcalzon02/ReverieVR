package io.github.mrcalzon02.reverievr;

/**
 * Correct the horizontal head-pivot handedness at the Cardboard-to-shell
 * boundary. Cardboard supplies a pose basis whose horizontal pivot currently
 * appears reversed in ReverieVR's world-to-eye path. Correct YAW alone:
 * pitch, roll, translation, IPD and each eye's projection remain untouched.
 *
 * The correction is applied once per center-head sample, before deriving
 * headset-forward vectors, the yaw offset, ray directions or eye matrices.
 */
final class HeadYawConvention {
    private HeadYawConvention() {}

    static boolean correctInPlace(float[] view) {
        if (view == null || view.length < 16) return false;
        for (int i = 0; i < 16; i++) {
            if (!Float.isFinite(view[i])) return false;
        }
        float heading = VrHeadingMath.yawFromHeadView(view);
        if (!Float.isFinite(heading)) return false;
        float angle = heading * 2.0f;
        float cosine = (float) Math.cos(angle);
        float sine = (float) Math.sin(angle);

        // Rotate the world-to-eye basis about world up. This mirrors only
        // horizontal yaw; world Y components, near/far, and eye IPD remain.
        for (int column = 0; column < 4; column++) {
            int i = column * 4;
            float x = view[i];
            float z = view[i + 2];
            view[i] = cosine * x + sine * z;
            view[i + 2] = -sine * x + cosine * z;
        }
        return true;
    }
}
