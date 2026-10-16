package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class HomeAtmosphereGeometryTest {
    private static void mesh(float[] vertices,int limit) {
        assertNotNull(vertices);
        assertTrue("empty",vertices.length>0);
        assertEquals("triangle packing",0,vertices.length%9);
        assertTrue("triangle budget",vertices.length<=limit*9);
        for(float v:vertices) assertTrue("nonfinite",Float.isFinite(v));
    }

    @Test public void skyIsFiniteAndInsideFortyMetreFrustum() {
        for(float[] triangles:new float[][]{
            HomeAtmosphereGeometry.skyBand(-1.35f,1.5f),
            HomeAtmosphereGeometry.skyBand(1.5f,6f),
            HomeAtmosphereGeometry.skyBand(6f,14f),
            HomeAtmosphereGeometry.highClouds(),
            HomeAtmosphereGeometry.sun(),
            HomeAtmosphereGeometry.distantHills(true),
            HomeAtmosphereGeometry.distantHills(false)}) {
            mesh(triangles,40);
            for(int i=0;i<triangles.length;i+=3) {
                float x=triangles[i], y=triangles[i+1], z=triangles[i+2];
                assertTrue("out of range",x*x+y*y+z*z<1600f);
            }
        }
    }

    @Test public void windowsHaveRealLayerDepthAndReasonableTriangleBudgets() {
        float[] sky=HomeAtmosphereGeometry.windowSky();
        float[] landscape=HomeAtmosphereGeometry.windowHorizon();
        mesh(sky,12);mesh(landscape,32);
        mesh(HomeAtmosphereGeometry.windowFrames(),68);
        for(int i=2;i<sky.length;i+=3)assertEquals(-4.0625f,sky[i],.02f);
        for(int i=2;i<landscape.length;i+=3)
            assertTrue("mountain behind glass or wall",landscape[i]>-4.065f);
    }

    @Test public void crittersAndBirdsAreBoundedAndDeterministic() {
        mesh(HomeAtmosphereGeometry.woodlandWildlife(),100);
        mesh(HomeAtmosphereGeometry.shoreWildlife(),100);
        mesh(HomeAtmosphereGeometry.flyingBird(),6);
        assertEquals(6,HomeAtmosphereGeometry.triangleCount(
            HomeAtmosphereGeometry.flyingBird()));
        assertArrayEquals(HomeAtmosphereGeometry.flyingBird(),
            HomeAtmosphereGeometry.flyingBird(),0f);
    }

    @Test public void allAmbientBedsAreDistinctAndSeamSafe() {
        short[] a=HomeAmbientAudio.generate(HomeEnvironment.WHITE_ROOM);
        short[] b=HomeAmbientAudio.generate(HomeEnvironment.FOREST_GLADE);
        short[] c=HomeAmbientAudio.generate(HomeEnvironment.DUNE_BEACH);
        assertEquals(88200,a.length);assertEquals(a.length,b.length);
        assertEquals(a.length,c.length);
        assertEquals(0,a[0]);assertEquals(0,a[a.length-1]);
        assertEquals(0,b[0]);assertEquals(0,c[c.length-1]);
        assertFalse(java.util.Arrays.equals(a,b));
        assertFalse(java.util.Arrays.equals(b,c));
        assertArrayEquals(b,HomeAmbientAudio.generate(HomeEnvironment.FOREST_GLADE));
    }
}
