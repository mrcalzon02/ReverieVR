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
                "927f596e9bfd0ba8809b86c1e09556e72eab170bb5e2a284f41b5c4a7bfffbfb",
                DiagnosticSubmissionClient.sha256(file)
            );
        } finally {
            if (file.isFile()) {
                file.delete();
            }
        }
    }
}
