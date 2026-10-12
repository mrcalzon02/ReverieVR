package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public final class HeadYawConventionTest {
    private static final float EPS = .0002f;

    private static float[] yawView(float degrees) {
        float a=(float)Math.toRadians(degrees);
        float c=(float)Math.cos(a),s=(float)Math.sin(a);
        return new float[] {
            c,0f,-s,0f,
            0f,1f,0f,0f,
            s,0f,c,0f,
            0f,0f,0f,1f
        };
    }

    @Test public void bothHeadPivotDirectionsReverseExactlyOnce() {
        for(float angle:new float[]{-85f,-35f,-5f,0f,5f,35f,85f}) {
            float[] view=yawView(angle);
            assertTrue(HeadYawConvention.correctInPlace(view));
            float[] expected=yawView(-angle);
            assertArrayEquals("Left/right head pivot "+angle,
                expected,view,EPS);
        }
    }

    @Test public void correctionDoesNotChangeVerticalHeadBasis() {
        float[] tilted=yawView(35f);
        tilted[1]=.18f; tilted[5]=.94f;
        tilted[9]=-.29f; tilted[13]=.06f;
        float[] y={tilted[1],tilted[5],tilted[9],tilted[13]};
        assertTrue(HeadYawConvention.correctInPlace(tilted));
        assertArrayEquals(y,new float[]{
            tilted[1],tilted[5],tilted[9],tilted[13]
        },0f);
    }

    @Test public void repeatedCorrectionRestoresOriginalYawBasis() {
        float[] view=yawView(42f);
        float[] original=view.clone();
        assertTrue(HeadYawConvention.correctInPlace(view));
        assertTrue(HeadYawConvention.correctInPlace(view));
        assertArrayEquals(original,view,EPS);
    }

    @Test public void invalidPoseLeavesOriginalUnmodified() {
        assertFalse(HeadYawConvention.correctInPlace(null));
        assertFalse(HeadYawConvention.correctInPlace(new float[8]));
        float[] view=yawView(15f);
        view[6]=Float.NaN;
        float[] old=view.clone();
        assertFalse(HeadYawConvention.correctInPlace(view));
        for(int i=0;i<view.length;i++) {
            if(Float.isNaN(old[i])) assertTrue(Float.isNaN(view[i]));
            else assertEquals(old[i],view[i],0f);
        }
    }
}
