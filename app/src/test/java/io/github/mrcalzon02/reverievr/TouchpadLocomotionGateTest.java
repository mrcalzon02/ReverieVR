package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class TouchpadLocomotionGateTest {
    @Test public void touchHeldOnSceneEntryCannotMoveUntilReleased() {
        TouchpadLocomotionGate gate = new TouchpadLocomotionGate();
        assertFalse(gate.allows(true, false, false));
        assertFalse(gate.allows(false, false, false));
        assertTrue(gate.allows(true, false, false));
    }
    @Test public void menuAndClickDisarmUntilFreshTouch() {
        TouchpadLocomotionGate gate = new TouchpadLocomotionGate();
        gate.allows(false, false, false);
        assertTrue(gate.allows(true, false, false));
        assertFalse(gate.allows(true, false, true));
        assertFalse(gate.allows(true, false, false));
        gate.allows(false, false, false);
        assertTrue(gate.allows(true, false, false));
        assertFalse(gate.allows(true, true, false));
        assertFalse(gate.allows(true, false, false));
        gate.allows(false, false, false);
        assertTrue(gate.allows(true, false, false));
    }
    @Test public void stalePacketsAndModuleChangeRequireRelease() {
        TouchpadLocomotionGate gate = new TouchpadLocomotionGate();
        gate.allows(false, false, false);
        assertTrue(gate.allows(true, false, false));
        assertFalse(gate.allows(true, false, true));
        assertFalse(gate.allows(true, false, false));
        gate.allows(false, false, false);
        assertTrue(gate.allows(true, false, false));
        gate.reset();
        assertFalse(gate.allows(true, false, false));
    }
}
