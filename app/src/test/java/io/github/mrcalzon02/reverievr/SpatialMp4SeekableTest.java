package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;

public final class SpatialMp4SeekableTest {
    private static final int MEDIA_BYTES = 9 * 1024 * 1024;

    @Test public void discoversMoovAfterLargeMdatWithoutReadingMedia() throws Exception {
        File file = temporaryFile();
        try {
            byte[] ftyp = ftyp();
            byte[] finalMoov = moov(2);
            byte[] deceptiveMoov = moov(1);
            long mdatLength = MEDIA_BYTES + 8L;
            long tail = ftyp.length + mdatLength;
            try (RandomAccessFile out = new RandomAccessFile(file, "rw")) {
                out.setLength(tail + finalMoov.length);
                out.seek(0);
                out.write(ftyp);
                out.write(be32((int) mdatLength));
                out.write(ascii("mdat"));
                // A plausible metadata box inside media is not metadata.
                out.write(deceptiveMoov);
                out.seek(tail);
                out.write(finalMoov);
                assertEquals(VideoProjection.STEREO_EQUIRECTANGULAR_360_SBS,
                    SpatialMp4Metadata.inspect(out.getChannel()).projection);
            }
            try (FileInputStream prefix = new FileInputStream(file)) {
                assertEquals(SpatialMp4Metadata.Status.UNKNOWN,
                    SpatialMp4Metadata.inspect(prefix).status);
            }
        } finally {
            file.delete();
        }
    }

    @Test public void extended64BitBoxLengthReachesTrailingMoov() throws Exception {
        File file = temporaryFile();
        try {
            byte[] ftyp = ftyp();
            byte[] moov = moov(1);
            long mdatSize = MEDIA_BYTES + 16L;
            long tail = ftyp.length + mdatSize;
            try (RandomAccessFile out = new RandomAccessFile(file, "rw")) {
                out.setLength(tail + moov.length);
                out.seek(0);
                out.write(ftyp);
                out.write(be32(1));
                out.write(ascii("mdat"));
                out.write(be64(mdatSize));
                out.seek(tail);
                out.write(moov);
                assertEquals(VideoProjection.STEREO_EQUIRECTANGULAR_360_TB,
                    SpatialMp4Metadata.inspect(out.getChannel()).projection);
            }
        } finally {
            file.delete();
        }
    }

    @Test public void multipleConflictingTopLevelTracksDoNotGuess() throws Exception {
        File file = temporaryFile();
        try (RandomAccessFile out = new RandomAccessFile(file, "rw")) {
            out.write(ftyp());
            out.write(moov(1));
            out.write(moov(2));
            assertEquals(SpatialMp4Metadata.Status.UNSUPPORTED,
                SpatialMp4Metadata.inspect(out.getChannel()).status);
        } finally {
            file.delete();
        }
    }

    @Test public void massiveMoovIsBoundedAndReturnsUnknown() throws Exception {
        File file = temporaryFile();
        try (RandomAccessFile out = new RandomAccessFile(file, "rw")) {
            out.write(ftyp());
            out.write(be32(SpatialMp4Metadata.MAX_SCAN_BYTES + 8));
            out.write(ascii("moov"));
            out.setLength(ftyp().length
                + SpatialMp4Metadata.MAX_SCAN_BYTES + 8L);
            assertEquals(SpatialMp4Metadata.Status.UNKNOWN,
                SpatialMp4Metadata.inspect(out.getChannel()).status);
        } finally {
            file.delete();
        }
    }

    @Test public void corruptOrEndlessMediaBoxesAreNeverSearched() throws Exception {
        File file = temporaryFile();
        try (RandomAccessFile out = new RandomAccessFile(file, "rw")) {
            out.write(ftyp());
            out.write(be32(MEDIA_BYTES));
            out.write(ascii("mdat"));
            out.write(moov(2)); // buried in invalid/truncated mdat
            assertEquals(SpatialMp4Metadata.Status.UNKNOWN,
                SpatialMp4Metadata.inspect(out.getChannel()).status);
        } finally {
            file.delete();
        }
    }

    @Test public void zeroLengthMdatConsumesRestOfFile() throws Exception {
        File file = temporaryFile();
        try (RandomAccessFile out = new RandomAccessFile(file, "rw")) {
            out.write(ftyp());
            out.write(be32(0));
            out.write(ascii("mdat"));
            out.write(moov(2));
            assertEquals(SpatialMp4Metadata.Status.UNKNOWN,
                SpatialMp4Metadata.inspect(out.getChannel()).status);
        } finally {
            file.delete();
        }
    }

    private static File temporaryFile() throws IOException {
        return File.createTempFile("reverie-spatial-", ".mp4");
    }

    private static byte[] ftyp() throws IOException {
        return box("ftyp", concat(ascii("isom"), be32(0)));
    }

    private static byte[] moov(int stereo) throws IOException {
        byte[] st3d = box("st3d", new byte[] {0, 0, 0, 0, (byte) stereo});
        byte[] equi = box("equi", new byte[20]);
        byte[] sv3d = box("sv3d", box("proj", equi));
        byte[] sample = box("avc1", concat(new byte[78], st3d, sv3d));
        byte[] stsd = box("stsd", concat(new byte[4], be32(1), sample));
        return box("moov",
            box("trak", box("mdia", box("minf", box("stbl", stsd)))));
    }

    private static byte[] box(String type, byte[] data) throws IOException {
        return concat(be32(data.length + 8), ascii(type), data);
    }

    private static byte[] ascii(String value) {
        return value.getBytes(StandardCharsets.US_ASCII);
    }

    private static byte[] be32(int v) {
        return new byte[] {
            (byte) (v >>> 24), (byte) (v >>> 16),
            (byte) (v >>> 8), (byte) v
        };
    }

    private static byte[] be64(long v) {
        return new byte[] {
            (byte) (v >>> 56), (byte) (v >>> 48),
            (byte) (v >>> 40), (byte) (v >>> 32),
            (byte) (v >>> 24), (byte) (v >>> 16),
            (byte) (v >>> 8), (byte) v
        };
    }

    private static byte[] concat(byte[]... values) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        for (byte[] bytes : values) out.write(bytes);
        return out.toByteArray();
    }
}
