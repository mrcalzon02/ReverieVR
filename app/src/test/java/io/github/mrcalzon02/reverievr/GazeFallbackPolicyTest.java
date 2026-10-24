package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class GazeFallbackPolicyTest {
    private static final long T = 10_000_000_000L;

    @Test public void noControllerAtStartupStillOffersImmediateGaze() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertTrue(policy.shouldUseGaze(VrPointerMode.AUTO, false, T));
        assertTrue(policy.shouldUseGaze(VrPointerMode.CONTROLLER, false, T));
    }

    @Test public void briefPoseOrConnectionLossCannotFlashHeadsetPointer() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, true, T));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false,
            T + 750_000_001L));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false,
            T + 2_250_000_001L));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false,
            T + GazeFallbackPolicy.CONTROLLER_LOSS_GRACE_NANOS - 1L));
    }

    @Test public void genuinelyLostControllerRestoresEmergencyGaze() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertFalse(policy.shouldUseGaze(VrPointerMode.CONTROLLER, true, T));
        assertTrue(policy.shouldUseGaze(VrPointerMode.CONTROLLER, false,
            T + GazeFallbackPolicy.CONTROLLER_LOSS_GRACE_NANOS));
    }

    @Test public void newLiveControllerRemovesGazeAndRestartsGrace() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, true, T));
        assertTrue(policy.shouldUseGaze(VrPointerMode.AUTO, false,
            T + GazeFallbackPolicy.CONTROLLER_LOSS_GRACE_NANOS));
        long resumed = T + GazeFallbackPolicy.CONTROLLER_LOSS_GRACE_NANOS + 1L;
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, true, resumed));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false,
            resumed + 1_000_000_000L));
        assertTrue(policy.shouldUseGaze(VrPointerMode.AUTO, false,
            resumed + GazeFallbackPolicy.CONTROLLER_LOSS_GRACE_NANOS));
    }

    @Test public void manualGazeRemainsAvailableEvenWithLiveController() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertTrue(policy.shouldUseGaze(VrPointerMode.GAZE, true, T));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, true, T + 1L));
        assertTrue(policy.shouldUseGaze(VrPointerMode.GAZE, false, T + 2L));
    }

    @Test public void irregularFrameClockDoesNotPrematurelyActivateFallback() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, true, T));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false, T - 1L));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false,
            T + 1_000_000_000L));
    }
}
