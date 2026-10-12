package io.github.mrcalzon02.reverievr;

import android.opengl.Matrix;

/** One panel transform shared by both per-eye draw and live pointer raycast. */
final class PhonePanelPose {
    private PhonePanelPose() {}

    static void write(float[] out, float yawDegrees, float pitchDegrees,
        float scale, float distanceOffset) {
        Matrix.setIdentityM(out, 0);
        Matrix.translateM(out, 0, 0.0f, 0.0f,
            PhonePanelControlGeometry.PIVOT_Z - distanceOffset);
        Matrix.rotateM(out, 0, yawDegrees, 0.0f, 1.0f, 0.0f);
        Matrix.rotateM(out, 0, pitchDegrees, 1.0f, 0.0f, 0.0f);
        Matrix.scaleM(out, 0, scale, scale, scale);
        Matrix.translateM(out, 0, 0.0f, 0.0f,
            -PhonePanelControlGeometry.PIVOT_Z);
    }
}
