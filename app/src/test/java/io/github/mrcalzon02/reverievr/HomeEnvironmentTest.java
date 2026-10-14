package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public final class HomeEnvironmentTest {
    @Test
    public void knownPreferenceValuesRoundTrip() {
        for (HomeEnvironment environment
             : HomeEnvironment.values()) {
            assertEquals(
                environment,
                HomeEnvironment.fromPreference(
                    environment.preferenceValue
                )
            );
        }
    }

    @Test
    public void whiteRoomBackdropMovesCloserWithoutMovingForegroundControls() {
        float[] coordinates = {-1f, 1f, -6f,
            0f, 0f, -3.0f, 1f, 2f, 6f};
        float[] adjusted =
            HomeEnvironmentRenderer.closeWhiteRoomRearBackdrop(coordinates);
        assertEquals(-4.1f, adjusted[2], .0001f);
        assertEquals(-3.0f, adjusted[5], .0001f);
        assertEquals(6f, adjusted[8], .0001f);
    }

    @Test
    public void unknownPreferenceFallsBackToWhiteRoom() {
        assertEquals(
            HomeEnvironment.WHITE_ROOM,
            HomeEnvironment.fromPreference(
                "not-a-real-environment"
            )
        );
        assertEquals(
            HomeEnvironment.WHITE_ROOM,
            HomeEnvironment.fromPreference(null)
        );
    }
}
