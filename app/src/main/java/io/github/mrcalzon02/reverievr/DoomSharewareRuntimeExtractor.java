package io.github.mrcalzon02.reverievr;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.SequenceInputStream;

/**
 * Reconstructs the original floppy-era split DOOM installer into an
 * app-private game tree without executing DEICE or DOS installer prompts.
 * The untouched upstream ZIP remains the only redistributed payload.
 */
final class DoomSharewareRuntimeExtractor {
    private static final int MAX_PREFIX_BYTES = 262144;
    private static final int MAX_FILES = 96;
    private static final long MAX_GAME_BYTES = 16L * 1024L * 1024L;

    private DoomSharewareRuntimeExtractor() {}

    static BoundedZipExtractor.ExtractionResult extract(
        File firstPart, File secondPart, File output
    ) throws IOException {
        if (firstPart == null || secondPart == null
            || !firstPart.isFile() || !secondPart.isFile()) {
            throw new IOException("The DOOM installer parts are missing.");
        }

        try (InputStream partOne = new FileInputStream(firstPart);
             InputStream partTwo = new FileInputStream(secondPart);
             BufferedInputStream stream = new BufferedInputStream(
                 new SequenceInputStream(partOne, partTwo),
                 MAX_PREFIX_BYTES + 8
             )) {
            stream.mark(MAX_PREFIX_BYTES + 8);
            final byte[] signature = {0x50,0x4b,0x03,0x04};
            int matched = 0;
            int offset = -1;
            for (int i=0; i<MAX_PREFIX_BYTES; i++) {
                int value=stream.read();
                if (value<0) break;
                if (value==(signature[matched]&0xff)) {
                    matched++;
                    if (matched==signature.length) {
                        offset=i-signature.length+1;
                        break;
                    }
                } else matched=value==0x50?1:0;
            }
            if (offset<0) {
                throw new IOException(
                    "Original DOOM parts contain no supported ZIP payload; "
                    + "DOS installer will not be launched."
                );
            }
            stream.reset();
            long skipped=0;
            while(skipped<offset) {
                long step=stream.skip(offset-skipped);
                if(step<=0)throw new IOException("Could not seek DOOM ZIP data.");
                skipped+=step;
            }
            BoundedZipExtractor.ExtractionResult result =
                BoundedZipExtractor.extract(
                    stream, output, MAX_FILES, MAX_GAME_BYTES
                );
            if(result.entryCount==0 || result.totalBytes==0)
                throw new IOException("The DOOM installer expanded no game files.");
            return result;
        }
    }
}
