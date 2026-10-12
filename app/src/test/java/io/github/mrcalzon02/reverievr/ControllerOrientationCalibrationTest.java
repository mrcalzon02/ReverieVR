package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public class ControllerOrientationCalibrationTest {
    private static float[] identity() {
        float[] m=new float[16]; m[0]=m[5]=m[10]=m[15]=1f; return m;
    }

    @Test public void identityControllerMatchesHeadsetIdentity() {
        float[] out=new float[16];
        assertTrue(ControllerOrientationCalibration.compute(identity(), 0,0,0,1,out));
        assertEquals(1f,out[0],0.0001f);
        assertEquals(1f,out[5],0.0001f);
        assertEquals(1f,out[10],0.0001f);
    }

    @Test public void controllerYawIsCancelledByFullCorrection() {
        float[] out=new float[16];
        assertTrue(ControllerOrientationCalibration.compute(identity(),
            0f,0.70710677f,0f,0.70710677f,out));
        float[] direction={-1f,0f,0f};
        float[] rotated=new float[3];
        ControllerOrientationCalibration.rotate(out,true,direction,rotated);
        assertEquals(0f,rotated[0],0.0002f);
        assertEquals(0f,rotated[1],0.0002f);
        assertEquals(-1f,rotated[2],0.0002f);
    }

    @Test public void headsetPitchAndRollAreCapturedAsFullRotation() {
        // Head-view = transpose of a 90-degree roll about world Z.
        float[] headView=identity();
        headView[0]=0f; headView[1]=-1f;
        headView[4]=1f; headView[5]=0f;
        float[] correction=new float[16];
        assertTrue(ControllerOrientationCalibration.compute(
            headView, 0f, 0f, 0f, 1f, correction));
        float[] source={1f,0f,0f};
        float[] target=new float[3];
        ControllerOrientationCalibration.rotate(correction,true,source,target);
        assertEquals(0f,target[0],0.0001f);
        assertEquals(1f,target[1],0.0001f);
        assertEquals(0f,target[2],0.0001f);

        // Head-view = transpose of a 90-degree pitch about world X.
        headView=identity();
        headView[5]=0f; headView[6]=-1f;
        headView[9]=1f; headView[10]=0f;
        assertTrue(ControllerOrientationCalibration.compute(
            headView, 0f, 0f, 0f, 1f, correction));
        source=new float[] {0f,0f,-1f};
        ControllerOrientationCalibration.rotate(correction,true,source,target);
        assertEquals(0f,target[0],0.0001f);
        assertEquals(1f,target[1],0.0001f);
        assertEquals(0f,target[2],0.0001f);
    }


    @Test public void recenterToHudCancelsCombinedControllerYawPitchAndRoll() {
        float qx=0.20f, qy=-0.30f, qz=0.10f, qw=0.925f;
        float norm=(float)Math.sqrt(qx*qx+qy*qy+qz*qz+qw*qw);
        qx/=norm; qy/=norm; qz/=norm; qw/=norm;
        float[] forward={
            -2f*(qx*qz+qw*qy),
            -2f*(qy*qz-qw*qx),
            -(1f-2f*qx*qx-2f*qy*qy)
        };
        float[] correction=new float[16];
        assertTrue(ControllerOrientationCalibration.computeToHud(
            qx,qy,qz,qw,correction));
        float[] corrected=new float[3];
        ControllerOrientationCalibration.rotate(
            correction,true,forward,corrected);
        assertEquals(0f,corrected[0],0.0003f);
        assertEquals(0f,corrected[1],0.0003f);
        assertEquals(-1f,corrected[2],0.0003f);
    }

    @Test public void invalidRecenterDoesNotOverwritePreviousHudCorrection() {
        float[] correction=identity();
        assertFalse(ControllerOrientationCalibration.computeToHud(
            Float.NaN,0f,0f,1f,correction));
        assertEquals(1f,correction[0],0f);
    }

    @Test public void rejectsInvalidQuaternionWithoutChangingMatrix() {
        float[] out=identity();
        assertFalse(ControllerOrientationCalibration.compute(identity(),0,0,0,0,out));
        assertFalse(ControllerOrientationCalibration.compute(identity(),Float.NaN,0,0,1,out));
        assertEquals(1f,out[0],0f);
    }
}
