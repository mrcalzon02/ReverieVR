package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import java.util.Random;
import static org.junit.Assert.*;

public class HomeAvatarProfileTest {
    @Test public void allFortyEightThemedCharactersRoundTrip() {
        HomeAvatarProfile p=HomeAvatarProfile.DEFAULT;
        assertEquals(48,HomeAvatarProfile.THEMES.length);
        for(int i=0;i<48;i++) {
            assertEquals(i,p.theme);
            assertEquals(p.encode(),HomeAvatarProfile.decode(p.encode()).encode());
            p=p.step(0,1);
        }
        assertEquals(0,p.theme);
        assertEquals(HomeAvatarProfile.DEFAULT.height,p.height);
    }
    @Test public void v1MigrationPreservesOldSelectedCosmetics() {
        HomeAvatarProfile p=HomeAvatarProfile.decode("v1:11:7:7:7:3");
        assertEquals(11,p.theme);assertEquals(7,p.color);
        assertEquals(7,p.hat);assertEquals(7,p.extra);assertEquals(3,p.eyes);
        assertEquals("v2",p.encode().substring(0,2));
        assertEquals(p.encode(),HomeAvatarProfile.decode(p.encode()).encode());
    }
    @Test public void bodyTypesIncludeSmallAndLargeForEveryPresentation() {
        for(int gender=0;gender<HomeAvatarProfile.PRESENTATIONS.length;gender++) {
            HomeAvatarProfile base=HomeAvatarProfile.DEFAULT.step(5,gender-2)
                .step(6,-HomeAvatarProfile.DEFAULT.physique);
            assertEquals(0,base.physique);
            for(int kind=0;kind<HomeAvatarProfile.PHYSIQUES.length;kind++) {
                HomeAvatarProfile p=base;
                for(int i=0;i<kind;i++)p=p.step(6,1);
                assertEquals(base.presentation,p.presentation);
                assertEquals(HomeAvatarProfile.BODY_PRESETS[kind][0],p.height);
                assertEquals(HomeAvatarProfile.BODY_PRESETS[kind][1],p.shoulders);
                assertEquals(HomeAvatarProfile.BODY_PRESETS[kind][4],p.muscle);
            }
        }
    }
    @Test public void allFieldsCanBeEditedAndSavedIndependently() {
        HomeAvatarProfile p=HomeAvatarProfile.DEFAULT;
        for(int field=0;field<15;field++) {
            if(field==6||field==0)continue;
            HomeAvatarProfile next=p.step(field,1);
            assertEquals(Math.floorMod(p.get(field)+1,HomeAvatarProfile.count(field)),next.get(field));
            assertEquals(next.encode(),HomeAvatarProfile.decode(next.encode()).encode());
            p=next;
        }
        assertEquals(3,HomeAvatarProfile.PAGE_FIELDS.length);
        assertEquals(5,HomeAvatarProfile.PAGE_FIELDS[0].length);
    }
    @Test public void corruptedOldOrFutureSavesResetSafely() {
        for(String s:new String[]{"","v3:0:0:0:0:0","v1:48:0:0:0:0",
            "v1:x:0:0:0:0","v2:1:2:3","v1:1:2:3:4:5:6"})
            assertEquals(HomeAvatarProfile.DEFAULT.encode(),HomeAvatarProfile.decode(s).encode());
    }
    @Test public void outfitGeometryAndAvatarBoundsStayFinite() {
        for(int i=0;i<HomeAvatarProfile.OUTFITS.length;i++) {
            HomeAvatarProfile p=HomeAvatarProfile.DEFAULT;
            for(int n=0;n<i;n++)p=p.step(12,1);
            validate(HomeAvatarBodyGeometry.torso(p));
            validate(HomeAvatarBodyGeometry.arms(p));
            validate(HomeAvatarBodyGeometry.outfit(p));
            validate(HomeAvatarBodyGeometry.hair(p));
            validate(HomeAvatarBodyGeometry.face(p));
            assertTrue(p.heightScale()>.4f && p.heightScale()<.8f);
        }
        for(int i=0;i<HomeAvatarProfile.HATS.length;i++) {
            float[] mesh=HomeAvatarWardrobeGeometry.headwear(i);
            if(i!=0)assertTrue("No mesh for hat "+i,mesh.length>0);
            validate(mesh);
        }
        for(int i=0;i<HomeAvatarProfile.EXTRAS.length;i++) {
            float[] mesh=HomeAvatarWardrobeGeometry.accessory(i);
            if(i!=0)assertTrue("No mesh for accessory "+i,mesh.length>0);
            validate(mesh);
        }
    }
    @Test public void nativeProfileSnapshotMatchesSavedFieldOrder() {
        HomeAvatarProfile p=HomeAvatarProfile.DEFAULT.step(0,1)
            .step(6,4).step(11,1).step(12,2);
        int[] values=p.forNativeModules();
        assertEquals(15,values.length);
        for(int i=0;i<15;i++)assertEquals(p.get(i),values[i]);
        assertEquals(values[0],HomeAvatarProfile.decode(p.encode()).theme);
    }
    @Test public void randomizeIsValidAndNonPersistent() {
        Random r=new Random(4567);
        for(int i=0;i<128;i++) {
            HomeAvatarProfile p=HomeAvatarProfile.random(r);
            assertEquals(p.encode(),HomeAvatarProfile.decode(p.encode()).encode());
        }
    }
    @Test public void wardrobeInteractionIsWorldSpaceAndFrontFacing() {
        assertTrue(HomeAvatarWardrobeGeometry.hitDistance(
            new float[]{0f,0f,0f},new float[]{1f,0f,-.9f})>0f);
        assertEquals(-1f,HomeAvatarWardrobeGeometry.hitDistance(
            new float[]{0f,0f,0f},new float[]{-1f,0f,-.9f}),0f);
    }
    private static void validate(float[] mesh) {
        assertEquals(0,mesh.length%9);
        for(float value:mesh)assertTrue(Float.isFinite(value));
    }
}
