package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class DosPauseGateTest {
    @Test
    public void overlayPauseSurvivesLifecycleResume() {
        DosPauseGate gate = new DosPauseGate();

        gate.setOverlayPaused(true);
        gate.setLifecyclePaused(true);
        gate.setLifecyclePaused(false);

        assertTrue(gate.isOverlayPaused());
        assertTrue(gate.isPaused());
    }

    @Test
    public void clearingOverlayDoesNotOverrideLifecyclePause() {
        DosPauseGate gate = new DosPauseGate();

        gate.setLifecyclePaused(true);
        gate.setOverlayPaused(true);
        gate.setOverlayPaused(false);

        assertFalse(gate.isOverlayPaused());
        assertTrue(gate.isPaused());
    }

    @Test
    public void clearRemovesEveryPauseReason() {
        DosPauseGate gate = new DosPauseGate();

        gate.setLifecyclePaused(true);
        gate.setOverlayPaused(true);
        gate.clear();

        assertFalse(gate.isOverlayPaused());
        assertFalse(gate.isPaused());
    }
}
