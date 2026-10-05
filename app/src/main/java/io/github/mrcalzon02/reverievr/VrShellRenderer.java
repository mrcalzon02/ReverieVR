package io.github.mrcalzon02.reverievr;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.opengl.Matrix;

import com.google.cardboard.sdk.CardboardView;
import com.google.cardboard.sdk.HeadTransform;
import com.google.cardboard.sdk.Viewport;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import javax.microedition.khronos.egl.EGLConfig;

final class VrShellRenderer implements CardboardView.Renderer {
    interface Host {
        void onExitToPhoneRequested();
        void onSetupCompleted();
        void onControllerRecenterRequested();
        void onUiActionRejected();
        PerformanceEnvironmentSnapshot
            getPerformanceEnvironmentSnapshot();
        void onVideoSurfaceTextureReady(SurfaceTexture surfaceTexture);
        void onVideoPlaybackRequested();
        void onVideoTogglePauseRequested();
        void onVideoSeekRequested(int deltaMillis);
        void onVideoStopRequested();
        boolean onDosPlaybackRequested(String moduleId);
        void onDosOverlayPauseRequested();
        void onDosOverlayResumeRequested();
        void onVolumeAdjustRequested(int direction);
        String getActiveBindingProfileName();
        String getActiveBindingTuningSummary();
        void onDosBindingProfileCycleRequested(
            int direction
        );
        void onDosBindingSensitivityAdjustRequested(
            int direction
        );
        void onDosBindingDeadzoneAdjustRequested(
            int direction
        );
        void onDosBindingResetRequested();
        void onDosStopRequested();
        boolean onNativeModulePlaybackRequested(
            String moduleId
        );
        void onNativeModuleStopRequested();
        void onHeadBindingDelta(
            float yawDeltaRadians,
            float pitchDeltaRadians
        );
    }

    private static final int TEXTURE_WIDTH = 1024;
    private static final int TEXTURE_HEIGHT = 768;
    private static final int HUD_TEXTURE_WIDTH = 512;
    private static final int HUD_TEXTURE_HEIGHT = 128;
    private static final float HUD_LOOK_UP_THRESHOLD = 0.72f;

    private static final float PANEL_HALF_WIDTH = 1.70f;
    private static final float PANEL_HALF_HEIGHT = 1.20f;
    private static final float PANEL_Z = -3.0f;

    private static final float Z_NEAR = 0.10f;
    private static final float Z_FAR = 30.0f;

    private static final int MODE_SETUP = 0;
    private static final int MODE_HOME = 1;
    private static final int MODE_VIDEO = 2;
    private static final int MODE_DOS = 3;
    private static final int MODE_DOS_LIBRARY = 4;
    private static final int MODE_NATIVE = 5;
    private static final int MODE_DOS_OVERLAY = 6;
    private static final int MODE_DOS_BINDINGS = 7;

    private static final int[][] HOME_BUTTONS = new int[][] {
        {140, 225, 884, 280},
        {140, 295, 884, 350},
        {140, 365, 884, 420},
        {140, 435, 884, 490},
        {140, 505, 884, 560},
        {140, 575, 884, 630}
    };

    private static final int[][] DOS_LIBRARY_BUTTONS = new int[][] {
        {140, 235, 884, 300},
        {140, 315, 884, 380},
        {140, 395, 884, 460},
        {140, 475, 884, 540},
        {140, 555, 884, 620}
    };

    private static final int[][] DOS_OVERLAY_BUTTONS = new int[][] {
        {100, 260, 480, 325},
        {544, 260, 924, 325},
        {100, 345, 480, 410},
        {544, 345, 924, 410},
        {100, 430, 480, 495},
        {544, 430, 924, 495},
        {220, 520, 804, 590}
    };

    private static final int[][] DOS_BINDING_BUTTONS = new int[][] {
        {100, 280, 480, 345},
        {544, 280, 924, 345},
        {100, 365, 480, 430},
        {544, 365, 924, 430},
        {100, 450, 480, 515},
        {544, 450, 924, 515},
        {100, 535, 480, 600},
        {544, 535, 924, 600}
    };

    private static final int[][] THREE_BUTTONS = new int[][] {
        {110, 500, 390, 590},
        {405, 500, 685, 590},
        {700, 500, 914, 590}
    };

    private static final int[][] TWO_BUTTONS = new int[][] {
        {160, 500, 500, 590},
        {524, 500, 864, 590}
    };

    private final ReveriePreferences preferences;
    private final Host host;
    private final float viewerInterLensMeters;
    private final VideoSurfaceRenderer videoRenderer;
    private final DosSession dosSession;
    private final DosSurfaceRenderer dosRenderer;
    private final NativeModuleRuntime nativeModuleRuntime;
    private final List<NativeModuleRuntime.Descriptor> nativeModules;
    private final FramePerformanceTracker performanceTracker =
        new FramePerformanceTracker();

    private final FloatBuffer vertexBuffer;
    private final FloatBuffer uvBuffer;
    private final FloatBuffer hudVertexBuffer;
    private final FloatBuffer hudUvBuffer;

    private final float[] rawHeadView = new float[16];
    private final float[] adjustedHeadView = new float[16];
    private final float[] eyeView = new float[16];
    private final float[] modelViewProjection = new float[16];
    private final float[] tempMatrix = new float[16];
    private final float[] hudIdentity = new float[16];
    private final float[] hudVertices = new float[12];
    private final float[] yawMatrix = new float[16];
    private final float[] headEuler = new float[3];
    private final float[] headForward = new float[3];
    private final float[] adjustedHeadForward = new float[3];

    private final AtomicBoolean selectRequested = new AtomicBoolean();
    private final AtomicBoolean backRequested = new AtomicBoolean();
    private final AtomicBoolean recenterRequested = new AtomicBoolean();
    private final AtomicInteger phoneBattery = new AtomicInteger(-1);
    private final AtomicInteger controllerBattery = new AtomicInteger(-1);
    private final AtomicInteger videoSeekRequestedMillis = new AtomicInteger();
    private final AtomicBoolean dosExitRequested = new AtomicBoolean();

    private volatile boolean controllerConnected;
    private volatile String controllerMessage = "Controller";
    private volatile String lastInputAction = "Waiting for input";
    private volatile String lastInputSource = "No action received yet";
    private volatile boolean textureDirty = true;
    private volatile boolean dosRuntimeAvailable;
    private volatile String[] dosModuleIds = new String[0];
    private volatile String[] dosModuleNames = new String[0];
    private volatile String activeDosModuleName = "";
    private int dosLibraryPage;
    private boolean nativeSurfaceReady;

    private int program;
    private int texture;
    private int hudTexture;
    private Bitmap uiBitmap;
    private Canvas uiCanvas;
    private Paint uiPaint;
    private Bitmap hudBitmap;
    private Canvas hudCanvas;
    private Paint hudPaint;
    private boolean textureStorageInitialized;
    private boolean hudTextureStorageInitialized;
    private volatile boolean hudTextureDirty = true;
    private volatile boolean hudDroppedDown;
    private boolean cachedShowPercentages;
    private int positionHandle;
    private int uvHandle;
    private int matrixHandle;
    private int textureHandle;

    private volatile int mode;
    private int setupStep;
    private volatile int hoveredButton = -1;
    private float yawOffsetRadians;
    private float userIpdMeters;
    private float uiScale;
    private boolean bindingHeadInitialized;
    private long lastPerformanceLogNanos;
    private float previousBindingYaw;
    private float previousBindingPitch;

    VrShellRenderer(
        ReveriePreferences preferences,
        float viewerInterLensMeters,
        DosSession dosSession,
        NativeModuleRuntime nativeModuleRuntime,
        Host host
    ) {
        this.preferences = preferences;
        this.viewerInterLensMeters = clamp(viewerInterLensMeters, 0.050f, 0.080f);
        this.host = host;
        this.dosSession = dosSession;
        this.nativeModuleRuntime =
            nativeModuleRuntime;
        nativeModules =
            NativeModuleRuntime.listBuiltIns();
        videoRenderer = new VideoSurfaceRenderer(host::onVideoSurfaceTextureReady);
        dosRenderer = new DosSurfaceRenderer(dosSession);

        userIpdMeters = preferences.getUserIpdMeters(this.viewerInterLensMeters);
        uiScale = preferences.getUiScale();

        if (preferences.isVrSetupCurrent()) {
            mode = MODE_HOME;
            setupStep = 0;
        } else {
            mode = MODE_SETUP;
            setupStep = clampInt(preferences.getVrSetupStep(), 0, 5);
        }

        float[] vertices = new float[] {
            -PANEL_HALF_WIDTH, -PANEL_HALF_HEIGHT, PANEL_Z,
             PANEL_HALF_WIDTH, -PANEL_HALF_HEIGHT, PANEL_Z,
            -PANEL_HALF_WIDTH,  PANEL_HALF_HEIGHT, PANEL_Z,
             PANEL_HALF_WIDTH,  PANEL_HALF_HEIGHT, PANEL_Z
        };

        float[] uvs = new float[] {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f
        };

        vertexBuffer = allocate(vertices);
        uvBuffer = allocate(uvs);

        hudVertexBuffer = allocate(new float[12]);
        hudUvBuffer = allocate(new float[] {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f
        });

        Matrix.setIdentityM(adjustedHeadView, 0);
        Matrix.setIdentityM(yawMatrix, 0);
        Matrix.setIdentityM(hudIdentity, 0);
        cachedShowPercentages = preferences.isShowPercentagesEnabled();
    }

    boolean handlesSelectAsShellAction() {
        int currentMode = mode;
        return currentMode != MODE_DOS
            && currentMode != MODE_NATIVE;
    }

    boolean canActivateSelect() {
        return mode == MODE_VIDEO
            || hoveredButton >= 0;
    }

    void requestSelect() {
        selectRequested.set(true);
    }

    void requestBack() {
        backRequested.set(true);
    }

    void requestRecenter() {
        recenterRequested.set(true);
    }

    void setPhoneBattery(int percentage) {
        phoneBattery.set(percentage);
        hudTextureDirty = true;
    }

    void setControllerBattery(int percentage) {
        controllerBattery.set(percentage);
        hudTextureDirty = true;
    }

    void setControllerState(boolean connected, String message) {
        controllerConnected = connected;
        controllerMessage = message == null ? "Controller" : message;
        textureDirty = true;
    }

    void setVideoAspectRatio(float aspectRatio) {
        videoRenderer.setVideoAspectRatio(aspectRatio);
    }

    void noteInputAction(VrInputAction action, String source) {
        if (action == null) {
            return;
        }

        lastInputAction = action.name().replace('_', ' ');
        lastInputSource =
            source == null || source.trim().isEmpty()
                ? "Unknown input source"
                : source.trim();
        textureDirty = true;
    }

    boolean consumeBackDuringInputTraining() {
        return mode == MODE_SETUP && setupStep == 1;
    }

    void requestVideoExit() {
        backRequested.set(true);
    }

    void requestDosExit() {
        dosExitRequested.set(true);
    }

    boolean isHostedInputSuppressed() {
        return mode == MODE_DOS_OVERLAY
            || mode == MODE_DOS_BINDINGS;
    }

    void setDosModules(
        boolean runtimeAvailable,
        List<DosGameModule> modules
    ) {
        dosRuntimeAvailable = runtimeAvailable;

        int count = modules == null ? 0 : modules.size();
        String[] ids = new String[count];
        String[] names = new String[count];

        for (int index = 0; index < count; index++) {
            DosGameModule module = modules.get(index);
            ids[index] = module.id;
            names[index] = module.displayName;
        }

        dosModuleIds = ids;
        dosModuleNames = names;

        int pageCount = Math.max(1, (count + 2) / 3);
        if (dosLibraryPage >= pageCount) {
            dosLibraryPage = pageCount - 1;
        }

        textureDirty = true;
    }

    void requestVideoSeek(int deltaMillis) {
        if (deltaMillis == 0 || mode != MODE_VIDEO) {
            videoSeekRequestedMillis.set(0);
            return;
        }
        videoSeekRequestedMillis.set(deltaMillis);
    }

    @Override
    public void onNewFrame(HeadTransform headTransform) {
        long frameNanos = System.nanoTime();
        performanceTracker.recordFrame(frameNanos);
        if (lastPerformanceLogNanos == 0L) {
            lastPerformanceLogNanos = frameNanos;
        } else if (frameNanos - lastPerformanceLogNanos >= 60000000000L) {
            FramePerformanceTracker.Snapshot snapshot =
                performanceTracker.snapshot();
            PerformanceEnvironmentSnapshot environment =
                host.getPerformanceEnvironmentSnapshot();
            if (environment == null) {
                environment =
                    PerformanceEnvironmentSnapshot.unavailable();
            }
            ReverieLog.milestone(
                "VR_PERFORMANCE",
                "mode="
                    + mode
                    + " "
                    + environment.toLogString()
                    + " "
                    + snapshot.toLogString()
            );
            lastPerformanceLogNanos = frameNanos;
        }

        headTransform.getHeadView(rawHeadView, 0);
        headTransform.getEulerAngles(headEuler, 0);
        headTransform.getForwardVector(headForward, 0);

        float bindingPitch = headEuler[0];
        float bindingYaw = headEuler[1];
        if (bindingHeadInitialized) {
            host.onHeadBindingDelta(
                wrapAngle(bindingYaw - previousBindingYaw),
                bindingPitch - previousBindingPitch
            );
        } else {
            bindingHeadInitialized = true;
        }
        previousBindingYaw = bindingYaw;
        previousBindingPitch = bindingPitch;

        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "VR_FRAME",
                "mode="
                    + mode
                    + " yaw="
                    + bindingYaw
                    + " pitch="
                    + bindingPitch
                    + " forwardX="
                    + headForward[0]
                    + " forwardY="
                    + headForward[1]
                    + " forwardZ="
                    + headForward[2]
                    + " hudDropped="
                    + hudDroppedDown
                    + " phoneBattery="
                    + phoneBattery.get()
                    + " controllerBattery="
                    + controllerBattery.get()
            );
        }

        if (recenterRequested.getAndSet(false)) {
            yawOffsetRadians = headEuler[1];
            host.onControllerRecenterRequested();
            textureDirty = true;
        }

        Matrix.setRotateM(
            yawMatrix,
            0,
            (float) Math.toDegrees(-yawOffsetRadians),
            0.0f,
            1.0f,
            0.0f
        );
        Matrix.multiplyMM(adjustedHeadView, 0, yawMatrix, 0, rawHeadView, 0);

        boolean showPercentages = preferences.isShowPercentagesEnabled();
        if (showPercentages != cachedShowPercentages) {
            cachedShowPercentages = showPercentages;
            hudTextureDirty = true;
        }

        boolean newHudDroppedDown =
            preferences.isLookUpRevealEnabled()
                && headForward[1] >= HUD_LOOK_UP_THRESHOLD;
        if (newHudDroppedDown != hudDroppedDown) {
            hudDroppedDown = newHudDroppedDown;
        }

        if (mode == MODE_NATIVE) {
            selectRequested.set(false);

            if (backRequested.getAndSet(false)) {
                if (nativeSurfaceReady
                    && nativeModuleRuntime != null) {
                    nativeModuleRuntime.releaseSurface();
                }
                nativeSurfaceReady = false;
                host.onNativeModuleStopRequested();
                mode = MODE_HOME;
                hoveredButton = -1;
                textureDirty = true;
                return;
            }

            if (nativeModuleRuntime == null
                || !nativeModuleRuntime.isRunning()) {
                nativeSurfaceReady = false;
                mode = MODE_HOME;
                hoveredButton = -1;
                textureDirty = true;
                return;
            }

            if (!nativeSurfaceReady) {
                nativeSurfaceReady =
                    nativeModuleRuntime.onSurfaceCreated();
                if (!nativeSurfaceReady) {
                    host.onNativeModuleStopRequested();
                    mode = MODE_HOME;
                    hoveredButton = -1;
                    textureDirty = true;
                    return;
                }
            }

            nativeModuleRuntime.update();
            return;
        }

        if (mode == MODE_DOS) {
            dosRenderer.updateFrame();
            selectRequested.set(false);

            if (dosExitRequested.getAndSet(false)) {
                activeDosModuleName = "";
                mode = MODE_HOME;
                hoveredButton = -1;
                textureDirty = true;
                return;
            }

            if (backRequested.getAndSet(false)) {
                host.onDosOverlayPauseRequested();
                mode = MODE_DOS_OVERLAY;
                hoveredButton = -1;
                textureDirty = true;
            }
            return;
        }

        if (mode == MODE_DOS_OVERLAY
            || mode == MODE_DOS_BINDINGS) {
            dosRenderer.updateFrame();

            if (dosExitRequested.getAndSet(false)) {
                activeDosModuleName = "";
                mode = MODE_HOME;
                hoveredButton = -1;
                textureDirty = true;
                return;
            }
        }

        if (mode == MODE_VIDEO) {
            videoRenderer.updateFrame();

            if (backRequested.getAndSet(false)) {
                host.onVideoStopRequested();
                videoSeekRequestedMillis.set(0);
                mode = MODE_HOME;
                hoveredButton = -1;
                textureDirty = true;
                return;
            }

            int seekMillis = videoSeekRequestedMillis.getAndSet(0);
            if (seekMillis != 0) {
                host.onVideoSeekRequested(seekMillis);
            }

            if (selectRequested.getAndSet(false)) {
                host.onVideoTogglePauseRequested();
            }
            return;
        }

        rotateYaw(headForward, -yawOffsetRadians, adjustedHeadForward);
        int newHover = calculateHoveredButton(adjustedHeadForward);
        if (newHover != hoveredButton) {
            hoveredButton = newHover;
            textureDirty = true;
        }

        if (backRequested.getAndSet(false)) {
            handleBack();
        }

        if (selectRequested.getAndSet(false)) {
            activateHoveredButton();
        }
    }

    @Override
    public void onDrawEye(CardboardView.Eye eye) {
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glClearColor(0.015f, 0.02f, 0.025f, 1.0f);
        GLES20.glClear(
            GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT
        );

        eye.applyHeadView(adjustedHeadView);

        float correctionHalf =
            (userIpdMeters - viewerInterLensMeters) * 0.5f;
        float eyeCorrection = eye.getEyeType() == CardboardView.Eye.LEFT
            ? correctionHalf
            : -correctionHalf;

        if (mode == MODE_VIDEO) {
            videoRenderer.drawEye(eye, eyeCorrection);
            drawPowerHudOverlay();
            return;
        }

        if (mode == MODE_DOS) {
            dosRenderer.drawEye(eye, eyeCorrection);
            drawPowerHudOverlay();
            return;
        }

        if (mode == MODE_DOS_OVERLAY
            || mode == MODE_DOS_BINDINGS) {
            dosRenderer.drawEye(eye, eyeCorrection);
            drawUiPanel(eye, eyeCorrection, true);
            drawPowerHudOverlay();
            return;
        }

        if (mode == MODE_NATIVE) {
            System.arraycopy(
                eye.getEyeView(),
                0,
                eyeView,
                0,
                16
            );

            Matrix.translateM(
                tempMatrix,
                0,
                eyeView,
                0,
                eyeCorrection,
                0.0f,
                0.0f
            );

            int eyeIndex =
                eye.getEyeType()
                    == CardboardView.Eye.LEFT
                    ? 0
                    : 1;

            boolean rendered =
                nativeModuleRuntime != null
                    && nativeSurfaceReady
                    && nativeModuleRuntime.renderEye(
                        eyeIndex,
                        tempMatrix,
                        eye.getPerspective(
                            Z_NEAR,
                            Z_FAR
                        )
                    );

            if (!rendered) {
                if (nativeSurfaceReady
                    && nativeModuleRuntime != null) {
                    nativeModuleRuntime.releaseSurface();
                }
                nativeSurfaceReady = false;
                host.onNativeModuleStopRequested();
                mode = MODE_HOME;
                hoveredButton = -1;
                textureDirty = true;
            }

            drawPowerHudOverlay();
            return;
        }

        drawUiPanel(eye, eyeCorrection, false);
        drawPowerHudOverlay();
    }

    private void drawUiPanel(
        CardboardView.Eye eye,
        float eyeCorrection,
        boolean forceForeground
    ) {
        if (forceForeground) {
            GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        }

        if (textureDirty) {
            rebuildTexture();
        }

        System.arraycopy(eye.getEyeView(), 0, eyeView, 0, 16);

        Matrix.translateM(
            tempMatrix,
            0,
            eyeView,
            0,
            eyeCorrection,
            0.0f,
            0.0f
        );

        Matrix.multiplyMM(
            modelViewProjection,
            0,
            eye.getPerspective(Z_NEAR, Z_FAR),
            0,
            tempMatrix,
            0
        );

        GLES20.glUseProgram(program);

        vertexBuffer.position(0);
        GLES20.glVertexAttribPointer(
            positionHandle,
            3,
            GLES20.GL_FLOAT,
            false,
            0,
            vertexBuffer
        );
        GLES20.glEnableVertexAttribArray(positionHandle);

        uvBuffer.position(0);
        GLES20.glVertexAttribPointer(
            uvHandle,
            2,
            GLES20.GL_FLOAT,
            false,
            0,
            uvBuffer
        );
        GLES20.glEnableVertexAttribArray(uvHandle);

        GLES20.glUniformMatrix4fv(matrixHandle, 1, false, modelViewProjection, 0);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glUniform1i(textureHandle, 0);

        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glDisable(GLES20.GL_BLEND);

        GLES20.glDisableVertexAttribArray(positionHandle);
        GLES20.glDisableVertexAttribArray(uvHandle);


    }

    @Override
    public void onFinishFrame(Viewport viewport) {
    }

    @Override
    public void onSurfaceChanged(int width, int height) {
        GLES20.glViewport(0, 0, width, height);
    }

    @Override
    public void onSurfaceCreated(EGLConfig config) {
        videoRenderer.onSurfaceCreated();
        dosRenderer.onSurfaceCreated();
        nativeSurfaceReady = false;

        program = buildProgram(VERTEX_SHADER, FRAGMENT_SHADER);
        positionHandle = GLES20.glGetAttribLocation(program, "a_Position");
        uvHandle = GLES20.glGetAttribLocation(program, "a_TexCoord");
        matrixHandle = GLES20.glGetUniformLocation(program, "u_Mvp");
        textureHandle = GLES20.glGetUniformLocation(program, "u_Texture");

        int[] textures = new int[2];
        GLES20.glGenTextures(2, textures, 0);
        texture = textures[0];
        hudTexture = textures[1];
        textureStorageInitialized = false;
        hudTextureStorageInitialized = false;
        ensureUiBitmap();
        ensureHudBitmap();
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MIN_FILTER,
            GLES20.GL_LINEAR
        );
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MAG_FILTER,
            GLES20.GL_LINEAR
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

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, hudTexture);
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MIN_FILTER,
            GLES20.GL_LINEAR
        );
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MAG_FILTER,
            GLES20.GL_LINEAR
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

        textureDirty = true;
        hudTextureDirty = true;
    }

    @Override
    public void onRendererShutdown() {
        videoRenderer.shutdown();
        dosRenderer.shutdown();

        if (nativeSurfaceReady
            && nativeModuleRuntime != null
            && nativeModuleRuntime.isRunning()) {
            nativeModuleRuntime.releaseSurface();
        }
        nativeSurfaceReady = false;

        if (texture != 0) {
            GLES20.glDeleteTextures(1, new int[] {texture}, 0);
            texture = 0;
        }
        if (hudTexture != 0) {
            GLES20.glDeleteTextures(1, new int[] {hudTexture}, 0);
            hudTexture = 0;
        }
        if (program != 0) {
            GLES20.glDeleteProgram(program);
            program = 0;
        }

        textureStorageInitialized = false;
        hudTextureStorageInitialized = false;
        if (uiBitmap != null && !uiBitmap.isRecycled()) {
            uiBitmap.recycle();
        }
        uiBitmap = null;
        uiCanvas = null;
        uiPaint = null;

        if (hudBitmap != null && !hudBitmap.isRecycled()) {
            hudBitmap.recycle();
        }
        hudBitmap = null;
        hudCanvas = null;
        hudPaint = null;
    }

    private int calculateHoveredButton(float[] forward) {
        if (forward == null || forward.length < 3 || forward[2] >= -0.05f) {
            return -1;
        }

        float t = PANEL_Z / forward[2];
        if (t <= 0.0f) {
            return -1;
        }

        float hitX = forward[0] * t;
        float hitY = forward[1] * t;

        if (Math.abs(hitX) > PANEL_HALF_WIDTH
            || Math.abs(hitY) > PANEL_HALF_HEIGHT) {
            return -1;
        }

        float px = ((hitX + PANEL_HALF_WIDTH) / (PANEL_HALF_WIDTH * 2.0f))
            * TEXTURE_WIDTH;
        float py = ((PANEL_HALF_HEIGHT - hitY) / (PANEL_HALF_HEIGHT * 2.0f))
            * TEXTURE_HEIGHT;

        int[][] buttons = activeButtons();
        for (int index = 0; index < buttons.length; index++) {
            int[] rect = buttons[index];
            if (px >= rect[0] && px <= rect[2]
                && py >= rect[1] && py <= rect[3]) {
                return index;
            }
        }
        return -1;
    }

    private void activateHoveredButton() {
        if (hoveredButton < 0) {
            return;
        }

        if (mode == MODE_HOME) {
            switch (hoveredButton) {
                case 0:
                    mode = MODE_SETUP;
                    setupStep = 0;
                    preferences.setVrSetupStep(0);
                    break;
                case 1:
                    mode = MODE_SETUP;
                    setupStep = 2;
                    preferences.setVrSetupStep(2);
                    break;
                case 2:
                    if (!preferences.hasSelectedVideo()) {
                        host.onExitToPhoneRequested();
                        return;
                    }

                    videoRenderer.setProjection(
                        preferences.getVideoProjection()
                    );
                    videoSeekRequestedMillis.set(0);
                    mode = MODE_VIDEO;
                    hoveredButton = -1;
                    host.onVideoPlaybackRequested();
                    return;
                case 3:
                    if (!dosRuntimeAvailable
                        || dosModuleIds.length == 0) {
                        host.onExitToPhoneRequested();
                        return;
                    }
                    dosLibraryPage = 0;
                    mode = MODE_DOS_LIBRARY;
                    break;
                case 4:
                    if (NativeModuleRuntime.isAvailable()
                        && !nativeModules.isEmpty()) {
                        NativeModuleRuntime.Descriptor module =
                            nativeModules.get(0);
                        if (host.onNativeModulePlaybackRequested(
                            module.id
                        )) {
                            nativeSurfaceReady = false;
                            mode = MODE_NATIVE;
                            hoveredButton = -1;
                            return;
                        }
                    }
                    host.onUiActionRejected();
                    break;
                case 5:
                    host.onExitToPhoneRequested();
                    return;
                default:
                    break;
            }
        } else if (mode == MODE_DOS_LIBRARY) {
            handleDosLibrarySelection(hoveredButton);
        } else if (mode == MODE_DOS_OVERLAY) {
            handleDosOverlaySelection(hoveredButton);
        } else if (mode == MODE_DOS_BINDINGS) {
            handleDosBindingSelection(hoveredButton);
        } else {
            handleSetupSelection(hoveredButton);
        }

        hoveredButton = -1;
        textureDirty = true;
    }

    private void handleDosLibrarySelection(int button) {
        String[] ids = dosModuleIds;
        int pageCount =
            Math.max(1, (ids.length + 2) / 3);

        if (button >= 0 && button <= 2) {
            int moduleIndex =
                dosLibraryPage * 3 + button;
            if (moduleIndex < ids.length
                && host.onDosPlaybackRequested(
                    ids[moduleIndex]
                )) {
                activeDosModuleName =
                    moduleIndex < dosModuleNames.length
                        ? dosModuleNames[moduleIndex]
                        : "DOS session";
                dosExitRequested.set(false);
                mode = MODE_DOS;
                hoveredButton = -1;
                return;
            }
            host.onUiActionRejected();
        } else if (button == 3) {
            if (pageCount > 1) {
                dosLibraryPage =
                    (dosLibraryPage + 1)
                        % pageCount;
            } else {
                host.onUiActionRejected();
            }
        } else if (button == 4) {
            mode = MODE_HOME;
        }

        hoveredButton = -1;
        textureDirty = true;
    }

    private void handleDosOverlaySelection(int button) {
        switch (button) {
            case 0:
                host.onDosOverlayResumeRequested();
                mode = MODE_DOS;
                break;
            case 1:
                requestRecenter();
                break;
            case 2:
                mode = MODE_DOS_BINDINGS;
                break;
            case 3:
                host.onVolumeAdjustRequested(-1);
                break;
            case 4:
                host.onVolumeAdjustRequested(1);
                break;
            case 5:
                activeDosModuleName = "";
                host.onDosStopRequested();
                mode = MODE_HOME;
                break;
            case 6:
                activeDosModuleName = "";
                host.onDosStopRequested();
                mode = MODE_HOME;
                host.onExitToPhoneRequested();
                break;
            default:
                return;
        }

        hoveredButton = -1;
        textureDirty = true;
    }

    private void handleDosBindingSelection(int button) {
        switch (button) {
            case 0:
                host.onDosBindingProfileCycleRequested(-1);
                break;
            case 1:
                host.onDosBindingProfileCycleRequested(1);
                break;
            case 2:
                host.onDosBindingSensitivityAdjustRequested(-1);
                break;
            case 3:
                host.onDosBindingSensitivityAdjustRequested(1);
                break;
            case 4:
                host.onDosBindingDeadzoneAdjustRequested(-1);
                break;
            case 5:
                host.onDosBindingDeadzoneAdjustRequested(1);
                break;
            case 6:
                host.onDosBindingResetRequested();
                break;
            case 7:
                mode = MODE_DOS_OVERLAY;
                break;
            default:
                return;
        }

        hoveredButton = -1;
        textureDirty = true;
    }

    private void handleSetupSelection(int button) {
        switch (setupStep) {
            case 0:
                if (button == 0) {
                    requestRecenter();
                    advanceSetup();
                } else if (button == 1) {
                    advanceSetup();
                }
                break;

            case 1:
                if (button == 0 || button == 1) {
                    advanceSetup();
                }
                break;

            case 2:
                if (button == 0) {
                    userIpdMeters = clamp(
                        userIpdMeters - 0.001f,
                        0.050f,
                        0.080f
                    );
                    preferences.setUserIpdMeters(userIpdMeters);
                } else if (button == 1) {
                    userIpdMeters = clamp(
                        userIpdMeters + 0.001f,
                        0.050f,
                        0.080f
                    );
                    preferences.setUserIpdMeters(userIpdMeters);
                } else if (button == 2) {
                    advanceSetup();
                }
                break;

            case 3:
                if (button == 0) {
                    uiScale = clamp(
                        uiScale - 0.05f,
                        0.75f,
                        1.50f
                    );
                    preferences.setUiScale(uiScale);
                } else if (button == 1) {
                    uiScale = clamp(
                        uiScale + 0.05f,
                        0.75f,
                        1.50f
                    );
                    preferences.setUiScale(uiScale);
                } else if (button == 2) {
                    advanceSetup();
                }
                break;

            case 4:
                if (button == 0) {
                    preferences.setBatteryHudEnabled(
                        !preferences.isBatteryHudEnabled()
                    );
                } else if (button == 1) {
                    preferences.setLookUpRevealEnabled(
                        !preferences.isLookUpRevealEnabled()
                    );
                } else if (button == 2) {
                    advanceSetup();
                }
                break;

            case 5:
                if (button == 0) {
                    preferences.markVrSetupCurrent();
                    mode = MODE_HOME;
                    setupStep = 0;
                    host.onSetupCompleted();
                } else if (button == 1) {
                    mode = MODE_HOME;
                }
                break;

            default:
                mode = MODE_HOME;
                setupStep = 0;
                break;
        }
    }

    private void advanceSetup() {
        setupStep = Math.min(5, setupStep + 1);
        if (setupStep == 1) {
            lastInputAction = "Waiting for input";
            lastInputSource = "Try your controller controls below";
        }
        preferences.setVrSetupStep(setupStep);
        hoveredButton = -1;
        textureDirty = true;
    }

    private void handleBack() {
        if (mode == MODE_HOME) {
            host.onExitToPhoneRequested();
            return;
        }

        if (mode == MODE_DOS_LIBRARY) {
            mode = MODE_HOME;
            hoveredButton = -1;
            textureDirty = true;
            return;
        }

        if (mode == MODE_DOS_BINDINGS) {
            mode = MODE_DOS_OVERLAY;
            hoveredButton = -1;
            textureDirty = true;
            return;
        }

        if (mode == MODE_DOS_OVERLAY) {
            host.onDosOverlayResumeRequested();
            mode = MODE_DOS;
            hoveredButton = -1;
            textureDirty = true;
            return;
        }

        if (setupStep > 0) {
            setupStep--;
            preferences.setVrSetupStep(setupStep);
        } else {
            mode = MODE_HOME;
        }

        hoveredButton = -1;
        textureDirty = true;
    }

    private int[][] activeButtons() {
        if (mode == MODE_HOME) {
            return HOME_BUTTONS;
        }

        if (mode == MODE_DOS_LIBRARY) {
            return DOS_LIBRARY_BUTTONS;
        }

        if (mode == MODE_DOS_OVERLAY) {
            return DOS_OVERLAY_BUTTONS;
        }

        if (mode == MODE_DOS_BINDINGS) {
            return DOS_BINDING_BUTTONS;
        }

        if (setupStep == 2 || setupStep == 3 || setupStep == 4) {
            return THREE_BUTTONS;
        }

        return TWO_BUTTONS;
    }

    private void rebuildTexture() {
        ensureUiBitmap();

        Canvas canvas = uiCanvas;
        Paint paint = uiPaint;
        paint.reset();
        paint.setAntiAlias(true);

        if (mode == MODE_DOS_OVERLAY
            || mode == MODE_DOS_BINDINGS) {
            canvas.drawColor(
                Color.TRANSPARENT,
                PorterDuff.Mode.CLEAR
            );
            paint.setColor(Color.argb(224, 16, 20, 26));
        } else {
            canvas.drawColor(Color.rgb(9, 12, 16));
            paint.setColor(Color.rgb(24, 29, 36));
        }
        canvas.drawRoundRect(42, 42, 982, 726, 28, 28, paint);

        paint.setColor(Color.rgb(56, 214, 200));
        paint.setTextSize(46.0f * uiScale);
        paint.setFakeBoldText(true);
        canvas.drawText("REVERIE VR", 90, 120, paint);

        paint.setFakeBoldText(false);
        paint.setTextSize(24.0f * uiScale);
        paint.setColor(Color.rgb(180, 190, 202));

        if (mode == MODE_HOME) {
            drawHome(canvas, paint);
        } else if (mode == MODE_DOS_LIBRARY) {
            drawDosLibrary(canvas, paint);
        } else if (mode == MODE_DOS_OVERLAY) {
            drawDosOverlay(canvas, paint);
        } else if (mode == MODE_DOS_BINDINGS) {
            drawDosBindingEditor(canvas, paint);
        } else {
            drawSetup(canvas, paint);
        }

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        if (textureStorageInitialized) {
            GLUtils.texSubImage2D(
                GLES20.GL_TEXTURE_2D,
                0,
                0,
                0,
                uiBitmap
            );
        } else {
            GLUtils.texImage2D(
                GLES20.GL_TEXTURE_2D,
                0,
                uiBitmap,
                0
            );
            textureStorageInitialized = true;
        }
        textureDirty = false;
    }

    private void ensureUiBitmap() {
        if (uiBitmap != null && !uiBitmap.isRecycled()) {
            return;
        }

        uiBitmap = Bitmap.createBitmap(
            TEXTURE_WIDTH,
            TEXTURE_HEIGHT,
            Bitmap.Config.ARGB_8888
        );
        uiCanvas = new Canvas(uiBitmap);
        uiPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    }

    private void drawHome(Canvas canvas, Paint paint) {
        paint.setColor(Color.WHITE);
        paint.setTextSize(31.0f * uiScale);
        canvas.drawText("HOME", 90, 175, paint);

        paint.setColor(Color.rgb(150, 162, 177));
        paint.setTextSize(20.0f * uiScale);
        canvas.drawText(
            controllerConnected ? "Controller ready" : controllerMessage,
            90,
            215,
            paint
        );

        String dosLabel;
        if (!dosRuntimeAvailable) {
            dosLabel = "DOS RUNTIME NOT BUILT";
        } else if (dosModuleIds.length == 0) {
            dosLabel = "IMPORT DOS MODULE ON PHONE";
        } else {
            dosLabel =
                "DOS LIBRARY  ("
                    + dosModuleIds.length
                    + ")";
        }

        String nativeLabel;
        if (!NativeModuleRuntime.isAvailable()) {
            nativeLabel =
                "NATIVE MODULE HOST UNAVAILABLE";
        } else if (nativeModules.isEmpty()) {
            nativeLabel =
                "NO NATIVE MODULES BUILT";
        } else {
            nativeLabel =
                "NATIVE: "
                    + shorten(
                        nativeModules
                            .get(0)
                            .displayName,
                        30
                    );
        }

        String[] labels = new String[] {
            "RUN VR SETUP",
            "OPTICAL / DISPLAY CALIBRATION",
            preferences.hasSelectedVideo()
                ? "PLAY SELECTED VIDEO"
                : "SELECT VIDEO ON PHONE",
            dosLabel,
            nativeLabel,
            "EXIT TO PHONE"
        };
        drawButtons(canvas, paint, labels, activeButtons());
    }

    private void drawDosLibrary(
        Canvas canvas,
        Paint paint
    ) {
        String[] names = dosModuleNames;
        int count = names.length;
        int pageCount =
            Math.max(1, (count + 2) / 3);
        int page =
            Math.min(dosLibraryPage, pageCount - 1);
        int start = page * 3;

        paint.setColor(Color.WHITE);
        paint.setTextSize(31.0f * uiScale);
        canvas.drawText(
            "DOS LIBRARY",
            90,
            175,
            paint
        );

        paint.setColor(Color.rgb(150, 162, 177));
        paint.setTextSize(20.0f * uiScale);
        canvas.drawText(
            String.format(
                Locale.US,
                "%d modules  •  page %d / %d",
                count,
                page + 1,
                pageCount
            ),
            90,
            215,
            paint
        );

        String[] labels = new String[5];
        for (int slot = 0; slot < 3; slot++) {
            int index = start + slot;
            labels[slot] =
                index < count
                    ? shorten(
                        names[index],
                        42
                    )
                    : "—";
        }

        labels[3] =
            pageCount > 1
                ? "NEXT PAGE"
                : "ONLY PAGE";
        labels[4] = "BACK TO HOME";

        drawButtons(
            canvas,
            paint,
            labels,
            activeButtons()
        );
    }

    private void drawDosOverlay(
        Canvas canvas,
        Paint paint
    ) {
        paint.setColor(Color.WHITE);
        paint.setTextSize(31.0f * uiScale);
        canvas.drawText(
            "DOS QUICK MENU",
            90,
            175,
            paint
        );

        paint.setColor(Color.rgb(184, 194, 207));
        paint.setTextSize(20.0f * uiScale);
        canvas.drawText(
            "Paused: "
                + shorten(activeDosModuleName, 42),
            90,
            210,
            paint
        );
        canvas.drawText(
            "Bindings: "
                + shorten(
                    host.getActiveBindingProfileName(),
                    46
                ),
            90,
            238,
            paint
        );

        String[] labels = new String[] {
            "RESUME",
            "RECENTER",
            "BINDINGS",
            "VOLUME -",
            "VOLUME +",
            "HOME",
            "EXIT VR"
        };
        drawButtons(
            canvas,
            paint,
            labels,
            DOS_OVERLAY_BUTTONS
        );
    }

    private void drawDosBindingEditor(
        Canvas canvas,
        Paint paint
    ) {
        paint.setColor(Color.WHITE);
        paint.setTextSize(31.0f * uiScale);
        canvas.drawText(
            "DOS BINDINGS",
            90,
            175,
            paint
        );

        paint.setColor(Color.rgb(184, 194, 207));
        paint.setTextSize(20.0f * uiScale);
        canvas.drawText(
            "Profile: "
                + shorten(
                    host.getActiveBindingProfileName(),
                    48
                ),
            90,
            210,
            paint
        );
        canvas.drawText(
            shorten(
                host.getActiveBindingTuningSummary(),
                60
            ),
            90,
            240,
            paint
        );

        String[] labels = new String[] {
            "PROFILE PREV",
            "PROFILE NEXT",
            "SENSITIVITY -10%",
            "SENSITIVITY +10%",
            "DEADZONE -0.02",
            "DEADZONE +0.02",
            "RESET TUNING",
            "BACK"
        };

        drawButtons(
            canvas,
            paint,
            labels,
            DOS_BINDING_BUTTONS
        );
    }

    private void drawSetup(Canvas canvas, Paint paint) {
        paint.setColor(Color.WHITE);
        paint.setTextSize(31.0f * uiScale);
        canvas.drawText(
            String.format(
                Locale.US,
                "SETUP  %d / 6",
                setupStep + 1
            ),
            90,
            175,
            paint
        );

        paint.setColor(Color.rgb(184, 194, 207));
        paint.setTextSize(23.0f * uiScale);

        String[] labels;
        switch (setupStep) {
            case 0:
                canvas.drawText(
                    "Face forward in a comfortable seated position.",
                    90,
                    255,
                    paint
                );
                canvas.drawText(
                    "Set this as your neutral direction.",
                    90,
                    300,
                    paint
                );
                labels =
                    new String[] {"RECENTER + NEXT", "SKIP"};
                break;

            case 1:
                canvas.drawText(
                    "Controller familiarization",
                    90,
                    235,
                    paint
                );
                paint.setTextSize(19.0f * uiScale);
                canvas.drawText(
                    "Try Select, Back, Home/Recenter, directional input and volume.",
                    90,
                    275,
                    paint
                );
                canvas.drawText(
                    "Back is captured on this page so you can test it safely.",
                    90,
                    308,
                    paint
                );

                paint.setColor(Color.WHITE);
                paint.setTextSize(32.0f * uiScale);
                canvas.drawText(
                    "Last action: " + shorten(lastInputAction, 28),
                    90,
                    365,
                    paint
                );

                paint.setColor(Color.rgb(184, 194, 207));
                paint.setTextSize(20.0f * uiScale);
                canvas.drawText(
                    "Source: " + shorten(lastInputSource, 52),
                    90,
                    405,
                    paint
                );
                labels = new String[] {"CONTINUE", "SKIP"};
                break;

            case 2:
                canvas.drawText(
                    "Virtual eye spacing",
                    90,
                    245,
                    paint
                );
                paint.setColor(Color.WHITE);
                paint.setTextSize(58.0f * uiScale);
                canvas.drawText(
                    String.format(
                        Locale.US,
                        "%.0f mm",
                        userIpdMeters * 1000.0f
                    ),
                    90,
                    340,
                    paint
                );
                paint.setColor(Color.rgb(184, 194, 207));
                paint.setTextSize(20.0f * uiScale);
                canvas.drawText(
                    String.format(
                        Locale.US,
                        "Viewer lens spacing profile: %.0f mm",
                        viewerInterLensMeters * 1000.0f
                    ),
                    90,
                    390,
                    paint
                );
                canvas.drawText(
                    "Adjust for easiest binocular fusion; reset later in Settings.",
                    90,
                    430,
                    paint
                );
                labels =
                    new String[] {"NARROWER", "WIDER", "NEXT"};
                break;

            case 3:
                canvas.drawText(
                    "Readability",
                    90,
                    245,
                    paint
                );
                paint.setColor(Color.WHITE);
                paint.setTextSize(48.0f * uiScale);
                canvas.drawText(
                    String.format(
                        Locale.US,
                        "UI scale  %.0f%%",
                        uiScale * 100.0f
                    ),
                    90,
                    335,
                    paint
                );
                paint.setColor(Color.rgb(184, 194, 207));
                paint.setTextSize(22.0f * uiScale);
                canvas.drawText(
                    "This text should be easy to read without leaning.",
                    90,
                    405,
                    paint
                );
                labels =
                    new String[] {"SMALLER", "LARGER", "NEXT"};
                break;

            case 4:
                canvas.drawText(
                    "Status HUD",
                    90,
                    245,
                    paint
                );
                canvas.drawText(
                    "Battery HUD: "
                        + onOff(preferences.isBatteryHudEnabled()),
                    90,
                    315,
                    paint
                );
                canvas.drawText(
                    "Look-up reveal: "
                        + onOff(preferences.isLookUpRevealEnabled()),
                    90,
                    365,
                    paint
                );
                labels =
                    new String[] {
                        "BATTERY HUD",
                        "LOOK-UP MODE",
                        "NEXT"
                    };
                break;

            case 5:
            default:
                canvas.drawText(
                    "Core setup is ready.",
                    90,
                    255,
                    paint
                );
                canvas.drawText(
                    "More calibration tools remain available from Settings.",
                    90,
                    305,
                    paint
                );
                labels =
                    new String[] {
                        "SAVE + HOME",
                        "HOME WITHOUT SAVING"
                    };
                break;
        }

        drawButtons(
            canvas,
            paint,
            labels,
            activeButtons()
        );
    }

    private void drawPowerHudOverlay() {
        if (!preferences.isBatteryHudEnabled()
            || program == 0
            || hudTexture == 0) {
            return;
        }

        if (hudTextureDirty) {
            rebuildPowerHudTexture();
        }

        updateHudVertices();

        GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        GLES20.glUseProgram(program);

        hudVertexBuffer.position(0);
        GLES20.glVertexAttribPointer(
            positionHandle,
            3,
            GLES20.GL_FLOAT,
            false,
            0,
            hudVertexBuffer
        );
        GLES20.glEnableVertexAttribArray(positionHandle);

        hudUvBuffer.position(0);
        GLES20.glVertexAttribPointer(
            uvHandle,
            2,
            GLES20.GL_FLOAT,
            false,
            0,
            hudUvBuffer
        );
        GLES20.glEnableVertexAttribArray(uvHandle);

        GLES20.glUniformMatrix4fv(
            matrixHandle,
            1,
            false,
            hudIdentity,
            0
        );

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(
            GLES20.GL_TEXTURE_2D,
            hudTexture
        );
        GLES20.glUniform1i(textureHandle, 0);

        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(
            GLES20.GL_SRC_ALPHA,
            GLES20.GL_ONE_MINUS_SRC_ALPHA
        );
        GLES20.glDrawArrays(
            GLES20.GL_TRIANGLE_STRIP,
            0,
            4
        );
        GLES20.glDisable(GLES20.GL_BLEND);

        GLES20.glDisableVertexAttribArray(positionHandle);
        GLES20.glDisableVertexAttribArray(uvHandle);
    }

    private void updateHudVertices() {
        float left = 0.30f;
        float right = 0.96f;
        float top = hudDroppedDown ? 0.60f : 0.96f;
        float bottom = hudDroppedDown ? 0.32f : 0.70f;

        hudVertices[0] = left;
        hudVertices[1] = bottom;
        hudVertices[2] = 0.0f;
        hudVertices[3] = right;
        hudVertices[4] = bottom;
        hudVertices[5] = 0.0f;
        hudVertices[6] = left;
        hudVertices[7] = top;
        hudVertices[8] = 0.0f;
        hudVertices[9] = right;
        hudVertices[10] = top;
        hudVertices[11] = 0.0f;

        hudVertexBuffer.position(0);
        hudVertexBuffer.put(hudVertices);
        hudVertexBuffer.position(0);
    }

    private void ensureHudBitmap() {
        if (hudBitmap != null && !hudBitmap.isRecycled()) {
            return;
        }

        hudBitmap = Bitmap.createBitmap(
            HUD_TEXTURE_WIDTH,
            HUD_TEXTURE_HEIGHT,
            Bitmap.Config.ARGB_8888
        );
        hudCanvas = new Canvas(hudBitmap);
        hudPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    }

    private void rebuildPowerHudTexture() {
        ensureHudBitmap();

        Canvas canvas = hudCanvas;
        Paint paint = hudPaint;
        paint.reset();
        paint.setAntiAlias(true);

        canvas.drawColor(
            Color.TRANSPARENT,
            PorterDuff.Mode.CLEAR
        );

        paint.setColor(Color.argb(218, 16, 20, 26));
        canvas.drawRoundRect(
            0.0f,
            0.0f,
            HUD_TEXTURE_WIDTH,
            HUD_TEXTURE_HEIGHT,
            18.0f,
            18.0f,
            paint
        );

        drawBatteryRow(
            canvas,
            paint,
            "PHONE",
            phoneBattery.get(),
            18.0f,
            20.0f
        );
        drawBatteryRow(
            canvas,
            paint,
            "CTRL",
            controllerBattery.get(),
            18.0f,
            74.0f
        );

        GLES20.glBindTexture(
            GLES20.GL_TEXTURE_2D,
            hudTexture
        );

        if (hudTextureStorageInitialized) {
            GLUtils.texSubImage2D(
                GLES20.GL_TEXTURE_2D,
                0,
                0,
                0,
                hudBitmap
            );
        } else {
            GLUtils.texImage2D(
                GLES20.GL_TEXTURE_2D,
                0,
                hudBitmap,
                0
            );
            hudTextureStorageInitialized = true;
        }

        hudTextureDirty = false;
    }

    private void drawBatteryRow(
        Canvas canvas,
        Paint paint,
        String label,
        int percentage,
        float x,
        float y
    ) {
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.rgb(220, 226, 234));
        paint.setTextSize(21.0f);
        paint.setFakeBoldText(true);
        canvas.drawText(label, x, y + 24.0f, paint);
        paint.setFakeBoldText(false);

        float barLeft = 104.0f;
        float barTop = y + 8.0f;
        float barRight = 404.0f;
        float barBottom = y + 34.0f;

        paint.setColor(Color.rgb(49, 58, 69));
        canvas.drawRoundRect(
            barLeft,
            barTop,
            barRight,
            barBottom,
            8.0f,
            8.0f,
            paint
        );

        if (percentage >= 0 && percentage <= 100) {
            float fillRight =
                barLeft
                    + ((barRight - barLeft)
                        * (percentage / 100.0f));
            if (fillRight > barLeft) {
                paint.setColor(Color.rgb(56, 214, 200));
                canvas.drawRoundRect(
                    barLeft,
                    barTop,
                    fillRight,
                    barBottom,
                    8.0f,
                    8.0f,
                    paint
                );
            }
        }

        if (cachedShowPercentages) {
            paint.setColor(Color.WHITE);
            paint.setTextSize(20.0f);
            String value =
                percentage >= 0 && percentage <= 100
                    ? percentage + "%"
                    : "--";
            canvas.drawText(
                value,
                426.0f,
                y + 28.0f,
                paint
            );
        }
    }

    private void drawReticle(Canvas canvas, Paint paint) {
        paint.setColor(Color.rgb(56, 214, 200));
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(3.0f);
        canvas.drawCircle(TEXTURE_WIDTH * 0.5f, TEXTURE_HEIGHT * 0.5f, 10.0f, paint);
        paint.setStyle(Paint.Style.FILL);
    }

    private void drawButtons(
        Canvas canvas,
        Paint paint,
        String[] labels,
        int[][] rectangles
    ) {
        for (int index = 0; index < labels.length && index < rectangles.length; index++) {
            int[] rect = rectangles[index];

            paint.setColor(
                index == hoveredButton
                    ? Color.rgb(42, 126, 121)
                    : Color.rgb(38, 46, 57)
            );
            canvas.drawRoundRect(
                rect[0],
                rect[1],
                rect[2],
                rect[3],
                18,
                18,
                paint
            );

            paint.setColor(Color.WHITE);
            paint.setTextSize(24.0f * uiScale);
            paint.setFakeBoldText(index == hoveredButton);
            float textWidth = paint.measureText(labels[index]);
            float x = rect[0] + ((rect[2] - rect[0]) - textWidth) * 0.5f;
            float y = rect[1] + ((rect[3] - rect[1]) * 0.5f)
                - ((paint.ascent() + paint.descent()) * 0.5f);
            canvas.drawText(labels[index], x, y, paint);
            paint.setFakeBoldText(false);
        }
    }

    private static String shorten(String value, int maxLength) {
        if (value == null) {
            return "";
        }

        String trimmed = value.trim();
        if (trimmed.length() <= maxLength) {
            return trimmed;
        }
        return trimmed.substring(0, Math.max(0, maxLength - 1)) + "…";
    }

    private static float wrapAngle(float value) {
        float result = value;
        while (result > Math.PI) {
            result -= (float) (Math.PI * 2.0);
        }
        while (result < -Math.PI) {
            result += (float) (Math.PI * 2.0);
        }
        return result;
    }

    private static void rotateYaw(
        float[] vector,
        float radians,
        float[] destination
    ) {
        float cos = (float) Math.cos(radians);
        float sin = (float) Math.sin(radians);

        destination[0] = cos * vector[0] + sin * vector[2];
        destination[1] = vector[1];
        destination[2] = -sin * vector[0] + cos * vector[2];
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
        int vertexShader = compileShader(GLES20.GL_VERTEX_SHADER, vertexSource);
        int fragmentShader = compileShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);

        int result = GLES20.glCreateProgram();
        GLES20.glAttachShader(result, vertexShader);
        GLES20.glAttachShader(result, fragmentShader);
        GLES20.glLinkProgram(result);

        int[] link = new int[1];
        GLES20.glGetProgramiv(result, GLES20.GL_LINK_STATUS, link, 0);
        if (link[0] == 0) {
            String error = GLES20.glGetProgramInfoLog(result);
            GLES20.glDeleteProgram(result);
            throw new IllegalStateException("VR shader link failed: " + error);
        }

        GLES20.glDeleteShader(vertexShader);
        GLES20.glDeleteShader(fragmentShader);
        return result;
    }

    private static int compileShader(int type, String source) {
        int shader = GLES20.glCreateShader(type);
        GLES20.glShaderSource(shader, source);
        GLES20.glCompileShader(shader);

        int[] compiled = new int[1];
        GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
        if (compiled[0] == 0) {
            String error = GLES20.glGetShaderInfoLog(shader);
            GLES20.glDeleteShader(shader);
            throw new IllegalStateException("VR shader compile failed: " + error);
        }
        return shader;
    }

    private static String onOff(boolean value) {
        return value ? "ON" : "OFF";
    }

    private static float clamp(float value, float minimum, float maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    private static int clampInt(int value, int minimum, int maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    private static final String VERTEX_SHADER =
        "uniform mat4 u_Mvp;\n"
            + "attribute vec4 a_Position;\n"
            + "attribute vec2 a_TexCoord;\n"
            + "varying vec2 v_TexCoord;\n"
            + "void main() {\n"
            + "  gl_Position = u_Mvp * a_Position;\n"
            + "  v_TexCoord = a_TexCoord;\n"
            + "}\n";

    private static final String FRAGMENT_SHADER =
        "precision mediump float;\n"
            + "uniform sampler2D u_Texture;\n"
            + "varying vec2 v_TexCoord;\n"
            + "void main() {\n"
            + "  gl_FragColor = texture2D(u_Texture, v_TexCoord);\n"
            + "}\n";
}
