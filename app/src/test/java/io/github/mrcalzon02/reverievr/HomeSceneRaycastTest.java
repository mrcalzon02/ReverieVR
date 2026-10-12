package io.github.mrcalzon02.reverievr;
import static org.junit.Assert.*;
import org.junit.Test;

public final class HomeSceneRaycastTest {
    private static HomeSceneRaycast.Mesh surface(float z) {
        return new HomeSceneRaycast.Mesh(new float[]{
            -1,-1,z, 1,-1,z, 1,1,z,
            -1,-1,z, 1,1,z, -1,1,z
        });
    }
    @Test public void strikesRealTrianglesAndTakesNearestSurface() {
        HomeSceneRaycast.Hit hit=new HomeSceneRaycast.Hit();
        float[] o={0f,0f,0f}, dir={0f,0f,-1f};
        HomeSceneRaycast.test(surface(-6f),o,dir,hit);
        assertEquals(6f,hit.distance,.00001f);
        HomeSceneRaycast.test(surface(-2f),o,dir,hit);
        assertEquals(2f,hit.distance,.00001f);
        assertTrue("decal faces camera",hit.normalZ>0.99f);
        HomeSceneRaycast.test(surface(-5f),o,dir,hit);
        assertEquals(2f,hit.distance,.00001f);
    }
    @Test public void triangleMissCannotUseOnlyMeshBoundingBox() {
        HomeSceneRaycast.Mesh triangle=new HomeSceneRaycast.Mesh(
            new float[]{0,0,-3, 1,0,-3, 0,1,-3});
        HomeSceneRaycast.Hit hit=new HomeSceneRaycast.Hit();
        HomeSceneRaycast.test(triangle,new float[]{.9f,.9f,0},
            new float[]{0,0,-1},hit);
        assertFalse(hit.hasContact());
        HomeSceneRaycast.test(triangle,new float[]{.25f,.25f,0},
            new float[]{0,0,-1},hit);
        assertEquals(3f,hit.distance,.00001f);
    }
    @Test public void hitsBothWindingDirectionsAndVerticalFloors() {
        HomeSceneRaycast.Hit hit=new HomeSceneRaycast.Hit();
        HomeSceneRaycast.test(new HomeSceneRaycast.Mesh(new float[]{
            -1,-1,-2, 0,1,-2, 1,-1,-2}),
            new float[]{0,0,0},new float[]{0,0,-1},hit);
        assertEquals(2f,hit.distance,.00001f);
        assertTrue(hit.normalZ>0f);
        hit.reset();
        HomeSceneRaycast.test(new HomeSceneRaycast.Mesh(new float[]{
            -1,-1,-1, 1,-1,-1, 0,-1,1}),
            new float[]{0,0,0},new float[]{0,-1,0},hit);
        assertEquals(1f,hit.distance,.00001f);
        assertTrue(hit.normalY>0f);
    }
    @Test public void rejectsBadMeshesAndDirections() {
        assertThrows(IllegalArgumentException.class,
            ()->new HomeSceneRaycast.Mesh(new float[10]));
        assertThrows(IllegalArgumentException.class,
            ()->new HomeSceneRaycast.Mesh(new float[]{0,0,Float.NaN,1,0,0,0,1,0}));
        HomeSceneRaycast.Hit hit=new HomeSceneRaycast.Hit();
        HomeSceneRaycast.test(surface(-2),new float[]{0,0,0},
            new float[]{Float.NaN,0,-1},hit);
        assertFalse(hit.hasContact());
        HomeSceneRaycast.test(surface(-2),new float[]{0,0,0},
            new float[]{0,0,1},hit);
        assertFalse(hit.hasContact());
    }
    @Test public void explicitReachLimitAndResetAreStable() {
        HomeSceneRaycast.Hit hit=new HomeSceneRaycast.Hit();
        HomeSceneRaycast.test(surface(-13),new float[]{0,0,0},
            new float[]{0,0,-1},hit);
        assertFalse(hit.hasContact());
        HomeSceneRaycast.test(surface(-4),new float[]{0,0,0},
            new float[]{0,0,-1},hit);
        assertTrue(hit.hasContact());
        hit.reset();
        assertFalse(hit.hasContact());
    }
}
