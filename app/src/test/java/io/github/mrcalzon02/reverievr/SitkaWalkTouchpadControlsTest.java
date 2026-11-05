package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class SitkaWalkTouchpadControlsTest {
    @Test public void padUsesNativeSignedAxesWithFiniteClamping() {
        assertEquals(-1.f,SitkaWalkTouchpadControls.axis(0.f),0.00001f);
        assertEquals(0.f,SitkaWalkTouchpadControls.axis(127.5f),0.00001f);
        assertEquals(1.f,SitkaWalkTouchpadControls.axis(255.f),0.00001f);
        assertEquals(-1.f,SitkaWalkTouchpadControls.axis(-400.f),0.00001f);
        assertEquals(1.f,SitkaWalkTouchpadControls.axis(900.f),0.00001f);
        assertEquals(0.f,SitkaWalkTouchpadControls.axis(Float.NaN),0.00001f);
    }
    @Test public void realGamepadWinsOverSitkaTouchMovement() {
        assertTrue(SitkaWalkTouchpadControls.useTouchpad(0.f,0.f,true));
        assertTrue(SitkaWalkTouchpadControls.useTouchpad(.05f,.06f,true));
        assertFalse(SitkaWalkTouchpadControls.useTouchpad(.8f,0.f,true));
        assertFalse(SitkaWalkTouchpadControls.useTouchpad(0.f,0.f,false));
        assertFalse(SitkaWalkTouchpadControls.useTouchpad(
            Float.NaN,0.f,true));
    }
    @Test public void clickAndStaleTouchMustRearmBeforeMovement() {
        TouchpadLocomotionGate gate=new TouchpadLocomotionGate();
        assertFalse(gate.allows(true,false,false));
        assertFalse(gate.allows(false,false,false));
        assertTrue(gate.allows(true,false,false));
        assertFalse(gate.allows(true,true,false));
        assertFalse(gate.allows(true,false,false));
        assertFalse(gate.allows(true,false,false));
        assertFalse(gate.allows(false,false,false));
        assertTrue(gate.allows(true,false,false));
        assertFalse(gate.allows(true,false,true));
    }
}
