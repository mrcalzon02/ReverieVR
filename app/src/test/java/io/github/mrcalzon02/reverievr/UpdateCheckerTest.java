package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class UpdateCheckerTest {
    @Test
    public void newerPhoneTestRunWins() {
        assertTrue(
            UpdateChecker.isPhoneTestNewer(
                22,
                1,
                21,
                1
            )
        );
    }

    @Test
    public void newerAttemptWithinSameRunWins() {
        assertTrue(
            UpdateChecker.isPhoneTestNewer(
                21,
                2,
                21,
                1
            )
        );
    }

    @Test
    public void sameOrOlderPhoneTestDoesNotWin() {
        assertFalse(
            UpdateChecker.isPhoneTestNewer(
                21,
                1,
                21,
                1
            )
        );
        assertFalse(
            UpdateChecker.isPhoneTestNewer(
                20,
                9,
                21,
                1
            )
        );
    }

    @Test
    public void localBuildDoesNotPretendPhoneTestOrdering() {
        assertFalse(
            UpdateChecker.isPhoneTestNewer(
                22,
                1,
                0,
                0
            )
        );
    }

    @Test
    public void headsetAssetRejectsControllerCompanion() {
        assertTrue(
            UpdateChecker.isHeadsetApkAssetName(
                "ReverieVR-phone-test-22-deadbee.apk"
            )
        );
        assertFalse(
            UpdateChecker.isHeadsetApkAssetName(
                "ReverieVR-Controller-phone-test-22-deadbee.apk"
            )
        );
    }
}
