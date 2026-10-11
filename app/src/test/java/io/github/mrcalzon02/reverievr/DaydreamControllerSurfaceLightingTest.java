package io.github.mrcalzon02.reverievr;
import org.junit.Test;
import static org.junit.Assert.*;

public final class DaydreamControllerSurfaceLightingTest {
    @Test public void faceButtonsHaveStableNormals() {
        assertTrue(DaydreamControllerSurfaceLighting.flatButtonNormal("MatAppButton"));
        assertTrue(DaydreamControllerSurfaceLighting.flatButtonNormal("MatHomeButton"));
        assertTrue(DaydreamControllerSurfaceLighting.flatButtonNormal("MatVolUpBut"));
        assertTrue(DaydreamControllerSurfaceLighting.flatButtonNormal("MatVolDownBut"));
    }
    @Test public void bodyAndPadKeepOriginalShading() {
        assertFalse(DaydreamControllerSurfaceLighting.flatButtonNormal("MatBody"));
        assertFalse(DaydreamControllerSurfaceLighting.flatButtonNormal("MatTouchpad"));
    }
}
