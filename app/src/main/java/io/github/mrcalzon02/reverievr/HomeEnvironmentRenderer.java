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
    // Tiny RGB555 palette generated once at class initialization. No texture
    // files or per-eye palette calculations for static room props.
    private static final float[] BIRCH_MOTIF_COLOR =
        RetroPrimitiveRecipe.color555(.88f, .70f, .44f);
    private static final float[] FOREST_WOOD_COLOR =
        RetroPrimitiveRecipe.color555(.40f, .29f, .17f);
    private static final float[] BEACH_STONE_COLOR =
        RetroPrimitiveRecipe.color555(.52f, .48f, .37f);

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
    private final Mesh mirrorFrame = mesh(
        quadVerticalX(2.17f, -1.02f, -0.92f, 0.92f, 1.02f));
    private final Mesh mirrorGlass = mesh(
        quadVerticalX(HeadsetMirrorProjection.PLANE_X,
            -HeadsetMirrorProjection.HALF_WIDTH,
            -HeadsetMirrorProjection.HALF_HEIGHT,
            HeadsetMirrorProjection.HALF_HEIGHT,
            HeadsetMirrorProjection.HALF_WIDTH));
    private final HeadsetMirrorProjection mirrorProjection =
        new HeadsetMirrorProjection();
    private final float[] mirrorLineScratch = new float[
        HeadsetMirrorProjection.MAX_VERTICES * 3];
    private final float[] mirrorRibbonScratch = new float[
        HeadsetMirrorProjection.MAX_VERTICES / 2 * 18];
    private final FloatBuffer mirrorRibbonVertices =
        ByteBuffer.allocateDirect(mirrorRibbonScratch.length * 4)
            .order(ByteOrder.nativeOrder()).asFloatBuffer();
    // Low-poly 1990s birch living-room details, allocated once.
    private final Mesh birchPanelSeams = mesh(buildBirchPanelSeams());
    private final Mesh birchBaseboards = mesh(buildBirchBaseboards());
    private final Mesh roomFurniture = mesh(buildRoomFurniture());
    // Separate low-poly detail meshes are built once per renderer, not per eye.
    private final Mesh birchDetailTrim =
        mesh(HomeEnvironmentDetailGeometry.birchTrim());
    private final Mesh birchDetailArt =
        mesh(HomeEnvironmentDetailGeometry.birchWallArt());
    private final Mesh birchDetailFurniture =
        mesh(HomeEnvironmentDetailGeometry.birchShelfAccents());
    private final Mesh birchDetailMotifs =
        mesh(HomeEnvironmentDetailGeometry.birchGraphicMotifs());
    private final Mesh forestDetailFoliage =
        mesh(HomeEnvironmentDetailGeometry.forestLayeredFoliage());
    private final Mesh forestDetailStones =
        mesh(HomeEnvironmentDetailGeometry.forestStones());
    private final Mesh forestDetailFlowers =
        mesh(HomeEnvironmentDetailGeometry.forestWildflowers());
    private final Mesh forestDetailFallenWood =
        mesh(HomeEnvironmentDetailGeometry.forestFallenWood(0x4f72));
    private final Mesh beachDetailRipples =
        mesh(HomeEnvironmentDetailGeometry.beachSandRipples());
    private final Mesh beachDetailTufts =
        mesh(HomeEnvironmentDetailGeometry.beachGrassTufts());
    private final Mesh beachDetailShells =
        mesh(HomeEnvironmentDetailGeometry.beachShells());
    private final Mesh beachDetailWavelets =
        mesh(HomeEnvironmentDetailGeometry.beachWavelets());
    private final Mesh beachDetailRockClusters =
        mesh(HomeEnvironmentDetailGeometry.beachRockClusters(0x6a35));
    private final Mesh duneShoreline = mesh(buildShoreline());
    private final Mesh beachDriftwood = mesh(buildDriftwood());
    private final Mesh forestUnderstory = mesh(buildForestUnderstory());
    // World-anchored portrait display assembly; never a headset-locked HUD.
    private final Mesh phonePanelFrame = mesh(
        quadVerticalZ(-0.68f, 0.68f, -0.72f, 1.05f, -2.27f));
    private final Mesh phonePanelGlass = mesh(
        quadVerticalZ(-0.61f, 0.61f, -0.65f, 0.92f, -2.255f));
    private final Mesh[] phonePanelControlKeys = buildPhoneControlKeys();
    private final Mesh[] phonePanelControlGlyphs = buildPhoneControlGlyphs();

    private static Mesh[] buildPhoneControlGlyphs() {
        Mesh[] glyphs = new Mesh[PhonePanelControlGeometry.COUNT];
        for (int i = 0; i < glyphs.length; i++) {
            float middle = PhonePanelControlGeometry.top(i)
                - PhonePanelControlGeometry.KEY_HEIGHT * 0.5f;
            float cx = (PhonePanelControlGeometry.left(i)
                + PhonePanelControlGeometry.right(i)) * 0.5f;
            java.util.List<Float> triangles = new java.util.ArrayList<>();
            // Each rail uses actual tiny low-poly glyphs, not bitmaps.
            if (i <= 1 || i == PhonePanelControlGeometry.SIZE_SMALLER_INDEX
                || i == PhonePanelControlGeometry.SIZE_LARGER_INDEX) {
                append(triangles, quadVerticalZ(cx-0.07f, cx+0.07f,
                    middle-0.012f, middle+0.012f, -2.237f));
                if (i == 1 || i == PhonePanelControlGeometry.SIZE_LARGER_INDEX)
                    append(triangles, quadVerticalZ(cx-0.01f, cx+0.01f,
                        middle-0.051f, middle+0.051f, -2.236f));
            } else if (i == 2 || i == 3) {
                float direction = i == 2 ? 1.0f : -1.0f;
                float tip = middle + direction * 0.048f;
                float bottom = middle - direction * 0.040f;
                append(triangles, quadVerticalZ(cx-0.01f, cx+0.01f,
                    Math.min(tip,bottom), Math.max(tip,bottom), -2.237f));
                append(triangles, quadVerticalZ(cx-0.06f, cx+0.06f,
                    tip-0.010f, tip+0.010f, -2.236f));
            } else if (i == PhonePanelControlGeometry.CLOSE_INDEX) {
                float x0=cx-0.065f, x1=cx+0.065f;
                float y0=middle-0.048f, y1=middle+0.048f;
                float w=0.012f, z=-2.236f;
                addTriangle(triangles, x0-w,y0,z, x0+w,y0,z, x1+w,y1,z);
                addTriangle(triangles, x0-w,y0,z, x1+w,y1,z, x1-w,y1,z);
                addTriangle(triangles, x0-w,y1,z, x0+w,y1,z, x1+w,y0,z);
                addTriangle(triangles, x0-w,y1,z, x1+w,y0,z, x1-w,y0,z);
            } else if (i == PhonePanelControlGeometry.DISTANCE_NEARER_INDEX
                || i == PhonePanelControlGeometry.DISTANCE_FARTHER_INDEX) {
                float sign = i == PhonePanelControlGeometry.DISTANCE_NEARER_INDEX
                    ? 1.0f : -1.0f;
                addTriangle(triangles, cx-0.065f, middle-sign*0.035f, -2.236f,
                    cx+0.065f, middle-sign*0.035f, -2.236f,
                    cx, middle+sign*0.055f, -2.236f);
            } else {
                // Existing roof-shaped orientation reset glyph.
                append(triangles, quadVerticalZ(cx-0.07f, cx+0.07f,
                    middle-0.045f, middle-0.027f, -2.237f));
                append(triangles, quadVerticalZ(cx-0.07f, cx-0.05f,
                    middle-0.045f, middle+0.040f, -2.237f));
                append(triangles, quadVerticalZ(cx+0.05f, cx+0.07f,
                    middle-0.045f, middle+0.040f, -2.237f));
            }
            float[] data = new float[triangles.size()];
            for (int j = 0; j < data.length; j++) data[j] = triangles.get(j);
            glyphs[i] = mesh(data);
        }
        return glyphs;
    }

    private final float[] phoneBaseMvp = new float[16];
    private final float[] phonePose = new float[16];
    private final float[] phoneRotatedMvp = new float[16];

    private static Mesh[] buildPhoneControlKeys() {
        Mesh[] keys = new Mesh[PhonePanelControlGeometry.COUNT];
        for (int i = 0; i < keys.length; i++) {
            float top = PhonePanelControlGeometry.top(i);
            keys[i] = mesh(quadVerticalZ(
                PhonePanelControlGeometry.left(i),
                PhonePanelControlGeometry.right(i),
                top - PhonePanelControlGeometry.KEY_HEIGHT, top,
                PhonePanelControlGeometry.RAY_Z));
        }
        return keys;
    }
    private final Mesh daisTop =
        mesh(quadHorizontal(-1.75f, 1.75f, -4.1f, -1.35f, -1.08f));
    private final Mesh daisFront =
        mesh(quadVerticalZ(-1.75f, 1.75f, -1.35f, -1.08f, -1.35f));
    // 51 batched, physically raised keycaps remain visible without labels.
    private final Mesh keyboardKeycaps = mesh(buildKeyboardKeycaps());

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
        // Corrected Cardboard eye view already contains Home locomotion.
        // A second translation here made the gray dais drift off its keys.
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
                draw(forestUnderstory, 0.17f, 0.42f, 0.16f, 1.0f);
                draw(forestDetailFoliage, 0.24f, 0.47f, 0.22f, 1.0f);
                draw(forestDetailStones, 0.43f, 0.47f, 0.34f, 1.0f);
                draw(forestDetailFlowers, 0.90f, 0.79f, 0.44f, 1.0f);
                draw(forestDetailFallenWood, FOREST_WOOD_COLOR[0],
                    FOREST_WOOD_COLOR[1], FOREST_WOOD_COLOR[2], 1.0f);
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
                draw(duneShoreline, 0.88f, 0.91f, 0.83f, 1.0f);
                draw(beachDriftwood, 0.38f, 0.27f, 0.16f, 1.0f);
                draw(beachDetailRipples, 0.76f, 0.64f, 0.43f, 1.0f);
                draw(beachDetailTufts, 0.36f, 0.41f, 0.23f, 1.0f);
                draw(beachDetailShells, 0.91f, 0.86f, 0.68f, 1.0f);
                draw(beachDetailWavelets, 0.43f, 0.70f, 0.73f, 1.0f);
                draw(beachDetailRockClusters, BEACH_STONE_COLOR[0],
                    BEACH_STONE_COLOR[1], BEACH_STONE_COLOR[2], 1.0f);
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
                draw(birchPanelSeams, 0.69f, 0.60f, 0.48f, 1.0f);
                draw(birchBaseboards, 0.78f, 0.70f, 0.56f, 1.0f);
                draw(roomFurniture, 0.66f, 0.56f, 0.45f, 1.0f);
                draw(birchDetailTrim, 0.80f, 0.74f, 0.64f, 1.0f);
                draw(birchDetailArt, 0.39f, 0.51f, 0.49f, 1.0f);
                draw(birchDetailFurniture, 0.79f, 0.66f, 0.42f, 1.0f);
                draw(birchDetailMotifs, BIRCH_MOTIF_COLOR[0],
                    BIRCH_MOTIF_COLOR[1], BIRCH_MOTIF_COLOR[2], 1.0f);
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
                draw(keyboardKeycaps, 0.16f, 0.21f, 0.28f, 1.0f);
                break;
        }
    }

    /**
     * Draw a genuinely world-space phone-panel housing in Home. The inner
     * surface is deliberately dark until user-consented MediaProjection is
     * available; never depict a fake live Android screen.
     */
    void drawPhonePanel(float yawDegrees, float pitchDegrees,
        float panelScale, float distanceOffset, int focusedKey) {
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_BLEND);
        System.arraycopy(mvp, 0, phoneBaseMvp, 0, 16);
        PhonePanelPose.write(phonePose, yawDegrees, pitchDegrees,
            panelScale, distanceOffset);
        Matrix.multiplyMM(phoneRotatedMvp, 0,
            phoneBaseMvp, 0, phonePose, 0);
        System.arraycopy(phoneRotatedMvp, 0, mvp, 0, 16);
        draw(phonePanelFrame, 0.08f, 0.12f, 0.16f, 1.0f);
        draw(phonePanelGlass, 0.015f, 0.027f, 0.040f, 1.0f);
        for (int i = 0; i < phonePanelControlKeys.length; i++) {
            boolean focused = focusedKey == i;
            draw(phonePanelControlKeys[i],
                focused ? 0.24f : 0.10f,
                focused ? 0.76f : 0.34f,
                focused ? 0.85f : 0.48f, 1.0f);
            draw(phonePanelControlGlyphs[i], 0.93f, 0.96f, 0.99f, 1.0f);
        }
        System.arraycopy(phoneBaseMvp, 0, mvp, 0, 16);
    }

    /**
     * Small planar diagnostic mirror on the white-room right side.
     * Shows only headset/eye and controller proxies, not the environment.
     * Drawn after the room and before UI with the same corrected eye MVP.
     */
    void drawMirrorEye(PlayerHeadRig rig, int eyeIndex,
        boolean controllerVisible, float controllerX,
        float controllerY, float controllerZ,
        float directionX, float directionY, float directionZ) {
        if (program == 0 || !mirrorProjection.prepare(rig, eyeIndex)) {
            return;
        }
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_BLEND);
        draw(mirrorFrame, 0.13f, 0.19f, 0.23f, 1.0f);
        draw(mirrorGlass, 0.08f, 0.14f, 0.18f, 1.0f);
        int headsetCount = mirrorProjection.writeHeadset(mirrorLineScratch);
        drawMirrorLines(headsetCount, 0.35f, 0.91f, 1.0f);
        if (controllerVisible) {
            int controllerCount = mirrorProjection.writeController(
                controllerX, controllerY, controllerZ,
                directionX, directionY, directionZ, mirrorLineScratch);
            drawMirrorLines(controllerCount, 1.0f, 0.72f, 0.26f);
        }
    }

    private void drawMirrorLines(int count, float red, float green,
        float blue) {
        if (count <= 0) return;
        // GLES2 implementations may clamp GL_LINES to one pixel. Convert
        // clipped segments to narrow, deterministic mirror-plane triangles.
        // Buffers are allocated once and reused for both eyes.
        int ribbonVertices = MirrorLineRibbons.write(
            mirrorLineScratch, count, mirrorRibbonScratch);
        if (ribbonVertices == 0) return;
        mirrorRibbonVertices.clear();
        mirrorRibbonVertices.put(mirrorRibbonScratch, 0, ribbonVertices * 3);
        mirrorRibbonVertices.position(0);
        GLES20.glUseProgram(program);
        GLES20.glVertexAttribPointer(positionHandle, 3,
            GLES20.GL_FLOAT, false, 0, mirrorRibbonVertices);
        GLES20.glEnableVertexAttribArray(positionHandle);
        GLES20.glUniformMatrix4fv(mvpHandle, 1, false, mvp, 0);
        GLES20.glUniform4f(colorHandle, red, green, blue, 1.0f);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, ribbonVertices);
        GLES20.glDisableVertexAttribArray(positionHandle);
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

    private static float[] buildBirchPanelSeams() {
        List<Float> values = new ArrayList<>();
        // Narrow vertical seams break up the featureless white room;
        // alternating wall spacing evokes the pale plywood of a 1990s den.
        for (int index = -5; index <= 5; index++) {
            float x = index;
            append(values, quadVerticalZ(x, x + 0.018f,
                -1.30f, 3.94f, -5.985f));
            float z = index;
            append(values, quadVerticalX(-5.985f, z,
                -1.30f, 3.94f, z + 0.018f));
            append(values, quadVerticalX(5.985f, z,
                -1.30f, 3.94f, z + 0.018f));
        }
        return toArray(values);
    }

    private static float[] buildBirchBaseboards() {
        List<Float> values = new ArrayList<>();
        append(values, quadVerticalZ(-5.98f, 5.98f,
            -1.34f, -1.16f, -5.965f));
        append(values, quadVerticalX(-5.965f, -5.98f,
            -1.34f, -1.16f, 5.98f));
        append(values, quadVerticalX(5.965f, -5.98f,
            -1.34f, -1.16f, 5.98f));
        return toArray(values);
    }

    private static float[] buildKeyboardKeycaps() {
        List<Float> triangles = new ArrayList<>();
        for (int[] rect : HomeKeyboardGeometry.keyRects()) {
            float left = HomeKeyboardGeometry.worldX(rect[0]);
            float right = HomeKeyboardGeometry.worldX(rect[2]);
            float back = HomeKeyboardGeometry.worldZ(rect[1]);
            float front = HomeKeyboardGeometry.worldZ(rect[3]);
            float bottom = HomeKeyboardGeometry.KEYCAP_BASE_Y;
            float top = HomeKeyboardGeometry.KEYCAP_TOP_Y;
            append(triangles, quadHorizontal(left, right, back, front, top));
            append(triangles, quadVerticalZ(left, right, bottom, top, front));
            append(triangles, quadVerticalZ(right, left, bottom, top, back));
            append(triangles, quadVerticalX(left, front, bottom, top, back));
            append(triangles, quadVerticalX(right, back, bottom, top, front));
        }
        return toArray(triangles);
    }

    private static float[] buildRoomFurniture() {
        List<Float> values = new ArrayList<>();
        // Deliberately simple, low-slung birch console and bench.
        // Both sit clear of the player/central interaction lane.
        append(values, quadHorizontal(-4.6f, -2.3f,
            -4.7f, -3.8f, -0.83f));
        append(values, quadVerticalZ(-4.6f, -2.3f,
            -1.34f, -0.83f, -3.8f));
        append(values, quadHorizontal(2.8f, 4.8f,
            -3.8f, -2.6f, -0.83f));
        append(values, quadVerticalZ(2.8f, 4.8f,
            -1.34f, -0.83f, -2.6f));
        return toArray(values);
    }

    private static float[] buildShoreline() {
        List<Float> values = new ArrayList<>();
        // Thin pale foam bands retain the original untextured GLES2 budget.
        for (int band = 0; band < 7; band++) {
            float z = -6.0f - band * 1.65f;
            float span = 2.0f + band * 0.45f;
            append(values, quadHorizontal(-span, span,
                z - 0.05f, z + 0.05f, -1.286f));
        }
        return toArray(values);
    }

    private static float[] buildDriftwood() {
        List<Float> values = new ArrayList<>();
        for (int piece = 0; piece < 9; piece++) {
            float x = (piece % 5 - 2) * 2.8f;
            float z = (piece / 5) * 3.9f - 1.0f;
            append(values, quadHorizontal(x - 0.45f, x + 0.45f,
                z - 0.075f, z + 0.075f, -1.335f));
        }
        return toArray(values);
    }

    private static float[] buildForestUnderstory() {
        List<Float> values = new ArrayList<>();
        for (int plant = 0; plant < 52; plant++) {
            float theta = plant * 2.399963f;
            float radius = 4.1f + (plant % 7) * 0.48f;
            float x = (float) Math.cos(theta) * radius;
            float z = (float) Math.sin(theta) * radius;
            appendFacingQuad(values, x, z, 0.30f,
                -1.35f, -0.48f + (plant % 3) * 0.12f);
        }
        return toArray(values);
    }

    private static void append(List<Float> target, float[] source) {
        for (float value : source) target.add(value);
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

    /** One nearest-hit search per Home input sample, never once per eye. */
    void hitScene(float[] origin, float[] direction,
                  HomeEnvironment environment, HomeSceneRaycast.Hit hit) {
        hit.reset();
        HomeEnvironment active = environment == null
            ? HomeEnvironment.WHITE_ROOM : environment;
        switch (active) {
            case FOREST_GLADE:
                hit(forestTrunks, origin, direction, hit);
                hit(forestCanopies, origin, direction, hit);
                hit(forestUnderstory, origin, direction, hit);
                hit(forestDetailFoliage, origin, direction, hit);
                hit(forestDetailStones, origin, direction, hit);
                hit(forestDetailFlowers, origin, direction, hit);
                hit(forestDetailFallenWood, origin, direction, hit);
                hit(outdoorGround, origin, direction, hit);
                break;
            case DUNE_BEACH:
                hit(beachDriftwood, origin, direction, hit);
                hit(duneRidges, origin, direction, hit);
                hit(beachGrass, origin, direction, hit);
                hit(beachDetailRipples, origin, direction, hit);
                hit(beachDetailTufts, origin, direction, hit);
                hit(beachDetailShells, origin, direction, hit);
                hit(beachDetailWavelets, origin, direction, hit);
                hit(beachDetailRockClusters, origin, direction, hit);
                hit(duneShoreline, origin, direction, hit);
                hit(ocean, origin, direction, hit);
                hit(outdoorGround, origin, direction, hit);
                break;
            case WHITE_ROOM:
            default:
                hit(whiteFloor, origin, direction, hit);
                hit(whiteCeiling, origin, direction, hit);
                hit(whiteBack, origin, direction, hit);
                hit(whiteFront, origin, direction, hit);
                hit(whiteLeft, origin, direction, hit);
                hit(whiteRight, origin, direction, hit);
                hit(birchPanelSeams, origin, direction, hit);
                hit(birchBaseboards, origin, direction, hit);
                hit(roomFurniture, origin, direction, hit);
                hit(birchDetailTrim, origin, direction, hit);
                hit(birchDetailArt, origin, direction, hit);
                hit(birchDetailFurniture, origin, direction, hit);
                hit(birchDetailMotifs, origin, direction, hit);
                hit(daisTop, origin, direction, hit);
                hit(daisFront, origin, direction, hit);
                // Keycap selection retains a 35mm visual/picking tolerance.
                hit(keyboardKeycaps, origin, direction, hit);
                break;
        }
    }

    private static void hit(Mesh mesh, float[] origin, float[] direction,
                            HomeSceneRaycast.Hit out) {
        HomeSceneRaycast.test(mesh.contact, origin, direction, out);
    }

    private static Mesh mesh(float[] values) {
        FloatBuffer buffer =
            ByteBuffer
                .allocateDirect(values.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        buffer.put(values);
        buffer.position(0);
        return new Mesh(buffer, values.length / 3,
            new HomeSceneRaycast.Mesh(values));
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
        final HomeSceneRaycast.Mesh contact;

        Mesh(FloatBuffer vertices, int vertexCount,
             HomeSceneRaycast.Mesh contact) {
            this.vertices = vertices;
            this.vertexCount = vertexCount;
            this.contact = contact;
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
