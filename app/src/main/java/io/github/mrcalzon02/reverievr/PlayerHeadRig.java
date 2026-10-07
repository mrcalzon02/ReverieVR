package io.github.mrcalzon02.reverievr;

/**
 * Shell-owned, allocation-free headset proxy. The center-head view and
 * calibrated IPD define a mirror-ready wireframe and oriented-box collider.
 * It does not alter Cardboard's optical matrices or provide room safety.
 */
final class PlayerHeadRig {
    static final float HALF_WIDTH = 0.110f;
    static final float HALF_HEIGHT = 0.085f;
    static final float HALF_DEPTH = 0.085f;
    static final float CENTER_BACK = 0.020f;
    static final float EYE_MARKER_RADIUS = 0.012f;
    static final int WIREFRAME_VERTICES = 36;
    static final int WIREFRAME_FLOATS = WIREFRAME_VERTICES * 3;

    private final float[] center = new float[3];
    private final float[] axisX = new float[3];
    private final float[] axisY = new float[3];
    private final float[] axisZ = new float[3];
    private float ipd;
    private boolean valid;

    boolean update(float[] view, float ipdMeters) {
        if (view == null || view.length < 16
            || !Float.isFinite(ipdMeters)
            || ipdMeters < 0.045f || ipdMeters > 0.085f) {
            valid = false;
            return false;
        }
        for (int i = 0; i < 16; i++) {
            if (!Float.isFinite(view[i])) {
                valid = false;
                return false;
            }
        }
        if (Math.abs(view[3]) > 0.001f
            || Math.abs(view[7]) > 0.001f
            || Math.abs(view[11]) > 0.001f
            || Math.abs(view[15] - 1.0f) > 0.001f) {
            valid = false;
            return false;
        }
        float xx = view[0]*view[0] + view[4]*view[4] + view[8]*view[8];
        float yy = view[1]*view[1] + view[5]*view[5] + view[9]*view[9];
        float zz = view[2]*view[2] + view[6]*view[6] + view[10]*view[10];
        float xy = view[0]*view[1] + view[4]*view[5] + view[8]*view[9];
        float xz = view[0]*view[2] + view[4]*view[6] + view[8]*view[10];
        float yz = view[1]*view[2] + view[5]*view[6] + view[9]*view[10];
        float determinant =
            view[0]*(view[5]*view[10] - view[9]*view[6])
            + view[4]*(view[9]*view[2] - view[1]*view[10])
            + view[8]*(view[1]*view[6] - view[5]*view[2]);
        if (Math.abs(xx-1.0f)>0.02f || Math.abs(yy-1.0f)>0.02f
            || Math.abs(zz-1.0f)>0.02f || Math.abs(xy)>0.02f
            || Math.abs(xz)>0.02f || Math.abs(yz)>0.02f
            || determinant < 0.98f || determinant > 1.02f) {
            valid = false;
            return false;
        }

        // Invert the rigid world-to-head view without an Android dependency.
        axisX[0]=view[0]; axisX[1]=view[4]; axisX[2]=view[8];
        axisY[0]=view[1]; axisY[1]=view[5]; axisY[2]=view[9];
        axisZ[0]=view[2]; axisZ[1]=view[6]; axisZ[2]=view[10];
        center[0]=-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]);
        center[1]=-(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]);
        center[2]=-(view[8]*view[12]+view[9]*view[13]+view[10]*view[14]);
        ipd=ipdMeters;
        valid=true;
        return true;
    }

    void reset() { valid=false; }
    boolean isValid() { return valid; }

    boolean copyHeadCenter(float[] out) {
        if (!valid || out == null || out.length < 3) return false;
        System.arraycopy(center, 0, out, 0, 3);
        return true;
    }

    boolean copyEyeCenters(float[] out) {
        if (!valid || out == null || out.length < 6) return false;
        for (int eye=0; eye<2; eye++) {
            float side = eye==0 ? -0.5f : 0.5f;
            for (int axis=0; axis<3; axis++) {
                out[eye*3+axis]=center[axis]+axisX[axis]*side*ipd;
            }
        }
        return true;
    }

    /** Exact sphere-vs-oriented-box collision probe for nearby tools. */
    boolean intersectsSphere(float x, float y, float z, float radius) {
        if (!valid || !Float.isFinite(x) || !Float.isFinite(y)
            || !Float.isFinite(z) || !Float.isFinite(radius)
            || radius < 0.0f) return false;
        float dx=x-center[0], dy=y-center[1], dz=z-center[2];
        float lx=dx*axisX[0]+dy*axisX[1]+dz*axisX[2];
        float ly=dx*axisY[0]+dy*axisY[1]+dz*axisY[2];
        float lz=dx*axisZ[0]+dy*axisZ[1]+dz*axisZ[2];
        float ex=Math.max(0.0f,Math.abs(lx)-HALF_WIDTH);
        float ey=Math.max(0.0f,Math.abs(ly)-HALF_HEIGHT);
        float ez=Math.max(0.0f,Math.abs(lz-CENTER_BACK)-HALF_DEPTH);
        return ex*ex+ey*ey+ez*ez <= radius*radius;
    }

    /**
     * World-space lines: 12 headset edges and two 3-axis eye markers.
     * Reserved for an external mirror/third-person pass, never the near-eye
     * forward view. The caller owns the output buffer.
     */
    int writeWireframe(float[] out) {
        if (!valid || out == null || out.length < WIREFRAME_FLOATS) return 0;
        int index=0;
        for (int y=-1; y<=1; y+=2) {
            float h=y*HALF_HEIGHT;
            index=segment(out,index,-HALF_WIDTH,h,CENTER_BACK-HALF_DEPTH,
                HALF_WIDTH,h,CENTER_BACK-HALF_DEPTH);
            index=segment(out,index,HALF_WIDTH,h,CENTER_BACK-HALF_DEPTH,
                HALF_WIDTH,h,CENTER_BACK+HALF_DEPTH);
            index=segment(out,index,HALF_WIDTH,h,CENTER_BACK+HALF_DEPTH,
                -HALF_WIDTH,h,CENTER_BACK+HALF_DEPTH);
            index=segment(out,index,-HALF_WIDTH,h,CENTER_BACK+HALF_DEPTH,
                -HALF_WIDTH,h,CENTER_BACK-HALF_DEPTH);
        }
        for (int x=-1; x<=1; x+=2) {
            for (int z=-1; z<=1; z+=2) {
                index=segment(out,index,
                    x*HALF_WIDTH,-HALF_HEIGHT,CENTER_BACK+z*HALF_DEPTH,
                    x*HALF_WIDTH,HALF_HEIGHT,CENTER_BACK+z*HALF_DEPTH);
            }
        }
        for (int eye=-1; eye<=1; eye+=2) {
            float x=eye*ipd*0.5f;
            index=segment(out,index,x-EYE_MARKER_RADIUS,0,0,
                x+EYE_MARKER_RADIUS,0,0);
            index=segment(out,index,x,-EYE_MARKER_RADIUS,0,
                x,EYE_MARKER_RADIUS,0);
            index=segment(out,index,x,0,-EYE_MARKER_RADIUS,
                x,0,EYE_MARKER_RADIUS);
        }
        return index/3;
    }

    private int segment(float[] out,int i,
        float ax,float ay,float az,float bx,float by,float bz) {
        return vertex(out,vertex(out,i,ax,ay,az),bx,by,bz);
    }

    private int vertex(float[] out,int i,float x,float y,float z) {
        out[i++]=center[0]+axisX[0]*x+axisY[0]*y+axisZ[0]*z;
        out[i++]=center[1]+axisX[1]*x+axisY[1]*y+axisZ[1]*z;
        out[i++]=center[2]+axisX[2]*x+axisY[2]*y+axisZ[2]*z;
        return i;
    }
}
