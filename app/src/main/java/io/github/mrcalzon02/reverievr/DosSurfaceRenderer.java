package io.github.mrcalzon02.reverievr;

import android.opengl.GLES20;
import android.opengl.Matrix;

import com.google.cardboard.sdk.CardboardView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

final class DosSurfaceRenderer {
    private static final float Z_NEAR = 0.10f;
    private static final float Z_FAR = 30.0f;

    private final DosSession session;
    private final FloatBuffer vertices;
    private final FloatBuffer uvs;

    private final float[] eyeView = new float[16];
    private final float[] correctedEyeView = new float[16];
    private final float[] model = new float[16];
    private final float[] modelView = new float[16];
    private final float[] mvp = new float[16];

    private int program;
    private int texture;
    private int positionHandle;
    private int uvHandle;
    private int mvpHandle;
    private int textureHandle;

    private long uploadedSerial;
    private int textureWidth;
    private int textureHeight;
    private float displayAspect = 4.0f / 3.0f;

    DosSurfaceRenderer(DosSession session) {
        this.session = session;
        vertices = allocate(new float[] {
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f
        });
        uvs = allocate(new float[] {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f
        });
    }

    void onSurfaceCreated() {
        abandonStaleContextObjects();

        program = buildProgram(VERTEX_SHADER, FRAGMENT_SHADER);
        positionHandle =
            GLES20.glGetAttribLocation(program, "a_Position");
        uvHandle =
            GLES20.glGetAttribLocation(program, "a_TexCoord");
        mvpHandle =
            GLES20.glGetUniformLocation(program, "u_Mvp");
        textureHandle =
            GLES20.glGetUniformLocation(program, "u_Texture");

        int[] textures = new int[1];
        GLES20.glGenTextures(1, textures, 0);
        texture = textures[0];

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MIN_FILTER,
            GLES20.GL_NEAREST
        );
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MAG_FILTER,
            GLES20.GL_NEAREST
        );
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_WRAP_S,
            GLES20.GL_CLAMP_TO_EDGE
        );
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_WRAP_T,
            GLES20.GL_CLAMP_TO_EDGE
        );
    }

    void updateFrame() {
        if (session == null || !session.isRunning()) {
            return;
        }

        long serialBefore = session.getFrameSerial();
        if (serialBefore <= 0L || serialBefore == uploadedSerial) {
            return;
        }

        int width = session.getFrameWidth();
        int height = session.getFrameHeight();
        if (width <= 0 || height <= 0) {
            return;
        }

        ByteBuffer frame = session.copyLatestFrame();
        if (frame == null) {
            return;
        }

        long serialAfter = session.getFrameSerial();
        if (serialBefore != serialAfter) {
            return;
        }

        long required = (long) width * (long) height * 4L;
        if (required <= 0L
            || required > Integer.MAX_VALUE
            || frame.remaining() < (int) required) {
            return;
        }

        RetroDisplayMode mode =
            RetroDisplayModeCatalog.resolve(width, height);
        displayAspect = mode.displayAspect();

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glPixelStorei(GLES20.GL_UNPACK_ALIGNMENT, 4);

        frame.position(0);
        if (width != textureWidth || height != textureHeight) {
            GLES20.glTexImage2D(
                GLES20.GL_TEXTURE_2D,
                0,
                GLES20.GL_RGBA,
                width,
                height,
                0,
                GLES20.GL_RGBA,
                GLES20.GL_UNSIGNED_BYTE,
                frame
            );
            textureWidth = width;
            textureHeight = height;

            ReverieLog.milestone(
                "DOS_VIDEO",
                "Guest framebuffer "
                    + width
                    + "x"
                    + height
                    + " mapped as "
                    + mode.label
                    + " aspect="
                    + displayAspect
            );
        } else {
            GLES20.glTexSubImage2D(
                GLES20.GL_TEXTURE_2D,
                0,
                0,
                0,
                width,
                height,
                GLES20.GL_RGBA,
                GLES20.GL_UNSIGNED_BYTE,
                frame
            );
        }

        uploadedSerial = serialAfter;
    }

    void drawEye(
        CardboardView.Eye eye,
        float eyeCorrectionMeters
    ) {
        GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_BLEND);

        if (program == 0 || texture == 0 || uploadedSerial <= 0L) {
            return;
        }

        System.arraycopy(eye.getEyeView(), 0, eyeView, 0, 16);
        Matrix.translateM(
            correctedEyeView,
            0,
            eyeView,
            0,
            eyeCorrectionMeters,
            0.0f,
            0.0f
        );

        float halfHeight = 0.95f;
        float halfWidth =
            Math.min(1.75f, halfHeight * displayAspect);

        Matrix.setIdentityM(model, 0);
        Matrix.translateM(model, 0, 0.0f, 0.0f, -3.2f);
        Matrix.scaleM(model, 0, halfWidth, halfHeight, 1.0f);

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

        GLES20.glUseProgram(program);

        vertices.position(0);
        GLES20.glVertexAttribPointer(
            positionHandle,
            3,
            GLES20.GL_FLOAT,
            false,
            0,
            vertices
        );
        GLES20.glEnableVertexAttribArray(positionHandle);

        uvs.position(0);
        GLES20.glVertexAttribPointer(
            uvHandle,
            2,
            GLES20.GL_FLOAT,
            false,
            0,
            uvs
        );
        GLES20.glEnableVertexAttribArray(uvHandle);

        GLES20.glUniformMatrix4fv(
            mvpHandle,
            1,
            false,
            mvp,
            0
        );

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glUniform1i(textureHandle, 0);

        GLES20.glDrawArrays(
            GLES20.GL_TRIANGLE_STRIP,
            0,
            4
        );

        GLES20.glDisableVertexAttribArray(positionHandle);
        GLES20.glDisableVertexAttribArray(uvHandle);
    }

    void shutdown() {
        if (texture != 0) {
            GLES20.glDeleteTextures(
                1,
                new int[] {texture},
                0
            );
            texture = 0;
        }
        if (program != 0) {
            GLES20.glDeleteProgram(program);
            program = 0;
        }

        uploadedSerial = 0L;
        textureWidth = 0;
        textureHeight = 0;
    }

    private void abandonStaleContextObjects() {
        program = 0;
        texture = 0;
        positionHandle = -1;
        uvHandle = -1;
        mvpHandle = -1;
        textureHandle = -1;
        uploadedSerial = 0L;
        textureWidth = 0;
        textureHeight = 0;
        displayAspect = 4.0f / 3.0f;
    }

    private static FloatBuffer allocate(float[] values) {
        ByteBuffer bytes =
            ByteBuffer.allocateDirect(values.length * 4)
                .order(ByteOrder.nativeOrder());
        FloatBuffer buffer = bytes.asFloatBuffer();
        buffer.put(values);
        buffer.position(0);
        return buffer;
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
                "Could not link DOS shader: " + message
            );
        }

        return result;
    }

    private static int compileShader(
        int type,
        String source
    ) {
        int shader = GLES20.glCreateShader(type);
        GLES20.glShaderSource(shader, source);
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
                "Could not compile DOS shader: " + message
            );
        }

        return shader;
    }

    private static final String VERTEX_SHADER =
        "uniform mat4 u_Mvp;\n"
            + "attribute vec3 a_Position;\n"
            + "attribute vec2 a_TexCoord;\n"
            + "varying vec2 v_TexCoord;\n"
            + "void main() {\n"
            + "  gl_Position = u_Mvp * vec4(a_Position, 1.0);\n"
            + "  v_TexCoord = a_TexCoord;\n"
            + "}\n";

    private static final String FRAGMENT_SHADER =
        "precision mediump float;\n"
            + "uniform sampler2D u_Texture;\n"
            + "varying vec2 v_TexCoord;\n"
            + "void main() {\n"
            + "  vec4 raw = texture2D(u_Texture, v_TexCoord);\n"
            + "  gl_FragColor = vec4(raw.b, raw.g, raw.r, 1.0);\n"
            + "}\n";
}
