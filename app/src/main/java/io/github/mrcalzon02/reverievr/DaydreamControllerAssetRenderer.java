package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.Matrix;

import com.google.cardboard.sdk.CardboardView;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

final class DaydreamControllerAssetRenderer {
    private static final String ASSET_PATH =
        "models/daydream/vr_controller_daydream.obj";
    private static final float MODEL_SCALE = 1.20f;
    private static final float Z_NEAR = 0.10f;
    private static final float Z_FAR = 30.0f;

    private static final float[] PRESSED_COLOR =
        new float[] {0.18f, 0.88f, 1.00f};
    private static final float[] BODY_COLOR =
        new float[] {0.62f, 0.64f, 0.67f};
    private static final float[] TOUCHPAD_COLOR =
        new float[] {0.39f, 0.41f, 0.44f};
    private static final float[] BUTTON_COLOR =
        new float[] {0.72f, 0.74f, 0.77f};

    private final List<MeshPart> parts;

    private final float[] assetModel = new float[16];
    private final float[] modelView = new float[16];
    private final float[] mvp = new float[16];

    private int program;
    private int positionHandle;
    private int normalHandle;
    private int mvpHandle;
    private int modelViewHandle;
    private int colorHandle;

    DaydreamControllerAssetRenderer(
        Context context
    ) {
        parts = load(context);
    }

    boolean isAvailable() {
        return !parts.isEmpty();
    }

    boolean isRenderable() {
        return isAvailable()
            && program != 0;
    }

    void onSurfaceCreated() {
        program = 0;
        if (!isAvailable()) {
            return;
        }

        try {
            program =
                buildProgram(
                    VERTEX_SHADER,
                    FRAGMENT_SHADER
                );
            positionHandle =
                GLES20.glGetAttribLocation(
                    program,
                    "a_Position"
                );
            normalHandle =
                GLES20.glGetAttribLocation(
                    program,
                    "a_Normal"
                );
            mvpHandle =
                GLES20.glGetUniformLocation(
                    program,
                    "u_Mvp"
                );
            modelViewHandle =
                GLES20.glGetUniformLocation(
                    program,
                    "u_ModelView"
                );
            colorHandle =
                GLES20.glGetUniformLocation(
                    program,
                    "u_Color"
                );
        } catch (RuntimeException exception) {
            program = 0;
            ReverieLog.error(
                "CONTROLLER_MODEL",
                "Daydream controller GL setup failed; using procedural fallback.",
                exception
            );
        }
    }

    void drawEye(
        CardboardView.Eye eye,
        float[] correctedEyeView,
        float[] root,
        boolean touchpadPressed,
        boolean homePressed,
        boolean appPressed,
        boolean volumeUpPressed,
        boolean volumeDownPressed
    ) {
        if (!isAvailable()
            || program == 0
            || eye == null
            || correctedEyeView == null
            || root == null) {
            return;
        }

        Matrix.scaleM(
            assetModel,
            0,
            root,
            0,
            MODEL_SCALE,
            MODEL_SCALE,
            MODEL_SCALE
        );
        Matrix.multiplyMM(
            modelView,
            0,
            correctedEyeView,
            0,
            assetModel,
            0
        );
        Matrix.multiplyMM(
            mvp,
            0,
            eye.getPerspective(
                Z_NEAR,
                Z_FAR
            ),
            0,
            modelView,
            0
        );

        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_BLEND);
        GLES20.glUseProgram(program);

        GLES20.glUniformMatrix4fv(
            mvpHandle,
            1,
            false,
            mvp,
            0
        );
        GLES20.glUniformMatrix4fv(
            modelViewHandle,
            1,
            false,
            modelView,
            0
        );

        for (MeshPart part : parts) {
            boolean pressed =
                isPressed(
                    part.material,
                    touchpadPressed,
                    homePressed,
                    appPressed,
                    volumeUpPressed,
                    volumeDownPressed
                );
            float[] color =
                materialColor(
                    part.material,
                    pressed
                );

            part.positions.position(0);
            GLES20.glVertexAttribPointer(
                positionHandle,
                3,
                GLES20.GL_FLOAT,
                false,
                0,
                part.positions
            );
            GLES20.glEnableVertexAttribArray(
                positionHandle
            );

            part.normals.position(0);
            GLES20.glVertexAttribPointer(
                normalHandle,
                3,
                GLES20.GL_FLOAT,
                false,
                0,
                part.normals
            );
            GLES20.glEnableVertexAttribArray(
                normalHandle
            );

            GLES20.glUniform4f(
                colorHandle,
                color[0],
                color[1],
                color[2],
                1.0f
            );
            GLES20.glDrawArrays(
                GLES20.GL_TRIANGLES,
                0,
                part.vertexCount
            );
        }

        GLES20.glDisableVertexAttribArray(
            positionHandle
        );
        GLES20.glDisableVertexAttribArray(
            normalHandle
        );
    }

    void shutdown() {
        if (program != 0) {
            GLES20.glDeleteProgram(program);
            program = 0;
        }
    }

    private static boolean isPressed(
        String material,
        boolean touchpadPressed,
        boolean homePressed,
        boolean appPressed,
        boolean volumeUpPressed,
        boolean volumeDownPressed
    ) {
        if ("MatTouchpad".equals(material)) {
            return touchpadPressed;
        }
        if ("MatHomeButton".equals(material)) {
            return homePressed;
        }
        if ("MatAppButton".equals(material)) {
            return appPressed;
        }
        if ("MatVolUpBut".equals(material)) {
            return volumeUpPressed;
        }
        if ("MatVolDownBut".equals(material)) {
            return volumeDownPressed;
        }
        return false;
    }

    private static float[] materialColor(
        String material,
        boolean pressed
    ) {
        if (pressed) {
            return PRESSED_COLOR;
        }

        if ("MatBody".equals(material)) {
            return BODY_COLOR;
        }
        if ("MatTouchpad".equals(material)) {
            return TOUCHPAD_COLOR;
        }

        return BUTTON_COLOR;
    }

    private static List<MeshPart> load(
        Context context
    ) {
        List<MeshPart> empty =
            new ArrayList<>();
        if (context == null) {
            return empty;
        }

        try (
            InputStream input =
                context
                    .getAssets()
                    .open(ASSET_PATH);
            BufferedReader reader =
                new BufferedReader(
                    new InputStreamReader(
                        input,
                        StandardCharsets.UTF_8
                    )
                )
        ) {
            List<float[]> vertices =
                new ArrayList<>();
            List<float[]> normals =
                new ArrayList<>();
            vertices.add(
                new float[] {
                    0.0f,
                    0.0f,
                    0.0f
                }
            );
            normals.add(
                new float[] {
                    0.0f,
                    1.0f,
                    0.0f
                }
            );

            Map<String, MeshBuilder> builders =
                new LinkedHashMap<>();
            String currentMaterial = "MatBody";
            builders.put(
                currentMaterial,
                new MeshBuilder(
                    currentMaterial
                )
            );

            String line;
            while ((line = reader.readLine())
                != null) {
                String trimmed = line.trim();
                if (trimmed.isEmpty()
                    || trimmed.startsWith("#")) {
                    continue;
                }

                if (trimmed.startsWith("v ")) {
                    vertices.add(
                        parseVector(trimmed)
                    );
                    continue;
                }

                if (trimmed.startsWith("vn ")) {
                    normals.add(
                        parseVector(trimmed)
                    );
                    continue;
                }

                if (trimmed.startsWith("usemtl ")) {
                    currentMaterial =
                        trimmed.substring(7).trim();
                    if (!builders.containsKey(
                            currentMaterial
                        )) {
                        builders.put(
                            currentMaterial,
                            new MeshBuilder(
                                currentMaterial
                            )
                        );
                    }
                    continue;
                }

                if (!trimmed.startsWith("f ")) {
                    continue;
                }

                String[] tokens =
                    trimmed.substring(2)
                        .trim()
                        .split("\\s+");
                if (tokens.length < 3) {
                    continue;
                }

                MeshBuilder builder =
                    builders.get(
                        currentMaterial
                    );
                FaceVertex first =
                    parseFaceVertex(
                        tokens[0],
                        vertices.size(),
                        normals.size()
                    );

                for (int index = 1;
                     index + 1 < tokens.length;
                     index++) {
                    FaceVertex second =
                        parseFaceVertex(
                            tokens[index],
                            vertices.size(),
                            normals.size()
                        );
                    FaceVertex third =
                        parseFaceVertex(
                            tokens[index + 1],
                            vertices.size(),
                            normals.size()
                        );
                    appendTriangle(
                        builder,
                        first,
                        second,
                        third,
                        vertices,
                        normals
                    );
                }
            }

            List<MeshPart> loaded =
                new ArrayList<>();
            for (MeshBuilder builder :
                builders.values()) {
                MeshPart part =
                    builder.build();
                if (part != null) {
                    loaded.add(part);
                }
            }

            if (!loaded.isEmpty()) {
                ReverieLog.milestone(
                    "CONTROLLER_MODEL",
                    "Vendored Daydream controller model loaded with "
                        + loaded.size()
                        + " material parts."
                );
            }
            return loaded;
        } catch (Exception exception) {
            ReverieLog.error(
                "CONTROLLER_MODEL",
                "Daydream controller model could not be loaded; "
                    + "procedural fallback remains available.",
                exception
            );
            return empty;
        }
    }

    private static float[] parseVector(
        String line
    ) {
        String[] values =
            line.split("\\s+");
        if (values.length < 4) {
            throw new IllegalArgumentException(
                "Malformed OBJ vector."
            );
        }
        return new float[] {
            Float.parseFloat(values[1]),
            Float.parseFloat(values[2]),
            Float.parseFloat(values[3])
        };
    }

    private static FaceVertex parseFaceVertex(
        String token,
        int vertexCount,
        int normalCount
    ) {
        String[] fields =
            token.split("/", -1);
        int vertex =
            resolveObjIndex(
                Integer.parseInt(fields[0]),
                vertexCount
            );
        int normal = 0;
        if (fields.length >= 3
            && !fields[2].isEmpty()) {
            normal =
                resolveObjIndex(
                    Integer.parseInt(fields[2]),
                    normalCount
                );
        }
        return new FaceVertex(
            vertex,
            normal
        );
    }

    private static int resolveObjIndex(
        int index,
        int size
    ) {
        int resolved =
            index > 0
                ? index
                : size + index;
        if (resolved <= 0
            || resolved >= size) {
            throw new IllegalArgumentException(
                "OBJ index is out of range."
            );
        }
        return resolved;
    }

    private static void appendTriangle(
        MeshBuilder builder,
        FaceVertex a,
        FaceVertex b,
        FaceVertex c,
        List<float[]> vertices,
        List<float[]> normals
    ) {
        float[] pa =
            vertices.get(a.vertex);
        float[] pb =
            vertices.get(b.vertex);
        float[] pc =
            vertices.get(c.vertex);

        float[] fallbackNormal =
            triangleNormal(
                pa,
                pb,
                pc
            );

        appendVertex(
            builder,
            pa,
            normalFor(
                a,
                normals,
                fallbackNormal
            )
        );
        appendVertex(
            builder,
            pb,
            normalFor(
                b,
                normals,
                fallbackNormal
            )
        );
        appendVertex(
            builder,
            pc,
            normalFor(
                c,
                normals,
                fallbackNormal
            )
        );
    }

    private static void appendVertex(
        MeshBuilder builder,
        float[] position,
        float[] normal
    ) {
        builder.positions.add(
            position[0]
        );
        builder.positions.add(
            position[1]
        );
        builder.positions.add(
            position[2]
        );
        builder.normals.add(
            normal[0]
        );
        builder.normals.add(
            normal[1]
        );
        builder.normals.add(
            normal[2]
        );
    }

    private static float[] normalFor(
        FaceVertex vertex,
        List<float[]> normals,
        float[] fallback
    ) {
        if (vertex.normal > 0
            && vertex.normal < normals.size()) {
            return normals.get(
                vertex.normal
            );
        }
        return fallback;
    }

    private static float[] triangleNormal(
        float[] a,
        float[] b,
        float[] c
    ) {
        float abX = b[0] - a[0];
        float abY = b[1] - a[1];
        float abZ = b[2] - a[2];
        float acX = c[0] - a[0];
        float acY = c[1] - a[1];
        float acZ = c[2] - a[2];

        float x =
            abY * acZ
                - abZ * acY;
        float y =
            abZ * acX
                - abX * acZ;
        float z =
            abX * acY
                - abY * acX;
        float length =
            (float) Math.sqrt(
                x * x
                    + y * y
                    + z * z
            );
        if (length <= 0.000001f) {
            return new float[] {
                0.0f,
                1.0f,
                0.0f
            };
        }
        return new float[] {
            x / length,
            y / length,
            z / length
        };
    }

    private static FloatBuffer buffer(
        float[] values
    ) {
        FloatBuffer result =
            ByteBuffer
                .allocateDirect(
                    values.length * 4
                )
                .order(
                    ByteOrder.nativeOrder()
                )
                .asFloatBuffer();
        result.put(values);
        result.position(0);
        return result;
    }

    private static int buildProgram(
        String vertexSource,
        String fragmentSource
    ) {
        int vertex =
            compileShader(
                GLES20.GL_VERTEX_SHADER,
                vertexSource
            );
        int fragment =
            compileShader(
                GLES20.GL_FRAGMENT_SHADER,
                fragmentSource
            );

        int result =
            GLES20.glCreateProgram();
        GLES20.glAttachShader(
            result,
            vertex
        );
        GLES20.glAttachShader(
            result,
            fragment
        );
        GLES20.glLinkProgram(result);

        int[] status = new int[1];
        GLES20.glGetProgramiv(
            result,
            GLES20.GL_LINK_STATUS,
            status,
            0
        );
        GLES20.glDeleteShader(vertex);
        GLES20.glDeleteShader(fragment);

        if (status[0] == 0) {
            String message =
                GLES20.glGetProgramInfoLog(
                    result
                );
            GLES20.glDeleteProgram(result);
            throw new IllegalStateException(
                "Daydream controller shader link failed: "
                    + message
            );
        }
        return result;
    }

    private static int compileShader(
        int type,
        String source
    ) {
        int shader =
            GLES20.glCreateShader(type);
        GLES20.glShaderSource(
            shader,
            source
        );
        GLES20.glCompileShader(shader);

        int[] status = new int[1];
        GLES20.glGetShaderiv(
            shader,
            GLES20.GL_COMPILE_STATUS,
            status,
            0
        );
        if (status[0] == 0) {
            String message =
                GLES20.glGetShaderInfoLog(
                    shader
                );
            GLES20.glDeleteShader(shader);
            throw new IllegalStateException(
                "Daydream controller shader compile failed: "
                    + message
            );
        }
        return shader;
    }

    private static final class FaceVertex {
        final int vertex;
        final int normal;

        FaceVertex(
            int vertex,
            int normal
        ) {
            this.vertex = vertex;
            this.normal = normal;
        }
    }

    private static final class MeshBuilder {
        final String material;
        final FloatValues positions =
            new FloatValues();
        final FloatValues normals =
            new FloatValues();

        MeshBuilder(
            String material
        ) {
            this.material = material;
        }

        MeshPart build() {
            if (positions.size == 0
                || positions.size
                    != normals.size) {
                return null;
            }
            float[] positionValues =
                positions.toArray();
            float[] normalValues =
                normals.toArray();
            return new MeshPart(
                material,
                buffer(positionValues),
                buffer(normalValues),
                positionValues.length / 3
            );
        }
    }

    private static final class MeshPart {
        final String material;
        final FloatBuffer positions;
        final FloatBuffer normals;
        final int vertexCount;

        MeshPart(
            String material,
            FloatBuffer positions,
            FloatBuffer normals,
            int vertexCount
        ) {
            this.material = material;
            this.positions = positions;
            this.normals = normals;
            this.vertexCount = vertexCount;
        }
    }

    private static final class FloatValues {
        private float[] values =
            new float[1024];
        private int size;

        void add(
            float value
        ) {
            if (size >= values.length) {
                float[] grown =
                    new float[
                        values.length * 2
                    ];
                System.arraycopy(
                    values,
                    0,
                    grown,
                    0,
                    values.length
                );
                values = grown;
            }
            values[size++] = value;
        }

        float[] toArray() {
            float[] result =
                new float[size];
            System.arraycopy(
                values,
                0,
                result,
                0,
                size
            );
            return result;
        }
    }

    private static final String VERTEX_SHADER =
        "uniform mat4 u_Mvp;\n"
            + "uniform mat4 u_ModelView;\n"
            + "attribute vec3 a_Position;\n"
            + "attribute vec3 a_Normal;\n"
            + "varying float v_Light;\n"
            + "void main() {\n"
            + "  vec3 normal = normalize(mat3(u_ModelView) * a_Normal);\n"
            + "  vec3 light = normalize(vec3(-0.35, 0.80, 0.45));\n"
            + "  v_Light = 0.38 + 0.62 * max(dot(normal, light), 0.0);\n"
            + "  gl_Position = u_Mvp * vec4(a_Position, 1.0);\n"
            + "}\n";

    private static final String FRAGMENT_SHADER =
        "precision mediump float;\n"
            + "uniform vec4 u_Color;\n"
            + "varying float v_Light;\n"
            + "void main() {\n"
            + "  gl_FragColor = vec4(u_Color.rgb * v_Light, u_Color.a);\n"
            + "}\n";
}
