package io.github.mrcalzon02.reverievr;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.Reader;
import java.util.ArrayList;

/**
 * Bounded, dependency-free Wavefront OBJ triangle importer for the future
 * immersive file viewer. No material scripts or external references execute.
 */
final class ObjMeshReader {
    static final int MAX_SOURCE_VERTICES = 200000;
    static final int MAX_TRIANGLES = 250000;
    static final int MAX_LINE_LENGTH = 8192;

    static final class Mesh {
        final float[] triangles;
        final float[] bounds;
        Mesh(float[] triangles, float[] bounds) {
            this.triangles = triangles;
            this.bounds = bounds;
        }
        int triangleCount() { return triangles.length / 9; }
    }

    static Mesh read(Reader input) throws IOException {
        if (input == null) throw new IOException("Missing OBJ source");
        BufferedReader reader = new BufferedReader(input);
        ArrayList<float[]> vertices = new ArrayList<>();
        ArrayList<float[]> faces = new ArrayList<>();
        String line;
        int lineNumber = 0;
        float[] bounds = {Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY,
            Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY,
            Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY};
        while ((line = reader.readLine()) != null) {
            ++lineNumber;
            if (line.length() > MAX_LINE_LENGTH)
                throw new IOException("OBJ line too long at " + lineNumber);
            line = line.trim();
            if (line.isEmpty() || line.charAt(0) == '#') continue;
            String[] parts = line.split("\\s+");
            if ("v".equals(parts[0])) {
                if (parts.length < 4 || vertices.size() >= MAX_SOURCE_VERTICES)
                    throw new IOException("OBJ vertex limit/format at " + lineNumber);
                float[] vertex = new float[3];
                for (int axis = 0; axis < 3; axis++) {
                    try { vertex[axis] = Float.parseFloat(parts[axis + 1]); }
                    catch (NumberFormatException error) {
                        throw new IOException("Invalid OBJ coordinate", error);
                    }
                    if (!Float.isFinite(vertex[axis]))
                        throw new IOException("Nonfinite OBJ coordinate");
                    bounds[axis] = Math.min(bounds[axis], vertex[axis]);
                    bounds[axis + 3] = Math.max(bounds[axis + 3], vertex[axis]);
                }
                vertices.add(vertex);
            } else if ("f".equals(parts[0])) {
                if (parts.length < 4 || parts.length > 128)
                    throw new IOException("Unsupported OBJ polygon at " + lineNumber);
                int[] indices = new int[parts.length - 1];
                for (int i = 0; i < indices.length; i++) {
                    String part = parts[i + 1].split("/", -1)[0];
                    int index;
                    try { index = Integer.parseInt(part); }
                    catch (NumberFormatException error) {
                        throw new IOException("Invalid OBJ face index", error);
                    }
                    if (index == 0) throw new IOException("OBJ index zero");
                    index = index > 0 ? index - 1 : vertices.size() + index;
                    if (index < 0 || index >= vertices.size())
                        throw new IOException("OBJ face references missing vertex");
                    indices[i] = index;
                }
                for (int i = 1; i + 1 < indices.length; i++) {
                    if (faces.size() / 3 >= MAX_TRIANGLES)
                        throw new IOException("OBJ triangle limit exceeded");
                    faces.add(vertices.get(indices[0]));
                    faces.add(vertices.get(indices[i]));
                    faces.add(vertices.get(indices[i + 1]));
                }
            }
        }
        if (vertices.isEmpty() || faces.isEmpty())
            throw new IOException("OBJ has no drawable triangles");
        float[] trianglePositions = new float[faces.size() * 3];
        for (int i = 0; i < faces.size(); i++)
            System.arraycopy(faces.get(i), 0, trianglePositions, i * 3, 3);
        return new Mesh(trianglePositions, bounds);
    }
}
