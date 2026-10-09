package io.github.mrcalzon02.reverievr;

import android.opengl.GLES20;
import android.opengl.Matrix;

import com.google.cardboard.sdk.CardboardView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

final class VrPointerRenderer {
    private static final float Z_NEAR = 0.10f;
    private static final float Z_FAR = 30.0f;

    private final FloatBuffer vertices =
        ByteBuffer
            .allocateDirect(18 * 4)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();

    private final float[] eyeView = new float[16];
    private final float[] correctedEyeView = new float[16];
    private final float[] mvp = new float[16];
    // Reused GL state query buffer; no per-eye allocation.
    private final float[] previousLineWidth = new float[1];

    private volatile boolean visible;
    private volatile float originX;
    private volatile float originY;
    private volatile float originZ;
    private volatile float endX;
    private volatile float endY;
    private volatile float endZ;
    private volatile boolean hitting;
    private volatile boolean pressed;
    private final float[] feedbackColor = new float[4];

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

    void setPointer(
        boolean visible,
        float originX,
        float originY,
        float originZ,
        float directionX,
        float directionY,
        float directionZ,
        float distance,
        boolean hitting,
        boolean pressed
    ) {
        this.visible = visible;
        this.originX = originX;
        this.originY = originY;
        this.originZ = originZ;

        float safeDistance =
            Math.max(
                0.25f,
                Math.min(distance, 12.0f)
            );

        this.endX =
            originX
                + directionX * safeDistance;
        this.endY =
            originY
                + directionY * safeDistance;
        this.endZ =
            originZ
                + directionZ * safeDistance;
        this.hitting = hitting;
        this.pressed = pressed;
    }

    void hide() {
        visible = false;
    }

    void drawEye(
        CardboardView.Eye eye,
        float eyeCorrectionMeters
    ) {
        if (!visible || program == 0) {
            return;
        }

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
        Matrix.multiplyMM(
            mvp,
            0,
            eye.getPerspective(
                Z_NEAR,
                Z_FAR
            ),
            0,
            correctedEyeView,
            0
        );

        int visualState = PointerVisualFeedback.resolve(hitting, pressed);
        float markerSize = PointerVisualFeedback.markerSize(visualState);
        PointerVisualFeedback.writeColor(visualState, feedbackColor);

        vertices.position(0);
        vertices.put(originX);
        vertices.put(originY);
        vertices.put(originZ);
        vertices.put(endX);
        vertices.put(endY);
        vertices.put(endZ);

        vertices.put(endX - markerSize);
        vertices.put(endY);
        vertices.put(endZ);
        vertices.put(endX + markerSize);
        vertices.put(endY);
        vertices.put(endZ);

        vertices.put(endX);
        vertices.put(endY - markerSize);
        vertices.put(endZ);
        vertices.put(endX);
        vertices.put(endY + markerSize);
        vertices.put(endZ);
        vertices.position(0);

        boolean depthWasEnabled = GLES20.glIsEnabled(GLES20.GL_DEPTH_TEST);
        GLES20.glGetFloatv(GLES20.GL_LINE_WIDTH, previousLineWidth, 0);
        GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        try {
        GLES20.glUseProgram(program);
        GLES20.glVertexAttribPointer(
            positionHandle,
            3,
            GLES20.GL_FLOAT,
            false,
            0,
            vertices
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
            feedbackColor[0],
            feedbackColor[1],
            feedbackColor[2],
            feedbackColor[3]
        );

        GLES20.glLineWidth(3.0f);
        GLES20.glDrawArrays(
            GLES20.GL_LINES,
            0,
            2
        );

        vertices.position(6);
        GLES20.glVertexAttribPointer(
            positionHandle,
            3,
            GLES20.GL_FLOAT,
            false,
            0,
            vertices
        );
        GLES20.glDrawArrays(
            GLES20.GL_LINES,
            0,
            4
        );

        } finally {
            GLES20.glDisableVertexAttribArray(positionHandle);
            GLES20.glLineWidth(previousLineWidth[0]);
            if (depthWasEnabled) {
                GLES20.glEnable(GLES20.GL_DEPTH_TEST);
            } else {
                GLES20.glDisable(GLES20.GL_DEPTH_TEST);
            }
        }
    }

    void shutdown() {
        if (program != 0) {
            GLES20.glDeleteProgram(program);
            program = 0;
        }
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
                GLES20.glGetProgramInfoLog(result);
            GLES20.glDeleteProgram(result);
            throw new IllegalStateException(
                "Pointer shader link failed: "
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
                GLES20.glGetShaderInfoLog(shader);
            GLES20.glDeleteShader(shader);
            throw new IllegalStateException(
                "Pointer shader compile failed: "
                    + message
            );
        }
        return shader;
    }

    private static final String VERTEX_SHADER =
        "uniform mat4 u_Mvp;\n"
            + "attribute vec3 a_Position;\n"
            + "void main() {\n"
            + "  gl_Position = "
            + "u_Mvp * vec4(a_Position, 1.0);\n"
            + "}\n";

    private static final String FRAGMENT_SHADER =
        "precision mediump float;\n"
            + "uniform vec4 u_Color;\n"
            + "void main() {\n"
            + "  gl_FragColor = u_Color;\n"
            + "}\n";
}
