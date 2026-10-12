package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

/** Regression for the full pose/view boundary: yaw, pitch AND roll. */
public final class HeadPoseViewTest {
    private static final float EPS=0.0003f;

    private static float[] identity() {
        return new float[]{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    }

    private static float[] axisPose(int axis, float degrees) {
        float[] p=identity();
        float angle=(float)Math.toRadians(degrees);
        float c=(float)Math.cos(angle),s=(float)Math.sin(angle);
        if(axis==0) { // Pitch about X
            p[5]=c; p[6]=s; p[9]=-s; p[10]=c;
        } else if(axis==1) { // Yaw about Y
            p[0]=c; p[2]=-s; p[8]=s; p[10]=c;
        } else { // Roll about Z
            p[0]=c; p[1]=s; p[4]=-s; p[5]=c;
        }
        return p;
    }

    private static float[] multiply(float[] a,float[] b) {
        float[] out=new float[16];
        for(int row=0;row<4;row++)
            for(int col=0;col<4;col++) {
                float v=0f;
                for(int k=0;k<4;k++)
                    v+=a[k*4+row]*b[col*4+k];
                out[col*4+row]=v;
            }
        return out;
    }

    @Test public void neutralViewAndTranslationRemainValid() {
        float[] pose=identity();
        pose[12]=0.4f;pose[13]=1.2f;pose[14]=-0.7f;
        assertTrue(HeadPoseView.invertRigidInPlace(pose));
        assertEquals(-0.4f,pose[12],EPS);
        assertEquals(-1.2f,pose[13],EPS);
        assertEquals(0.7f,pose[14],EPS);
    }

    @Test public void eachHeadPivotInvertsInBothDirections() {
        for(int axis=0;axis<3;axis++)
            for(float angle:new float[]{-87f,-30f,-5f,5f,30f,87f}) {
                float[] pose=axisPose(axis,angle);
                float[] expected=axisPose(axis,-angle);
                assertTrue("axis "+axis+" angle "+angle,
                    HeadPoseView.invertRigidInPlace(pose));
                assertArrayEquals("axis "+axis+" angle "+angle,
                    expected,pose,EPS);
            }
    }

    @Test public void combinedTwistYawPitchAndNeckTranslationInvertTogether() {
        float[] pose=multiply(axisPose(1,37f),
            multiply(axisPose(0,-26f),axisPose(2,19f)));
        pose[12]=.32f;pose[13]=1.65f;pose[14]=-.11f;
        float[] original=pose.clone();
        assertTrue(HeadPoseView.invertRigidInPlace(pose));
        assertArrayEquals(identity(),multiply(pose,original),EPS);
        assertArrayEquals(identity(),multiply(original,pose),EPS);
    }

    @Test public void nonRigidAndMalformedPosesAreRejectedUnchanged() {
        assertFalse(HeadPoseView.invertRigidInPlace(null));
        assertFalse(HeadPoseView.invertRigidInPlace(new float[3]));
        float[] scale=identity();scale[5]=1.5f;
        assertFalse(HeadPoseView.invertRigidInPlace(scale));
        assertEquals(1.5f,scale[5],0f);
        float[] invalid=identity();invalid[3]=.2f;
        assertFalse(HeadPoseView.invertRigidInPlace(invalid));
        float[] nan=identity();nan[9]=Float.NaN;
        assertFalse(HeadPoseView.invertRigidInPlace(nan));
    }
}
