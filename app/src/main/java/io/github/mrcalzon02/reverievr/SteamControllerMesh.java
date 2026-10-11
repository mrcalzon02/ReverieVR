package io.github.mrcalzon02.reverievr;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** Exact read contract for the bundled 2015 Steam Controller RVSC0001 asset. */
final class SteamControllerMesh {
    static final class Part {
        final String name;
        final FloatBuffer vertices;
        final int count;
        Part(String name, FloatBuffer vertices, int count) {
            this.name = name;
            this.vertices = vertices;
            this.count = count;
        }
    }
    static List<Part> read(InputStream input) throws IOException {
        DataInputStream data = new DataInputStream(input);
        byte[] magic = new byte[8];
        data.readFully(magic);
        if (!"RVSC0001".equals(new String(magic, "US-ASCII")))
            throw new IOException("Incorrect Steam Controller mesh header");
        int partCount = Integer.reverseBytes(data.readInt());
        if (partCount != 22)
            throw new IOException("Steam Controller part count mismatch");
        List<Part> result = new ArrayList<>(partCount);
        int totalVertices = 0;
        for (int i = 0; i < partCount; i++) {
            int nameSize = Integer.reverseBytes(data.readInt());
            int vertexCount = Integer.reverseBytes(data.readInt());
            if (nameSize <= 0 || nameSize > 160 ||
                vertexCount <= 0 || vertexCount > 100000 ||
                vertexCount % 3 != 0 || totalVertices + vertexCount > 40000)
                throw new IOException("Steam Controller mesh limits exceeded");
            byte[] nameBytes = new byte[nameSize];
            data.readFully(nameBytes);
            String name = new String(nameBytes, "US-ASCII");
            int floats = vertexCount * 6;
            byte[] bytes = new byte[floats * 4];
            data.readFully(bytes);
            ByteBuffer source = ByteBuffer.wrap(bytes)
                .order(ByteOrder.LITTLE_ENDIAN);
            ByteBuffer direct = ByteBuffer.allocateDirect(bytes.length)
                .order(ByteOrder.nativeOrder());
            FloatBuffer target = direct.asFloatBuffer();
            for (int n = 0; n < floats; n++) {
                float value = source.getFloat();
                if (!Float.isFinite(value))
                    throw new IOException("Nonfinite Steam Controller geometry");
                target.put(value);
            }
            target.position(0);
            result.add(new Part(name, target, vertexCount));
            totalVertices += vertexCount;
        }
        if (data.read() != -1)
            throw new IOException("Unexpected Steam Controller mesh tail");
        return Collections.unmodifiableList(result);
    }
    private SteamControllerMesh() {}
}
