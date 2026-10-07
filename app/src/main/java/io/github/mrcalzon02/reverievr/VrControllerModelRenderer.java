package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.Matrix;

import com.google.cardboard.sdk.CardboardView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

final class VrControllerModelRenderer {
    private static final float Z_NEAR = 0.10f;
    private static final float Z_FAR = 30.0f;

    private final FloatBuffer cube =
        buffer(
            new float[] {
                -0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,
                -0.5f,  0.5f, -0.5f,

                -0.5f, -0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,
                 0.5f, -0.5f,  0.5f,
                -0.5f, -0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,

                -0.5f, -0.5f, -0.5f,
                -0.5f,  0.5f, -0.5f,
                -0.5f,  0.5f,  0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f,  0.5f,  0.5f,
                -0.5f, -0.5f,  0.5f,

                 0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,
                 0.5f, -0.5f, -0.5f,
                 0.5f,  0.5f,  0.5f,
                 0.5f,  0.5f, -0.5f,

                -0.5f,  0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,
                 0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f, -0.5f,
                 0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f,

                -0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f,  0.5f,
                 0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f,  0.5f,
                 0.5f, -0.5f,  0.5f
            }
        );

    private final FloatBuffer disc =
        buildDisc(20);
    private final DaydreamControllerAssetRenderer
        assetRenderer;
    private final float[] springVertices =
        new float[ControllerGhostTether.VERTEX_COUNT * 3];
    private final FloatBuffer springMesh =
        buffer(new float[ControllerGhostTether.VERTEX_COUNT * 3]);
    private final boolean[] previousDepthWrite = new boolean[1];
    private final int[] previousBlendSourceRgb = new int[1];
    private final int[] previousBlendDestinationRgb = new int[1];
    private final int[] previousBlendSourceAlpha = new int[1];
    private final int[] previousBlendDestinationAlpha = new int[1];
    private final int[] previousBlendEquationRgb = new int[1];
    private final int[] previousBlendEquationAlpha = new int[1];
    private int springVertexCount;
    private boolean ghostVisible;
    private float ghostX;
    private float ghostY;
    private float ghostZ;
    private boolean ghostPass;
    private float ghostOpacity;

    private final float[] eyeView = new float[16];
    private final float[] correctedEyeView = new float[16];
    private final float[] root = new float[16];
    private final float[] rotation = new float[16];
    private final float[] yawCalibration = new float[16];
    private final float[] calibratedRotation = new float[16];
    private final float[] local = new float[16];
    private final float[] model = new float[16];
    private final float[] modelView = new float[16];
    private final float[] mvp = new float[16];

    private volatile boolean visible;
    private volatile boolean tracked;
    private volatile float anchorX = 0.28f;
    private volatile float anchorY = -0.34f;
    private volatile float anchorZ = -0.48f;
    private volatile float qx;
    private volatile float qy;
    private volatile float qz;
    private volatile float qw = 1.0f;
    private volatile float virtualYaw;
    private volatile float virtualPitch;
    private volatile float yawCalibrationRadians;

    private volatile boolean touchpadPressed;
    private volatile boolean homePressed;
    private volatile boolean appPressed;
    private volatile boolean volumeUpPressed;
    private volatile boolean volumeDownPressed;

    private int program;
    private int positionHandle;
    private int mvpHandle;
    private int colorHandle;

    VrControllerModelRenderer(
        Context context
    ) {
        assetRenderer =
            new DaydreamControllerAssetRenderer(
                context
            );
    }

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
        assetRenderer.onSurfaceCreated();
    }

    void setAnchor(
        float x,
        float y,
        float z
    ) {
        anchorX = x;
        anchorY = y;
        anchorZ = z;
    }

    void setYawCalibration(
        float radians
    ) {
        yawCalibrationRadians = radians;
    }

    void setTrackedPose(
        float x,
        float y,
        float z,
        float w
    ) {
        visible = true;
        tracked = true;
        qx = x;
        qy = y;
        qz = z;
        qw = w;
    }

    void setVirtualAim(
        float yaw,
        float pitch
    ) {
        visible = true;
        tracked = false;
        virtualYaw = yaw;
        virtualPitch = pitch;
    }

    void setButtonState(
        boolean touchpadPressed,
        boolean homePressed,
        boolean appPressed,
        boolean volumeUpPressed,
        boolean volumeDownPressed
    ) {
        this.touchpadPressed = touchpadPressed;
        this.homePressed = homePressed;
        this.appPressed = appPressed;
        this.volumeUpPressed = volumeUpPressed;
        this.volumeDownPressed = volumeDownPressed;
    }

    void hide() {
        visible = false;
        clearGhost();
    }

    void setGhostTarget(float x, float y, float z) {
        if (!Float.isFinite(x) || !Float.isFinite(y)
            || !Float.isFinite(z)) {
            clearGhost();
            return;
        }
        ghostX = x;
        ghostY = y;
        ghostZ = z;
        ghostVisible = true;
        springVertexCount = ControllerGhostTether.write(
            springVertices,
            anchorX, anchorY, anchorZ,
            ghostX, ghostY, ghostZ
        );
        if (springVertexCount > 0) {
            springMesh.position(0);
            springMesh.put(springVertices, 0, springVertexCount * 3);
            springMesh.position(0);
        }
    }

    void clearGhost() {
        ghostVisible = false;
        springVertexCount = 0;
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

        Matrix.setIdentityM(root, 0);
        Matrix.translateM(
            root,
            0,
            anchorX,
            anchorY,
            anchorZ
        );

        if (tracked) {
            quaternionMatrix(
                qx,
                qy,
                qz,
                qw,
                rotation
            );
            Matrix.setRotateM(
                yawCalibration,
                0,
                (float) Math.toDegrees(
                    -yawCalibrationRadians
                ),
                0.0f,
                1.0f,
                0.0f
            );
            Matrix.multiplyMM(
                calibratedRotation,
                0,
                yawCalibration,
                0,
                rotation,
                0
            );
            System.arraycopy(
                calibratedRotation,
                0,
                rotation,
                0,
                16
            );
        } else {
            Matrix.setIdentityM(rotation, 0);
            Matrix.rotateM(
                rotation,
                0,
                (float) Math.toDegrees(virtualYaw),
                0.0f,
                1.0f,
                0.0f
            );
            Matrix.rotateM(
                rotation,
                0,
                (float) Math.toDegrees(virtualPitch),
                1.0f,
                0.0f,
                0.0f
            );
        }

        Matrix.multiplyMM(
            model,
            0,
            root,
            0,
            rotation,
            0
        );
        System.arraycopy(model, 0, root, 0, 16);

        if (ghostVisible) {
            drawGhostEye(eye);
            // Ghost and live handset share rotation but never position.
            Matrix.setIdentityM(root, 0);
            Matrix.translateM(root, 0, anchorX, anchorY, anchorZ);
            Matrix.multiplyMM(model, 0, root, 0, rotation, 0);
            System.arraycopy(model, 0, root, 0, 16);
        }

        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_BLEND);

        if (assetRenderer.isRenderable()) {
            assetRenderer.drawEye(
                eye,
                correctedEyeView,
                root,
                touchpadPressed,
                homePressed,
                appPressed,
                volumeUpPressed,
                volumeDownPressed,
                1.0f
            );
            return;
        }

        drawCube(
            eye,
            0.0f,
            0.0f,
            0.0f,
            0.105f,
            0.040f,
            0.220f,
            0.56f,
            0.59f,
            0.62f
        );

        drawDisc(
            eye,
            0.0f,
            0.022f,
            -0.055f,
            0.036f,
            touchpadPressed
                ? 0.22f
                : 0.30f,
            touchpadPressed
                ? 0.88f
                : 0.42f,
            touchpadPressed
                ? 1.00f
                : 0.48f
        );

        drawCube(
            eye,
            0.0f,
            0.024f,
            0.025f,
            0.030f,
            0.010f,
            0.022f,
            homePressed ? 0.20f : 0.72f,
            homePressed ? 0.90f : 0.74f,
            homePressed ? 1.00f : 0.76f
        );

        drawCube(
            eye,
            0.0f,
            0.024f,
            0.060f,
            0.026f,
            0.010f,
            0.018f,
            appPressed ? 0.20f : 0.68f,
            appPressed ? 0.90f : 0.70f,
            appPressed ? 1.00f : 0.72f
        );

        drawCube(
            eye,
            0.059f,
            0.003f,
            -0.002f,
            0.010f,
            0.020f,
            0.032f,
            volumeUpPressed ? 0.20f : 0.62f,
            volumeUpPressed ? 0.90f : 0.65f,
            volumeUpPressed ? 1.00f : 0.68f
        );

        drawCube(
            eye,
            0.059f,
            0.003f,
            0.042f,
            0.010f,
            0.020f,
            0.032f,
            volumeDownPressed ? 0.20f : 0.62f,
            volumeDownPressed ? 0.90f : 0.65f,
            volumeDownPressed ? 1.00f : 0.68f
        );

        drawCube(
            eye,
            0.0f,
            0.0f,
            -0.122f,
            0.028f,
            0.020f,
            0.025f,
            0.15f,
            0.78f,
            0.88f
        );
    }

    private void drawGhostEye(CardboardView.Eye eye) {
        float dx = ghostX - anchorX;
        float dy = ghostY - anchorY;
        float dz = ghostZ - anchorZ;
        float distance = (float) Math.sqrt(dx * dx + dy * dy + dz * dz);
        float opacity = ControllerGhostTether.opacity(distance);
        if (opacity <= 0.0f) return;

        boolean wasBlending = GLES20.glIsEnabled(GLES20.GL_BLEND);
        boolean wasDepthTest = GLES20.glIsEnabled(GLES20.GL_DEPTH_TEST);
        GLES20.glGetBooleanv(
            GLES20.GL_DEPTH_WRITEMASK, previousDepthWrite, 0
        );
        GLES20.glGetIntegerv(
            GLES20.GL_BLEND_SRC_RGB, previousBlendSourceRgb, 0
        );
        GLES20.glGetIntegerv(
            GLES20.GL_BLEND_DST_RGB, previousBlendDestinationRgb, 0
        );
        GLES20.glGetIntegerv(
            GLES20.GL_BLEND_SRC_ALPHA, previousBlendSourceAlpha, 0
        );
        GLES20.glGetIntegerv(
            GLES20.GL_BLEND_DST_ALPHA, previousBlendDestinationAlpha, 0
        );
        GLES20.glGetIntegerv(
            GLES20.GL_BLEND_EQUATION_RGB, previousBlendEquationRgb, 0
        );
        GLES20.glGetIntegerv(
            GLES20.GL_BLEND_EQUATION_ALPHA, previousBlendEquationAlpha, 0
        );
        try {
            GLES20.glEnable(GLES20.GL_DEPTH_TEST);
            GLES20.glDepthMask(false);
            GLES20.glEnable(GLES20.GL_BLEND);
            GLES20.glBlendFunc(
                GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA
            );
            Matrix.setIdentityM(root, 0);
            Matrix.translateM(root, 0, ghostX, ghostY, ghostZ);
            Matrix.multiplyMM(model, 0, root, 0, rotation, 0);
            System.arraycopy(model, 0, root, 0, 16);

            if (assetRenderer.isRenderable()) {
                assetRenderer.drawEye(
                    eye, correctedEyeView, root,
                    false, false, false, false, false, opacity
                );
            } else {
                ghostPass = true;
                ghostOpacity = opacity;
                drawCube(
                    eye, 0.0f, 0.0f, 0.0f,
                    0.105f, 0.040f, 0.220f,
                    0.42f, 0.78f, 0.92f
                );
                drawDisc(
                    eye, 0.0f, 0.022f, -0.055f,
                    0.036f, 0.45f, 0.85f, 0.98f
                );
                drawCube(
                    eye, 0.0f, 0.024f, 0.042f,
                    0.030f, 0.010f, 0.050f,
                    0.42f, 0.78f, 0.92f
                );
            }

            if (springVertexCount > 1) {
                ghostPass = true;
                ghostOpacity = Math.min(0.64f, opacity * 2.0f);
                Matrix.setIdentityM(local, 0);
                drawMesh(
                    eye, springMesh, springVertexCount,
                    GLES20.GL_LINE_STRIP, local,
                    0.30f, 0.86f, 0.98f
                );
            }
        } finally {
            ghostPass = false;
            ghostOpacity = 1.0f;
            GLES20.glDepthMask(previousDepthWrite[0]);
            GLES20.glBlendFuncSeparate(
                previousBlendSourceRgb[0],
                previousBlendDestinationRgb[0],
                previousBlendSourceAlpha[0],
                previousBlendDestinationAlpha[0]
            );
            GLES20.glBlendEquationSeparate(
                previousBlendEquationRgb[0],
                previousBlendEquationAlpha[0]
            );
            if (wasBlending) {
                GLES20.glEnable(GLES20.GL_BLEND);
            } else {
                GLES20.glDisable(GLES20.GL_BLEND);
            }
            if (wasDepthTest) {
                GLES20.glEnable(GLES20.GL_DEPTH_TEST);
            } else {
                GLES20.glDisable(GLES20.GL_DEPTH_TEST);
            }
        }
    }

    private void drawCube(
        CardboardView.Eye eye,
        float x,
        float y,
        float z,
        float sx,
        float sy,
        float sz,
        float red,
        float green,
        float blue
    ) {
        Matrix.setIdentityM(local, 0);
        Matrix.translateM(
            local,
            0,
            x,
            y,
            z
        );
        Matrix.scaleM(
            local,
            0,
            sx,
            sy,
            sz
        );
        Matrix.multiplyMM(
            model,
            0,
            root,
            0,
            local,
            0
        );
        drawMesh(
            eye,
            cube,
            36,
            GLES20.GL_TRIANGLES,
            model,
            red,
            green,
            blue
        );
    }

    private void drawDisc(
        CardboardView.Eye eye,
        float x,
        float y,
        float z,
        float radius,
        float red,
        float green,
        float blue
    ) {
        Matrix.setIdentityM(local, 0);
        Matrix.translateM(
            local,
            0,
            x,
            y,
            z
        );
        Matrix.rotateM(
            local,
            0,
            -90.0f,
            1.0f,
            0.0f,
            0.0f
        );
        Matrix.scaleM(
            local,
            0,
            radius,
            radius,
            radius
        );
        Matrix.multiplyMM(
            model,
            0,
            root,
            0,
            local,
            0
        );
        drawMesh(
            eye,
            disc,
            22,
            GLES20.GL_TRIANGLE_FAN,
            model,
            red,
            green,
            blue
        );
    }

    private void drawMesh(
        CardboardView.Eye eye,
        FloatBuffer mesh,
        int count,
        int primitive,
        float[] modelMatrix,
        float red,
        float green,
        float blue
    ) {
        Matrix.multiplyMM(
            modelView,
            0,
            correctedEyeView,
            0,
            modelMatrix,
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

        GLES20.glUseProgram(program);
        mesh.position(0);
        GLES20.glVertexAttribPointer(
            positionHandle,
            3,
            GLES20.GL_FLOAT,
            false,
            0,
            mesh
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
            ghostPass ? red * 0.65f + 0.12f : red,
            ghostPass ? green * 0.65f + 0.35f : green,
            ghostPass ? blue * 0.65f + 0.35f : blue,
            ghostPass ? ghostOpacity : 1.0f
        );
        GLES20.glDrawArrays(
            primitive,
            0,
            count
        );
        GLES20.glDisableVertexAttribArray(
            positionHandle
        );
    }

    void shutdown() {
        clearGhost();
        assetRenderer.shutdown();
        if (program != 0) {
            GLES20.glDeleteProgram(program);
            program = 0;
        }
    }

    private static FloatBuffer buildDisc(
        int segments
    ) {
        float[] values =
            new float[(segments + 2) * 3];
        int offset = 0;
        values[offset++] = 0.0f;
        values[offset++] = 0.0f;
        values[offset++] = 0.0f;

        for (int index = 0;
             index <= segments;
             index++) {
            float angle =
                (float) (
                    Math.PI
                        * 2.0
                        * index
                        / segments
                );
            values[offset++] =
                (float) Math.cos(angle);
            values[offset++] =
                (float) Math.sin(angle);
            values[offset++] = 0.0f;
        }
        return buffer(values);
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

    private static void quaternionMatrix(
        float x,
        float y,
        float z,
        float w,
        float[] result
    ) {
        float xx = x * x;
        float yy = y * y;
        float zz = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        result[0] = 1.0f - 2.0f * (yy + zz);
        result[1] = 2.0f * (xy + wz);
        result[2] = 2.0f * (xz - wy);
        result[3] = 0.0f;

        result[4] = 2.0f * (xy - wz);
        result[5] = 1.0f - 2.0f * (xx + zz);
        result[6] = 2.0f * (yz + wx);
        result[7] = 0.0f;

        result[8] = 2.0f * (xz + wy);
        result[9] = 2.0f * (yz - wx);
        result[10] = 1.0f - 2.0f * (xx + yy);
        result[11] = 0.0f;

        result[12] = 0.0f;
        result[13] = 0.0f;
        result[14] = 0.0f;
        result[15] = 1.0f;
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
                "Controller model shader link failed: "
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
                "Controller model shader compile failed: "
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
