package io.github.mrcalzon02.reverievr;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.zip.CRC32;
import java.util.zip.DataFormatException;
import java.util.zip.Inflater;

/**
 * Bounded Spherical Video V2 mshp decoder (RFC mesh projection), intended
 * for calibrated camera-native VR180. This class only decodes camera geometry;
 * a video player must separately synchronize any camm orientation track.
 */
final class Vr180MeshProjection {
    static final int MAX_INFLATED_BYTES = 2 * 1024 * 1024;
    static final int MAX_COORDINATES = 65536;
    static final int MAX_VERTICES = 30000;
    static final int MAX_RENDER_VERTICES = 90000;

    static final class EyeMesh {
        final float[] xyz;
        final float[] uv;
        final int vertexCount;
        EyeMesh(float[] xyz, float[] uv, int vertexCount) {
            this.xyz = xyz;
            this.uv = uv;
            this.vertexCount = vertexCount;
        }
    }

    static final class Projection {
        final EyeMesh[] meshes;
        final int packing;
        Projection(EyeMesh[] meshes, int packing) {
            this.meshes = meshes;
            this.packing = packing;
        }
        EyeMesh forEye(boolean leftEye) {
            return meshes[meshes.length == 1 || leftEye ? 0 : 1];
        }
        boolean usesPackedStereoCrop() {
            return packing == 1 || packing == 2;
        }
    }

    private Vr180MeshProjection() { }

    static Projection decode(byte[] bytes, int start, int end, int packing)
            throws IOException {
        if (bytes == null || start < 0 || end > bytes.length
                || end - start < 12 || end - start > MAX_INFLATED_BYTES
                || packing < 0 || packing > 3) {
            throw new IOException("Invalid calibrated mesh metadata boundaries");
        }
        if (unsigned32(bytes, start) != 0) {
            throw new IOException("Unknown mshp full-box version or flags");
        }
        long expected = unsigned32(bytes, start + 4);
        CRC32 crc = new CRC32();
        crc.update(bytes, start + 8, end - start - 8);
        if (crc.getValue() != expected) {
            throw new IOException("Calibrated mesh CRC mismatch");
        }
        String encoding = new String(bytes, start + 8, 4,
            StandardCharsets.US_ASCII);
        byte[] raw;
        if ("raw ".equals(encoding)) {
            raw = Arrays.copyOfRange(bytes, start + 12, end);
        } else if ("dfl8".equals(encoding)) {
            raw = inflate(bytes, start + 12, end - start - 12);
        } else {
            throw new IOException("Unsupported calibrated mesh compression");
        }
        if (raw.length < 8) {
            throw new IOException("Empty calibrated projection mesh");
        }
        EyeMesh[] eyes = new EyeMesh[2];
        int count = 0;
        int pos = 0;
        while (pos < raw.length) {
            if (raw.length - pos < 8) {
                throw new IOException("Truncated mesh box header");
            }
            long size = unsigned32(raw, pos);
            String type = new String(raw, pos + 4, 4,
                StandardCharsets.US_ASCII);
            if (size < 8 || size > raw.length - pos) {
                throw new IOException("Invalid mesh box size");
            }
            if ("mesh".equals(type)) {
                if (count >= eyes.length) {
                    throw new IOException("More than two mesh projections");
                }
                eyes[count++] = parseEye(raw, pos + 8, pos + (int) size);
            }
            pos += (int) size;
        }
        if (count == 0 || (packing == 3 && count != 2)
                || (packing == 0 && count != 1)) {
            throw new IOException("Mesh count inconsistent with stereo packing");
        }
        return new Projection(Arrays.copyOf(eyes, count), packing);
    }

    private static byte[] inflate(byte[] bytes, int start, int count)
            throws IOException {
        Inflater inflater = new Inflater(true);
        try {
            inflater.setInput(bytes, start, count);
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] block = new byte[8192];
            while (!inflater.finished()) {
                int read;
                try {
                    read = inflater.inflate(block);
                } catch (DataFormatException bad) {
                    throw new IOException("Corrupt raw-deflate mesh", bad);
                }
                if (read == 0) {
                    throw new IOException("Truncated or stalled deflate mesh");
                }
                if (out.size() + read > MAX_INFLATED_BYTES) {
                    throw new IOException("Decompressed projection exceeds cap");
                }
                out.write(block, 0, read);
            }
            if (inflater.getRemaining() != 0) {
                throw new IOException("Trailing compressed projection bytes");
            }
            return out.toByteArray();
        } finally {
            inflater.end();
        }
    }

    private static EyeMesh parseEye(byte[] bytes, int start, int end)
            throws IOException {
        Bits bits = new Bits(bytes, start, end);
        int coordinateCount = bits.count32(MAX_COORDINATES);
        float[] coordinates = new float[coordinateCount];
        for (int i = 0; i < coordinates.length; i++) {
            coordinates[i] = Float.intBitsToFloat((int) bits.read(32));
            if (!Float.isFinite(coordinates[i])) {
                throw new IOException("Nonfinite calibrated mesh coordinate");
            }
        }
        int vertexCount = bits.count32(MAX_VERTICES);
        int coordBits = width(coordinateCount);
        float[] vertex = new float[vertexCount * 5];
        int[] previous = new int[5];
        for (int v = 0; v < vertexCount; v++) {
            for (int axis = 0; axis < 5; axis++) {
                long packedDelta = bits.read(coordBits);
                int delta = ((packedDelta & 1) != 0)
                    ? (int) -((packedDelta + 1) / 2)
                    : (int) (packedDelta / 2);
                int index = previous[axis] + delta;
                if (index < 0 || index >= coordinateCount) {
                    throw new IOException("Mesh coordinate index outside table");
                }
                previous[axis] = index;
                float value = coordinates[index];
                if (axis >= 3 && (value < 0 || value > 1)) {
                    throw new IOException("UV coordinate outside eye image");
                }
                vertex[v * 5 + axis] = value;
            }
        }
        bits.alignZero();
        int listCount = bits.count32(64);
        int indexBits = width(vertexCount);
        Grow mesh = new Grow();
        for (int list = 0; list < listCount; list++) {
            int texture = (int) bits.read(8);
            int type = (int) bits.read(8);
            if (texture != 0 || type > 2) {
                throw new IOException("Unsupported projection texture or topology");
            }
            int indexCount = bits.count32(MAX_RENDER_VERTICES);
            if (indexCount < 3) {
                throw new IOException("Empty mesh triangle list");
            }
            int[] indices = new int[indexCount];
            int previousIndex = 0;
            for (int i = 0; i < indices.length; i++) {
                long encoded = bits.read(indexBits);
                int delta = (encoded & 1) == 0
                    ? (int) (encoded / 2)
                    : (int) -((encoded + 1) / 2);
                int index = previousIndex + delta;
                if (index < 0 || index >= vertexCount) {
                    throw new IOException("Mesh vertex index outside table");
                }
                indices[i] = index;
                previousIndex = index;
            }
            bits.alignZero();
            if (type == 0) {
                if (indexCount % 3 != 0) {
                    throw new IOException("Incomplete triangle index list");
                }
                for (int i = 0; i < indexCount; i += 3) {
                    mesh.triangle(vertex, indices[i], indices[i + 1],
                        indices[i + 2]);
                }
            } else {
                for (int i = 2; i < indexCount; i++) {
                    int a = type == 2 ? indices[0] : indices[i - 2];
                    int b = indices[i - 1];
                    if (type == 1 && (i & 1) != 0) {
                        int swap = a; a = b; b = swap;
                    }
                    mesh.triangle(vertex, a, b, indices[i]);
                }
            }
        }
        if (mesh.count == 0) {
            throw new IOException("Projection has no renderable triangles");
        }
        return mesh.finish();
    }

    private static int width(int size) {
        // ceil(log2(size*2)), never zero for valid counts.
        return 32 - Integer.numberOfLeadingZeros(size * 2 - 1);
    }

    private static long unsigned32(byte[] bytes, int start) {
        return ((long) (bytes[start] & 255) << 24)
            | ((long) (bytes[start + 1] & 255) << 16)
            | ((long) (bytes[start + 2] & 255) << 8)
            | (bytes[start + 3] & 255);
    }

    private static final class Grow {
        float[] xyz = new float[1024 * 3];
        float[] uv = new float[1024 * 2];
        int count;

        void triangle(float[] vertices, int a, int b, int c)
                throws IOException {
            if (count + 3 > MAX_RENDER_VERTICES) {
                throw new IOException("Projection exceeds GPU triangle budget");
            }
            put(vertices, a);
            put(vertices, b);
            put(vertices, c);
        }

        private void put(float[] source, int index) {
            if (count * 3 == xyz.length) {
                xyz = Arrays.copyOf(xyz, Math.min(MAX_RENDER_VERTICES * 3,
                    xyz.length * 2));
                uv = Arrays.copyOf(uv, Math.min(MAX_RENDER_VERTICES * 2,
                    uv.length * 2));
            }
            int src = index * 5;
            xyz[count * 3] = source[src];
            xyz[count * 3 + 1] = source[src + 1];
            xyz[count * 3 + 2] = source[src + 2];
            uv[count * 2] = source[src + 3];
            uv[count * 2 + 1] = source[src + 4];
            count++;
        }

        EyeMesh finish() {
            return new EyeMesh(Arrays.copyOf(xyz, count * 3),
                Arrays.copyOf(uv, count * 2), count);
        }
    }

    private static final class Bits {
        final byte[] bytes;
        final int limit;
        int bit;

        Bits(byte[] bytes, int start, int end) {
            this.bytes = bytes;
            this.limit = end * 8;
            this.bit = start * 8;
        }

        long read(int width) throws IOException {
            if (width < 1 || width > 32 || (long) bit + width > limit) {
                throw new IOException("Truncated projection mesh bitstream");
            }
            long result = 0;
            for (int i = 0; i < width; i++) {
                int one = (bytes[bit >>> 3] >>> (7 - (bit & 7))) & 1;
                result = (result << 1) | one;
                bit++;
            }
            return result;
        }

        int count32(int maximum) throws IOException {
            long value = read(32);
            if ((value & 0x80000000L) != 0 || value == 0
                    || value > maximum) {
                throw new IOException("Projection count outside safe bounds");
            }
            return (int) value;
        }

        void alignZero() throws IOException {
            while ((bit & 7) != 0) {
                if (read(1) != 0) {
                    throw new IOException("Nonzero projection padding");
                }
            }
        }
    }
}
