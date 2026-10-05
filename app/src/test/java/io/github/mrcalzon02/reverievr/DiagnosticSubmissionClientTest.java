package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

import org.junit.Test;

public final class DiagnosticSubmissionClientTest {
    @Test
    public void diagnosticIdsAreStableFormatAndUnique() {
        String first =
            DiagnosticSubmissionClient.newDiagnosticId();
        String second =
            DiagnosticSubmissionClient.newDiagnosticId();

        assertTrue(
            first.matches(
                "^revdiag-[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-"
                    + "[89ab][0-9a-f]{3}-[0-9a-f]{12}$"
            )
        );
        assertTrue(
            second.matches(
                "^revdiag-[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-"
                    + "[89ab][0-9a-f]{3}-[0-9a-f]{12}$"
            )
        );
        assertNotEquals(first, second);
    }

    @Test
    public void sha256MatchesKnownVector() throws Exception {
        File file =
            File.createTempFile(
                "reverie-diagnostic-hash-",
                ".bin"
            );

        try {
            try (FileOutputStream output =
                     new FileOutputStream(file)) {
                output.write(
                    "ReverieVR diagnostics"
                        .getBytes(StandardCharsets.UTF_8)
                );
            }

            assertEquals(
                "5b620d0a7dddbf9b0e9c14448c6a558b6902d80e02fc6edcc5e0cbe84bd5a456",
                DiagnosticSubmissionClient.sha256(file)
            );
        } finally {
            if (file.isFile()) {
                file.delete();
            }
        }
    }
}
