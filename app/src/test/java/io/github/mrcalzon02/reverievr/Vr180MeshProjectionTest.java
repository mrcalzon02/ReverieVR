package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.zip.CRC32;
import java.util.zip.Deflater;

public final class Vr180MeshProjectionTest {
    @Test public void rawMeshExtractsCalibratedEyeTriangles() throws Exception {
        byte[] mshp = projection("raw ", 2, 1);
        Vr180MeshProjection.Projection result =
            Vr180MeshProjection.decode(mshp, 0, mshp.length, 2);
        assertEquals(1, result.meshes.length);
        assertEquals(2, result.packing);
        assertEquals(3, result.forEye(true).vertexCount);
        assertEquals(-1, result.forEye(true).xyz[0], 0.001f);
        assertEquals(1, result.forEye(true).xyz[3], 0.001f);
        assertEquals(0, result.forEye(true).uv[0], 0.001f);
        assertEquals(1, result.forEye(true).uv[2], 0.001f);
    }

    @Test public void deflateStereoCustomCarriesDistinctEyeMeshes() throws Exception {
        byte[] mshp = projection("dfl8", 3, 2);
        Vr180MeshProjection.Projection result =
            Vr180MeshProjection.decode(mshp, 0, mshp.length, 3);
        assertEquals(2, result.meshes.length);
        assertTrue(!result.usesPackedStereoCrop());
        assertNotNull(result.forEye(false));
        assertEquals(3, result.forEye(false).vertexCount);
        assertEquals(1, result.forEye(false).uv[2], 0.001f);
    }

    @Test public void entireSpatialMp4MetadataPathUsesVerifiedMesh()
        throws Exception {
        byte[] mshp = projection("raw ", 2, 2);
        byte[] st3d = box("st3d", new byte[] {0, 0, 0, 0, 2});
        byte[] sv3d = box("sv3d", box("proj",
            box("mshp", mshp)));
        byte[] sample = box("avc1",
            concat(new byte[78], st3d, sv3d));
        byte[] stsd = box("stsd",
            concat(new byte[4], be32(1), sample));
        byte[] moov = box("moov",
            box("trak", box("mdia",
                box("minf", box("stbl", stsd)))));
        byte[] mp4 = concat(box("ftyp", new byte[8]), moov);
        SpatialMp4Metadata.Result result =
            SpatialMp4Metadata.inspect(new ByteArrayInputStream(mp4));
        assertEquals(SpatialMp4Metadata.Status.SUPPORTED, result.status);
        assertEquals(VideoProjection.CALIBRATED_MESH_VR180,
            result.projection);
        assertEquals(2, result.calibratedMesh.meshes.length);
    }

    @Test public void corruptedCrcAndIncorrectMeshCountFailClosed()
        throws Exception {
        byte[] valid = projection("raw ", 3, 2);
        valid[4] ^= 1;
        fails(valid, 3);
        fails(projection("raw ", 3, 1), 3);
        fails(projection("raw ", 0, 2), 0);
    }

    @Test public void unknownEncodingAndTruncatedPayloadFailClosed()
        throws Exception {
        byte[] wrong = projection("xxxx", 2, 1);
        fails(wrong, 2);
        byte[] valid = projection("dfl8", 2, 1);
        byte[] truncated = java.util.Arrays.copyOf(
            valid, valid.length - 3);
        fails(truncated, 2);
    }

    private static void fails(byte[] data, int stereo) {
        boolean rejected = false;
        try {
            Vr180MeshProjection.decode(data, 0, data.length, stereo);
        } catch (IOException expected) {
            rejected = true;
        }
        assertTrue("Invalid mesh must be rejected", rejected);
    }

    private static byte[] projection(
            String encoding, int stereo, int eyes) throws IOException {
        byte[] raw = eyes == 2
            ? concat(mesh(), mesh()) : mesh();
        byte[] data = "dfl8".equals(encoding) ? deflate(raw) : raw;
        byte[] crcContent = concat(encoding.getBytes(
            StandardCharsets.US_ASCII), data);
        CRC32 crc = new CRC32();
        crc.update(crcContent);
        return concat(be32(0), be32((int) crc.getValue()), crcContent);
    }

    private static byte[] mesh() throws IOException {
        BitWriter w = new BitWriter();
        float[] coords = {-1f, 0f, 1f, 0.5f};
        w.write(coords.length, 32);
        for (float x : coords) {
            w.write(Float.floatToIntBits(x) & 0xffffffffL, 32);
        }
        int[][] vertices = {
            {0, 0, 0, 1, 1},
            {2, 0, 0, 2, 1},
            {1, 2, 0, 3, 2}
        };
        w.write(vertices.length, 32);
        int[] previous = new int[5];
        for (int[] vertex : vertices) {
            for (int i = 0; i < 5; i++) {
                int delta = vertex[i] - previous[i];
                previous[i] = vertex[i];
                w.write(delta >= 0 ? 2 * delta : -2 * delta - 1, 3);
            }
        }
        w.align();
        w.write(1, 32);
        w.write(0, 8); // video texture
        w.write(0, 8); // triangles
        w.write(3, 32);
        w.write(0, 3);
        w.write(2, 3);
        w.write(2, 3);
        w.align();
        return box("mesh", w.bytes());
    }

    private static byte[] deflate(byte[] bytes) throws IOException {
        Deflater d = new Deflater(6, true);
        try {
            d.setInput(bytes);
            d.finish();
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] block = new byte[1024];
            while (!d.finished()) {
                int n = d.deflate(block);
                if (n <= 0) throw new IOException("Deflate stalled");
                out.write(block, 0, n);
            }
            return out.toByteArray();
        } finally {
            d.end();
        }
    }

    private static byte[] box(String type, byte[] payload)
        throws IOException {
        return concat(be32(payload.length + 8),
            type.getBytes(StandardCharsets.US_ASCII), payload);
    }

    private static byte[] be32(int value) {
        return new byte[] {(byte) (value >>> 24),
            (byte) (value >>> 16), (byte) (value >>> 8), (byte) value};
    }

    private static byte[] concat(byte[]... parts)
            throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        for (byte[] data : parts) out.write(data);
        return out.toByteArray();
    }

    private static final class BitWriter {
        final ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        int accumulator;
        int pending;
        void write(long value, int bits) {
            for (int i = bits - 1; i >= 0; i--) {
                accumulator = (accumulator << 1)
                    | (int) ((value >>> i) & 1);
                pending++;
                if (pending == 8) {
                    bytes.write(accumulator);
                    accumulator = 0;
                    pending = 0;
                }
            }
        }
        void align() {
            if (pending != 0) write(0, 8 - pending);
        }
        byte[] bytes() {
            align();
            return bytes.toByteArray();
        }
    }
}
