package io.github.mrcalzon02.reverievr;

/**
 * CardboardView.getPose() returns the headset's local-to-world rigid pose.
 * HeadTransform, eye.applyHeadView(), scene rays and Cardboard stereo require
 * the inverse (world-to-head view). Invert the entire rigid transform once,
 * rather than independently flipping yaw, pitch or roll.
 *
 * Mutates one frame-local matrix; does not modify stereo IPD, eye projection,
 * body position, HUD overlay placement or controller quaternion.
 */
final class HeadPoseView {
    private HeadPoseView() {}

    static boolean invertRigidInPlace(float[] pose) {
        if (pose == null || pose.length < 16) return false;
        for (int i = 0; i < 16; i++) {
            if (!Float.isFinite(pose[i])) return false;
        }
        if (Math.abs(pose[3]) > 0.001f
            || Math.abs(pose[7]) > 0.001f
            || Math.abs(pose[11]) > 0.001f
            || Math.abs(pose[15] - 1f) > 0.001f) return false;

        float r00=pose[0], r10=pose[1], r20=pose[2];
        float r01=pose[4], r11=pose[5], r21=pose[6];
        float r02=pose[8], r12=pose[9], r22=pose[10];
        float tx=pose[12], ty=pose[13], tz=pose[14];

        // Rigid inverse = [R^T, -R^T t]. Rotation is orthonormal because
        // the Cardboard head tracker supplies a normalized quaternion.
        float c0=r00*r00+r10*r10+r20*r20;
        float c1=r01*r01+r11*r11+r21*r21;
        float c2=r02*r02+r12*r12+r22*r22;
        float cross=r00*r01+r10*r11+r20*r21;
        float determinant=r00*(r11*r22-r12*r21)
            -r01*(r10*r22-r12*r20)
            +r02*(r10*r21-r11*r20);
        if (Math.abs(c0-1f)>.03f || Math.abs(c1-1f)>.03f
            || Math.abs(c2-1f)>.03f || Math.abs(cross)>.03f
            || determinant<.97f || determinant>1.03f) return false;

        pose[0]=r00; pose[1]=r01; pose[2]=r02;
        pose[4]=r10; pose[5]=r11; pose[6]=r12;
        pose[8]=r20; pose[9]=r21; pose[10]=r22;
        pose[12]=-(r00*tx+r10*ty+r20*tz);
        pose[13]=-(r01*tx+r11*ty+r21*tz);
        pose[14]=-(r02*tx+r12*ty+r22*tz);
        return true;
    }
}
