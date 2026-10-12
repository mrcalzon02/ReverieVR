package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public final class HeadsetLensUnwarpTest {
    @Test public void offIsAnExactNoOpAcrossTheEyeImage() {
        assertEquals(1.0f,
            HeadsetLensUnwarp.sourceRadiusScale(0.0f, 0.0f, 0), 0.0001f);
        assertEquals(1.0f,
            HeadsetLensUnwarp.sourceRadiusScale(0.5f, 0.5f, 0), 0.0001f);
        assertEquals(1.0f,
            HeadsetLensUnwarp.sourceRadiusScale(1.0f, 1.0f, 0), 0.0001f);
        assertEquals("OFF", HeadsetLensUnwarp.displayName(0));
    }

    @Test public void centerRemainsFixedWithBothCorrectionDirections() {
        assertEquals(1.0f,
            HeadsetLensUnwarp.sourceRadiusScale(0.5f, 0.5f, 6), 0.0001f);
        assertEquals(1.0f,
            HeadsetLensUnwarp.sourceRadiusScale(0.5f, 0.5f, -6), 0.0001f);
    }

    @Test public void eachEyeGetsBoundedSignedRadialRemap() {
        assertEquals(1.3f,
            HeadsetLensUnwarp.sourceRadiusScale(1.0f, 1.0f, 6), 0.0001f);
        assertEquals(0.7f,
            HeadsetLensUnwarp.sourceRadiusScale(0.0f, 0.0f, -6), 0.0001f);
        assertEquals(1.15f,
            HeadsetLensUnwarp.sourceRadiusScale(1.0f, 0.5f, 6), 0.0001f);
        assertEquals(0.85f,
            HeadsetLensUnwarp.sourceRadiusScale(0.5f, 1.0f, -6), 0.0001f);
    }

    @Test public void adjustmentStopsAtBothLimits() {
        assertEquals(-6, HeadsetLensUnwarp.adjust(-6, -1));
        assertEquals(6, HeadsetLensUnwarp.adjust(6, 1));
        assertEquals(-6, HeadsetLensUnwarp.clampStep(-100));
        assertEquals(6, HeadsetLensUnwarp.clampStep(100));
        assertEquals(0, HeadsetLensUnwarp.adjust(-1, 1));
        assertEquals(0, HeadsetLensUnwarp.adjust(1, -1));
        assertEquals(-0.3f, HeadsetLensUnwarp.coefficient(-99), 0.0001f);
        assertEquals(0.3f, HeadsetLensUnwarp.coefficient(99), 0.0001f);
        assertEquals("+30%", HeadsetLensUnwarp.displayName(6));
        assertEquals("-30%", HeadsetLensUnwarp.displayName(-6));
    }
}
