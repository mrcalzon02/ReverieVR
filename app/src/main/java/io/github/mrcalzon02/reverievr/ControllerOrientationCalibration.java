package io.github.mrcalzon02.reverievr;

/** Allocation-free 3D aim calibration for a tracked controller and headset. */
final class ControllerOrientationCalibration {
    private ControllerOrientationCalibration() {}

    // headView contains the world-to-head rotation. The target rotation
    // is its transpose; correction = target * inverse(controller rotation).
    static boolean compute(float[] headView, float x, float y, float z, float w,
                           float[] out) {
        if (headView == null || headView.length < 16 || out == null || out.length < 16)
            return false;
        for (int i = 0; i < 16; i++) if (!Float.isFinite(headView[i])) return false;
        if (!Float.isFinite(x) || !Float.isFinite(y)
                || !Float.isFinite(z) || !Float.isFinite(w)) return false;
        float norm = (float) Math.sqrt(x*x + y*y + z*z + w*w);
        if (!Float.isFinite(norm) || norm < 0.0001f) return false;
        x /= norm; y /= norm; z /= norm; w /= norm;
        float xx=x*x, yy=y*y, zz=z*z, xy=x*y, xz=x*z, yz=y*z;
        float wx=w*x, wy=w*y, wz=w*z;
        // Controller's rotation, row-major 3x3.
        float[] r = {
            1-2*(yy+zz), 2*(xy-wz), 2*(xz+wy),
            2*(xy+wz), 1-2*(xx+zz), 2*(yz-wx),
            2*(xz-wy), 2*(yz+wx), 1-2*(xx+yy)
        };
        // For a column-major world-to-view matrix, the inverse rotation
        // is its transpose. Row i col k of head-world = view row k col i.
        for (int row=0; row<3; row++) {
            for (int col=0; col<3; col++) {
                float sum=0f;
                for (int k=0; k<3; k++) {
                    float headWorld = headView[row*4+k];
                    float controllerInverse = r[col*3+k];
                    sum += headWorld * controllerInverse;
                }
                out[col*4+row]=sum;
            }
        }
        out[3]=out[7]=out[11]=out[12]=out[13]=out[14]=0f;
        out[15]=1f;
        return true;
    }

    static void rotate(float[] correction, boolean enabled, float[] source, float[] out) {
        float x=source[0], y=source[1], z=source[2];
        if (!enabled) { out[0]=x; out[1]=y; out[2]=z; return; }
        out[0]=correction[0]*x+correction[4]*y+correction[8]*z;
        out[1]=correction[1]*x+correction[5]*y+correction[9]*z;
        out[2]=correction[2]*x+correction[6]*y+correction[10]*z;
    }
}
