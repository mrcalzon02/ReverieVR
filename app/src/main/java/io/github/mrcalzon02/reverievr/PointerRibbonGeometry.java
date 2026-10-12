package io.github.mrcalzon02.reverievr;

import java.nio.FloatBuffer;

/** GLES2-safe pointer geometry: a thin beam and two crossing target bars. */
final class PointerRibbonGeometry {
    private static final float MIN_LENGTH = 0.000001f;
    private static final float BEAM_HALF_WIDTH = 0.003f;

    private PointerRibbonGeometry() {}

    /** Writes exactly 18 vertices (54 floats), without allocating or touching GL state. */
    static void fill(FloatBuffer out, float ox, float oy, float oz,
                     float ex, float ey, float ez, float markerSize) {
        if (out == null || out.remaining() < 54) {
            throw new IllegalArgumentException("Pointer ribbon requires 54 floats");
        }
        float dx = ex - ox;
        float dy = ey - oy;
        float xyLength = (float) Math.sqrt(dx * dx + dy * dy);
        float perpendicularX = xyLength > MIN_LENGTH ? -dy / xyLength : 1f;
        float perpendicularY = xyLength > MIN_LENGTH ? dx / xyLength : 0f;
        quad(out, ox + perpendicularX * BEAM_HALF_WIDTH, oy + perpendicularY * BEAM_HALF_WIDTH, oz,
             ox - perpendicularX * BEAM_HALF_WIDTH, oy - perpendicularY * BEAM_HALF_WIDTH, oz,
             ex + perpendicularX * BEAM_HALF_WIDTH, ey + perpendicularY * BEAM_HALF_WIDTH, ez,
             ex - perpendicularX * BEAM_HALF_WIDTH, ey - perpendicularY * BEAM_HALF_WIDTH, ez);
        float halfBar = markerSize;
        float halfWidth = markerSize * 0.12f;
        quad(out, ex - halfBar, ey + halfWidth, ez, ex - halfBar, ey - halfWidth, ez,
             ex + halfBar, ey + halfWidth, ez, ex + halfBar, ey - halfWidth, ez);
        quad(out, ex - halfWidth, ey + halfBar, ez, ex - halfWidth, ey - halfBar, ez,
             ex + halfWidth, ey + halfBar, ez, ex + halfWidth, ey - halfBar, ez);
    }

    /**
     * Face a primitive cross directly along the impacted triangle plane.
     * Small camera-side bias prevents z-fighting, without a decal texture.
     * The beam ends at the exact nearest triangle intersection.
     */
    static void fillSurface(FloatBuffer out, float ox, float oy, float oz,
                            float ex, float ey, float ez, float markerSize,
                            float nx, float ny, float nz) {
        if (out == null || out.remaining() < 54)
            throw new IllegalArgumentException("Pointer ribbon requires 54 floats");
        float dx=ex-ox, dy=ey-oy;
        float xy=(float)Math.sqrt(dx*dx+dy*dy);
        float px=xy>MIN_LENGTH ? -dy/xy : 1f;
        float py=xy>MIN_LENGTH ? dx/xy : 0f;
        quad(out,ox+px*BEAM_HALF_WIDTH,oy+py*BEAM_HALF_WIDTH,oz,
            ox-px*BEAM_HALF_WIDTH,oy-py*BEAM_HALF_WIDTH,oz,
            ex+px*BEAM_HALF_WIDTH,ey+py*BEAM_HALF_WIDTH,ez,
            ex-px*BEAM_HALF_WIDTH,ey-py*BEAM_HALF_WIDTH,ez);
        float tx,ty,tz;
        if (Math.abs(ny)>.9f) {tx=1f;ty=0f;tz=0f;}
        else {
            tx=nz;ty=0f;tz=-nx;
            float inv=1f/(float)Math.sqrt(tx*tx+tz*tz);
            tx*=inv;tz*=inv;
        }
        float bx=ny*tz-nz*ty, by=nz*tx-nx*tz, bz=nx*ty-ny*tx;
        float cx=ex+nx*.012f,cy=ey+ny*.012f,cz=ez+nz*.012f;
        float w=markerSize*.12f;
        quad(out,cx-tx*markerSize-bx*w,cy-ty*markerSize-by*w,cz-tz*markerSize-bz*w,
            cx-tx*markerSize+bx*w,cy-ty*markerSize+by*w,cz-tz*markerSize+bz*w,
            cx+tx*markerSize-bx*w,cy+ty*markerSize-by*w,cz+tz*markerSize-bz*w,
            cx+tx*markerSize+bx*w,cy+ty*markerSize+by*w,cz+tz*markerSize+bz*w);
        quad(out,cx-bx*markerSize-tx*w,cy-by*markerSize-ty*w,cz-bz*markerSize-tz*w,
            cx-bx*markerSize+tx*w,cy-by*markerSize+ty*w,cz-bz*markerSize+tz*w,
            cx+bx*markerSize-tx*w,cy+by*markerSize-ty*w,cz+bz*markerSize-tz*w,
            cx+bx*markerSize+tx*w,cy+by*markerSize+ty*w,cz+bz*markerSize+tz*w);
    }

    private static void quad(FloatBuffer out,
                             float ax, float ay, float az, float bx, float by, float bz,
                             float cx, float cy, float cz, float dx, float dy, float dz) {
        vertex(out, ax, ay, az); vertex(out, bx, by, bz); vertex(out, cx, cy, cz);
        vertex(out, cx, cy, cz); vertex(out, bx, by, bz); vertex(out, dx, dy, dz);
    }

    private static void vertex(FloatBuffer out, float x, float y, float z) {
        out.put(x); out.put(y); out.put(z);
    }
}
