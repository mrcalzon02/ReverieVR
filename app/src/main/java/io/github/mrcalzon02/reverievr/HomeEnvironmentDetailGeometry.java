package io.github.mrcalzon02.reverievr;

import java.util.Arrays;

/**
 * Deterministic, allocation-once detail layer for ReverieVR's three homes.
 * Static flat-color triangles only: no textures, particles, new shaders or
 * per-frame generation. Keep the central VR menu and desk lanes clear.
 */
final class HomeEnvironmentDetailGeometry {
    private HomeEnvironmentDetailGeometry() {}

    static float[] birchTrim() {
        Builder b = new Builder();
        // Ceiling coves and a waist-high rail pick out room proportions.
        b.verticalZ(-5.94f, 5.94f, 3.65f, 3.83f, -5.952f);
        b.verticalX(-5.952f, -5.94f, 3.65f, 3.83f, 5.94f);
        b.verticalX(5.952f, -5.94f, 3.65f, 3.83f, 5.94f);
        b.verticalZ(-5.94f, 5.94f, 0.26f, 0.34f, -5.951f);
        b.verticalX(-5.950f, -5.94f, 0.26f, 0.34f, 5.94f);
        b.verticalX(5.950f, -5.94f, 0.26f, 0.34f, 5.94f);
        // Two understated art frames outside the center status display.
        for (int sign : new int[] {-1, 1}) {
            float left = sign < 0 ? -5.18f : 3.31f;
            float right = sign < 0 ? -3.31f : 5.18f;
            b.verticalZ(left, left + .09f, .62f, 2.56f, -5.948f);
            b.verticalZ(right - .09f, right, .62f, 2.56f, -5.948f);
            b.verticalZ(left, right, .62f, .71f, -5.948f);
            b.verticalZ(left, right, 2.47f, 2.56f, -5.948f);
        }
        return b.finish();
    }

    static float[] birchWallArt() {
        Builder b = new Builder();
        // Inset color panels, a little behind the light birch mouldings.
        b.verticalZ(-5.03f, -3.46f, .79f, 2.38f, -5.950f);
        b.verticalZ(3.46f, 5.03f, .79f, 2.38f, -5.950f);
        return b.finish();
    }

    static float[] birchShelfAccents() {
        Builder b = new Builder();
        // A small cluster of upright books on the left-hand console.
        for (int i = 0; i < 7; i++) {
            float left = -4.46f + i * .16f;
            b.verticalZ(left, left + .115f, -.82f,
                -.39f + (i % 3) * .065f, -3.77f);
        }
        // Compact angular ceramic lamp silhouette on the right console.
        b.verticalZ(3.42f, 3.55f, -.82f, -.38f, -2.55f);
        b.triangle(3.17f, -.37f, -2.55f, 3.80f, -.37f, -2.55f,
            3.61f, .02f, -2.55f);
        b.triangle(3.17f, -.37f, -2.55f, 3.61f, .02f, -2.55f,
            3.38f, .02f, -2.55f);
        return b.finish();
    }

    private static final float[][] TREES = {
        {-7f,-6f},{-3.5f,-8f},{0f,-9f},{4f,-8f},{7.5f,-5f},
        {8f,1f},{5f,7f},{0f,9f},{-5f,7f},{-8f,1f}
    };

    static float[] forestLayeredFoliage() {
        Builder b = new Builder();
        for (int i = 0; i < TREES.length; i++) {
            float x = TREES[i][0], z = TREES[i][1];
            // Offset under-canopy silhouettes break up single-cardboard trees.
            b.facing(x - .55f, z + .12f, 1.04f, .25f, 2.72f);
            b.facing(x + .66f, z - .10f, .95f, .55f, 2.95f);
        }
        return b.finish();
    }

    static float[] forestStones() {
        Builder b = new Builder();
        for (int i = 0; i < 22; i++) {
            float theta = i * 2.399963f;
            float radius = 5.65f + (i % 4) * .72f;
            float x = (float)Math.cos(theta) * radius;
            float z = (float)Math.sin(theta) * radius;
            float w = .16f + (i % 3) * .05f;
            // Low, faceted pairs read as mossy stones from either eye.
            b.triangle(x - w, -1.34f, z, x, -1.14f, z - .12f,
                x + w, -1.34f, z);
            b.triangle(x - w, -1.34f, z, x + w, -1.34f, z,
                x, -1.34f, z + .24f);
        }
        return b.finish();
    }

    static float[] forestWildflowers() {
        Builder b = new Builder();
        for (int i = 0; i < 24; i++) {
            float theta = i * 2.399963f + .35f;
            float radius = 5.4f + (i % 5) * .58f;
            float x = (float)Math.cos(theta) * radius;
            float z = (float)Math.sin(theta) * radius;
            float y = -.91f + (i % 3) * .07f;
            b.facing(x, z, .075f, y - .08f, y + .08f);
            b.triangle(x - .10f, y, z, x + .10f, y, z,
                x, y + .11f, z);
        }
        return b.finish();
    }

    static float[] beachSandRipples() {
        Builder b = new Builder();
        for (int side : new int[] {-1, 1}) {
            for (int i = 0; i < 12; i++) {
                float z = -4.7f + (i % 6) * 1.18f;
                float x = side * (3.45f + (i / 6) * 2.2f);
                b.horizontal(x - .46f, x + .46f,
                    z - .038f, z + .038f, -1.339f);
            }
        }
        return b.finish();
    }

    static float[] beachGrassTufts() {
        Builder b = new Builder();
        for (int i = 0; i < 20; i++) {
            float theta = i * 2.399963f;
            float r = 4.75f + (i % 5) * .64f;
            float x = (float)Math.cos(theta) * r;
            float z = (float)Math.sin(theta) * r;
            for (int blade = 0; blade < 3; blade++) {
                float spread = (blade - 1) * .19f;
                b.triangle(x + spread, -1.34f, z,
                    x + spread + .15f, -.68f + blade * .10f, z + .05f,
                    x + spread + .09f, -1.34f, z + .14f);
            }
        }
        return b.finish();
    }

    static float[] beachShells() {
        Builder b = new Builder();
        for (int i = 0; i < 26; i++) {
            float theta = i * 2.399963f + .8f;
            float r = 5.3f + (i % 5) * .66f;
            float x = (float)Math.cos(theta) * r;
            float z = (float)Math.sin(theta) * r;
            b.triangle(x - .12f, -1.336f, z - .10f,
                x + .12f, -1.336f, z - .10f,
                x, -1.332f, z + .13f);
        }
        return b.finish();
    }

    static float[] beachWavelets() {
        Builder b = new Builder();
        for (int i = 0; i < 17; i++) {
            float z = -6.6f - i * .95f;
            float x = (i % 3 - 1) * 2.9f;
            float w = 1.1f + (i % 4) * .33f;
            // Above the sea plane, with 13mm depth clearance.
            b.horizontal(x - w, x + w, z - .035f, z + .035f, -1.283f);
        }
        return b.finish();
    }

    /** Triangle count is bounded at construction, never generated per frame. */
    static int triangles(float[] vertices) {
        if (vertices == null || vertices.length % 9 != 0) return -1;
        return vertices.length / 9;
    }

    private static final class Builder {
        private float[] values = new float[256];
        private int used;

        void triangle(float ax,float ay,float az,float bx,float by,float bz,
                      float cx,float cy,float cz) {
            add(ax,ay,az); add(bx,by,bz); add(cx,cy,cz);
        }
        private void add(float x,float y,float z) {
            if (used + 3 > values.length)
                values = Arrays.copyOf(values, values.length * 2);
            values[used++] = x; values[used++] = y; values[used++] = z;
        }
        void verticalZ(float left,float right,float bottom,float top,float z) {
            triangle(left,bottom,z,right,bottom,z,right,top,z);
            triangle(left,bottom,z,right,top,z,left,top,z);
        }
        void verticalX(float x,float back,float bottom,float top,float front) {
            triangle(x,bottom,back,x,bottom,front,x,top,front);
            triangle(x,bottom,back,x,top,front,x,top,back);
        }
        void horizontal(float left,float right,float back,float front,float y) {
            triangle(left,y,back,right,y,back,right,y,front);
            triangle(left,y,back,right,y,front,left,y,front);
        }
        void facing(float x,float z,float halfWidth,float bottom,float top) {
            float len = (float)Math.sqrt(x*x+z*z);
            float rx = len < .001f ? 1f : z/len;
            float rz = len < .001f ? 0f : -x/len;
            float x0 = x-rx*halfWidth, z0 = z-rz*halfWidth;
            float x1 = x+rx*halfWidth, z1 = z+rz*halfWidth;
            triangle(x0,bottom,z0,x1,bottom,z1,x1,top,z1);
            triangle(x0,bottom,z0,x1,top,z1,x0,top,z0);
        }
        float[] finish() {return Arrays.copyOf(values,used);}
    }
}
