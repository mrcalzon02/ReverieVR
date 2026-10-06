package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public final class VrPointerModeTest {
    @Test
    public void knownValuesRoundTrip() {
        for (VrPointerMode mode : VrPointerMode.values()) {
            assertEquals(
                mode,
                VrPointerMode.fromPreference(
                    mode.preferenceValue
                )
            );
        }
    }

    @Test
    public void unknownValueFallsBackToAuto() {
        assertEquals(
            VrPointerMode.AUTO,
            VrPointerMode.fromPreference(
                "unknown-mode"
            )
        );
        assertEquals(
            VrPointerMode.AUTO,
            VrPointerMode.fromPreference(null)
        );
    }

    @Test
    public void cycleCoversAllModes() {
        assertEquals(
            VrPointerMode.GAZE,
            VrPointerMode.AUTO.next()
        );
        assertEquals(
            VrPointerMode.CONTROLLER,
            VrPointerMode.GAZE.next()
        );
        assertEquals(
            VrPointerMode.AUTO,
            VrPointerMode.CONTROLLER.next()
        );
    }
}
