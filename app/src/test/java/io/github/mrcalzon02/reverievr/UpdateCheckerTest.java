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
    public void trustedDigestRequiresSha256AndFullHex() {
        assertTrue(
            UpdateChecker.isValidSha256Digest(
                "sha256:"
                    + "0123456789abcdef"
                    + "0123456789abcdef"
                    + "0123456789abcdef"
                    + "0123456789abcdef"
            )
        );
        assertFalse(
            UpdateChecker.isValidSha256Digest(
                "0123456789abcdef"
            )
        );
        assertFalse(
            UpdateChecker.isValidSha256Digest(
                "sha256:deadbeef"
            )
        );
        assertFalse(
            UpdateChecker.isValidSha256Digest(null)
        );
    }

    @Test
    public void phoneTestTagMapsToRerunSafeVersionCode() {
        org.junit.Assert.assertEquals(
            32001,
            UpdateChecker.versionCodeForPhoneTest(
                32,
                1
            )
        );
        org.junit.Assert.assertEquals(
            32002,
            UpdateChecker.versionCodeForPhoneTest(
                32,
                2
            )
        );
        org.junit.Assert.assertEquals(
            0,
            UpdateChecker.versionCodeForPhoneTest(
                0,
                1
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
