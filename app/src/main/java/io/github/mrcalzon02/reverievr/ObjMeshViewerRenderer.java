package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.Matrix;
import com.google.cardboard.sdk.CardboardView;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/** Immersive two-eye, actual triangle geometry, no video/flat-panel fallback. */
final class ObjMeshViewerRenderer {
    static final String LOCAL_MODEL_FILE = "model-viewer.obj";
    private final File file;
    private ObjMeshReader.Mesh mesh;
    private FloatBuffer vertices;
    private int program, positionHandle, shadeHandle, mvpHandle;
    private float spinDegrees;
    private final float[] eyeView = new float[16];
    private final float[] model = new float[16];
    private final float[] combined = new float[16];
    private final float[] mvp = new float[16];

    ObjMeshViewerRenderer(Context context) {
        file = new File(context.getFilesDir(), LOCAL_MODEL_FILE);
    }

    boolean load() {
        try (FileReader reader = new FileReader(file)) {
            ObjMeshReader.Mesh loaded = ObjMeshReader.read(reader);
            // Face normals are generated once, never recomputed per eye.
            // Interleaved xyz + brightness gives useful shape cues without
            // texture dependencies or another GPU buffer.
            FloatBuffer prepared = ByteBuffer.allocateDirect(
                loaded.triangles.length / 3 * 16)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
            for (int at = 0; at < loaded.triangles.length; at += 9) {
                float[] v = loaded.triangles;
                float ax = v[at + 3] - v[at];
                float ay = v[at + 4] - v[at + 1];
                float az = v[at + 5] - v[at + 2];
                float bx = v[at + 6] - v[at];
                float by = v[at + 7] - v[at + 1];
                float bz = v[at + 8] - v[at + 2];
                float nx = ay * bz - az * by;
                float ny = az * bx - ax * bz;
                float nz = ax * by - ay * bx;
                float length = (float) Math.sqrt(nx * nx + ny * ny + nz * nz);
                float shade = length <= 0.000001f ? 0.5f
                    : 0.32f + 0.68f * Math.abs(
                        (nx * 0.38f + ny * 0.72f + nz * 0.58f)
                            / (length * 0.9946f));
                for (int vertex = 0; vertex < 3; vertex++) {
                    prepared.put(v, at + vertex * 3, 3);
                    prepared.put(Math.min(1.0f, shade));
                }
            }
            prepared.position(0);
            mesh = loaded;
            vertices = prepared;
            spinDegrees = 0.0f;
            return true;
        } catch (IOException | OutOfMemoryError failure) {
            ReverieLog.error("VR_MODEL_VIEWER", "Model load failed.", failure);
            mesh = null;
            vertices = null;
            return false;
        }
    }

    boolean hasModel() { return file.isFile() && file.length() > 0; }
    void rotate(float deltaDegrees) {
        spinDegrees = (spinDegrees + deltaDegrees) % 360.0f;
    }

    void onSurfaceCreated() {
        shutdown();
        final String vertex = "uniform mat4 u_Mvp;"
            + "attribute vec3 a_Pos; attribute float a_Shade;"
            + "varying float v_Shade;"
            + "void main(){v_Shade=a_Shade;"
            + "gl_Position=u_Mvp*vec4(a_Pos,1.0);}";
        final String fragment = "precision mediump float;"
            + "varying float v_Shade;"
            + "void main(){gl_FragColor=vec4("
            + "0.78*v_Shade,0.86*v_Shade,0.93*v_Shade,1.0);}";
        int vs = shader(GLES20.GL_VERTEX_SHADER, vertex);
        int fs = shader(GLES20.GL_FRAGMENT_SHADER, fragment);
        if (vs == 0 || fs == 0) {
            if (vs != 0) GLES20.glDeleteShader(vs);
            if (fs != 0) GLES20.glDeleteShader(fs);
            throw new IllegalStateException("Model shader compilation failed");
        }
        program = GLES20.glCreateProgram();
        GLES20.glAttachShader(program, vs);
        GLES20.glAttachShader(program, fs);
        GLES20.glBindAttribLocation(program, 0, "a_Pos");
        GLES20.glBindAttribLocation(program, 1, "a_Shade");
        GLES20.glLinkProgram(program);
        GLES20.glDeleteShader(vs);
        GLES20.glDeleteShader(fs);
        int[] linked = new int[1];
        GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linked, 0);
        if (linked[0] == 0) {
            shutdown();
            throw new IllegalStateException("Model viewer shader link failed");
        }
        positionHandle = GLES20.glGetAttribLocation(program, "a_Pos");
        shadeHandle = GLES20.glGetAttribLocation(program, "a_Shade");
        mvpHandle = GLES20.glGetUniformLocation(program, "u_Mvp");
    }

    void drawEye(CardboardView.Eye eye, float eyeCorrection) {
        if (program == 0 || mesh == null || vertices == null) return;
        float centerX = (mesh.bounds[0] + mesh.bounds[3]) * 0.5f;
        float centerY = (mesh.bounds[1] + mesh.bounds[4]) * 0.5f;
        float centerZ = (mesh.bounds[2] + mesh.bounds[5]) * 0.5f;
        float extent = Math.max(mesh.bounds[3] - mesh.bounds[0],
            Math.max(mesh.bounds[4] - mesh.bounds[1],
                mesh.bounds[5] - mesh.bounds[2]));
        float scale = extent > 0.000001f ? 1.8f / extent : 1.0f;
        System.arraycopy(eye.getEyeView(), 0, eyeView, 0, 16);
        Matrix.translateM(eyeView, 0, eyeCorrection, 0.0f, 0.0f);
        Matrix.setIdentityM(model, 0);
        Matrix.translateM(model, 0, 0.0f, 0.0f, -3.3f);
        Matrix.rotateM(model, 0, spinDegrees, 0.0f, 1.0f, 0.0f);
        Matrix.scaleM(model, 0, scale, scale, scale);
        Matrix.translateM(model, 0, -centerX, -centerY, -centerZ);
        Matrix.multiplyMM(combined, 0, eyeView, 0, model, 0);
        Matrix.multiplyMM(mvp, 0, eye.getPerspective(0.1f, 40.0f),
            0, combined, 0);
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_CULL_FACE);
        GLES20.glUseProgram(program);
        GLES20.glUniformMatrix4fv(mvpHandle, 1, false, mvp, 0);
        vertices.position(0);
        GLES20.glVertexAttribPointer(positionHandle, 3,
            GLES20.GL_FLOAT, false, 16, vertices);
        GLES20.glEnableVertexAttribArray(positionHandle);
        vertices.position(3);
        GLES20.glVertexAttribPointer(shadeHandle, 1,
            GLES20.GL_FLOAT, false, 16, vertices);
        GLES20.glEnableVertexAttribArray(shadeHandle);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0,
            mesh.triangles.length / 3);
        GLES20.glDisableVertexAttribArray(shadeHandle);
        GLES20.glDisableVertexAttribArray(positionHandle);
    }

    void shutdown() {
        if (program != 0) GLES20.glDeleteProgram(program);
        program = 0;
    }

    private static int shader(int type, String source) {
        int result = GLES20.glCreateShader(type);
        GLES20.glShaderSource(result, source);
        GLES20.glCompileShader(result);
        int[] status = new int[1];
        GLES20.glGetShaderiv(result, GLES20.GL_COMPILE_STATUS, status, 0);
        if (status[0] == 0) {
            GLES20.glDeleteShader(result);
            return 0;
        }
        return result;
    }
}
