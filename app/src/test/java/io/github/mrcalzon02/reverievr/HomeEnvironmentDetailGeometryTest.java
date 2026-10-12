package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public final class HomeEnvironmentDetailGeometryTest {
    private static final float[][] WHITE = {
        HomeEnvironmentDetailGeometry.birchTrim(),
        HomeEnvironmentDetailGeometry.birchWallArt(),
        HomeEnvironmentDetailGeometry.birchShelfAccents(),
        HomeEnvironmentDetailGeometry.birchGraphicMotifs()
    };
    private static final float[][] FOREST = {
        HomeEnvironmentDetailGeometry.forestLayeredFoliage(),
        HomeEnvironmentDetailGeometry.forestStones(),
        HomeEnvironmentDetailGeometry.forestWildflowers(),
        HomeEnvironmentDetailGeometry.forestFallenWood(0x4f72)
    };
    private static final float[][] BEACH = {
        HomeEnvironmentDetailGeometry.beachSandRipples(),
        HomeEnvironmentDetailGeometry.beachGrassTufts(),
        HomeEnvironmentDetailGeometry.beachShells(),
        HomeEnvironmentDetailGeometry.beachWavelets(),
        HomeEnvironmentDetailGeometry.beachRockClusters(0x6a35)
    };

    private static void verify(String theme,float[][] meshes,int maxTriangles) {
        int count = 0;
        for (float[] mesh : meshes) {
            assertTrue(theme+" mesh has content", mesh.length > 0);
            assertEquals(theme+" triangles", 0, mesh.length % 9);
            count += HomeEnvironmentDetailGeometry.triangles(mesh);
            for (int i = 0; i < mesh.length; i += 3) {
                float x=mesh[i], y=mesh[i+1], z=mesh[i+2];
                assertTrue(theme+" finite X",Float.isFinite(x));
                assertTrue(theme+" finite Y",Float.isFinite(y));
                assertTrue(theme+" finite Z",Float.isFinite(z));
                assertTrue(theme+" world X",Math.abs(x) <= 24f);
                assertTrue(theme+" world Z",Math.abs(z) <= 24f);
                assertTrue(theme+" height",y >= -1.35f && y <= 4f);
                // Background props stay clear of the nearby center
                // headset menu, pointer origin and keyboard dais.
                assertFalse(theme+" blocked central interaction corridor",
                    Math.abs(x) < 2.3f && z > -5.25f && z < 0f
                        && y > -1.30f && y < 1.8f);
            }
        }
        assertTrue(theme+" adds visible geometry",count >= 25);
        assertTrue(theme+" bounded triangle budget: "+count,count <= maxTriangles);
    }

    @Test public void allThreeRoomsStayInsideGalaxyS9GeometryBudget() {
        verify("birch",WHITE,95);
        verify("forest",FOREST,230);
        verify("beach",BEACH,240);
    }

    @Test public void newHomeDetailsRemainInExistingBatchBudgets() {
        // The visual pass extends already-batched meshes. It must not
        // introduce another texture, draw call or runtime generator.
        assertEquals(18, HomeEnvironmentDetailGeometry.triangles(
            HomeEnvironmentDetailGeometry.birchGraphicMotifs()));
        assertEquals(96, HomeEnvironmentDetailGeometry.triangles(
            HomeEnvironmentDetailGeometry.forestWildflowers()));
        assertEquals(50, HomeEnvironmentDetailGeometry.triangles(
            HomeEnvironmentDetailGeometry.beachWavelets()));
    }

    @Test public void eachEnvironmentIsSeedlessAndDeterministic() {
        assertArrayEquals(HomeEnvironmentDetailGeometry.birchTrim(),
            HomeEnvironmentDetailGeometry.birchTrim(),0f);
        assertArrayEquals(HomeEnvironmentDetailGeometry.forestWildflowers(),
            HomeEnvironmentDetailGeometry.forestWildflowers(),0f);
        assertArrayEquals(HomeEnvironmentDetailGeometry.beachWavelets(),
            HomeEnvironmentDetailGeometry.beachWavelets(),0f);
    }

    @Test public void seededPrimitivePropsAreStableButCanVaryBySeed() {
        assertArrayEquals(HomeEnvironmentDetailGeometry.forestFallenWood(0x4f72),
            HomeEnvironmentDetailGeometry.forestFallenWood(0x4f72), 0f);
        assertArrayEquals(HomeEnvironmentDetailGeometry.beachRockClusters(0x6a35),
            HomeEnvironmentDetailGeometry.beachRockClusters(0x6a35), 0f);
        assertFalse(java.util.Arrays.equals(
            HomeEnvironmentDetailGeometry.forestFallenWood(5),
            HomeEnvironmentDetailGeometry.forestFallenWood(6)));
        assertFalse(java.util.Arrays.equals(
            HomeEnvironmentDetailGeometry.beachRockClusters(5),
            HomeEnvironmentDetailGeometry.beachRockClusters(6)));
    }

    @Test public void generatedVerticesAreCoarseAndQuantizedAtConstruction() {
        for (float[][] environment : new float[][][]{WHITE, FOREST, BEACH})
            for (float[] vertices : environment)
                for (float value : vertices)
                    assertEquals(value * RetroPrimitiveRecipe.POSITION_GRID,
                        Math.round(value * RetroPrimitiveRecipe.POSITION_GRID),
                        .0001f);
    }

    @Test public void usesOnlyCompleteTriangles() {
        assertEquals(-1,HomeEnvironmentDetailGeometry.triangles(null));
        assertEquals(-1,HomeEnvironmentDetailGeometry.triangles(new float[4]));
    }
}
