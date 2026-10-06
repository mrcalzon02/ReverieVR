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
        void onVrFirstFrameRendered();
        void onVrRendererFailure(
            String phase,
            Throwable throwable
        );
        void onExitToPhoneRequested();
        void onSetupCompleted();
        void onControllerRecenterRequested();
        void onUiFocusChanged();
        void onUiActionRejected();
        PerformanceEnvironmentSnapshot
            getPerformanceEnvironmentSnapshot();
        void onVideoSurfaceTextureReady(SurfaceTexture surfaceTexture);
        void onVideoPlaybackRequested();
        void onMediaSelectionRequested();
        void onDosImportRequested();
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
    private static final int MODE_MEDIA_LIBRARY = 8;
    private static final int MODE_ENVIRONMENT = 9;

    private static final int HOME_LEFT_PIXEL_LEFT = 24;
    private static final int HOME_LEFT_PIXEL_RIGHT = 248;
    private static final int HOME_CENTER_PIXEL_LEFT = 266;
    private static final int HOME_CENTER_PIXEL_RIGHT = 744;
    private static final int HOME_RIGHT_PIXEL_LEFT = 762;
    private static final int HOME_RIGHT_PIXEL_RIGHT = 1000;
    private static final int HOME_PIXEL_TOP = 60;
    private static final int HOME_PIXEL_BOTTOM = 710;

    private static final float HOME_LEFT_WORLD_LEFT = -2.35f;
    private static final float HOME_LEFT_WORLD_RIGHT = -0.95f;
    private static final float HOME_CENTER_WORLD_LEFT = -1.15f;
    private static final float HOME_CENTER_WORLD_RIGHT = 1.15f;
    private static final float HOME_RIGHT_WORLD_LEFT = 0.95f;
    private static final float HOME_RIGHT_WORLD_RIGHT = 2.35f;
    private static final float HOME_WORLD_BOTTOM = -1.20f;
    private static final float HOME_WORLD_TOP = 1.20f;
    private static final float HOME_LEFT_WORLD_Z = -3.25f;
    private static final float HOME_CENTER_WORLD_Z = -3.0f;
    private static final float HOME_RIGHT_WORLD_Z = -3.25f;

    private static final int[][] HOME_BUTTONS = new int[][] {
        {42, 180, 230, 238},
        {42, 248, 230, 306},
        {42, 316, 230, 374},
        {42, 384, 230, 442},
        {42, 452, 230, 510},
        {42, 520, 230, 578},
        {780, 190, 982, 248},
        {780, 258, 982, 316},
        {780, 326, 982, 384},
        {780, 394, 982, 452},
        {780, 462, 982, 520}
    };

    private static final int[][] DOS_LIBRARY_BUTTONS = new int[][] {
        {140, 235, 884, 300},
        {140, 315, 884, 380},
        {140, 395, 884, 460},
        {140, 475, 884, 540},
        {140, 555, 884, 620}
    };

    private static final int[][] MEDIA_BUTTONS = new int[][] {
        {140, 235, 884, 300},
        {140, 315, 884, 380},
        {140, 395, 884, 460},
        {140, 475, 884, 540},
        {140, 555, 884, 620}
    };

    private static final int[][] ENVIRONMENT_BUTTONS = new int[][] {
        {140, 265, 884, 330},
        {140, 345, 884, 410},
        {140, 425, 884, 490},
        {140, 535, 884, 600}
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
    private final HomeEnvironmentRenderer homeEnvironmentRenderer;
    private final VrPointerRenderer pointerRenderer;
    private final DosSession dosSession;
    private final DosSurfaceRenderer dosRenderer;
    private final NativeModuleRuntime nativeModuleRuntime;
    private final List<NativeModuleRuntime.Descriptor> nativeModules;
    private final FramePerformanceTracker performanceTracker =
        new FramePerformanceTracker();

    private final FloatBuffer vertexBuffer;
    private final FloatBuffer uvBuffer;
    private final FloatBuffer homeLeftVertexBuffer;
    private final FloatBuffer homeLeftUvBuffer;
    private final FloatBuffer homeCenterVertexBuffer;
    private final FloatBuffer homeCenterUvBuffer;
    private final FloatBuffer homeRightVertexBuffer;
    private final FloatBuffer homeRightUvBuffer;
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
    private final float[] controllerForward = new float[3];
    private final float[] adjustedControllerForward = new float[3];
    private final float[] activePointerOrigin = new float[3];
    private final float[] activePointerDirection = new float[3];

    private final AtomicBoolean firstFrameReported =
        new AtomicBoolean();
    private final AtomicBoolean selectRequested =
        new AtomicBoolean();
    private final AtomicBoolean backRequested =
        new AtomicBoolean();
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
    private volatile boolean rendererFailed;

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
    private volatile long controllerPoseReceivedAtNanos;
    private volatile float controllerOrientationX;
    private volatile float controllerOrientationY;
    private volatile float controllerOrientationZ;
    private volatile float controllerOrientationW;
    private volatile boolean controllerPoseValid;
    private volatile boolean controllerPointerActive;
    private volatile float controllerPointerDistance = 6.0f;
    private volatile boolean controllerPointerHit;
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
        homeEnvironmentRenderer =
            new HomeEnvironmentRenderer();
        pointerRenderer = new VrPointerRenderer();
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

        homeLeftVertexBuffer =
            allocate(
                panelVertices(
                    HOME_LEFT_WORLD_LEFT,
                    HOME_LEFT_WORLD_RIGHT,
                    HOME_WORLD_BOTTOM,
                    HOME_WORLD_TOP,
                    HOME_LEFT_WORLD_Z
                )
            );
        homeLeftUvBuffer =
            allocate(
                panelUvs(
                    HOME_LEFT_PIXEL_LEFT,
                    HOME_LEFT_PIXEL_RIGHT,
                    HOME_PIXEL_TOP,
                    HOME_PIXEL_BOTTOM
                )
            );
        homeCenterVertexBuffer =
            allocate(
                panelVertices(
                    HOME_CENTER_WORLD_LEFT,
                    HOME_CENTER_WORLD_RIGHT,
                    HOME_WORLD_BOTTOM,
                    HOME_WORLD_TOP,
                    HOME_CENTER_WORLD_Z
                )
            );
        homeCenterUvBuffer =
            allocate(
                panelUvs(
                    HOME_CENTER_PIXEL_LEFT,
                    HOME_CENTER_PIXEL_RIGHT,
                    HOME_PIXEL_TOP,
                    HOME_PIXEL_BOTTOM
                )
            );
        homeRightVertexBuffer =
            allocate(
                panelVertices(
                    HOME_RIGHT_WORLD_LEFT,
                    HOME_RIGHT_WORLD_RIGHT,
                    HOME_WORLD_BOTTOM,
                    HOME_WORLD_TOP,
                    HOME_RIGHT_WORLD_Z
                )
            );
        homeRightUvBuffer =
            allocate(
                panelUvs(
                    HOME_RIGHT_PIXEL_LEFT,
                    HOME_RIGHT_PIXEL_RIGHT,
                    HOME_PIXEL_TOP,
                    HOME_PIXEL_BOTTOM
                )
            );

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
        if (!connected) {
            controllerPoseValid = false;
            controllerPointerActive = false;
            pointerRenderer.hide();
        }
        textureDirty = true;
    }

    void setControllerPose(ControllerSnapshot snapshot) {
        if (snapshot == null) {
            return;
        }

        float x = snapshot.orientationX;
        float y = snapshot.orientationY;
        float z = snapshot.orientationZ;
        float w = snapshot.orientationW;

        float lengthSquared =
            x * x
                + y * y
                + z * z
                + w * w;
        if (!Float.isFinite(lengthSquared)
            || lengthSquared < 0.25f
            || lengthSquared > 2.25f) {
            controllerPoseValid = false;
            return;
        }

        controllerOrientationX = x;
        controllerOrientationY = y;
        controllerOrientationZ = z;
        controllerOrientationW = w;
        controllerPoseReceivedAtNanos =
            snapshot.receivedAtNanos > 0L
                ? snapshot.receivedAtNanos
                : System.nanoTime();
        controllerPoseValid = true;
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
        if (rendererFailed) {
            return;
        }

        try {
            onNewFrameInternal(headTransform);
        } catch (RuntimeException | LinkageError failure) {
            reportRendererFailure(
                "new-frame",
                failure
            );
        }
    }

    private void onNewFrameInternal(
        HeadTransform headTransform
    ) {
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
                mode = MODE_DOS_LIBRARY;
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
                mode = MODE_DOS_LIBRARY;
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
                mode = MODE_MEDIA_LIBRARY;
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

        rotateYaw(
            headForward,
            -yawOffsetRadians,
            adjustedHeadForward
        );

        boolean usingControllerPointer =
            updateActivePointer(frameNanos);
        UiRayHit hit =
            calculateUiRayHit(
                activePointerOrigin,
                activePointerDirection
            );
        int newHover = hit.buttonIndex;

        controllerPointerActive =
            usingControllerPointer;
        controllerPointerDistance =
            hit.distance > 0.0f
                ? hit.distance
                : 6.0f;
        controllerPointerHit =
            newHover >= 0;

        if (usingControllerPointer) {
            pointerRenderer.setPointer(
                true,
                activePointerOrigin[0],
                activePointerOrigin[1],
                activePointerOrigin[2],
                activePointerDirection[0],
                activePointerDirection[1],
                activePointerDirection[2],
                controllerPointerDistance,
                controllerPointerHit
            );
        } else {
            pointerRenderer.hide();
        }

        if (newHover != hoveredButton) {
            hoveredButton = newHover;
            textureDirty = true;
            if (newHover >= 0) {
                host.onUiFocusChanged();
            }
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
        if (rendererFailed) {
            return;
        }

        try {
            onDrawEyeInternal(eye);
        } catch (RuntimeException | LinkageError failure) {
            reportRendererFailure(
                "draw-eye",
                failure
            );
        }
    }

    private void onDrawEyeInternal(
        CardboardView.Eye eye
    ) {
        eye.applyHeadView(adjustedHeadView);

        float correctionHalf =
            (userIpdMeters - viewerInterLensMeters) * 0.5f;
        float eyeCorrection = eye.getEyeType() == CardboardView.Eye.LEFT
            ? correctionHalf
            : -correctionHalf;

        if (mode != MODE_NATIVE) {
            homeEnvironmentRenderer.drawEye(
                eye,
                eyeCorrection,
                preferences.getHomeEnvironment()
            );
        } else {
            GLES20.glEnable(GLES20.GL_DEPTH_TEST);
            GLES20.glClearColor(
                0.015f,
                0.02f,
                0.025f,
                1.0f
            );
            GLES20.glClear(
                GLES20.GL_COLOR_BUFFER_BIT
                    | GLES20.GL_DEPTH_BUFFER_BIT
            );
        }

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

    private void reportRendererFailure(
        String phase,
        Throwable throwable
    ) {
        if (rendererFailed) {
            return;
        }
        rendererFailed = true;

        ReverieLog.error(
            "VR_RENDERER",
            "Renderer failed during "
                + phase
                + ".",
            throwable
        );
        host.onVrRendererFailure(
            phase,
            throwable
        );
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
        Matrix.multiplyMM(
            modelViewProjection,
            0,
            eye.getPerspective(
                Z_NEAR,
                Z_FAR
            ),
            0,
            tempMatrix,
            0
        );

        GLES20.glUseProgram(program);
        GLES20.glUniformMatrix4fv(
            matrixHandle,
            1,
            false,
            modelViewProjection,
            0
        );
        GLES20.glActiveTexture(
            GLES20.GL_TEXTURE0
        );
        GLES20.glBindTexture(
            GLES20.GL_TEXTURE_2D,
            texture
        );
        GLES20.glUniform1i(
            textureHandle,
            0
        );
        GLES20.glEnable(
            GLES20.GL_BLEND
        );
        GLES20.glBlendFunc(
            GLES20.GL_SRC_ALPHA,
            GLES20.GL_ONE_MINUS_SRC_ALPHA
        );

        if (mode == MODE_HOME) {
            drawTexturedPanel(
                homeLeftVertexBuffer,
                homeLeftUvBuffer
            );
            drawTexturedPanel(
                homeCenterVertexBuffer,
                homeCenterUvBuffer
            );
            drawTexturedPanel(
                homeRightVertexBuffer,
                homeRightUvBuffer
            );
        } else {
            drawTexturedPanel(
                vertexBuffer,
                uvBuffer
            );
        }

        GLES20.glDisable(
            GLES20.GL_BLEND
        );
    }

    private void drawTexturedPanel(
        FloatBuffer vertices,
        FloatBuffer uvs
    ) {
        vertices.position(0);
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

        uvs.position(0);
        GLES20.glVertexAttribPointer(
            uvHandle,
            2,
            GLES20.GL_FLOAT,
            false,
            0,
            uvs
        );
        GLES20.glEnableVertexAttribArray(
            uvHandle
        );

        GLES20.glDrawArrays(
            GLES20.GL_TRIANGLE_STRIP,
            0,
            4
        );

        GLES20.glDisableVertexAttribArray(
            positionHandle
        );
        GLES20.glDisableVertexAttribArray(
            uvHandle
        );
    }

    @Override
    public void onFinishFrame(Viewport viewport) {
        if (!rendererFailed
            && firstFrameReported.compareAndSet(
                false,
                true
            )) {
            host.onVrFirstFrameRendered();
        }
    }

    @Override
    public void onSurfaceChanged(int width, int height) {
        GLES20.glViewport(0, 0, width, height);
    }

    @Override
    public void onSurfaceCreated(EGLConfig config) {
        if (rendererFailed) {
            return;
        }

        try {
            initializeSurface(config);
        } catch (RuntimeException | LinkageError failure) {
            reportRendererFailure(
                "surface-create",
                failure
            );
        }
    }

    private void initializeSurface(
        EGLConfig config
    ) {
        homeEnvironmentRenderer.onSurfaceCreated();
        pointerRenderer.onSurfaceCreated();
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
        pointerRenderer.shutdown();
        homeEnvironmentRenderer.shutdown();

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

    private boolean updateActivePointer(
        long frameNanos
    ) {
        VrPointerMode pointerMode =
            preferences.getVrPointerMode();

        long poseAgeNanos =
            frameNanos
                - controllerPoseReceivedAtNanos;
        boolean freshControllerPose =
            controllerConnected
                && controllerPoseValid
                && controllerPoseReceivedAtNanos > 0L
                && poseAgeNanos >= 0L
                && poseAgeNanos <= 750000000L;

        boolean useController =
            freshControllerPose
                && (
                    pointerMode
                        == VrPointerMode.CONTROLLER
                    || pointerMode
                        == VrPointerMode.AUTO
                );

        if (useController) {
            quaternionForward(
                controllerOrientationX,
                controllerOrientationY,
                controllerOrientationZ,
                controllerOrientationW,
                controllerForward
            );
            rotateYaw(
                controllerForward,
                -yawOffsetRadians,
                adjustedControllerForward
            );
            normalizeDirection(
                adjustedControllerForward
            );

            activePointerOrigin[0] = 0.28f;
            activePointerOrigin[1] = -0.34f;
            activePointerOrigin[2] = -0.48f;

            System.arraycopy(
                adjustedControllerForward,
                0,
                activePointerDirection,
                0,
                3
            );
            return true;
        }

        if (pointerMode
            == VrPointerMode.CONTROLLER) {
            activePointerOrigin[0] = 0.0f;
            activePointerOrigin[1] = 0.0f;
            activePointerOrigin[2] = 0.0f;
            activePointerDirection[0] = 0.0f;
            activePointerDirection[1] = 0.0f;
            activePointerDirection[2] = 0.0f;
            return false;
        }

        activePointerOrigin[0] = 0.0f;
        activePointerOrigin[1] = 0.0f;
        activePointerOrigin[2] = 0.0f;
        System.arraycopy(
            adjustedHeadForward,
            0,
            activePointerDirection,
            0,
            3
        );
        normalizeDirection(
            activePointerDirection
        );
        return false;
    }

    private UiRayHit calculateUiRayHit(
        float[] origin,
        float[] direction
    ) {
        if (origin == null
            || origin.length < 3
            || direction == null
            || direction.length < 3
            || Math.abs(direction[2]) < 0.0001f) {
            return UiRayHit.miss();
        }

        if (mode == MODE_HOME) {
            UiRayHit best =
                hitPanel(
                    origin,
                    direction,
                    HOME_LEFT_WORLD_LEFT,
                    HOME_LEFT_WORLD_RIGHT,
                    HOME_WORLD_BOTTOM,
                    HOME_WORLD_TOP,
                    HOME_LEFT_WORLD_Z,
                    HOME_LEFT_PIXEL_LEFT,
                    HOME_LEFT_PIXEL_RIGHT,
                    HOME_PIXEL_TOP,
                    HOME_PIXEL_BOTTOM
                );

            UiRayHit center =
                hitPanel(
                    origin,
                    direction,
                    HOME_CENTER_WORLD_LEFT,
                    HOME_CENTER_WORLD_RIGHT,
                    HOME_WORLD_BOTTOM,
                    HOME_WORLD_TOP,
                    HOME_CENTER_WORLD_Z,
                    HOME_CENTER_PIXEL_LEFT,
                    HOME_CENTER_PIXEL_RIGHT,
                    HOME_PIXEL_TOP,
                    HOME_PIXEL_BOTTOM
                );
            best = nearer(best, center);

            UiRayHit right =
                hitPanel(
                    origin,
                    direction,
                    HOME_RIGHT_WORLD_LEFT,
                    HOME_RIGHT_WORLD_RIGHT,
                    HOME_WORLD_BOTTOM,
                    HOME_WORLD_TOP,
                    HOME_RIGHT_WORLD_Z,
                    HOME_RIGHT_PIXEL_LEFT,
                    HOME_RIGHT_PIXEL_RIGHT,
                    HOME_PIXEL_TOP,
                    HOME_PIXEL_BOTTOM
                );
            return nearer(best, right);
        }

        return hitPanel(
            origin,
            direction,
            -PANEL_HALF_WIDTH,
            PANEL_HALF_WIDTH,
            -PANEL_HALF_HEIGHT,
            PANEL_HALF_HEIGHT,
            PANEL_Z,
            0,
            TEXTURE_WIDTH,
            0,
            TEXTURE_HEIGHT
        );
    }

    private UiRayHit hitPanel(
        float[] origin,
        float[] direction,
        float worldLeft,
        float worldRight,
        float worldBottom,
        float worldTop,
        float worldZ,
        int pixelLeft,
        int pixelRight,
        int pixelTop,
        int pixelBottom
    ) {
        float t =
            (worldZ - origin[2])
                / direction[2];
        if (!Float.isFinite(t)
            || t <= 0.0f) {
            return UiRayHit.miss();
        }

        float hitX =
            origin[0]
                + direction[0] * t;
        float hitY =
            origin[1]
                + direction[1] * t;

        if (hitX < worldLeft
            || hitX > worldRight
            || hitY < worldBottom
            || hitY > worldTop) {
            return UiRayHit.miss();
        }

        float px =
            pixelLeft
                + (
                    (hitX - worldLeft)
                        / (worldRight - worldLeft)
                ) * (pixelRight - pixelLeft);
        float py =
            pixelTop
                + (
                    (worldTop - hitY)
                        / (worldTop - worldBottom)
                ) * (pixelBottom - pixelTop);

        return new UiRayHit(
            buttonAtPixel(px, py),
            t
        );
    }

    private int buttonAtPixel(
        float px,
        float py
    ) {
        int[][] buttons = activeButtons();
        for (int index = 0;
             index < buttons.length;
             index++) {
            int[] rect = buttons[index];
            if (px >= rect[0]
                && px <= rect[2]
                && py >= rect[1]
                && py <= rect[3]) {
                return index;
            }
        }
        return -1;
    }

    private static UiRayHit nearer(
        UiRayHit first,
        UiRayHit second
    ) {
        if (first.distance <= 0.0f) {
            return second;
        }
        if (second.distance <= 0.0f) {
            return first;
        }
        return first.distance <= second.distance
            ? first
            : second;
    }

    private static void quaternionForward(
        float x,
        float y,
        float z,
        float w,
        float[] destination
    ) {
        destination[0] =
            -2.0f * (x * z + w * y);
        destination[1] =
            -2.0f * (y * z - w * x);
        destination[2] =
            -(
                1.0f
                    - 2.0f * x * x
                    - 2.0f * y * y
            );
    }

    private static void normalizeDirection(
        float[] direction
    ) {
        float length =
            (float) Math.sqrt(
                direction[0] * direction[0]
                    + direction[1] * direction[1]
                    + direction[2] * direction[2]
            );
        if (!Float.isFinite(length)
            || length < 0.0001f) {
            direction[0] = 0.0f;
            direction[1] = 0.0f;
            direction[2] = 0.0f;
            return;
        }

        direction[0] /= length;
        direction[1] /= length;
        direction[2] /= length;
    }

    private static final class UiRayHit {
        final int buttonIndex;
        final float distance;

        UiRayHit(
            int buttonIndex,
            float distance
        ) {
            this.buttonIndex = buttonIndex;
            this.distance = distance;
        }

        static UiRayHit miss() {
            return new UiRayHit(-1, -1.0f);
        }
    }

    private void activateHoveredButton() {
        if (hoveredButton < 0) {
            return;
        }

        if (mode == MODE_HOME) {
            switch (hoveredButton) {
                case 0:
                    mode = MODE_MEDIA_LIBRARY;
                    break;
                case 1:
                    dosLibraryPage = 0;
                    mode = MODE_DOS_LIBRARY;
                    break;
                case 2:
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
                case 3:
                    mode = MODE_ENVIRONMENT;
                    break;
                case 4:
                    mode = MODE_SETUP;
                    setupStep = 0;
                    preferences.setVrSetupStep(0);
                    break;
                case 5:
                    host.onExitToPhoneRequested();
                    return;
                case 6:
                    preferences.setBatteryHudEnabled(
                        !preferences.isBatteryHudEnabled()
                    );
                    hudTextureDirty = true;
                    break;
                case 7:
                    preferences.setShowPercentagesEnabled(
                        !preferences.isShowPercentagesEnabled()
                    );
                    hudTextureDirty = true;
                    break;
                case 8:
                    preferences.setLookUpRevealEnabled(
                        !preferences.isLookUpRevealEnabled()
                    );
                    break;
                case 9:
                    preferences.setVrPointerMode(
                        preferences
                            .getVrPointerMode()
                            .next()
                    );
                    break;
                case 10:
                    requestRecenter();
                    break;
                default:
                    break;
            }
        } else if (mode == MODE_MEDIA_LIBRARY) {
            handleMediaSelection(hoveredButton);
        } else if (mode == MODE_ENVIRONMENT) {
            handleEnvironmentSelection(hoveredButton);
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

    private void handleMediaSelection(int button) {
        switch (button) {
            case 0:
                if (!preferences.hasSelectedVideo()) {
                    host.onMediaSelectionRequested();
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

            case 1:
                VideoProjection current =
                    preferences.getVideoProjection();
                VideoProjection next =
                    current
                        == VideoProjection.MONO_EQUIRECTANGULAR_360
                        ? VideoProjection.FLAT_CINEMA
                        : VideoProjection.MONO_EQUIRECTANGULAR_360;
                preferences.setVideoProjection(next);
                break;

            case 2:
                host.onMediaSelectionRequested();
                return;

            case 3:
                requestRecenter();
                break;

            case 4:
                mode = MODE_HOME;
                break;

            default:
                return;
        }

        hoveredButton = -1;
        textureDirty = true;
    }

    private void handleEnvironmentSelection(
        int button
    ) {
        if (button >= 0 && button <= 2) {
            HomeEnvironment[] environments =
                HomeEnvironment.values();
            if (button < environments.length) {
                preferences.setHomeEnvironment(
                    environments[button]
                );
            }
        } else if (button == 3) {
            mode = MODE_HOME;
        } else {
            return;
        }

        hoveredButton = -1;
        textureDirty = true;
    }

    private void handleDosLibrarySelection(int button) {
        String[] ids = dosModuleIds;
        int pageCount =
            Math.max(1, (ids.length + 2) / 3);

        if (button >= 0 && button <= 2) {
            if (ids.length == 0 && button == 0) {
                host.onDosImportRequested();
                return;
            }

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
                mode = MODE_DOS_LIBRARY;
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

        if (mode == MODE_DOS_LIBRARY
            || mode == MODE_MEDIA_LIBRARY
            || mode == MODE_ENVIRONMENT) {
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

        if (mode == MODE_MEDIA_LIBRARY) {
            return MEDIA_BUTTONS;
        }

        if (mode == MODE_ENVIRONMENT) {
            return ENVIRONMENT_BUTTONS;
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

        if (mode == MODE_HOME) {
            canvas.drawColor(
                Color.TRANSPARENT,
                PorterDuff.Mode.CLEAR
            );
        } else if (mode == MODE_DOS_OVERLAY
            || mode == MODE_DOS_BINDINGS) {
            canvas.drawColor(
                Color.TRANSPARENT,
                PorterDuff.Mode.CLEAR
            );
            paint.setColor(Color.argb(224, 16, 20, 26));
            canvas.drawRoundRect(
                42,
                42,
                982,
                726,
                28,
                28,
                paint
            );
        } else {
            canvas.drawColor(Color.rgb(9, 12, 16));
            paint.setColor(Color.rgb(24, 29, 36));
            canvas.drawRoundRect(
                42,
                42,
                982,
                726,
                28,
                28,
                paint
            );

            paint.setColor(Color.rgb(56, 214, 200));
            paint.setTextSize(46.0f * uiScale);
            paint.setFakeBoldText(true);
            canvas.drawText(
                "REVERIE VR",
                90,
                120,
                paint
            );

            paint.setFakeBoldText(false);
            paint.setTextSize(24.0f * uiScale);
            paint.setColor(Color.rgb(180, 190, 202));
        }

        if (mode == MODE_HOME) {
            drawHome(canvas, paint);
        } else if (mode == MODE_MEDIA_LIBRARY) {
            drawMediaLibrary(canvas, paint);
        } else if (mode == MODE_ENVIRONMENT) {
            drawEnvironmentMenu(canvas, paint);
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

    private void drawHome(
        Canvas canvas,
        Paint paint
    ) {
        drawHomePanelBackground(
            canvas,
            paint,
            HOME_LEFT_PIXEL_LEFT,
            HOME_LEFT_PIXEL_RIGHT,
            HOME_PIXEL_TOP,
            HOME_PIXEL_BOTTOM
        );
        drawHomePanelBackground(
            canvas,
            paint,
            HOME_CENTER_PIXEL_LEFT,
            HOME_CENTER_PIXEL_RIGHT,
            HOME_PIXEL_TOP,
            HOME_PIXEL_BOTTOM
        );
        drawHomePanelBackground(
            canvas,
            paint,
            HOME_RIGHT_PIXEL_LEFT,
            HOME_RIGHT_PIXEL_RIGHT,
            HOME_PIXEL_TOP,
            HOME_PIXEL_BOTTOM
        );

        paint.setFakeBoldText(true);
        paint.setColor(Color.rgb(56, 214, 235));
        paint.setTextSize(28.0f * uiScale);
        canvas.drawText(
            "REVERIE VR",
            40,
            115,
            paint
        );
        paint.setFakeBoldText(false);

        paint.setColor(Color.WHITE);
        paint.setTextSize(24.0f * uiScale);
        canvas.drawText(
            "HOME",
            42,
            158,
            paint
        );

        String dosLabel =
            !dosRuntimeAvailable
                ? "DOS unavailable"
                : (
                    dosModuleIds.length == 0
                        ? "DOS Games"
                        : "DOS Games  •  "
                            + dosModuleIds.length
                );

        String nativeLabel =
            NativeModuleRuntime.isAvailable()
                && !nativeModules.isEmpty()
                ? "Native Apps"
                : "Native Apps";

        String[] leftLabels =
            new String[] {
                "Media",
                dosLabel,
                nativeLabel,
                "Environment",
                "Setup / Comfort",
                "Exit to Phone"
            };
        drawHomeButtons(
            canvas,
            paint,
            leftLabels,
            0,
            6
        );

        paint.setFakeBoldText(true);
        paint.setColor(Color.WHITE);
        paint.setTextSize(30.0f * uiScale);
        canvas.drawText(
            "Welcome to ReverieVR",
            292,
            150,
            paint
        );
        paint.setFakeBoldText(false);

        paint.setColor(Color.rgb(160, 176, 194));
        paint.setTextSize(17.0f * uiScale);
        canvas.drawText(
            preferences
                .getHomeEnvironment()
                .displayName,
            292,
            185,
            paint
        );

        paint.setColor(Color.rgb(34, 45, 58));
        canvas.drawRoundRect(
            292,
            220,
            718,
            326,
            18,
            18,
            paint
        );
        paint.setColor(Color.WHITE);
        paint.setTextSize(20.0f * uiScale);
        canvas.drawText(
            "Controller",
            314,
            255,
            paint
        );
        paint.setColor(
            controllerConnected
                ? Color.rgb(93, 224, 177)
                : Color.rgb(231, 174, 87)
        );
        paint.setTextSize(17.0f * uiScale);
        canvas.drawText(
            controllerConnected
                ? "Connected • pointer ready"
                : shorten(
                    controllerMessage,
                    34
                ),
            314,
            286,
            paint
        );

        paint.setColor(Color.rgb(34, 45, 58));
        canvas.drawRoundRect(
            292,
            348,
            718,
            454,
            18,
            18,
            paint
        );
        paint.setColor(Color.WHITE);
        paint.setTextSize(20.0f * uiScale);
        canvas.drawText(
            "Media",
            314,
            383,
            paint
        );
        paint.setColor(Color.rgb(160, 176, 194));
        paint.setTextSize(17.0f * uiScale);
        canvas.drawText(
            preferences.hasSelectedVideo()
                ? shorten(
                    preferences
                        .getSelectedVideoDisplayName(),
                    36
                )
                : "No media selected",
            314,
            414,
            paint
        );

        paint.setColor(Color.rgb(34, 45, 58));
        canvas.drawRoundRect(
            292,
            476,
            718,
            582,
            18,
            18,
            paint
        );
        paint.setColor(Color.WHITE);
        paint.setTextSize(20.0f * uiScale);
        canvas.drawText(
            "Input",
            314,
            511,
            paint
        );
        paint.setColor(Color.rgb(160, 176, 194));
        paint.setTextSize(17.0f * uiScale);
        canvas.drawText(
            "Pointer: "
                + preferences
                    .getVrPointerMode()
                    .displayName,
            314,
            542,
            paint
        );

        paint.setColor(Color.rgb(143, 160, 179));
        paint.setTextSize(15.0f * uiScale);
        canvas.drawText(
            "Look or point • click to select",
            292,
            635,
            paint
        );

        paint.setColor(Color.WHITE);
        paint.setTextSize(23.0f * uiScale);
        canvas.drawText(
            "QUICK OPTIONS",
            780,
            158,
            paint
        );

        String[] rightLabels =
            new String[] {
                toggleLabel(
                    "Battery HUD",
                    preferences.isBatteryHudEnabled()
                ),
                toggleLabel(
                    "Percentages",
                    preferences.isShowPercentagesEnabled()
                ),
                toggleLabel(
                    "Look-Up Reveal",
                    preferences.isLookUpRevealEnabled()
                ),
                "Pointer: "
                    + preferences
                        .getVrPointerMode()
                        .displayName,
                "Recenter View"
            };
        drawHomeButtons(
            canvas,
            paint,
            rightLabels,
            6,
            5
        );

        paint.setColor(Color.rgb(126, 145, 165));
        paint.setTextSize(13.0f * uiScale);
        canvas.drawText(
            "Build "
                + BuildConfig.PHONE_TEST_RUN_NUMBER
                + " • "
                + shorten(
                    BuildConfig.SOURCE_REVISION,
                    7
                ),
            780,
            650,
            paint
        );
    }

    private void drawHomePanelBackground(
        Canvas canvas,
        Paint paint,
        float left,
        float right,
        float top,
        float bottom
    ) {
        paint.setColor(
            Color.argb(
                225,
                11,
                20,
                31
            )
        );
        canvas.drawRoundRect(
            left,
            top,
            right,
            bottom,
            22,
            22,
            paint
        );

        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(2.0f);
        paint.setColor(
            Color.argb(
                150,
                65,
                145,
                184
            )
        );
        canvas.drawRoundRect(
            left + 2,
            top + 2,
            right - 2,
            bottom - 2,
            20,
            20,
            paint
        );
        paint.setStyle(Paint.Style.FILL);
    }

    private void drawHomeButtons(
        Canvas canvas,
        Paint paint,
        String[] labels,
        int startIndex,
        int count
    ) {
        int end =
            Math.min(
                HOME_BUTTONS.length,
                startIndex + count
            );
        for (int index = startIndex;
             index < end;
             index++) {
            String label =
                labels[index - startIndex];
            int[] rect =
                HOME_BUTTONS[index];
            boolean focused =
                index == hoveredButton;

            paint.setColor(
                focused
                    ? Color.rgb(
                        24,
                        112,
                        151
                    )
                    : Color.rgb(
                        25,
                        37,
                        50
                    )
            );
            canvas.drawRoundRect(
                rect[0],
                rect[1],
                rect[2],
                rect[3],
                14,
                14,
                paint
            );

            if (focused) {
                paint.setStyle(
                    Paint.Style.STROKE
                );
                paint.setStrokeWidth(3.0f);
                paint.setColor(
                    Color.rgb(
                        68,
                        220,
                        245
                    )
                );
                canvas.drawRoundRect(
                    rect[0] + 1,
                    rect[1] + 1,
                    rect[2] - 1,
                    rect[3] - 1,
                    13,
                    13,
                    paint
                );
                paint.setStyle(
                    Paint.Style.FILL
                );
            }

            paint.setColor(Color.WHITE);
            paint.setTextSize(
                16.0f * uiScale
            );
            paint.setFakeBoldText(focused);
            canvas.drawText(
                shorten(label, 20),
                rect[0] + 12,
                rect[1] + 35,
                paint
            );
            paint.setFakeBoldText(false);
        }
    }

    private static String toggleLabel(
        String label,
        boolean enabled
    ) {
        return label
            + (enabled ? "  ON" : "  OFF");
    }

    private void drawMediaLibrary(
        Canvas canvas,
        Paint paint
    ) {
        paint.setColor(Color.WHITE);
        paint.setTextSize(31.0f * uiScale);
        canvas.drawText(
            "MEDIA",
            90,
            175,
            paint
        );

        paint.setColor(Color.rgb(150, 162, 177));
        paint.setTextSize(20.0f * uiScale);

        String selected =
            preferences.hasSelectedVideo()
                ? shorten(
                    preferences.getSelectedVideoDisplayName(),
                    48
                )
                : "No media selected";
        canvas.drawText(
            selected,
            90,
            215,
            paint
        );

        VideoProjection projection =
            preferences.getVideoProjection();
        String[] labels = new String[] {
            preferences.hasSelectedVideo()
                ? "PLAY ON FLOATING SCREEN"
                : "CHOOSE MEDIA ON PHONE",
            projection
                == VideoProjection.MONO_EQUIRECTANGULAR_360
                ? "DISPLAY: MONO 360°"
                : "DISPLAY: FLOATING SCREEN",
            preferences.hasSelectedVideo()
                ? "CHANGE MEDIA ON PHONE"
                : "OPEN MEDIA PICKER ON PHONE",
            "RECENTER",
            "BACK TO HOME"
        };

        drawButtons(
            canvas,
            paint,
            labels,
            MEDIA_BUTTONS
        );
    }

    private void drawEnvironmentMenu(
        Canvas canvas,
        Paint paint
    ) {
        paint.setColor(Color.WHITE);
        paint.setTextSize(31.0f * uiScale);
        canvas.drawText(
            "HOME ENVIRONMENT",
            90,
            175,
            paint
        );

        paint.setColor(Color.rgb(150, 162, 177));
        paint.setTextSize(20.0f * uiScale);
        canvas.drawText(
            "Current: "
                + preferences
                    .getHomeEnvironment()
                    .displayName,
            90,
            215,
            paint
        );

        HomeEnvironment current =
            preferences.getHomeEnvironment();
        HomeEnvironment[] environments =
            HomeEnvironment.values();
        String[] labels = new String[] {
            environmentLabel(
                environments[0],
                current
            ),
            environmentLabel(
                environments[1],
                current
            ),
            environmentLabel(
                environments[2],
                current
            ),
            "BACK TO HOME"
        };

        drawButtons(
            canvas,
            paint,
            labels,
            ENVIRONMENT_BUTTONS
        );
    }

    private static String environmentLabel(
        HomeEnvironment environment,
        HomeEnvironment current
    ) {
        return (
            environment == current
                ? "✓  "
                : ""
        ) + environment.displayName.toUpperCase(Locale.US);
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
                    : (
                        count == 0 && slot == 0
                            ? "IMPORT DOS MODULE ON PHONE"
                            : "—"
                    );
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
            "DOS LIBRARY",
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
