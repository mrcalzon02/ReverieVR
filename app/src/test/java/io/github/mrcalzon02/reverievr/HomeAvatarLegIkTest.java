package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class HomeAvatarLegIkTest {
    private static float distance(float[] a,int p,int q) {
        return (float)Math.sqrt(
            (a[p]-a[q])*(a[p]-a[q]) +
            (a[p+1]-a[q+1])*(a[p+1]-a[q+1]) +
            (a[p+2]-a[q+2])*(a[p+2]-a[q+2]));
    }
    @Test public void bothLegsRespectRigidBoneLengths() {
        HomeAvatarLegIk rig=new HomeAvatarLegIk();
        float[] j=new float[HomeAvatarLegIk.JOINT_FLOATS];
        for(int frame=0;frame<45;frame++) {
            rig.update(frame*.018f,frame*.012f,
                1_000_000_000L+frame*20_000_000L,true);
            rig.copyJoints(j);
            for(int i=0;i<2;i++) {
                int p=i*9;
                assertEquals(HomeAvatarLegIk.THIGH_METERS,
                    distance(j,p,p+3),.0005f);
                assertEquals(HomeAvatarLegIk.SHIN_METERS,
                    distance(j,p+3,p+6),.0005f);
                assertTrue(j[p+3]<j[p]); // knee bends toward viewer
            }
        }
    }
    @Test public void movingLegsAlternateAndStanceStaysGrounded() {
        HomeAvatarLegIk rig=new HomeAvatarLegIk();
        float[] before=new float[HomeAvatarLegIk.JOINT_FLOATS];
        float[] after=new float[HomeAvatarLegIk.JOINT_FLOATS];
        rig.copyJoints(before);
        for(int i=0;i<30;i++)
            rig.update(i*.018f,0f,1_000_000_000L+i*20_000_000L,true);
        rig.copyJoints(after);
        assertTrue(rig.motionBlend()>.5f);
        assertTrue(Math.abs(before[8]-after[8])>.005f
            ||Math.abs(before[17]-after[17])>.005f);
        assertTrue(Math.abs(after[7]+.79f)<.11f);
        assertTrue(Math.abs(after[16]+.79f)<.11f);
    }
    @Test public void movementStopsAtBoundsWithoutFakeWalking() {
        HomeAvatarLegIk rig=new HomeAvatarLegIk();
        for(int i=0;i<30;i++)
            rig.update(i*.010f,0f,1_000_000_000L+i*20_000_000L,true);
        for(int i=30;i<70;i++)
            rig.update(.29f,0f,1_000_000_000L+i*20_000_000L,true);
        assertTrue(rig.motionBlend()<.01f);
        float[] mesh=new float[HomeAvatarLegIk.MESH_FLOATS];
        rig.writeMesh(mesh);
        for(float f:mesh)assertTrue(Float.isFinite(f));
    }
    @Test public void ignoresInvalidPosesAndRecoversAfterSceneChange() {
        HomeAvatarLegIk rig=new HomeAvatarLegIk();
        float[] a=new float[HomeAvatarLegIk.JOINT_FLOATS];
        float[] b=new float[HomeAvatarLegIk.JOINT_FLOATS];
        rig.copyJoints(a);
        rig.update(Float.NaN,0f,1000L,true);
        rig.copyJoints(b);
        assertArrayEquals(a,b,0f);
        rig.update(0f,0f,1_000_000_000L,true);
        rig.update(100f,0f,1_020_000_000L,true); // teleport: no swing
        assertEquals(0f,rig.motionBlend(),.00001f);
        rig.update(0f,0f,1_040_000_000L,false);
        assertEquals(0f,rig.motionBlend(),0f);
    }
}
