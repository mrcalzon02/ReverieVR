package io.github.mrcalzon02.reverievr;
import org.junit.Test;
import static org.junit.Assert.*;
public class HomeAvatarProfileTest {
    @Test public void twelvePresetIdentitiesSaveAndWrap() {
        HomeAvatarProfile p=HomeAvatarProfile.DEFAULT;
        for(int i=0;i<12;i++) {
            assertEquals(i,p.theme);
            assertEquals(p.encode(),HomeAvatarProfile.decode(p.encode()).encode());
            p=p.step(0,1);
        }
        assertEquals(HomeAvatarProfile.DEFAULT.encode(),p.encode());
    }
    @Test public void fourCustomizationsCycleSeparately() {
        HomeAvatarProfile p=HomeAvatarProfile.DEFAULT;
        for(int i=1;i<=4;i++)p=p.step(i,-1);
        assertEquals(7,p.color);assertEquals(7,p.hat);
        assertEquals(7,p.extra);assertEquals(3,p.eyes);
        assertEquals(p.encode(),HomeAvatarProfile.decode(p.encode()).encode());
    }
    @Test public void invalidSaveFallsBackSafely() {
        for(String s:new String[]{"","v2:0:0:0:0:0","v1:12:0:0:0:0",
            "v1:x:0:0:0:0","v1:1:2:3","v1:1:2:3:4:5:6"})
            assertEquals(HomeAvatarProfile.DEFAULT.encode(),
                HomeAvatarProfile.decode(s).encode());
    }
    @Test public void cabinetRayAndMeshes() {
        assertTrue(HomeAvatarWardrobeGeometry.hitDistance(
            new float[]{0,0,0},new float[]{1,0,-.9f})>0);
        assertEquals(-1,HomeAvatarWardrobeGeometry.hitDistance(
            new float[]{0,0,0},new float[]{-1,0,-.9f}),0);
        assertEquals(-1,HomeAvatarWardrobeGeometry.hitDistance(
            new float[]{0,0,0},new float[]{1,0,1}),0);
        float[][] meshes={HomeAvatarWardrobeGeometry.wardrobeCase(),
            HomeAvatarWardrobeGeometry.wardrobeDoor(),
            HomeAvatarWardrobeGeometry.wardrobeTrim(),
            HomeAvatarWardrobeGeometry.wardrobeHandles(),
            HomeAvatarWardrobeGeometry.legs(),
            HomeAvatarWardrobeGeometry.head()};
        for(float[] mesh:meshes)verify(mesh);
        for(int i=0;i<8;i++) {
            verify(HomeAvatarWardrobeGeometry.headwear(i));
            verify(HomeAvatarWardrobeGeometry.accessory(i));
        }
    }
    private void verify(float[] mesh) {
        assertEquals(0,mesh.length%9);
        for(float v:mesh)assertTrue(Float.isFinite(v));
    }
}
