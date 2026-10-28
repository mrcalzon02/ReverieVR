package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class ControllerPoseVisibilityTest {
    private static final long BASE = 5_000_000_000L;

    @Test
    public void shortPoseGapsDoNotDisableLiveInput() {
        assertTrue(ControllerPoseVisibility.live(true, true, BASE,
            BASE + 100_000_000L));
        assertTrue(ControllerPoseVisibility.live(true, true, BASE,
            BASE + ControllerPoseVisibility.LIVE_POSE_NANOS));
        assertFalse(ControllerPoseVisibility.holdModel(true, true, BASE,
            BASE + ControllerPoseVisibility.LIVE_POSE_NANOS));
    }

    @Test
    public void briefDropoutRetainsModelButNeverStaleInput() {
        long delayed = BASE + ControllerPoseVisibility.LIVE_POSE_NANOS + 1L;
        assertFalse(ControllerPoseVisibility.live(true,true,BASE,delayed));
        assertTrue(ControllerPoseVisibility.holdModel(true,true,BASE,delayed));
        assertTrue(ControllerPoseVisibility.holdModel(true,true,BASE,
            BASE + 120_000_000_000L));
        assertFalse(ControllerPoseVisibility.live(true,true,BASE,
            BASE + 120_000_000_000L));
    }

    @Test
    public void prolongedGapFreezesModelAndNeverRoutesInput() {
        long expired = BASE + 120_000_000_000L + 1L;
        assertTrue(ControllerPoseVisibility.holdModel(true,true,BASE,expired));
        assertFalse(ControllerPoseVisibility.live(true,true,BASE,expired));
    }

    @Test
    public void disconnectAndProviderSwitchHideImmediately() {
        long delayed = BASE + 1_000_000_000L;
        assertFalse(ControllerPoseVisibility.live(false,true,BASE,delayed));
        assertFalse(ControllerPoseVisibility.holdModel(false,true,BASE,delayed));
        assertFalse(ControllerPoseVisibility.holdModel(true,false,BASE,delayed));
    }

    @Test
    public void missingFutureOrInvalidTimestampsCannotKeepGhostAlive() {
        assertFalse(ControllerPoseVisibility.holdModel(true,true,0L,BASE));
        assertFalse(ControllerPoseVisibility.live(true,true,BASE,BASE - 1L));
        assertFalse(ControllerPoseVisibility.holdModel(true,true,
            BASE,BASE - 1L));
    }

    @Test
    public void resumedPoseRestoresLiveInputImmediately() {
        final long resumed = BASE + 3_000_000_000L;
        assertTrue(ControllerPoseVisibility.live(true,true,resumed,
            resumed + 10_000_000L));
        assertFalse(ControllerPoseVisibility.holdModel(true,true,resumed,
            resumed + 10_000_000L));
    }
}
