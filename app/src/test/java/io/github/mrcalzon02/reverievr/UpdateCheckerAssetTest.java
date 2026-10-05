package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class UpdateCheckerAssetTest {
    @Test
    public void acceptsHeadsetApkNames() {
        assertTrue(UpdateChecker.isHeadsetApkAssetName("ReverieVR-v0.2.0.apk"));
        assertTrue(UpdateChecker.isHeadsetApkAssetName("ReverieVR.apk"));
        assertTrue(UpdateChecker.isHeadsetApkAssetName("app-release.apk"));
    }

    @Test
    public void rejectsControllerCompanionApkNames() {
        assertFalse(
            UpdateChecker.isHeadsetApkAssetName(
                "ReverieVR-Controller-v0.2.0.apk"
            )
        );
        assertFalse(
            UpdateChecker.isHeadsetApkAssetName(
                "controller-app-release.apk"
            )
        );
    }

    @Test
    public void rejectsNonApkAssets() {
        assertFalse(UpdateChecker.isHeadsetApkAssetName("ReverieVR-v0.2.0.zip"));
        assertFalse(UpdateChecker.isHeadsetApkAssetName(""));
        assertFalse(UpdateChecker.isHeadsetApkAssetName(null));
    }
}
