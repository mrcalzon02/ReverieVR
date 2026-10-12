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

    /** Two angular wall-panel motifs, built from a handful of hard-edged facets. */
    static float[] birchGraphicMotifs() {
        Builder b = new Builder();
        for (float center : new float[]{-4.25f, 4.25f}) {
            // Mountains / sun rays: intentionally flat, not photo textures.
            b.triangle(center - .65f, 1.04f, -5.942f,
                center - .12f, 1.72f, -5.942f,
                center + .18f, 1.04f, -5.942f);
            b.triangle(center - .16f, 1.04f, -5.942f,
                center + .40f, 1.96f, -5.942f,
                center + .66f, 1.04f, -5.942f);
            b.triangle(center - .48f, 2.12f, -5.941f,
                center - .37f, 2.28f, -5.941f,
                center - .26f, 2.12f, -5.941f);
            // Faceted sunrise crest: another twelve triangles across
            // both murals without bitmaps or runtime geometry work.
            b.triangle(center + .28f, 1.93f, -5.941f,
                center + .40f, 2.08f, -5.941f,
                center + .52f, 1.93f, -5.941f);
            b.triangle(center + .30f, 2.08f, -5.941f,
                center + .40f, 2.18f, -5.941f,
                center + .50f, 2.08f, -5.941f);
            for (int ray=0;ray<4;ray++) {
                float x = center - .18f + ray*.20f;
                b.triangle(x-.03f,2.30f,-5.941f,
                    x,2.39f,-5.941f,
                    x+.03f,2.30f,-5.941f);
            }
        }
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

    /**
     * Fixed-seed fallen logs sit outside the Home gaze/keyboard corridor.
     * Cardboard trunks need only five or six faces, never cylinders.
     */
    static float[] forestFallenWood(int seed) {
        Builder b = new Builder();
        float[][] positions = {
            {-6.0f, -5.4f}, {5.8f, 4.5f}, {-7.1f, 2.2f}, {6.9f, -4.8f}
        };
        for (int i = 0; i < positions.length; i++) {
            float x = positions[i][0] + RetroPrimitiveRecipe.offset(seed, i);
            float z = positions[i][1] + RetroPrimitiveRecipe.offset(seed, i + 19);
            // Six triangles per log. Flat, low-slung and deliberately angular.
            b.horizontal(x - .76f, x + .76f, z - .16f, z + .16f, -1.16f);
            b.verticalZ(x - .76f, x + .76f, -1.34f, -1.16f, z + .16f);
            b.triangle(x - .76f, -1.34f, z - .16f,
                x - .76f, -1.16f, z, x - .76f, -1.34f, z + .16f);
            b.triangle(x + .76f, -1.34f, z - .16f,
                x + .76f, -1.34f, z + .16f, x + .76f, -1.16f, z);
        }
        return b.finish();
    }

    static float[] forestWildflowers() {
        Builder b = new Builder();
        for (int i = 0; i < 24; i++) {
            float theta = i * 2.399963f + .35f;
            float radius = 5.95f + (i % 5) * .58f;
            float x = (float)Math.cos(theta) * radius;
            float z = (float)Math.sin(theta) * radius;
            float y = -.91f + (i % 3) * .07f;
            b.facing(x, z, .075f, y - .08f, y + .08f);
            b.triangle(x - .10f, y, z, x + .10f, y, z,
                x, y + .11f, z);
        }
        // Golden, flat-faceted mushrooms near the tree ring, away from
        // the desk and central controller/menu rays.
        for (int i=0;i<8;i++) {
            float angle=i*.785398f+.28f;
            float radius=6.18f+(i%2)*1.24f;
            float x=(float)Math.cos(angle)*radius;
            float z=(float)Math.sin(angle)*radius;
            b.facing(x,z,.040f,-1.33f,-1.08f);
            b.triangle(x-.18f,-1.10f,z,
                       x,-.97f,z,
                       x+.18f,-1.10f,z);
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
            float r = 6.05f + (i % 5) * .64f;
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
            float r = 6.10f + (i % 5) * .66f;
            float x = (float)Math.cos(theta) * r;
            float z = (float)Math.sin(theta) * r;
            b.triangle(x - .12f, -1.336f, z - .10f,
                x + .12f, -1.336f, z - .10f,
                x, -1.332f, z + .13f);
        }
        return b.finish();
    }

    /** Fixed-seed, three-face tide stones with no bitmap or normal map. */
    static float[] beachRockClusters(int seed) {
        Builder b = new Builder();
        float[][] positions = {
            {-6.6f,-3.6f}, {-7.8f,-4.8f}, {-5.7f,2.5f},
            {6.4f,-3.4f}, {8.1f,-5.0f}, {5.9f,2.3f}
        };
        for (int i = 0; i < positions.length; i++) {
            float x = positions[i][0] + RetroPrimitiveRecipe.offset(seed, i);
            float z = positions[i][1] + RetroPrimitiveRecipe.offset(seed, i + 35);
            float r = .24f + (i % 3) * .06f;
            b.triangle(x-r,-1.34f,z-.16f,
                x,-1.03f,z-.11f, x+r,-1.34f,z-.16f);
            b.triangle(x+r,-1.34f,z-.16f,
                x,-1.03f,z-.11f, x+.09f,-1.34f,z+.19f);
            b.triangle(x+.09f,-1.34f,z+.19f,
                x,-1.03f,z-.11f, x-r,-1.34f,z-.16f);
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
        // Eight shallow tidal pools at the flanks of the beach.
        // These share the sea's existing pale-blue detail mesh/material.
        for(int side : new int[]{-1,1}) {
            for(int i=0;i<4;i++) {
                float x=side*(5.15f+(i%2)*.60f);
                float z=-4.95f-(i/2)*1.33f;
                b.horizontal(x-.37f,x+.37f,z-.20f,z+.20f,-1.329f);
            }
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
        float[] finish() {
            float[] packed = Arrays.copyOf(values, used);
            // PS1-like coarse vertex positions are baked once during boot,
            // never snapped separately for each Cardboard eye or frame.
            for (int i = 0; i < packed.length; i++)
                packed[i] = RetroPrimitiveRecipe.snapPosition(packed[i]);
            return packed;
        }
    }
}
