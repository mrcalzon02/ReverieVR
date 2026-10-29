package io.github.mrcalzon02.reverievr;

/**
 * Low-poly depth for legacy wardrobe silhouettes. The old portrait authoring
 * coordinates describe a Y/Z cross section; this turns each triangle into
 * a closed, finite X-depth prism, not a camera-facing billboard.
 *
 * Only avatar parts use this helper. The cabinet and mirror stay planar,
 * and all player collision volumes remain unchanged.
 */
final class HomeAvatarVolumeGeometry {
    private HomeAvatarVolumeGeometry() {}

    static final int FLOATS_PER_INPUT_FLOAT = 8;

    static float[] extrude(float[] triangles, float halfDepth) {
        if (triangles == null || triangles.length % 9 != 0
            || !Float.isFinite(halfDepth) || halfDepth <= 0f) {
            throw new IllegalArgumentException("Avatar triangles/depth invalid");
        }
        float[] result = new float[triangles.length * FLOATS_PER_INPUT_FLOAT];
        write(triangles, result, halfDepth);
        return result;
    }

    /** No heap allocations for the locomotion-driven leg mesh. */
    static void write(float[] triangles, float[] output, float halfDepth) {
        if (triangles == null || output == null || triangles.length % 9 != 0
            || output.length < triangles.length * FLOATS_PER_INPUT_FLOAT
            || !Float.isFinite(halfDepth) || halfDepth <= 0f) {
            throw new IllegalArgumentException("Invalid avatar prism buffer");
        }
        int at = 0;
        for (int t = 0; t < triangles.length; t += 9) {
            // Front/back caps and three side quads. Both face and silhouette
            // retain their original Y/Z coordinates while gaining parallax.
            at = tri(output, at, triangles, t,  halfDepth, 0, 1, 2);
            at = tri(output, at, triangles, t, -halfDepth, 2, 1, 0);
            for (int edge = 0; edge < 3; edge++) {
                int a = edge, b = (edge + 1) % 3;
                at = vertex(output, at, triangles, t, a, halfDepth);
                at = vertex(output, at, triangles, t, b, halfDepth);
                at = vertex(output, at, triangles, t, b, -halfDepth);
                at = vertex(output, at, triangles, t, a, halfDepth);
                at = vertex(output, at, triangles, t, b, -halfDepth);
                at = vertex(output, at, triangles, t, a, -halfDepth);
            }
        }
        if (at != triangles.length * FLOATS_PER_INPUT_FLOAT) {
            throw new IllegalStateException("Avatar volume count mismatch");
        }
    }

    private static int tri(float[] output, int at, float[] source,
        int start, float halfDepth, int a, int b, int c) {
        at = vertex(output, at, source, start, a, halfDepth);
        at = vertex(output, at, source, start, b, halfDepth);
        return vertex(output, at, source, start, c, halfDepth);
    }

    private static int vertex(float[] output, int at, float[] source,
        int start, int vertex, float halfDepth) {
        int index = start + vertex * 3;
        output[at++] = source[index] + halfDepth;
        output[at++] = source[index + 1];
        output[at++] = source[index + 2];
        return at;
    }
}
