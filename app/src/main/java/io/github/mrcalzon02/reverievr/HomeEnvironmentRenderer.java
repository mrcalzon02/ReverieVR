package io.github.mrcalzon02.reverievr;

import android.opengl.GLES20;
import android.opengl.Matrix;

import com.google.cardboard.sdk.CardboardView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.List;

final class HomeEnvironmentRenderer {
    private static final float Z_NEAR = 0.10f;
    private static final float Z_FAR = 40.0f;

    private final Mesh whiteFloor =
        mesh(quadHorizontal(-6.0f, 6.0f, -6.0f, 6.0f, -1.35f));
    private final Mesh whiteCeiling =
        mesh(quadHorizontal(-6.0f, 6.0f, -6.0f, 6.0f, 4.0f));
    private final Mesh whiteBack =
        mesh(quadVerticalZ(-6.0f, 6.0f, -1.35f, 4.0f, -6.0f));
    private final Mesh whiteFront =
        mesh(quadVerticalZ(6.0f, -6.0f, -1.35f, 4.0f, 6.0f));
    private final Mesh whiteLeft =
        mesh(quadVerticalX(-6.0f, 6.0f, -1.35f, 4.0f, -6.0f));
    private final Mesh whiteRight =
        mesh(quadVerticalX(6.0f, -6.0f, -1.35f, 4.0f, 6.0f));
    private final Mesh daisTop =
        mesh(quadHorizontal(-1.75f, 1.75f, -4.1f, -1.35f, -1.08f));
    private final Mesh daisFront =
        mesh(quadVerticalZ(-1.75f, 1.75f, -1.35f, -1.08f, -1.35f));

    private final Mesh outdoorGround =
        mesh(quadHorizontal(-24.0f, 24.0f, -24.0f, 24.0f, -1.35f));
    private final Mesh forestTrunks =
        mesh(buildForestTrunks());
    private final Mesh forestCanopies =
        mesh(buildForestCanopies());
    private final Mesh ocean =
        mesh(quadHorizontal(-24.0f, 24.0f, -30.0f, -6.0f, -1.30f));
    private final Mesh duneRidges =
        mesh(buildDuneRidges());
    private final Mesh beachGrass =
        mesh(buildBeachGrass());

    private final float[] eyeView = new float[16];
    private final float[] correctedEyeView = new float[16];
    private final float[] model = new float[16];
    private final float[] modelView = new float[16];
    private final float[] mvp = new float[16];

    private int program;
    private int positionHandle;
    private int mvpHandle;
    private int colorHandle;

    void onSurfaceCreated() {
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
        mvpHandle =
            GLES20.glGetUniformLocation(
                program,
                "u_Mvp"
            );
        colorHandle =
            GLES20.glGetUniformLocation(
                program,
                "u_Color"
            );
    }

    void drawEye(
        CardboardView.Eye eye,
        float eyeCorrectionMeters,
        HomeEnvironment environment
    ) {
        HomeEnvironment safe =
            environment == null
                ? HomeEnvironment.WHITE_ROOM
                : environment;

        switch (safe) {
            case FOREST_GLADE:
                GLES20.glClearColor(
                    0.48f,
                    0.70f,
                    0.90f,
                    1.0f
                );
                break;
            case DUNE_BEACH:
                GLES20.glClearColor(
                    0.50f,
                    0.72f,
                    0.92f,
                    1.0f
                );
                break;
            case WHITE_ROOM:
            default:
                GLES20.glClearColor(
                    0.90f,
                    0.91f,
                    0.92f,
                    1.0f
                );
                break;
        }

        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_BLEND);
        GLES20.glClear(
            GLES20.GL_COLOR_BUFFER_BIT
                | GLES20.GL_DEPTH_BUFFER_BIT
        );

        System.arraycopy(
            eye.getEyeView(),
            0,
            eyeView,
            0,
            16
        );
        Matrix.translateM(
            correctedEyeView,
            0,
            eyeView,
            0,
            eyeCorrectionMeters,
            0.0f,
            0.0f
        );
        Matrix.setIdentityM(model, 0);
        Matrix.multiplyMM(
            modelView,
            0,
            correctedEyeView,
            0,
            model,
            0
        );
        Matrix.multiplyMM(
            mvp,
            0,
            eye.getPerspective(Z_NEAR, Z_FAR),
            0,
            modelView,
            0
        );

        switch (safe) {
            case FOREST_GLADE:
                draw(
                    outdoorGround,
                    0.30f,
                    0.55f,
                    0.20f,
                    1.0f
                );
                draw(
                    forestTrunks,
                    0.30f,
                    0.20f,
                    0.10f,
                    1.0f
                );
                draw(
                    forestCanopies,
                    0.10f,
                    0.34f,
                    0.10f,
                    1.0f
                );
                break;

            case DUNE_BEACH:
                draw(
                    outdoorGround,
                    0.82f,
                    0.71f,
                    0.49f,
                    1.0f
                );
                draw(
                    ocean,
                    0.10f,
                    0.42f,
                    0.62f,
                    1.0f
                );
                draw(
                    duneRidges,
                    0.72f,
                    0.59f,
                    0.37f,
                    1.0f
                );
                draw(
                    beachGrass,
                    0.28f,
                    0.36f,
                    0.18f,
                    1.0f
                );
                break;

            case WHITE_ROOM:
            default:
                draw(
                    whiteFloor,
                    0.82f,
                    0.83f,
                    0.84f,
                    1.0f
                );
                draw(
                    whiteCeiling,
                    0.94f,
                    0.94f,
                    0.94f,
                    1.0f
                );
                draw(
                    whiteBack,
                    0.91f,
                    0.91f,
                    0.92f,
                    1.0f
                );
                draw(
                    whiteFront,
                    0.90f,
                    0.90f,
                    0.91f,
                    1.0f
                );
                draw(
                    whiteLeft,
                    0.88f,
                    0.89f,
                    0.90f,
                    1.0f
                );
                draw(
                    whiteRight,
                    0.88f,
                    0.89f,
                    0.90f,
                    1.0f
                );
                draw(
                    daisTop,
                    0.55f,
                    0.56f,
                    0.58f,
                    1.0f
                );
                draw(
                    daisFront,
                    0.43f,
                    0.44f,
                    0.46f,
                    1.0f
                );
                break;
        }
    }

    void shutdown() {
        if (program != 0) {
            GLES20.glDeleteProgram(program);
            program = 0;
        }
    }

    private void draw(
        Mesh mesh,
        float red,
        float green,
        float blue,
        float alpha
    ) {
        if (program == 0
            || mesh == null
            || mesh.vertexCount == 0) {
            return;
        }

        GLES20.glUseProgram(program);
        mesh.vertices.position(0);
        GLES20.glVertexAttribPointer(
            positionHandle,
            3,
            GLES20.GL_FLOAT,
            false,
            0,
            mesh.vertices
        );
        GLES20.glEnableVertexAttribArray(
            positionHandle
        );
        GLES20.glUniformMatrix4fv(
            mvpHandle,
            1,
            false,
            mvp,
            0
        );
        GLES20.glUniform4f(
            colorHandle,
            red,
            green,
            blue,
            alpha
        );
        GLES20.glDrawArrays(
            GLES20.GL_TRIANGLES,
            0,
            mesh.vertexCount
        );
        GLES20.glDisableVertexAttribArray(
            positionHandle
        );
    }

    private static float[] buildForestTrunks() {
        List<Float> values = new ArrayList<>();
        float[][] positions = new float[][] {
            {-7.0f, -6.0f},
            {-3.5f, -8.0f},
            {0.0f, -9.0f},
            {4.0f, -8.0f},
            {7.5f, -5.0f},
            {8.0f, 1.0f},
            {5.0f, 7.0f},
            {0.0f, 9.0f},
            {-5.0f, 7.0f},
            {-8.0f, 1.0f}
        };

        for (float[] position : positions) {
            appendFacingQuad(
                values,
                position[0],
                position[1],
                0.34f,
                -1.35f,
                1.35f
            );
        }
        return toArray(values);
    }

    private static float[] buildForestCanopies() {
        List<Float> values = new ArrayList<>();
        float[][] positions = new float[][] {
            {-7.0f, -6.0f},
            {-3.5f, -8.0f},
            {0.0f, -9.0f},
            {4.0f, -8.0f},
            {7.5f, -5.0f},
            {8.0f, 1.0f},
            {5.0f, 7.0f},
            {0.0f, 9.0f},
            {-5.0f, 7.0f},
            {-8.0f, 1.0f}
        };

        for (float[] position : positions) {
            appendFacingQuad(
                values,
                position[0],
                position[1],
                1.65f,
                0.55f,
                3.20f
            );
        }
        return toArray(values);
    }

    private static float[] buildDuneRidges() {
        List<Float> values = new ArrayList<>();
        appendFacingQuad(
            values,
            -6.0f,
            8.0f,
            4.0f,
            -1.35f,
            0.15f
        );
        appendFacingQuad(
            values,
            2.0f,
            10.0f,
            5.5f,
            -1.35f,
            0.40f
        );
        appendFacingQuad(
            values,
            8.0f,
            6.0f,
            3.5f,
            -1.35f,
            0.05f
        );
        return toArray(values);
    }

    private static float[] buildBeachGrass() {
        List<Float> values = new ArrayList<>();
        float[][] positions = new float[][] {
            {-3.8f, -1.2f},
            {-4.5f, 0.7f},
            {3.7f, -0.8f},
            {4.6f, 1.1f},
            {-2.8f, 3.2f},
            {2.9f, 3.5f}
        };
        for (float[] position : positions) {
            appendFacingQuad(
                values,
                position[0],
                position[1],
                0.10f,
                -1.35f,
                -0.45f
            );
        }
        return toArray(values);
    }

    private static void appendFacingQuad(
        List<Float> values,
        float x,
        float z,
        float halfWidth,
        float bottom,
        float top
    ) {
        float length =
            (float) Math.sqrt(
                (x * x) + (z * z)
            );
        float rightX =
            length <= 0.001f
                ? 1.0f
                : z / length;
        float rightZ =
            length <= 0.001f
                ? 0.0f
                : -x / length;

        float x0 = x - rightX * halfWidth;
        float z0 = z - rightZ * halfWidth;
        float x1 = x + rightX * halfWidth;
        float z1 = z + rightZ * halfWidth;

        addTriangle(
            values,
            x0,
            bottom,
            z0,
            x1,
            bottom,
            z1,
            x1,
            top,
            z1
        );
        addTriangle(
            values,
            x0,
            bottom,
            z0,
            x1,
            top,
            z1,
            x0,
            top,
            z0
        );
    }

    private static float[] quadHorizontal(
        float left,
        float right,
        float back,
        float front,
        float y
    ) {
        return new float[] {
            left, y, back,
            right, y, back,
            right, y, front,
            left, y, back,
            right, y, front,
            left, y, front
        };
    }

    private static float[] quadVerticalZ(
        float left,
        float right,
        float bottom,
        float top,
        float z
    ) {
        return new float[] {
            left, bottom, z,
            right, bottom, z,
            right, top, z,
            left, bottom, z,
            right, top, z,
            left, top, z
        };
    }

    private static float[] quadVerticalX(
        float x,
        float back,
        float bottom,
        float top,
        float front
    ) {
        return new float[] {
            x, bottom, back,
            x, bottom, front,
            x, top, front,
            x, bottom, back,
            x, top, front,
            x, top, back
        };
    }

    private static void addTriangle(
        List<Float> values,
        float ax,
        float ay,
        float az,
        float bx,
        float by,
        float bz,
        float cx,
        float cy,
        float cz
    ) {
        values.add(ax);
        values.add(ay);
        values.add(az);
        values.add(bx);
        values.add(by);
        values.add(bz);
        values.add(cx);
        values.add(cy);
        values.add(cz);
    }

    private static float[] toArray(
        List<Float> values
    ) {
        float[] result =
            new float[values.size()];
        for (int index = 0;
             index < values.size();
             index++) {
            result[index] = values.get(index);
        }
        return result;
    }

    private static Mesh mesh(float[] values) {
        FloatBuffer buffer =
            ByteBuffer
                .allocateDirect(values.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        buffer.put(values);
        buffer.position(0);
        return new Mesh(
            buffer,
            values.length / 3
        );
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

        int result = GLES20.glCreateProgram();
        GLES20.glAttachShader(result, vertex);
        GLES20.glAttachShader(result, fragment);
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
                GLES20.glGetProgramInfoLog(result);
            GLES20.glDeleteProgram(result);
            throw new IllegalStateException(
                "Home environment shader link failed: "
                    + message
            );
        }
        return result;
    }

    private static int compileShader(
        int type,
        String source
    ) {
        int shader = GLES20.glCreateShader(type);
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
                GLES20.glGetShaderInfoLog(shader);
            GLES20.glDeleteShader(shader);
            throw new IllegalStateException(
                "Home environment shader compile failed: "
                    + message
            );
        }
        return shader;
    }

    private static final class Mesh {
        final FloatBuffer vertices;
        final int vertexCount;

        Mesh(
            FloatBuffer vertices,
            int vertexCount
        ) {
            this.vertices = vertices;
            this.vertexCount = vertexCount;
        }
    }

    private static final String VERTEX_SHADER =
        "uniform mat4 u_Mvp;\n"
            + "attribute vec3 a_Position;\n"
            + "void main() {\n"
            + "  gl_Position = u_Mvp * vec4(a_Position, 1.0);\n"
            + "}\n";

    private static final String FRAGMENT_SHADER =
        "precision mediump float;\n"
            + "uniform vec4 u_Color;\n"
            + "void main() {\n"
            + "  gl_FragColor = u_Color;\n"
            + "}\n";
}
