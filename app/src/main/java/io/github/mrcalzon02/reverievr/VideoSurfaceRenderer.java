package io.github.mrcalzon02.reverievr;

import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.Matrix;

import com.google.cardboard.sdk.CardboardView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.concurrent.atomic.AtomicBoolean;

final class VideoSurfaceRenderer {
    interface SurfaceListener {
        void onVideoSurfaceTextureReady(SurfaceTexture surfaceTexture);
    }

    private static final float Z_NEAR = 0.10f;
    private static final float Z_FAR = 30.0f;
    private static final float SPHERE_RADIUS = 10.0f;
    private static final int SPHERE_LATITUDE_SEGMENTS = 24;
    private static final int SPHERE_LONGITUDE_SEGMENTS = 48;

    private final SurfaceListener surfaceListener;
    private final AtomicBoolean frameAvailable = new AtomicBoolean();

    private final FloatBuffer flatVertices;
    private final FloatBuffer flatUvs;
    private final FloatBuffer sphereVertices;
    private final FloatBuffer sphereUvs;
    private final int sphereVertexCount;

    private final float[] surfaceTransform = new float[16];
    private final float[] eyeView = new float[16];
    private final float[] correctedEyeView = new float[16];
    private final float[] model = new float[16];
    private final float[] modelView = new float[16];
    private final float[] mvp = new float[16];

    private volatile VideoProjection projection = VideoProjection.FLAT_CINEMA;
    private volatile float videoAspectRatio = 16.0f / 9.0f;

    private int program;
    private int texture;
    private int positionHandle;
    private int uvHandle;
    private int mvpHandle;
    private int surfaceTransformHandle;
    private int textureHandle;

    private SurfaceTexture surfaceTexture;

    VideoSurfaceRenderer(SurfaceListener surfaceListener) {
        this.surfaceListener = surfaceListener;

        flatVertices = allocate(new float[] {
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f
        });
        flatUvs = allocate(new float[] {
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f
        });

        SphereMesh sphere = buildSphere();
        sphereVertices = sphere.vertices;
        sphereUvs = sphere.uvs;
        sphereVertexCount = sphere.vertexCount;

        Matrix.setIdentityM(surfaceTransform, 0);
    }

    void setProjection(VideoProjection projection) {
        this.projection = projection == null
            ? VideoProjection.FLAT_CINEMA
            : projection;
    }

    void setVideoAspectRatio(float aspectRatio) {
        if (Float.isFinite(aspectRatio)
            && aspectRatio >= 0.25f
            && aspectRatio <= 5.0f) {
            videoAspectRatio = aspectRatio;
        }
    }

    void onSurfaceCreated() {
        abandonStaleContextObjects();

        program = buildProgram(VERTEX_SHADER, FRAGMENT_SHADER);
        positionHandle = GLES20.glGetAttribLocation(program, "a_Position");
        uvHandle = GLES20.glGetAttribLocation(program, "a_TexCoord");
        mvpHandle = GLES20.glGetUniformLocation(program, "u_Mvp");
        surfaceTransformHandle =
            GLES20.glGetUniformLocation(program, "u_SurfaceTransform");
        textureHandle = GLES20.glGetUniformLocation(program, "u_Texture");

        int[] textures = new int[1];
        GLES20.glGenTextures(1, textures, 0);
        texture = textures[0];

        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, texture);
        GLES20.glTexParameteri(
            GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
            GLES20.GL_TEXTURE_MIN_FILTER,
            GLES20.GL_LINEAR
        );
        GLES20.glTexParameteri(
            GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
            GLES20.GL_TEXTURE_MAG_FILTER,
            GLES20.GL_LINEAR
        );
        GLES20.glTexParameteri(
            GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
            GLES20.GL_TEXTURE_WRAP_S,
            GLES20.GL_CLAMP_TO_EDGE
        );
        GLES20.glTexParameteri(
            GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
            GLES20.GL_TEXTURE_WRAP_T,
            GLES20.GL_CLAMP_TO_EDGE
        );

        surfaceTexture = new SurfaceTexture(texture);
        surfaceTexture.setOnFrameAvailableListener(
            ignored -> frameAvailable.set(true)
        );

        surfaceListener.onVideoSurfaceTextureReady(surfaceTexture);
    }

    void updateFrame() {
        SurfaceTexture active = surfaceTexture;
        if (active == null || !frameAvailable.getAndSet(false)) {
            return;
        }

        try {
            active.updateTexImage();
            active.getTransformMatrix(surfaceTransform);
        } catch (RuntimeException ignored) {
            // Surface is being torn down or the decoder has not produced a frame yet.
        }
    }

    void drawEye(CardboardView.Eye eye, float eyeCorrectionMeters) {
        GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_BLEND);
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

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

        if (projection == VideoProjection.MONO_EQUIRECTANGULAR_360) {
            drawSphere(eye);
        } else {
            drawFlat(eye);
        }
    }

    void shutdown() {
        releaseSurfaceTexture();

        if (texture != 0) {
            GLES20.glDeleteTextures(1, new int[] {texture}, 0);
            texture = 0;
        }
        if (program != 0) {
            GLES20.glDeleteProgram(program);
            program = 0;
        }
        frameAvailable.set(false);
    }

    private void abandonStaleContextObjects() {
        releaseSurfaceTexture();

        // onSurfaceCreated means a new EGL context. Numeric GL object names
        // from the previous context are no longer ours to delete here.
        texture = 0;
        program = 0;
        positionHandle = -1;
        uvHandle = -1;
        mvpHandle = -1;
        surfaceTransformHandle = -1;
        textureHandle = -1;
        frameAvailable.set(false);
        Matrix.setIdentityM(surfaceTransform, 0);
    }

    private void releaseSurfaceTexture() {
        SurfaceTexture activeSurface = surfaceTexture;
        surfaceTexture = null;
        if (activeSurface != null) {
            activeSurface.setOnFrameAvailableListener(null);
            activeSurface.release();
        }
        frameAvailable.set(false);
    }

    private void drawFlat(CardboardView.Eye eye) {
        float halfHeight = 0.95f;
        float halfWidth = Math.min(1.75f, halfHeight * videoAspectRatio);

        Matrix.setIdentityM(model, 0);
        Matrix.translateM(model, 0, 0.0f, 0.0f, -3.2f);
        Matrix.scaleM(model, 0, halfWidth, halfHeight, 1.0f);

        Matrix.multiplyMM(modelView, 0, correctedEyeView, 0, model, 0);
        Matrix.multiplyMM(
            mvp,
            0,
            eye.getPerspective(Z_NEAR, Z_FAR),
            0,
            modelView,
            0
        );

        drawGeometry(flatVertices, flatUvs, 4, GLES20.GL_TRIANGLE_STRIP);
    }

    private void drawSphere(CardboardView.Eye eye) {
        Matrix.setIdentityM(model, 0);
        Matrix.multiplyMM(modelView, 0, correctedEyeView, 0, model, 0);
        Matrix.multiplyMM(
            mvp,
            0,
            eye.getPerspective(Z_NEAR, Z_FAR),
            0,
            modelView,
            0
        );

        drawGeometry(
            sphereVertices,
            sphereUvs,
            sphereVertexCount,
            GLES20.GL_TRIANGLES
        );
    }

    private void drawGeometry(
        FloatBuffer vertices,
        FloatBuffer uvs,
        int vertexCount,
        int primitive
    ) {
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

        GLES20.glUniformMatrix4fv(mvpHandle, 1, false, mvp, 0);
        GLES20.glUniformMatrix4fv(
            surfaceTransformHandle,
            1,
            false,
            surfaceTransform,
            0
        );

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, texture);
        GLES20.glUniform1i(textureHandle, 0);

        GLES20.glDrawArrays(primitive, 0, vertexCount);

        GLES20.glDisableVertexAttribArray(positionHandle);
        GLES20.glDisableVertexAttribArray(uvHandle);
    }

    private static SphereMesh buildSphere() {
        int vertexCount =
            SPHERE_LATITUDE_SEGMENTS * SPHERE_LONGITUDE_SEGMENTS * 6;
        float[] vertices = new float[vertexCount * 3];
        float[] uvs = new float[vertexCount * 2];

        int vertexOffset = 0;
        int uvOffset = 0;

        for (int latitude = 0;
             latitude < SPHERE_LATITUDE_SEGMENTS;
             latitude++) {
            float v0 = latitude / (float) SPHERE_LATITUDE_SEGMENTS;
            float v1 = (latitude + 1) / (float) SPHERE_LATITUDE_SEGMENTS;

            for (int longitude = 0;
                 longitude < SPHERE_LONGITUDE_SEGMENTS;
                 longitude++) {
                float u0 = longitude / (float) SPHERE_LONGITUDE_SEGMENTS;
                float u1 = (longitude + 1) / (float) SPHERE_LONGITUDE_SEGMENTS;

                vertexOffset = putSphereVertex(
                    vertices, vertexOffset, u0, v0
                );
                uvOffset = putUv(uvs, uvOffset, u0, v0);

                vertexOffset = putSphereVertex(
                    vertices, vertexOffset, u0, v1
                );
                uvOffset = putUv(uvs, uvOffset, u0, v1);

                vertexOffset = putSphereVertex(
                    vertices, vertexOffset, u1, v1
                );
                uvOffset = putUv(uvs, uvOffset, u1, v1);

                vertexOffset = putSphereVertex(
                    vertices, vertexOffset, u0, v0
                );
                uvOffset = putUv(uvs, uvOffset, u0, v0);

                vertexOffset = putSphereVertex(
                    vertices, vertexOffset, u1, v1
                );
                uvOffset = putUv(uvs, uvOffset, u1, v1);

                vertexOffset = putSphereVertex(
                    vertices, vertexOffset, u1, v0
                );
                uvOffset = putUv(uvs, uvOffset, u1, v0);
            }
        }

        return new SphereMesh(
            allocate(vertices),
            allocate(uvs),
            vertexCount
        );
    }

    private static int putSphereVertex(
        float[] target,
        int offset,
        float u,
        float v
    ) {
        double theta = v * Math.PI;
        double phi = (u - 0.5) * Math.PI * 2.0;

        float sinTheta = (float) Math.sin(theta);
        target[offset++] =
            SPHERE_RADIUS * sinTheta * (float) Math.sin(phi);
        target[offset++] =
            SPHERE_RADIUS * (float) Math.cos(theta);
        target[offset++] =
            -SPHERE_RADIUS * sinTheta * (float) Math.cos(phi);
        return offset;
    }

    private static int putUv(float[] target, int offset, float u, float v) {
        target[offset++] = u;
        target[offset++] = v;
        return offset;
    }

    private static FloatBuffer allocate(float[] values) {
        FloatBuffer buffer = ByteBuffer
            .allocateDirect(values.length * 4)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();
        buffer.put(values);
        buffer.position(0);
        return buffer;
    }

    private static int buildProgram(String vertexSource, String fragmentSource) {
        int vertex = compileShader(GLES20.GL_VERTEX_SHADER, vertexSource);
        int fragment = compileShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);

        int result = GLES20.glCreateProgram();
        GLES20.glAttachShader(result, vertex);
        GLES20.glAttachShader(result, fragment);
        GLES20.glLinkProgram(result);

        int[] linked = new int[1];
        GLES20.glGetProgramiv(result, GLES20.GL_LINK_STATUS, linked, 0);
        if (linked[0] == 0) {
            String message = GLES20.glGetProgramInfoLog(result);
            GLES20.glDeleteProgram(result);
            GLES20.glDeleteShader(vertex);
            GLES20.glDeleteShader(fragment);
            throw new IllegalStateException(
                "Video shader link failed: " + message
            );
        }

        GLES20.glDeleteShader(vertex);
        GLES20.glDeleteShader(fragment);
        return result;
    }

    private static int compileShader(int type, String source) {
        int shader = GLES20.glCreateShader(type);
        GLES20.glShaderSource(shader, source);
        GLES20.glCompileShader(shader);

        int[] compiled = new int[1];
        GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
        if (compiled[0] == 0) {
            String message = GLES20.glGetShaderInfoLog(shader);
            GLES20.glDeleteShader(shader);
            throw new IllegalStateException(
                "Video shader compile failed: " + message
            );
        }
        return shader;
    }

    private static final class SphereMesh {
        final FloatBuffer vertices;
        final FloatBuffer uvs;
        final int vertexCount;

        SphereMesh(
            FloatBuffer vertices,
            FloatBuffer uvs,
            int vertexCount
        ) {
            this.vertices = vertices;
            this.uvs = uvs;
            this.vertexCount = vertexCount;
        }
    }

    private static final String VERTEX_SHADER =
        "uniform mat4 u_Mvp;\n"
            + "uniform mat4 u_SurfaceTransform;\n"
            + "attribute vec4 a_Position;\n"
            + "attribute vec2 a_TexCoord;\n"
            + "varying vec2 v_TexCoord;\n"
            + "void main() {\n"
            + "  gl_Position = u_Mvp * a_Position;\n"
            + "  v_TexCoord = "
            + "(u_SurfaceTransform * vec4(a_TexCoord, 0.0, 1.0)).xy;\n"
            + "}\n";

    private static final String FRAGMENT_SHADER =
        "#extension GL_OES_EGL_image_external : require\n"
            + "precision mediump float;\n"
            + "uniform samplerExternalOES u_Texture;\n"
            + "varying vec2 v_TexCoord;\n"
            + "void main() {\n"
            + "  gl_FragColor = texture2D(u_Texture, v_TexCoord);\n"
            + "}\n";
}
