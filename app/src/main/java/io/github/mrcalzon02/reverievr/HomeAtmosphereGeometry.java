package io.github.mrcalzon02.reverievr;

import java.util.Arrays;

/**
 * Low-poly, texture-free atmosphere meshes for the three headset rooms.
 * All recipes run once during renderer construction; no per-eye allocations,
 * external textures, procedural randomness at runtime or camera-facing UI.
 */
final class HomeAtmosphereGeometry {
    private HomeAtmosphereGeometry() {}

    /** Four-sided inward sky curtain, bounded inside the 40m far plane. */
    static float[] skyBand(float bottom, float top) {
        Builder b = new Builder();
        float h = 25f;
        b.verticalZ(-h, h, bottom, top, -h);
        b.verticalZ(-h, h, bottom, top, h);
        b.verticalX(-h, h, bottom, top, -h);
        b.verticalX(h, h, bottom, top, -h);
        return b.finish();
    }

    static float[] distantHills(boolean forest) {
        Builder b = new Builder();
        // Scenic horizon at the terrain edge; three stepped parallax ranges.
        if (forest) {
            b.hill(-25f, -22f, 0.55f, -13f, 1.90f, -3f, .75f);
            b.hill(-7f, -22.2f, .15f, 5f, 1.45f, 18f, .45f);
            b.hill(9f, -22.35f, .30f, 20f, 1.25f, 25f, .15f);
        } else {
            // Asymmetric headland; leave most of the ocean horizon open.
            b.hill(-25f, -23.8f, -.40f, -16f, .60f, -9f, -.70f);
            b.hill(-23f, -23.6f, -.60f, -11f, -.65f, -5f, -.85f);
        }
        return b.finish();
    }

    static float[] highClouds() {
        Builder b = new Builder();
        // Blocky distant clouds, distinct from the solid sky color bands.
        b.verticalZ(-18f, -12f, 6.1f, 6.65f, -24.7f);
        b.verticalZ(-16f, -9f, 6.45f, 6.95f, -24.65f);
        b.verticalZ(6f, 15f, 8.3f, 8.7f, -24.7f);
        b.verticalZ(8f, 12f, 8.65f, 9.05f, -24.65f);
        return b.finish();
    }

    static float[] sun() {
        Builder b = new Builder();
        b.disc(11.7f, 7.3f, -24.5f, .85f, 12);
        return b.finish();
    }

    /**
     * Actual rear-wall window apertures (not overpainted wall panels).
     * Rear wall already stands at -4.1 m after the original room closure.
     */
    static float[] windowedRearWall() {
        Builder b=new Builder();
        float z=-4.1f;
        b.verticalZ(-6f,-5.34f,-1.35f,4f,z);
        b.verticalZ(-5.34f,-3.29f,-1.35f,.03f,z);
        b.verticalZ(-5.34f,-3.29f,2.45f,4f,z);
        b.verticalZ(-3.29f,3.34f,-1.35f,4f,z);
        b.verticalZ(3.34f,5.39f,-1.35f,.03f,z);
        b.verticalZ(3.34f,5.39f,2.45f,4f,z);
        b.verticalZ(5.39f,6f,-1.35f,4f,z);
        return b.finish();
    }

    static float[] windowSky() {
        Builder b = new Builder();
        // Inset picture-like openings on the rear birch wall (z=-4.1).
        // The wall remains sealed; the opaque scenic panes are at -4.065.
        for (float left : new float[]{-5.34f, 3.34f}) {
            b.verticalZ(left, left+2.05f, .03f, 2.45f, -4.19f);
        }
        return b.finish();
    }

    static float[] windowHorizon() {
        Builder b = new Builder();
        for (float left : new float[]{-5.34f, 3.34f}) {
            b.verticalZ(left, left+2.05f, .03f, .56f, -4.177f);
            b.triangle(left, .56f,-4.175f, left+.54f, 1.12f,-4.175f,
                       left+1.0f,.56f,-4.175f);
            b.triangle(left+.80f,.56f,-4.175f, left+1.52f, 1.04f,-4.175f,
                       left+2.05f,.56f,-4.175f);
        }
        return b.finish();
    }

    static float[] windowFrames() {
        Builder b = new Builder();
        for (float left : new float[]{-5.34f, 3.34f}) {
            float right = left+2.05f, z=-4.034f;
            b.verticalZ(left-.09f,right+.09f,2.45f,2.53f,z);
            b.verticalZ(left-.09f,right+.09f,-.05f,.03f,z);
            b.verticalZ(left-.09f,left,.03f,2.45f,z);
            b.verticalZ(right,right+.09f,.03f,2.45f,z);
            b.verticalZ(left+1.005f,left+1.045f,.03f,2.45f,z);
            b.verticalZ(left,right,1.17f,1.21f,z);
            // Low-poly sill protrudes into the room, no transparent sorting.
            b.horizontal(left-.14f,right+.14f,-4.11f,-3.89f,-.04f);
        }
        return b.finish();
    }

    static float[] woodlandWildlife() {
        Builder b = new Builder();
        // A pair of quiet rabbits near the forest edge, and a distant deer.
        rabbit(b, -2.7f,-4.3f, .72f);
        rabbit(b, 3.5f,-6.0f, .56f);
        deer(b, 5.0f,-10.5f, .90f);
        return b.finish();
    }

    static float[] shoreWildlife() {
        Builder b = new Builder();
        crab(b,-2.0f,-4.6f, .72f);
        crab(b, 4.2f,-5.0f, .65f);
        return b.finish();
    }

    static float[] flyingBird() {
        Builder b = new Builder();
        // Wing-tips are faceted triangles: six triangles, no alpha blending.
        b.triangle(-.86f,.08f,0f,-.12f,.02f,0f,-.43f,-.16f,0f);
        b.triangle(.86f,.08f,0f,.12f,.02f,0f,.43f,-.16f,0f);
        b.triangle(-.13f,.02f,0f,.13f,.02f,0f,0f,-.11f,0f);
        b.triangle(-.86f,.08f,0f,-.58f,.19f,0f,-.43f,-.16f,0f);
        b.triangle(.86f,.08f,0f,.58f,.19f,0f,.43f,-.16f,0f);
        b.triangle(-.11f,-.07f,0f,.11f,-.07f,0f,0f,-.18f,0f);
        return b.finish();
    }

    private static void rabbit(Builder b,float x,float z,float s) {
        float y=-1.28f;
        b.facing(x,z,s*.36f,y,y+s*.31f);
        b.facing(x-s*.15f,z,s*.10f,y+s*.20f,y+s*.63f);
        b.facing(x+s*.12f,z,s*.09f,y+s*.25f,y+s*.68f);
        b.facing(x+s*.28f,z,s*.18f,y+s*.11f,y+s*.37f);
    }

    private static void deer(Builder b,float x,float z,float s) {
        float y=-1.32f;
        b.facing(x,z,s*.60f,y+s*.63f,y+s*1.10f);
        b.facing(x+s*.53f,z,s*.14f,y+s*.83f,y+s*1.67f);
        b.facing(x+s*.69f,z,s*.19f,y+s*1.46f,y+s*1.70f);
        for(float dx:new float[]{-.42f,-.15f,.26f,.47f})
            b.facing(x+s*dx,z,s*.08f,y,y+s*.78f);
        b.facing(x+s*.65f,z,s*.035f,y+s*1.6f,y+s*1.99f);
        b.facing(x+s*.78f,z,s*.035f,y+s*1.6f,y+s*1.89f);
    }

    private static void crab(Builder b,float x,float z,float s) {
        float y=-1.29f;
        b.facing(x,z,s*.30f,y,y+s*.22f);
        for(int side:new int[]{-1,1}) {
            b.facing(x+side*s*.37f,z,s*.16f,y+s*.02f,y+s*.08f);
            b.facing(x+side*s*.56f,z,s*.12f,y+s*.12f,y+s*.32f);
        }
    }

    static int triangleCount(float[] mesh) {
        return mesh == null || mesh.length%9!=0 ? -1 : mesh.length/9;
    }

    private static final class Builder {
        private float[] v = new float[256];
        private int used;
        private void vertex(float x,float y,float z) {
            if(used+3>v.length)v=Arrays.copyOf(v,v.length*2);
            v[used++]=x;v[used++]=y;v[used++]=z;
        }
        private void triangle(float ax,float ay,float az,float bx,float by,float bz,
                              float cx,float cy,float cz) {
            vertex(ax,ay,az);vertex(bx,by,bz);vertex(cx,cy,cz);
        }
        private void verticalZ(float l,float r,float low,float high,float z) {
            triangle(l,low,z,r,low,z,r,high,z);
            triangle(l,low,z,r,high,z,l,high,z);
        }
        private void verticalX(float x,float back,float low,float high,float front) {
            triangle(x,low,back,x,low,front,x,high,front);
            triangle(x,low,back,x,high,front,x,high,back);
        }
        private void horizontal(float l,float r,float back,float front,float y) {
            triangle(l,y,back,r,y,back,r,y,front);
            triangle(l,y,back,r,y,front,l,y,front);
        }
        private void hill(float left,float z,float edge,float peakX,float peakY,
                          float right,float endY) {
            float bottom=-1.35f;
            triangle(left,bottom,z,peakX,peakY,z,peakX,bottom,z);
            triangle(peakX,bottom,z,peakX,peakY,z,right,endY,z);
            triangle(peakX,bottom,z,right,endY,z,right,bottom,z);
        }
        private void disc(float x,float y,float z,float radius,int count) {
            for(int i=0;i<count;i++) {
                double a=Math.PI*2.0*i/count,b=Math.PI*2.0*(i+1)/count;
                triangle(x,y,z,x+radius*(float)Math.cos(a),
                         y+radius*(float)Math.sin(a),z,
                         x+radius*(float)Math.cos(b),
                         y+radius*(float)Math.sin(b),z);
            }
        }
        private void facing(float x,float z,float width,float low,float top) {
            float len=(float)Math.sqrt(x*x+z*z);
            float rx=len<.01f?1f:z/len,rz=len<.01f?0f:-x/len;
            float lx=x-rx*width, lz=z-rz*width;
            float hx=x+rx*width, hz=z+rz*width;
            triangle(lx,low,lz,hx,low,hz,hx,top,hz);
            triangle(lx,low,lz,hx,top,hz,lx,top,lz);
        }
        private float[] finish() {
            float[] done=Arrays.copyOf(v,used);
            for(int i=0;i<done.length;i++)
                done[i]=RetroPrimitiveRecipe.snapPosition(done[i]);
            return done;
        }
    }
}
