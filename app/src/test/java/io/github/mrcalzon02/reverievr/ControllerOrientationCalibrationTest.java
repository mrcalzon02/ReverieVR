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

    @Test public void rejectsInvalidQuaternionWithoutChangingMatrix() {
        float[] out=identity();
        assertFalse(ControllerOrientationCalibration.compute(identity(),0,0,0,0,out));
        assertFalse(ControllerOrientationCalibration.compute(identity(),Float.NaN,0,0,1,out));
        assertEquals(1f,out[0],0f);
    }
}
