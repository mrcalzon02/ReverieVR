package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class VersionUtilsTest {
    @Test
    public void stableReleaseBeatsMatchingDevelopmentBuild() {
        assertTrue(VersionUtils.isNewer("v0.1.0", "0.1.0-dev"));
    }

    @Test
    public void higherSemanticVersionWins() {
        assertTrue(VersionUtils.isNewer("v0.2.0", "0.1.9"));
        assertTrue(VersionUtils.isNewer("1.0.0", "0.99.99"));
    }

    @Test
    public void equalOrOlderVersionDoesNotWin() {
        assertFalse(VersionUtils.isNewer("0.1.0", "0.1.0"));
        assertFalse(VersionUtils.isNewer("0.0.9", "0.1.0"));
    }
}
