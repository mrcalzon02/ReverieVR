package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import org.junit.Test;

public final class UpdatePackageVerifierTest {
    @Test
    public void releaseNotesDeclareTimestampVersionIndependentOfRunTag() {
        // The real Phone Test #248 uses run 248, attempt 1,
        // and timestamp-based Android versionCode 214260770.
        assertEquals(
            214260770L,
            UpdatePackageVerifier.declaredPhoneTestVersionCode(
                "Run: 248 attempt 1\nAndroid versionCode: 214260770\n"
            )
        );
        assertEquals(
            214260770L,
            UpdatePackageVerifier.declaredPhoneTestVersionCode(
                "Android versionCode: 214260770\r\n"
            )
        );
    }

    @Test
    public void absentMalformedOrRepeatedVersionMetadataIsRejected() {
        assertEquals(
            -1L,
            UpdatePackageVerifier.declaredPhoneTestVersionCode(null)
        );
        assertEquals(
            -1L,
            UpdatePackageVerifier.declaredPhoneTestVersionCode(
                "Run: 248 attempt 1\n"
            )
        );
        assertEquals(
            -1L,
            UpdatePackageVerifier.declaredPhoneTestVersionCode(
                "Android versionCode: 214260770-extra\n"
            )
        );
        assertEquals(
            -1L,
            UpdatePackageVerifier.declaredPhoneTestVersionCode(
                "Android versionCode: 0\n"
            )
        );
        assertEquals(
            -1L,
            UpdatePackageVerifier.declaredPhoneTestVersionCode(
                "Android versionCode: 2100000001\n"
            )
        );
        assertEquals(
            -1L,
            UpdatePackageVerifier.declaredPhoneTestVersionCode(
                "Android versionCode: 214260770\n"
                    + "Android versionCode: 214260771\n"
            )
        );
    }

    @Test
    public void digestNormalizationRequiresTrustedShape() {
        String hex =
            "0123456789abcdef"
                + "0123456789abcdef"
                + "0123456789abcdef"
                + "0123456789abcdef";

        assertEquals(
            hex,
            UpdatePackageVerifier
                .normalizeSha256Digest(
                    "sha256:" + hex
                )
        );
        assertNull(
            UpdatePackageVerifier
                .normalizeSha256Digest(
                    hex
                )
        );
    }

    @Test
    public void signerFingerprintAcceptsColonFormatting() {
        String compact =
            "0123456789abcdef"
                + "0123456789abcdef"
                + "0123456789abcdef"
                + "0123456789abcdef";

        String colonSeparated =
            compact.replaceAll(
                "(..)(?!$)",
                "$1:"
            );

        assertEquals(
            compact,
            UpdatePackageVerifier
                .normalizeSha256Fingerprint(
                    colonSeparated
                )
        );
        assertNull(
            UpdatePackageVerifier
                .normalizeSha256Fingerprint(
                    "not-a-certificate"
                )
        );
    }
    @Test
    public void candidateSignerMayDeferToAndroidInstallerWhenArchiveParserOmitsIt() {
        String pinned =
            "0123456789abcdef"
                + "0123456789abcdef"
                + "0123456789abcdef"
                + "0123456789abcdef";

        assertEquals(
            true,
            UpdatePackageVerifier
                .candidateSignerAcceptable(
                    pinned,
                    null
                )
        );
        assertEquals(
            true,
            UpdatePackageVerifier
                .candidateSignerAcceptable(
                    pinned,
                    pinned
                )
        );
        assertEquals(
            false,
            UpdatePackageVerifier
                .candidateSignerAcceptable(
                    pinned,
                    "ffffffffffffffff"
                        + "ffffffffffffffff"
                        + "ffffffffffffffff"
                        + "ffffffffffffffff"
                )
        );
    }

}
