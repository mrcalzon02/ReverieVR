package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

/** Prevent the wardrobe avatar from quietly regressing to flat sprites. */
public final class HomeAvatarVolumeGeometryTest {
    @Test public void realDepthAndClosedSidesForEachTriangle() {
        float x=HomeAvatarWardrobeGeometry.MIRROR_X;
        float[] portrait={x,1f,-.3f, x,.4f,.3f, x,-.4f,-.3f};
        float[] volume=HomeAvatarVolumeGeometry.extrude(portrait,.12f);
        assertEquals(portrait.length*8,volume.length);
        int front=0,back=0;
        for(int i=0;i<volume.length;i+=3) {
            assertTrue(Float.isFinite(volume[i]));
            assertTrue(Float.isFinite(volume[i+1]));
            assertTrue(Float.isFinite(volume[i+2]));
            if(Math.abs(volume[i]-(x+.12f))<.0001f)++front;
            if(Math.abs(volume[i]-(x-.12f))<.0001f)++back;
        }
        assertTrue(front>=9);
        assertTrue(back>=9);
        assertTrue(front+back==volume.length/3);
    }

    @Test public void morphologiesRemainVolumetricAndReusable() {
        for(int shape=0;shape<7;shape++) {
            HomeAvatarProfile p=HomeAvatarProfile.DEFAULT
                .step(HomeAvatarProfile.FIELD_SHOULDERS,shape);
            float[] torso=HomeAvatarVolumeGeometry.extrude(
                HomeAvatarBodyGeometry.torso(p),.11f);
            assertTrue(torso.length>=72);
            float min=Float.POSITIVE_INFINITY,max=Float.NEGATIVE_INFINITY;
            for(int i=0;i<torso.length;i+=3){
                min=Math.min(min,torso[i]);max=Math.max(max,torso[i]);
            }
            assertTrue(max-min>.21f);
        }
        float[] gait=new float[HomeAvatarLegIk.MESH_FLOATS];
        new HomeAvatarLegIk().writeMesh(gait);
        float[] out=new float[gait.length*
            HomeAvatarVolumeGeometry.FLOATS_PER_INPUT_FLOAT];
        HomeAvatarVolumeGeometry.write(gait,out,.046f);
        boolean nonzero=false;
        for(float component:out){
            assertFalse(Float.isNaN(component));
            assertTrue(Float.isFinite(component));
            if(component!=0f)nonzero=true;
        }
        assertTrue(nonzero);
    }

    @Test public void rejectsBrokenOrUndersizedInputs() {
        boolean thrown=false;
        try {HomeAvatarVolumeGeometry.extrude(new float[8],.1f);}
        catch(IllegalArgumentException expected){thrown=true;}
        assertTrue(thrown);
        thrown=false;
        try {HomeAvatarVolumeGeometry.write(new float[9],new float[71],.1f);}
        catch(IllegalArgumentException expected){thrown=true;}
        assertTrue(thrown);
    }
}
