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
                    start + index * 20000000L
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
