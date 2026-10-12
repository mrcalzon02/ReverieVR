package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public final class SpatialMp4MetadataTest {
    @Test public void detectsPackedStereoAndSphericalProjection() throws Exception {
        assertMode(VideoProjection.FLAT_STEREO_SBS, movie(stereo(2)));
        assertMode(VideoProjection.FLAT_STEREO_TB, movie(stereo(1)));
        assertMode(VideoProjection.MONO_EQUIRECTANGULAR_360, movie(spatial(equi(0))));
        assertMode(VideoProjection.STEREO_EQUIRECTANGULAR_360_SBS,
            movie(stereo(2), spatial(equi(0))));
        assertMode(VideoProjection.STEREO_EQUIRECTANGULAR_360_TB,
            movie(stereo(1), spatial(equi(0))));
    }

    @Test public void rejectsProjectionThatRendererCannotRepresent() throws Exception {
        assertUnsupported(movie(stereo(3), spatial(box("mshp", new byte[8]))));
        assertUnsupported(movie(stereo(4)));
        assertUnsupported(movie(spatial(box("mshp", new byte[8]))));
        assertUnsupported(movie(spatial(equi(1))));
        assertUnsupported(movie(spatial(box("prhd",
            concat(new byte[4], be32(1), be32(0), be32(0))), equi(0))));
        assertUnsupported(movie(spatial(box("equi", new byte[4]))));
    }

    @Test public void doesNotScanFakeBoxesInMdatOrUnrelatedFile() throws Exception {
        byte[] fake = concat(stereo(2), spatial(equi(0)));
        assertEquals(SpatialMp4Metadata.Status.UNKNOWN,
            SpatialMp4Metadata.inspect(new ByteArrayInputStream(
                concat(box("ftyp", new byte[8]), box("mdat", fake))))
                .status);
        assertEquals(SpatialMp4Metadata.Status.UNKNOWN,
            SpatialMp4Metadata.inspect(new ByteArrayInputStream(fake)).status);
    }

    @Test public void respectsBoundedTruncatedOrMissingMetadata() throws Exception {
        byte[] full = movie(stereo(2));
        byte[] truncated = java.util.Arrays.copyOf(full, full.length - 3);
        assertEquals(SpatialMp4Metadata.Status.UNKNOWN,
            SpatialMp4Metadata.inspect(new ByteArrayInputStream(truncated))
                .status);
        byte[] huge = concat(box("ftyp", new byte[8]),
            be32(SpatialMp4Metadata.MAX_SCAN_BYTES + 512),
            "mdat".getBytes(StandardCharsets.US_ASCII), new byte[16]);
        assertEquals(SpatialMp4Metadata.Status.UNKNOWN,
            SpatialMp4Metadata.inspect(new ByteArrayInputStream(huge)).status);
    }

    @Test public void conflictingVideoTracksAreNotSilentlyChosen() throws Exception {
        byte[] one = track(stereo(2));
        byte[] two = track(stereo(1));
        byte[] bytes = concat(box("ftyp", new byte[8]), box("moov",
            concat(one, two)));
        assertUnsupported(bytes);
    }

    private static void assertMode(VideoProjection mode, byte[] bytes)
        throws IOException {
        SpatialMp4Metadata.Result found =
            SpatialMp4Metadata.inspect(new ByteArrayInputStream(bytes));
        assertEquals(SpatialMp4Metadata.Status.SUPPORTED, found.status);
        assertEquals(mode, found.projection);
    }

    private static void assertUnsupported(byte[] bytes) throws IOException {
        SpatialMp4Metadata.Result found =
            SpatialMp4Metadata.inspect(new ByteArrayInputStream(bytes));
        assertEquals(SpatialMp4Metadata.Status.UNSUPPORTED, found.status);
        assertNull(found.projection);
    }

    private static byte[] movie(byte[]... children) throws IOException {
        return concat(box("ftyp", new byte[8]), box("moov",
            track(children)));
    }

    private static byte[] track(byte[]... children) throws IOException {
        byte[] visual = box("avc1", concat(new byte[78], concat(children)));
        byte[] stsd = box("stsd", concat(new byte[4], be32(1), visual));
        return box("trak", box("mdia", box("minf",
            box("stbl", stsd))));
    }

    private static byte[] stereo(int mode) throws IOException {
        return box("st3d", new byte[]{0, 0, 0, 0, (byte) mode});
    }

    private static byte[] spatial(byte[]... projections) throws IOException {
        return box("sv3d", box("proj", concat(projections)));
    }

    private static byte[] equi(int cropLeft) throws IOException {
        return box("equi", concat(new byte[4], be32(0), be32(0),
            be32(cropLeft), be32(0)));
    }

    private static byte[] box(String type, byte[] payload) throws IOException {
        return concat(be32(payload.length + 8),
            type.getBytes(StandardCharsets.US_ASCII), payload);
    }

    private static byte[] be32(int value) {
        return new byte[]{(byte) (value >>> 24), (byte) (value >>> 16),
            (byte) (value >>> 8), (byte) value};
    }

    private static byte[] concat(byte[]... values) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        for (byte[] value : values) out.write(value);
        return out.toByteArray();
    }
}
