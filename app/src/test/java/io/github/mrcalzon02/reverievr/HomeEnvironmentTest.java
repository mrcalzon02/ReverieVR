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
