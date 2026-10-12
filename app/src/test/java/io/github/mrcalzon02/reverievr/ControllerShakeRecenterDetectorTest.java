package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class ControllerShakeRecenterDetectorTest {
    @Test
    public void twoSharpImpulsesTriggerRecenter() {
        ControllerShakeRecenterDetector detector =
            new ControllerShakeRecenterDetector();

        long start = 1000000000L;
        assertFalse(
            detector.sample(
                2.1f,
                0.0f,
                0.0f,
                start
            )
        );
        assertTrue(
            detector.sample(
                -2.2f,
                0.0f,
                0.0f,
                start + 120000000L
            )
        );
    }

    @Test
    public void sameDirectionImpactsCannotTriggerRecenter() {
        ControllerShakeRecenterDetector detector =
            new ControllerShakeRecenterDetector();
        long start = 1000000000L;
        assertFalse(detector.sample(2.5f, 0f, 0f, start));
        assertFalse(detector.sample(2.6f, 0f, 0f, start + 120000000L));
        assertFalse(detector.sample(2.7f, 0f, 0f, start + 240000000L));
        assertTrue(detector.sample(-2.6f, 0f, 0f, start + 360000000L));
    }

    @Test
    public void cancellingPendingShakeDoesNotClearCooldown() {
        ControllerShakeRecenterDetector detector =
            new ControllerShakeRecenterDetector();
        long start = 1000000000L;
        detector.sample(2.5f, 0f, 0f, start);
        assertTrue(detector.sample(-2.5f, 0f, 0f, start + 120000000L));
        detector.cancelPendingImpulse();
        assertFalse(detector.sample(2.5f, 0f, 0f, start + 350000000L));
        assertFalse(detector.sample(-2.5f, 0f, 0f, start + 470000000L));
        assertFalse(detector.sample(2.5f, 0f, 0f, start + 3300000000L));
        assertTrue(detector.sample(-2.5f, 0f, 0f, start + 3420000000L));
    }


    @Test
    public void highAccelerationDaydreamShakeRemainsInGUnits() {
        ControllerShakeRecenterDetector detector =
            new ControllerShakeRecenterDetector();
        long start=1000000000L;
        assertFalse(detector.sample(4.6f,0f,0f,start,true));
        assertTrue(detector.sample(-4.8f,0f,0f,
            start+120000000L,true));
    }

    @Test
    public void phoneMotionUsesMetersPerSecondSquared() {
        ControllerShakeRecenterDetector detector =
            new ControllerShakeRecenterDetector();
        long start=1000000000L;
        assertFalse(detector.sample(22f,0f,0f,start,false));
        assertTrue(detector.sample(-24f,0f,0f,
            start+140000000L,false));
    }

    @Test
    public void normalGravityDoesNotTrigger() {
        ControllerShakeRecenterDetector detector =
            new ControllerShakeRecenterDetector();

        long start = 1000000000L;
        for (int index = 0; index < 30; index++) {
            assertFalse(
                detector.sample(
                    0.0f,
                    9.81f,
                    0.0f,
                    start + index * 20000000L, false
                )
            );
        }
    }

    @Test
    public void oneImpactIsNotEnough() {
        ControllerShakeRecenterDetector detector =
            new ControllerShakeRecenterDetector();

        assertFalse(
            detector.sample(
                24.0f,
                0.0f,
                0.0f,
                1000000000L
            )
        );
    }

    @Test
    public void cooldownPreventsRepeatedRecenters() {
        ControllerShakeRecenterDetector detector =
            new ControllerShakeRecenterDetector();

        long start = 1000000000L;
        detector.sample(22.0f, 0.0f, 0.0f, start);
        assertTrue(
            detector.sample(
                -22.0f,
                0.0f,
                0.0f,
                start + 100000000L
            )
        );

        assertFalse(
            detector.sample(
                22.0f,
                0.0f,
                0.0f,
                start + 350000000L
            )
        );
        assertFalse(
            detector.sample(
                -22.0f,
                0.0f,
                0.0f,
                start + 450000000L
            )
        );
    }
}
