package io.github.mrcalzon02.reverievr;

/** One -Z-forward convention for the VR pointer ray and its virtual model. */
final class VirtualControllerAim {
    private VirtualControllerAim() {}

    static void writeDirection(float yaw, float pitch, float[] out) {
        float cosPitch = (float) Math.cos(pitch);
        out[0] = (float) Math.sin(yaw) * cosPitch;
        out[1] = (float) Math.sin(pitch);
        out[2] = -(float) Math.cos(yaw) * cosPitch;
    }

    static float modelYawDegrees(float rayYaw) {
        return (float) Math.toDegrees(-rayYaw);
    }
}
