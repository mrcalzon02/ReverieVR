package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class GazeFallbackPolicyTest {
    private static final long T = 10_000_000_000L;

    @Test public void headsetOnlyStartupStillHasImmediateGaze() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertTrue(policy.shouldUseGaze(VrPointerMode.AUTO, false, false, T));
        assertTrue(policy.shouldUseGaze(VrPointerMode.CONTROLLER, false, false, T));
    }

    @Test public void transportReadyBeforeFirstPoseDoesNotFlashGaze() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false, true, T));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false, true,
            T + GazeFallbackPolicy.CONNECTED_POSE_STALL_NANOS - 1));
        assertTrue(policy.shouldUseGaze(VrPointerMode.AUTO, false, true,
            T + GazeFallbackPolicy.CONNECTED_POSE_STALL_NANOS));
    }

    @Test public void connectedControllerKeepsGazeOffDuringPoseStalls() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, true, true, T));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false, true,
            T + 750_000_001L));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false, true,
            T + GazeFallbackPolicy.CONTROLLER_LOSS_GRACE_NANOS));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false, true,
            T + GazeFallbackPolicy.CONNECTED_POSE_STALL_NANOS - 1L));
        assertTrue(policy.shouldUseGaze(VrPointerMode.AUTO, false, true,
            T + GazeFallbackPolicy.CONNECTED_POSE_STALL_NANOS));
    }

    @Test public void disconnectedControllerRestoresRecoveryAfterGrace() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertFalse(policy.shouldUseGaze(VrPointerMode.CONTROLLER, true, true, T));
        assertFalse(policy.shouldUseGaze(VrPointerMode.CONTROLLER, false, false,
            T + GazeFallbackPolicy.CONTROLLER_LOSS_GRACE_NANOS - 1L));
        assertTrue(policy.shouldUseGaze(VrPointerMode.CONTROLLER, false, false,
            T + GazeFallbackPolicy.CONTROLLER_LOSS_GRACE_NANOS));
    }

    @Test public void reconnectWithoutPoseDoesNotBrieflyEnableGaze() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, true, true, T));
        assertTrue(policy.shouldUseGaze(VrPointerMode.AUTO, false, false,
            T + GazeFallbackPolicy.CONTROLLER_LOSS_GRACE_NANOS));
        long reconnected = T + GazeFallbackPolicy.CONTROLLER_LOSS_GRACE_NANOS + 1L;
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false, true, reconnected));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false, true,
            reconnected + 1_000_000_000L));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, true, true,
            reconnected + 2_000_000_000L));
    }

    @Test public void explicitGazeRemainsAvailableWithController() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertTrue(policy.shouldUseGaze(VrPointerMode.GAZE, true, true, T));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, true, true, T + 1L));
        assertTrue(policy.shouldUseGaze(VrPointerMode.GAZE, false, true, T + 2L));
    }

    @Test public void irregularClockCannotPrematurelyActivateFallback() {
        GazeFallbackPolicy policy = new GazeFallbackPolicy();
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, true, true, T));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false, true, T - 1L));
        assertFalse(policy.shouldUseGaze(VrPointerMode.AUTO, false, true,
            T + 1_000_000_000L));
    }
}
