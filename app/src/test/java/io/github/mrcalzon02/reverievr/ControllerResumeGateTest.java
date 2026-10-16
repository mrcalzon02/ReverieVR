package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public final class ControllerResumeGateTest {
    private static ControllerSnapshot packet(boolean touch, boolean click,
                                             boolean menu) {
        return new ControllerSnapshot(1_000_000_000L, 0, 1,
            0f, 0f, 0f, 1f, 0f, 0f, 0f, 0f, 0f, 0f,
            touch, 100, 120, false, false,
            menu, false, click, 0);
    }

    @Test public void heldButtonsCannotReactivateAtReconnect() {
        ControllerResumeGate gate = new ControllerResumeGate();
        assertFalse(gate.accept(packet(true, true, false)));
        assertFalse(gate.accept(packet(false, false, true)));
        assertTrue(gate.accept(packet(false, false, false)));
        assertTrue(gate.accept(packet(true, true, false)));
        gate.requireNeutral();
        assertFalse(gate.accept(packet(true, true, false)));
        assertTrue(gate.accept(packet(false, false, false)));
    }

    @Test public void ignoresNullAndAlwaysAllowsNormalHeldInputAfterNeutral() {
        ControllerResumeGate gate = new ControllerResumeGate();
        assertFalse(gate.accept(null));
        assertTrue(gate.accept(packet(false, false, false)));
        assertTrue(gate.accept(packet(false, false, true)));
        assertTrue(gate.accept(packet(false, false, true)));
    }
}
