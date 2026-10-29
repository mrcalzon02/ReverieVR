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
        mesh(closeWhiteRoomRearBackdrop(quadHorizontal(-6.0f, 6.0f,
            -6.0f, 6.0f, -1.35f)));
    private final Mesh whiteCeiling =
        mesh(closeWhiteRoomRearBackdrop(quadHorizontal(-6.0f, 6.0f,
            -6.0f, 6.0f, 4.0f)));
    private final Mesh whiteBack =
        mesh(HomeAtmosphereGeometry.windowedRearWall());
    private final Mesh whiteFront =
        mesh(quadVerticalZ(6.0f, -6.0f, -1.35f, 4.0f, 6.0f));
    private final Mesh whiteLeft =
        mesh(closeWhiteRoomRearBackdrop(quadVerticalX(-6.0f, 6.0f,
            -1.35f, 4.0f, -6.0f)));
    private final Mesh whiteRight =
        mesh(closeWhiteRoomRearBackdrop(quadVerticalX(6.0f, -6.0f,
            -1.35f, 4.0f, 6.0f)));
    private final Mesh mirrorFrame = mesh(
        quadVerticalX(2.17f, -1.02f, -0.92f, 0.92f, 1.02f));
    private final Mesh mirrorGlass = mesh(
        quadVerticalX(HeadsetMirrorProjection.PLANE_X,
            -HeadsetMirrorProjection.HALF_WIDTH,
            -HeadsetMirrorProjection.HALF_HEIGHT,
            HeadsetMirrorProjection.HALF_HEIGHT,
            HeadsetMirrorProjection.HALF_WIDTH));
    private final Mesh wardrobeCase=mesh(HomeAvatarWardrobeGeometry.wardrobeCase());
    private final Mesh wardrobeDoor=mesh(HomeAvatarWardrobeGeometry.wardrobeDoor());
    private final Mesh wardrobeTrim=mesh(HomeAvatarWardrobeGeometry.wardrobeTrim());
    private final Mesh wardrobeHandles=mesh(HomeAvatarWardrobeGeometry.wardrobeHandles());
    // IK triangles are generated once per world frame and drawn per eye.
    private final HomeAvatarLegIk legIk=new HomeAvatarLegIk();
    private final float[] legTriangles=new float[HomeAvatarLegIk.MESH_FLOATS];
    private final float[] legVolumeTriangles=new float[
        HomeAvatarLegIk.MESH_FLOATS * HomeAvatarVolumeGeometry.FLOATS_PER_INPUT_FLOAT];
    private final FloatBuffer legBuffer=ByteBuffer.allocateDirect(
        legVolumeTriangles.length*4).order(ByteOrder.nativeOrder()).asFloatBuffer();

    // Dynamic meshes are created ONLY when the body/garment selection changes,
    // once per onNewFrame, and shared unchanged between the stereo eye passes.
    private HomeAvatarProfile builtAvatar;
    private Mesh avatarArms=avatarVolume(HomeAvatarBodyGeometry.arms(HomeAvatarProfile.DEFAULT),.055f);
    private Mesh avatarTorso=avatarVolume(HomeAvatarBodyGeometry.torso(HomeAvatarProfile.DEFAULT),.105f);
    private Mesh avatarOutfit=avatarVolume(HomeAvatarBodyGeometry.outfit(HomeAvatarProfile.DEFAULT),.118f);
    private Mesh avatarHair=avatarVolume(HomeAvatarBodyGeometry.hair(HomeAvatarProfile.DEFAULT),.112f);
    private final Mesh avatarHead=avatarVolume(HomeAvatarWardrobeGeometry.head(),.14f);
    private Mesh avatarFace=avatarVolume(HomeAvatarBodyGeometry.face(HomeAvatarProfile.DEFAULT),.145f);
    // The mirror draws projected, clipped triangles into one reused per-eye buffer.
    // Portrait meshes remain immutable and are only rebuilt for wardrobe changes.
    private final float[] avatarHeadCenter = new float[3];
    private float[] avatarMirrorScratch = new float[2048];
    private FloatBuffer avatarMirrorVertices = ByteBuffer.allocateDirect(2048 * 4)
        .order(ByteOrder.nativeOrder()).asFloatBuffer();
    void setAvatarLegFrame(float x,float z,long frameNanos,
        boolean walkingEnabled,HomeAvatarProfile profile) {
        HomeAvatarProfile p=profile==null?HomeAvatarProfile.DEFAULT:profile;
        if(builtAvatar==null || !builtAvatar.sameGeometry(p)) {
            avatarArms=avatarVolume(HomeAvatarBodyGeometry.arms(p),.055f);
            avatarTorso=avatarVolume(HomeAvatarBodyGeometry.torso(p),.105f);
            avatarOutfit=avatarVolume(HomeAvatarBodyGeometry.outfit(p),.118f);
            avatarHair=avatarVolume(HomeAvatarBodyGeometry.hair(p),.112f);
            avatarFace=avatarVolume(HomeAvatarBodyGeometry.face(p),.145f);
            legIk.setBodyShape(p);
            builtAvatar=p;
        }
        legIk.update(x,z,frameNanos,walkingEnabled);
        legIk.writeMesh(legTriangles);
        HomeAvatarVolumeGeometry.write(legTriangles,legVolumeTriangles,.046f);
        legBuffer.clear();
        legBuffer.put(legVolumeTriangles);
        legBuffer.position(0);
    }
    private final Mesh[] avatarEyes=makeAvatarParts(0);
    private final Mesh[] avatarHats=makeAvatarParts(1);
    private final Mesh[] avatarAccessories=makeAvatarParts(2);
    private static Mesh[] makeAvatarParts(int type) {
        int count=type==0?HomeAvatarProfile.EYES.length:
            type==1?HomeAvatarProfile.HATS.length:HomeAvatarProfile.EXTRAS.length;
        Mesh[] meshes=new Mesh[count];
        // Geometry beyond the legacy eight hats/props receives a real
        // author-created mesh from its matching identity family.
        for(int i=0;i<count;i++)meshes[i]=avatarVolume(type==0
            ?HomeAvatarWardrobeGeometry.eyes(i):type==1
                ?HomeAvatarWardrobeGeometry.headwear(i)
                :HomeAvatarWardrobeGeometry.accessory(i),
                type==0?.035f:type==1?.092f:.062f);
        return meshes;
    }
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
    private final Mesh birchPanelSeams =
        mesh(closeWhiteRoomRearBackdrop(buildBirchPanelSeams()));
    private final Mesh birchBaseboards =
        mesh(closeWhiteRoomRearBackdrop(buildBirchBaseboards()));
    private final Mesh roomFurniture = mesh(buildRoomFurniture());
    // Separate low-poly detail meshes are built once per renderer, not per eye.
    private final Mesh birchDetailTrim =
        mesh(closeWhiteRoomRearBackdrop(HomeEnvironmentDetailGeometry.birchTrim()));
    private final Mesh birchDetailArt =
        mesh(closeWhiteRoomRearBackdrop(HomeEnvironmentDetailGeometry.birchWallArt()));
    private final Mesh birchDetailFurniture =
        mesh(HomeEnvironmentDetailGeometry.birchShelfAccents());
    private final Mesh birchDetailMotifs =
        mesh(closeWhiteRoomRearBackdrop(HomeEnvironmentDetailGeometry.birchGraphicMotifs()));
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
        mesh(quadHorizontal(-1.75f, 1.75f,
            HomeKeyboardGeometry.BACK_Z - 0.35f,
            HomeKeyboardGeometry.FRONT_Z + 0.17f,
            HomeKeyboardGeometry.DESK_TOP_Y));
    private final Mesh daisFront =
        mesh(quadVerticalZ(-1.75f, 1.75f, -1.35f,
            HomeKeyboardGeometry.DESK_TOP_Y,
            HomeKeyboardGeometry.FRONT_Z + 0.17f));
    // 51 batched, physically raised keycaps remain visible without labels.
    private final Mesh keyboardKeycaps = mesh(buildKeyboardKeycaps());

    // Pre-baked GLES2 atmosphere. Only two bird translations animate.
    private final Mesh skyLow=mesh(HomeAtmosphereGeometry.skyBand(-1.35f,1.5f));
    private final Mesh skyMid=mesh(HomeAtmosphereGeometry.skyBand(1.5f,6f));
    private final Mesh skyHigh=mesh(HomeAtmosphereGeometry.skyBand(6f,14f));
    private final Mesh clouds=mesh(HomeAtmosphereGeometry.highClouds());
    private final Mesh sun=mesh(HomeAtmosphereGeometry.sun());
    private final Mesh forestHills=mesh(HomeAtmosphereGeometry.distantHills(true));
    private final Mesh beachHills=mesh(HomeAtmosphereGeometry.distantHills(false));
    private final Mesh forestAnimals=mesh(HomeAtmosphereGeometry.woodlandWildlife());
    private final Mesh beachAnimals=mesh(HomeAtmosphereGeometry.shoreWildlife());
    private final Mesh bird=mesh(HomeAtmosphereGeometry.flyingBird());
    private final Mesh windowSky=mesh(HomeAtmosphereGeometry.windowSky());
    private final Mesh windowHorizon=mesh(HomeAtmosphereGeometry.windowHorizon());
    private final Mesh windowFrames=mesh(HomeAtmosphereGeometry.windowFrames());
    private final float[] ambientBaseMvp=new float[16];
    private long firstAmbientFrameNanos;
    private float ambientSeconds;

    // The GL-thread frame clock advances once, before left/right eye drawing.
    void setAmbientFrameTime(long nanos) {
        if(nanos<=0L)return;
        if(firstAmbientFrameNanos==0L || nanos<firstAmbientFrameNanos)
            firstAmbientFrameNanos=nanos;
        ambientSeconds=((nanos-firstAmbientFrameNanos)/1_000_000_000f)%10000f;
    }

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
                drawOutdoorSky(true);
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
                draw(forestAnimals,.47f,.32f,.20f,1f);
                drawFlyingBirds(false);
                break;

            case DUNE_BEACH:
                drawOutdoorSky(false);
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
                draw(beachAnimals,.55f,.23f,.14f,1f);
                drawFlyingBirds(true);
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
                // Inset outdoor view just in front of the back wall; no
                // transparency, stencil, extra shader or shell input changes.
                draw(windowSky,.47f,.72f,.88f,1f);
                draw(windowHorizon,.34f,.52f,.43f,1f);
                drawWindowBirds();
                draw(windowFrames,.58f,.47f,.34f,1f);
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
                draw(wardrobeCase,.43f,.30f,.25f,1f);
                draw(wardrobeDoor,.63f,.48f,.36f,1f);
                draw(wardrobeTrim,.86f,.73f,.49f,1f);
                draw(wardrobeHandles,.94f,.80f,.43f,1f);
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

    private void drawOutdoorSky(boolean forest) {
        if(forest) {
            draw(skyLow,.73f,.83f,.78f,1f);
            draw(skyMid,.48f,.70f,.86f,1f);
            draw(skyHigh,.31f,.54f,.77f,1f);
            draw(forestHills,.31f,.45f,.34f,1f);
        } else {
            draw(skyLow,.91f,.81f,.66f,1f);
            draw(skyMid,.56f,.76f,.90f,1f);
            draw(skyHigh,.36f,.60f,.83f,1f);
            draw(beachHills,.50f,.50f,.39f,1f);
        }
        draw(clouds,.89f,.93f,.90f,1f);
        draw(sun,.96f,.88f,.65f,1f);
    }

    private void drawWindowBirds() {
        // Recessed birds stay inside the two real wall apertures without
        // requiring stencil buffers or transparent window rendering.
        System.arraycopy(mvp,0,ambientBaseMvp,0,16);
        for(int i=0;i<2;i++) {
            float progress=(ambientSeconds*.10f+i*.37f)%1f;
            float x=(i==0?-5.20f:3.45f)+progress*1.50f;
            float y=1.60f+(float)Math.sin(ambientSeconds*1.6f+i)*.10f;
            Matrix.translateM(mvp,0,ambientBaseMvp,0,x,y,-4.16f);
            Matrix.scaleM(mvp,0,.15f,.15f,.15f);
            draw(bird,.28f,.32f,.34f,1f);
        }
        System.arraycopy(ambientBaseMvp,0,mvp,0,16);
    }

    private void drawFlyingBirds(boolean beach) {
        // The phase is shared by both eyes. Restore the world MVP exactly.
        System.arraycopy(mvp,0,ambientBaseMvp,0,16);
        for(int i=0;i<2;i++) {
            float t=ambientSeconds, speed=beach?1.16f:.83f;
            float x=-9f+((t*speed+i*9f)%19f);
            float y=(beach?3.5f:4.1f)
                +(float)Math.sin(t*1.8f+i*2.6f)*.24f;
            float z=beach?-11f-i*3f:-10.5f-i*2.4f;
            Matrix.translateM(mvp,0,ambientBaseMvp,0,x,y,z);
            draw(bird,beach?.32f:.20f,beach?.37f:.24f,
                beach?.40f:.25f,1f);
        }
        System.arraycopy(ambientBaseMvp,0,mvp,0,16);
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
     * Shows per-eye projected avatar, headset and controller proxies.
     * Drawn after the room and before UI with the same corrected eye MVP.
     */
    void drawMirrorEye(PlayerHeadRig rig, int eyeIndex,
        boolean controllerVisible, float controllerX,
        float controllerY, float controllerZ,
        float directionX, float directionY, float directionZ,
        HomeAvatarProfile profile) {
        if (program == 0 || !mirrorProjection.prepare(rig, eyeIndex)) {
            return;
        }
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_BLEND);
        draw(mirrorFrame, 0.13f, 0.19f, 0.23f, 1.0f);
        draw(mirrorGlass, 0.08f, 0.14f, 0.18f, 1.0f);
        if (rig.copyHeadCenter(avatarHeadCenter)) {
            // Keep real scene occlusion, but don't let the first silhouette
            // layer erase later costume/face layers in the projected mirror.
            // The headset cage is rendered at a separate foreground depth.
            GLES20.glDepthMask(false);
            drawAvatarInMirror(profile);
            GLES20.glDepthMask(true);
        }
        int headsetCount = mirrorProjection.writeHeadset(mirrorLineScratch);
        drawMirrorLines(headsetCount, 0.35f, 0.91f, 1.0f);
        if (controllerVisible) {
            int controllerCount = mirrorProjection.writeController(
                controllerX, controllerY, controllerZ,
                directionX, directionY, directionZ, mirrorLineScratch);
            drawMirrorLines(controllerCount, 1.0f, 0.72f, 0.26f);
        }
    }

    /**
     * Draw the selected avatar as a world-positioned per-eye reflection,
     * rather than triangles permanently glued onto the mirror glass.
     * Daydream does not track the user's limbs: the existing IK gait remains
     * a procedural cosmetic animation, not a pose-tracking claim.
     */
    private void drawAvatarInMirror(HomeAvatarProfile chosen) {
        HomeAvatarProfile p = chosen == null ? HomeAvatarProfile.DEFAULT : chosen;
        float[] c = HomeAvatarProfile.RGB[p.color];
        boolean armor = p.outfit == 4 || p.outfit == 17 || p.outfit == 20;
        boolean ghost = p.theme == 9;
        float scale = p.heightScale();
        drawProjectedAvatarPart(legBuffer, legVolumeTriangles.length / 3,
            scale, armor ? .32f : c[0] * .57f,
            armor ? .36f : c[1] * .57f, armor ? .44f : c[2] * .57f);
        drawProjectedAvatarPart(avatarArms, scale,
            c[0] * .82f, c[1] * .82f, c[2] * .82f);
        drawProjectedAvatarPart(avatarTorso, scale, c[0], c[1], c[2]);
        float[] fabric = HomeAvatarProfile.RGB[p.fabric];
        drawProjectedAvatarPart(avatarOutfit, scale,
            fabric[0], fabric[1], fabric[2]);
        drawProjectedAvatarPart(avatarHead, scale,
            ghost ? .73f : .72f, ghost ? .89f : .65f, ghost ? .95f : .56f);
        if (mirrorProjection.avatarFaceVisible()) {
            drawProjectedAvatarPart(avatarFace, scale,
                ghost ? .55f : .85f, ghost ? .74f : .73f, ghost ? .88f : .67f);
            drawProjectedAvatarPart(avatarEyes[p.eyes], scale,
                p.eyes == 2 ? .10f : .08f,
                p.eyes == 2 ? .91f : .22f, p.eyes == 2 ? 1f : .30f);
        }
        drawProjectedAvatarPart(avatarHair, scale,
            c[0] * .45f, c[1] * .43f, c[2] * .40f);
        drawProjectedAvatarPart(avatarHats[p.hat], scale,
            c[0] * .82f, c[1] * .82f, c[2] * .82f);
        drawProjectedAvatarPart(avatarAccessories[p.extra], scale,
            .99f, .79f, .33f);
    }

    private void drawProjectedAvatarPart(Mesh mesh, float scale,
        float red, float green, float blue) {
        if (mesh != null) {
            drawProjectedAvatarPart(mesh.vertices, mesh.vertexCount,
                scale, red, green, blue);
        }
    }

    private void drawProjectedAvatarPart(FloatBuffer source, int vertexCount,
        float scale, float red, float green, float blue) {
        if (program == 0 || vertexCount <= 0) return;
        // A clipped triangle can generate at most five output triangles.
        int required = vertexCount * 15;
        if (required > avatarMirrorScratch.length) {
            avatarMirrorScratch = new float[required];
            avatarMirrorVertices = ByteBuffer.allocateDirect(required * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        }
        int count = mirrorProjection.writeAvatarTriangles(
            source, vertexCount,
            avatarHeadCenter[0], avatarHeadCenter[1], avatarHeadCenter[2],
            scale, avatarMirrorScratch);
        if (count == 0) return;
        avatarMirrorVertices.clear();
        avatarMirrorVertices.put(avatarMirrorScratch, 0, count * 3);
        avatarMirrorVertices.position(0);
        GLES20.glUseProgram(program);
        GLES20.glVertexAttribPointer(positionHandle, 3,
            GLES20.GL_FLOAT, false, 0, avatarMirrorVertices);
        GLES20.glEnableVertexAttribArray(positionHandle);
        GLES20.glUniformMatrix4fv(mvpHandle, 1, false, mvp, 0);
        GLES20.glUniform4f(colorHandle, red, green, blue, 1f);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, count);
        GLES20.glDisableVertexAttribArray(positionHandle);
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
            float topBack = HomeKeyboardGeometry.keycapTopAt(back);
            float topFront = HomeKeyboardGeometry.keycapTopAt(front);
            float bottomBack = HomeKeyboardGeometry.keycapBaseAt(back);
            float bottomFront = HomeKeyboardGeometry.keycapBaseAt(front);
            addTriangle(triangles, left, topBack, back,
                right, topBack, back, left, topFront, front);
            addTriangle(triangles, right, topBack, back,
                right, topFront, front, left, topFront, front);
            append(triangles, quadVerticalZ(left, right,
                bottomFront, topFront, front));
            append(triangles, quadVerticalZ(right, left,
                bottomBack, topBack, back));
            addTriangle(triangles, left, bottomBack, back,
                left, topBack, back, left, topFront, front);
            addTriangle(triangles, left, bottomBack, back,
                left, topFront, front, left, bottomFront, front);
            addTriangle(triangles, right, bottomBack, back,
                right, topFront, front, right, topBack, back);
            addTriangle(triangles, right, bottomBack, back,
                right, bottomFront, front, right, topFront, front);
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

    // Pull the white room's rear backdrop forward, from z=-6 to z=-4.1,
    // visually connecting the room to the VR screens at about z=-3.
    // The desk, controls, furniture, mirror and outdoors retain their exact
    // positions. Pre-transform the mesh ONCE so its raycast geometry agrees
    // with the displayed triangles (no per-eye transform or extra draw cost).
    static float[] closeWhiteRoomRearBackdrop(float[] vertices) {
        for (int i = 2; i < vertices.length; i += 3) {
            if (vertices[i] < -3.5f) {
                vertices[i] = -3.5f + (vertices[i] + 3.5f) * 0.24f;
            }
        }
        return vertices;
    }

    private static Mesh avatarVolume(float[] silhouettes,float halfDepth) {
        return mesh(HomeAvatarVolumeGeometry.extrude(silhouettes,halfDepth));
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
