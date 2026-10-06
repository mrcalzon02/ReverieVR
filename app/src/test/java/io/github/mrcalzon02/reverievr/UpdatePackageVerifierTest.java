package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import org.junit.Test;

public final class UpdatePackageVerifierTest {
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
}
