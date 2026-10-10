package io.github.mrcalzon02.reverievr;

import android.content.Context;
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
        void onVrKeyboardTextCommitted(String text);
        void onModelImportRequested();
        void onVrRendererFailure(
            String phase,
            Throwable throwable
        );
        void onExitToPhoneRequested();
        void onSetupCompleted();
        void onUiFocusChanged();
        void onUiActionRejected();
        void onNativeModuleFeedbackRequested(
            int feedbackFlags
        );
        void onControllerSpringRecenterRequested();
        PerformanceEnvironmentSnapshot
            getPerformanceEnvironmentSnapshot();
        void onVideoSurfaceTextureReady(SurfaceTexture surfaceTexture);
        void onVideoPlaybackRequested();
        void onMediaSelectionRequested();
        void onDosImportRequested();
        void onUpdateCheckRequested();
        void onBluetoothSettingsRequested();
        void onControllerPairingRequested();
        void onVideoTogglePauseRequested();
        void onVideoSeekRequested(int deltaMillis);
        void onVideoStopRequested();
        boolean onDosPlaybackRequested(String moduleId);
        void onDosOverlayPauseRequested();
        void onDosOverlayResumeRequested();
        void onVolumeAdjustRequested(int direction);
        void onBrightnessAdjustRequested(int direction);
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
    private static final float SHELL_VIEW_CONTRACTION = 0.81f;
    private static final float EYE_CONTENT_VIEWPORT_SCALE = 0.738f;

    private static final float PANEL_HALF_WIDTH =
        1.70f * SHELL_VIEW_CONTRACTION;
    private static final float PANEL_HALF_HEIGHT =
        1.20f * SHELL_VIEW_CONTRACTION;
    private static final float PANEL_Z = -3.0f;

    private static final float Z_NEAR = 0.10f;
    private static final float Z_FAR = 30.0f;

    private static final float CONTROLLER_ANCHOR_X = 0.28f;
    private static final float CONTROLLER_ANCHOR_Y = -0.34f;
    private static final float CONTROLLER_ANCHOR_Z = -0.48f;
    private static final float CONTROLLER_EMITTER_FORWARD_METERS = 0.066f;
    private static final float STANDARD_GRAVITY_METERS_PER_SECOND_SQUARED =
        9.80665f;

    private static final int INITIAL_HEADING_STABLE_FRAME_TARGET = 8;
    private static final float INITIAL_HEADING_STABLE_DELTA_RADIANS =
        (float) Math.toRadians(1.5);
    private static final long INITIAL_HEADING_SETTLE_TIMEOUT_NANOS =
        750000000L;

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
    private static final int MODE_CONTROLLER = 10;
    private static final int MODE_NATIVE_LIBRARY = 11;
    private static final int MODE_KEYBOARD = 12;
    private static final int MODE_MODEL_VIEWER = 13;

    private static final int HOME_LEFT_PIXEL_LEFT = 24;
    private static final int HOME_LEFT_PIXEL_RIGHT = 248;
    private static final int HOME_CENTER_PIXEL_LEFT = 266;
    private static final int HOME_CENTER_PIXEL_RIGHT = 744;
    private static final int HOME_RIGHT_PIXEL_LEFT = 762;
    private static final int HOME_RIGHT_PIXEL_RIGHT = 1000;
    private static final int HOME_PIXEL_TOP = 60;
    private static final int HOME_PIXEL_BOTTOM = 710;

    private static final float HOME_LEFT_WORLD_LEFT = -2.43f;
    private static final float HOME_LEFT_WORLD_RIGHT = -1.17f;
    private static final float HOME_CENTER_WORLD_LEFT = -1.035f;
    private static final float HOME_CENTER_WORLD_RIGHT = 1.035f;
    private static final float HOME_RIGHT_WORLD_LEFT = 1.17f;
    private static final float HOME_RIGHT_WORLD_RIGHT = 2.43f;
    private static final float HOME_WORLD_BOTTOM = -1.08f;
    private static final float HOME_WORLD_TOP = 1.08f;
    private static final float HOME_LEFT_WORLD_Z_OUTER = -2.92f;
    private static final float HOME_LEFT_WORLD_Z_INNER = -3.18f;
    private static final float HOME_CENTER_WORLD_Z = -3.0f;
    private static final float HOME_RIGHT_WORLD_Z_INNER = -3.18f;
    private static final float HOME_RIGHT_WORLD_Z_OUTER = -2.92f;

    private static final int[][] ORIENTATION_MENU_BUTTONS =
        new int[][] {
            {220, 248, 500, 304},
            {524, 248, 804, 304},
            {220, 320, 500, 376},
            {524, 320, 804, 376},
            {220, 392, 500, 448},
            {524, 392, 804, 448},
            {220, 464, 500, 520},
            {524, 464, 804, 520}
        };

    private static final int[][] QUICK_SETTINGS_BUTTONS =
        new int[][] {
            {220, 248, 500, 296},
            {524, 248, 804, 296},
            {220, 306, 500, 354},
            {524, 306, 804, 354},
            {220, 364, 500, 412},
            {524, 364, 804, 412},
            {220, 422, 500, 470},
            {524, 422, 804, 470},
            {220, 480, 500, 528},
            {524, 480, 804, 528}
        };

    private static final int[][] HOME_BUTTONS = new int[][] {
        {42, 180, 230, 238},
        {42, 248, 230, 306},
        {42, 316, 230, 374},
        {42, 384, 230, 442},
        {42, 452, 230, 510},
        {42, 520, 230, 578},
        {780, 176, 982, 224},
        {780, 232, 982, 280},
        {780, 288, 982, 336},
        {780, 344, 982, 392},
        {780, 400, 982, 448},
        {780, 456, 982, 504},
        {780, 512, 982, 560},
        {314, 592, 696, 644},
        {292, 220, 718, 326},
        {292, 348, 718, 454},
        {292, 476, 718, 582}
    };

    private static final int[][] KEYBOARD_BUTTONS = makeKeyboardButtons();

    private static int[][] makeKeyboardButtons() {
        int[][] buttons = new int[51][4];
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 10; col++) {
                int left = 72 + col * 88;
                int top = 238 + row * 84;
                buttons[row * 10 + col] =
                    new int[] {left, top, left + 82, top + 74};
            }
        }
        buttons[50] = new int[] {340, 684, 684, 736};
        return buttons;
    }

    private static final int[][] NATIVE_LIBRARY_BUTTONS = new int[][] {
        {140, 235, 884, 300},
        {140, 315, 884, 380},
        {140, 395, 884, 460},
        {140, 475, 884, 540},
        {140, 555, 884, 620}
    };

    private static final int[][] CONTROLLER_BUTTONS = new int[][] {
        {120, 300, 500, 365},
        {524, 300, 904, 365},
        {120, 385, 500, 450},
        {524, 385, 904, 450},
        {120, 470, 500, 535},
        {524, 470, 904, 535},
        {120, 555, 500, 620},
        {524, 555, 904, 620}
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
    private final ObjMeshViewerRenderer modelViewer;
    private final VrPointerRenderer pointerRenderer;
    private final VrControllerModelRenderer controllerModelRenderer;
    private final DosSession dosSession;
    private final DosSurfaceRenderer dosRenderer;
    private final NativeModuleRuntime nativeModuleRuntime;
    private final List<NativeModuleRuntime.Descriptor> nativeModules;
    private final VrKeyboardEditor keyboardEditor = new VrKeyboardEditor();
    private final FramePerformanceTracker performanceTracker =
        new FramePerformanceTracker();
    private final EyeRenderPerformanceTracker eyeRenderPerformanceTracker =
        new EyeRenderPerformanceTracker();
    // Reused only by the serialized GL renderer callback thread.
    private final int[] eyeViewportScratch = new int[4];
    private final int[] eyeScissorScratch = new int[4];
    private final int[] eyeScissorEnabledScratch = new int[1];

    private final FloatBuffer vertexBuffer;
    private final FloatBuffer keyboardSlabVertexBuffer;
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
    private final float[] inverseAdjustedHeadView = new float[16];
    private final float[] controllerAnchorView = new float[4];
    private final float[] controllerAnchorWorld = new float[4];
    private final float[] controllerGhostWorld = new float[4];
    private final float[] controllerAnchorForwardView =
        new float[] {0.0f, 0.0f, -1.0f, 0.0f};
    private final float[] controllerAnchorForwardWorld = new float[4];
    private final ControllerBodyAnchor controllerBodyAnchor =
        new ControllerBodyAnchor();
    private final PlayerHeadRig playerHeadRig = new PlayerHeadRig();
    private boolean controllerHeadsetOverlap;
    private long controllerAnchorLastFrameNanos;
    private final float[] hudIdentity = new float[16];
    private final float[] hudVertices = new float[12];
    private final float[] yawMatrix = new float[16];
    private final float[] headEuler = new float[3];
    private final float[] headForward = new float[3];
    private final float[] adjustedHeadForward = new float[3];
    private final float[] headMotionWorld = new float[3];
    private final float[] adjustedHeadMotionWorld = new float[3];
    private final BoundedInertialTranslation
        headInertialTranslation =
            new BoundedInertialTranslation();
    private final BoundedViewRelativeLocomotion nativeLocomotion =
        new BoundedViewRelativeLocomotion();
    private final TouchpadLocomotionGate nativeLocomotionGate =
        new TouchpadLocomotionGate();
    private final float[] nativeLocomotionBounds =
        new float[2];
    private String nativeLocomotionModuleId = "";
    private long nativeLocomotionLastFrameNanos;
    private final float[] controllerForward = new float[3];
    private final float[] adjustedControllerForward = new float[3];
    private final float[] controllerAccelerationWorld = new float[3];
    private final float[] adjustedControllerAccelerationWorld = new float[3];
    private final float[] controllerAccelerationWorld4 = new float[4];
    private final float[] controllerAccelerationView4 = new float[4];
    private final float[] controllerGravityWorld = new float[3];
    private final ControllerInertialTranslation
        controllerInertialTranslation =
            new ControllerInertialTranslation();
    private final ControllerShakeRecenterDetector
        controllerShakeRecenterDetector =
            new ControllerShakeRecenterDetector();
    private final float[] activePointerOrigin = new float[3];
    private final float[] activePointerDirection = new float[3];
    private final float[] orientationMenuRayOrigin = new float[3];
    private final float[] orientationMenuRayDirection = new float[3];

    private final AtomicBoolean firstFrameReported =
        new AtomicBoolean();
    private final AtomicBoolean selectRequested =
        new AtomicBoolean();
    private final AtomicBoolean backRequested =
        new AtomicBoolean();
    private final AtomicBoolean recenterRequested = new AtomicBoolean();
    private final AtomicBoolean controllerPositionRecenterRequested =
        new AtomicBoolean();
    private final AtomicInteger phoneBattery = new AtomicInteger(-1);
    private final AtomicInteger controllerBattery = new AtomicInteger(-1);
    private final AtomicInteger videoSeekRequestedMillis = new AtomicInteger();
    private final AtomicBoolean dosExitRequested = new AtomicBoolean();
    private final long vrSessionStartedNanos =
        System.nanoTime();

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
    private int nativeLibraryPage;
    private boolean nativeSurfaceReady;
    private volatile boolean rendererFailed;
    private volatile boolean orientationMenuVisible;
    private boolean quickMenuSettingsVisible;
    private boolean keyboardQuickMenuFocusActive;
    private volatile float orientationMenuYawRadians;
    private boolean shellHeadingInitialized;
    private long initialHeadingStartedNanos;
    private int initialHeadingStableFrames;
    private float initialHeadingCandidateYaw = Float.NaN;
    private float controllerYawCalibrationRadians;
    private final float[] controllerOrientationCorrection = new float[16];
    private boolean fullControllerOrientationCalibration;

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
    private volatile float controllerAccelerationX;
    private volatile float controllerAccelerationY;
    private volatile float controllerAccelerationZ;
    private volatile long controllerAccelerationAtNanos;
    private volatile boolean controllerPoseValid;
    private volatile boolean controllerTouchpadPressed;
    private volatile ControllerSnapshot locomotionControllerSnapshot;
    private boolean viewerTouchActive;
    private float viewerPreviousTouchX;
    private float viewerPreviousTouchY;
    private volatile boolean controllerHomePressed;
    private volatile boolean controllerAppPressed;
    private volatile boolean controllerVolumeUpPressed;
    private volatile boolean controllerVolumeDownPressed;
    private volatile boolean gamepadPointerAvailable;
    private volatile float virtualPointerAxisX;
    private volatile float virtualPointerAxisY;
    private volatile float virtualPointerYaw;
    private volatile float virtualPointerPitch;
    private long virtualPointerLastFrameNanos;
    private volatile boolean controllerPointerActive;
    private volatile float controllerPointerDistance = 6.0f;
    private volatile boolean controllerPointerHit;
    private volatile int activeNativePointerKind =
        NativeModuleRuntime.POINTER_NONE;
    private volatile String activePointerSource = "Gaze";
    private volatile float headLinearAccelerationX;
    private volatile float headLinearAccelerationY;
    private volatile float headLinearAccelerationZ;
    private volatile long headLinearAccelerationAtNanos;
    private long headInertialLastFrameNanos;
    private long controllerInertialLastFrameNanos;
    private boolean controllerRecenterFeedbackActive;
    private boolean controllerGravityInitialized;
    private float yawOffsetRadians;
    private float userIpdMeters;
    private float uiScale;
    private boolean bindingHeadInitialized;
    private boolean controllerTrainingReturn;
    private long lastPerformanceLogNanos;
    private long lastStereoDiagnosticLogNanos;
    private int stereoEyeMask;
    private final int[] stereoLeftViewport = new int[4];
    private final int[] stereoRightViewport = new int[4];
    private long lastQuickMenuStatusRefreshNanos;
    private float previousBindingYaw;
    private float previousBindingPitch;

    VrShellRenderer(
        Context context,
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
            NativeModuleRuntime.listBuiltIns(
                preferences.getLoggingMode()
                    == LoggingMode.DEVELOPMENT
            );

        if (preferences.getLoggingMode()
            == LoggingMode.DEVELOPMENT
            && findNativeModule(
                NativeModuleRuntime.ID_RED_LEDGER
            ) != null) {
            ReverieLog.milestone(
                "NATIVE_MODULE",
                "Development native module admission enabled."
            );
        }
        videoRenderer = new VideoSurfaceRenderer(host::onVideoSurfaceTextureReady);
        homeEnvironmentRenderer =
            new HomeEnvironmentRenderer();
        modelViewer = new ObjMeshViewerRenderer(context);
        pointerRenderer = new VrPointerRenderer();
        controllerModelRenderer =
            new VrControllerModelRenderer(
                context
            );
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
        // Keyboard occupies the existing horizontal gray dais.
        // Top of the keyboard texture faces the far end of the slab.
        keyboardSlabVertexBuffer = allocate(new float[] {
            -1.73f, -1.065f, -1.36f,
             1.73f, -1.065f, -1.36f,
            -1.73f, -1.065f, -4.08f,
             1.73f, -1.065f, -4.08f
        });
        uvBuffer = allocate(uvs);

        homeLeftVertexBuffer =
            allocate(
                panelVertices(
                    HOME_LEFT_WORLD_LEFT,
                    HOME_LEFT_WORLD_RIGHT,
                    HOME_WORLD_BOTTOM,
                    HOME_WORLD_TOP,
                    HOME_LEFT_WORLD_Z_OUTER,
                    HOME_LEFT_WORLD_Z_INNER
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
                    HOME_RIGHT_WORLD_Z_INNER,
                    HOME_RIGHT_WORLD_Z_OUTER
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

    boolean isOrientationMenuVisible() {
        return orientationMenuVisible;
    }

    boolean handlesSelectAsShellAction() {
        if (orientationMenuVisible) {
            return true;
        }

        int currentMode = mode;
        return currentMode != MODE_DOS
            && currentMode != MODE_NATIVE;
    }

    boolean canActivateSelect() {
        if (orientationMenuVisible) {
            return hoveredButton >= 0;
        }

        return mode == MODE_VIDEO
            || mode == MODE_MODEL_VIEWER
            || hoveredButton >= 0;
    }

    void requestSelect() {
        selectRequested.set(true);
    }

    void requestBack() {
        backRequested.set(true);
    }

    void toggleOrientationMenu() {
        boolean opening =
            !orientationMenuVisible;
        orientationMenuVisible = opening;
        quickMenuSettingsVisible = false;
        keyboardQuickMenuFocusActive = false;
        hoveredButton = -1;
        selectRequested.set(false);
        backRequested.set(false);
        if (opening) {
            /*
             * The current headset pose belongs to the renderer thread.
             * Defer quick-menu placement to onNewFrameInternal() so opening
             * the menu from an input/UI callback cannot capture a stale or
             * zeroed headForward vector and silently fall back to shell yaw 0.
             */
            lastQuickMenuStatusRefreshNanos = 0L;
        }
        textureDirty = true;
    }

    void requestRecenter() {
        recenterRequested.set(true);
    }

    void setHeadLinearAcceleration(
        float x,
        float y,
        float z,
        long timestampNanos
    ) {
        if (!Float.isFinite(x)
            || !Float.isFinite(y)
            || !Float.isFinite(z)
            || Math.abs(x) > 50.0f
            || Math.abs(y) > 50.0f
            || Math.abs(z) > 50.0f) {
            return;
        }

        headLinearAccelerationX = x;
        headLinearAccelerationY = y;
        headLinearAccelerationZ = z;
        headLinearAccelerationAtNanos =
            timestampNanos > 0L
                ? timestampNanos
                : System.nanoTime();
    }

    void setPhoneBattery(int percentage) {
        if (percentage < -1 || percentage > 100) {
            return;
        }
        if (phoneBattery.getAndSet(percentage) != percentage) {
            hudTextureDirty = true;
        }
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
            locomotionControllerSnapshot = null;
            controllerPointerActive = false;
            controllerAccelerationAtNanos = 0L;
            controllerInertialTranslation.reset();
            controllerShakeRecenterDetector.reset();
            controllerPositionRecenterRequested.set(false);
            controllerInertialLastFrameNanos = 0L;
            controllerGravityInitialized = false;
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
        controllerAccelerationX = snapshot.accelXG;
        controllerAccelerationY = snapshot.accelYG;
        controllerAccelerationZ = snapshot.accelZG;
        controllerPoseReceivedAtNanos =
            snapshot.receivedAtNanos > 0L
                ? snapshot.receivedAtNanos
                : System.nanoTime();
        controllerAccelerationAtNanos =
            controllerPoseReceivedAtNanos;
        if (snapshot.touching || snapshot.touchpadPressed) {
            controllerShakeRecenterDetector.cancelPendingImpulse();
        } else if (controllerShakeRecenterDetector.sample(
                controllerAccelerationX,
                controllerAccelerationY,
                controllerAccelerationZ,
                controllerAccelerationAtNanos
            )) {
            controllerPositionRecenterRequested.set(true);
        }
        controllerTouchpadPressed =
            snapshot.touchpadPressed;
        controllerHomePressed =
            snapshot.homePressed;
        controllerAppPressed =
            snapshot.menuPressed;
        controllerVolumeUpPressed =
            snapshot.volumeUpPressed;
        controllerVolumeDownPressed =
            snapshot.volumeDownPressed;
        controllerPoseValid = true;
        locomotionControllerSnapshot = snapshot;
    }

    void setGamepadPointerAvailable(
        boolean available
    ) {
        gamepadPointerAvailable = available;
        if (!available) {
            virtualPointerAxisX = 0.0f;
            virtualPointerAxisY = 0.0f;
            virtualPointerLastFrameNanos = 0L;
        }
    }

    void setVirtualPointerAxes(
        float horizontal,
        float vertical
    ) {
        virtualPointerAxisX =
            clamp(horizontal, -1.0f, 1.0f);
        virtualPointerAxisY =
            clamp(vertical, -1.0f, 1.0f);
    }

    boolean requestPointerNavigation(
        VrInputAction action
    ) {
        if (action == null
            || !gamepadPointerAvailable) {
            return false;
        }

        if (!orientationMenuVisible
            && (
                mode == MODE_VIDEO
                    || mode == MODE_DOS
                    || mode == MODE_NATIVE
            )) {
            return false;
        }

        VrPointerMode pointerMode =
            preferences.getVrPointerMode();
        if (pointerMode == VrPointerMode.GAZE) {
            return false;
        }

        float step =
            (float) Math.toRadians(4.5);
        switch (action) {
            case NAV_LEFT:
                virtualPointerYaw -= step;
                break;
            case NAV_RIGHT:
                virtualPointerYaw += step;
                break;
            case NAV_UP:
                virtualPointerPitch += step;
                break;
            case NAV_DOWN:
                virtualPointerPitch -= step;
                break;
            default:
                return false;
        }

        clampVirtualPointerAngles();
        return true;
    }

    boolean requestQuickMenuKeyboardNavigation(
        VrInputAction action
    ) {
        if (!orientationMenuVisible
            || action == null) {
            return false;
        }

        int[][] buttons =
            activeButtons();
        if (buttons.length == 0) {
            return false;
        }

        int current =
            hoveredButton;
        if (current < 0
            || current >= buttons.length
            || !isButtonEnabled(current)) {
            current =
                firstEnabledButton(
                    buttons.length
                );
        }
        if (current < 0) {
            return false;
        }

        int next = current;
        switch (action) {
            case NAV_LEFT:
                if ((current & 1) == 1
                    && isButtonEnabled(
                        current - 1
                    )) {
                    next = current - 1;
                }
                break;

            case NAV_RIGHT:
                if ((current & 1) == 0
                    && current + 1
                        < buttons.length
                    && isButtonEnabled(
                        current + 1
                    )) {
                    next = current + 1;
                }
                break;

            case NAV_UP:
                next =
                    findEnabledVerticalButton(
                        current,
                        -2,
                        buttons.length
                    );
                break;

            case NAV_DOWN:
                next =
                    findEnabledVerticalButton(
                        current,
                        2,
                        buttons.length
                    );
                break;

            default:
                return false;
        }

        keyboardQuickMenuFocusActive = true;
        if (next != hoveredButton) {
            hoveredButton = next;
            textureDirty = true;
            host.onUiFocusChanged();
        }
        return true;
    }

    private int firstEnabledButton(
        int buttonCount
    ) {
        for (int index = 0;
             index < buttonCount;
             index++) {
            if (isButtonEnabled(index)) {
                return index;
            }
        }
        return -1;
    }

    private int findEnabledVerticalButton(
        int current,
        int step,
        int buttonCount
    ) {
        int candidate =
            current + step;
        while (candidate >= 0
            && candidate < buttonCount) {
            if (isButtonEnabled(candidate)) {
                return candidate;
            }
            candidate += step;
        }
        return current;
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

        if (orientationMenuVisible
            && !lastInputSource.startsWith(
                "Keyboard quick menu"
            )
            && !lastInputSource.equals(
                "Keyboard Menu key"
            )) {
            keyboardQuickMenuFocusActive = false;
        }

        textureDirty = true;
    }

    boolean consumeBackDuringInputTraining() {
        return !orientationMenuVisible
            && mode == MODE_SETUP
            && setupStep == 1;
    }

    void requestVideoExit() {
        backRequested.set(true);
    }

    void requestDosExit() {
        dosExitRequested.set(true);
    }

    boolean isHostedInputSuppressed() {
        return orientationMenuVisible
            || mode == MODE_DOS_OVERLAY
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
                    + " phoneBatteryPct="
                    + phoneBattery.get()
                    + " "
                    + snapshot.toLogString()
            );
            if (ReverieLog.isDevelopment()) {
                ReverieLog.dev(
                    "VR_EYE_CPU",
                    eyeRenderPerformanceTracker.snapshot().toLogString()
                );
                if (mode == MODE_NATIVE) {
                    ReverieLog.dev(
                        "VR_LOCOMOTION",
                        "module=" + nativeLocomotionModuleId
                            + " xMeters=" + nativeLocomotion.x()
                            + " zMeters=" + nativeLocomotion.z()
                    );
                }
            }
            lastPerformanceLogNanos = frameNanos;
        }

        headTransform.getHeadView(rawHeadView, 0);
        headTransform.getEulerAngles(headEuler, 0);
        headTransform.getForwardVector(headForward, 0);

        if (!shellHeadingInitialized) {
            updateInitialShellHeading(
                frameNanos
            );
        }

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
                    + " headOffset=("
                    + headInertialTranslation.x()
                    + ","
                    + headInertialTranslation.y()
                    + ","
                    + headInertialTranslation.z()
                    + ")"
            );
        }

        if (recenterRequested.getAndSet(false)
            && !recenterOnHeadset(
                frameNanos
            )) {
            ReverieLog.milestone(
                "VR_HEADING",
                "Headset recenter ignored because no horizontal forward heading was available."
            );
        }

        if (orientationMenuVisible) {
            // A Quick Menu is shell HUD content. Follow current headset
            // yaw every frame instead of freezing its world heading at open.
            // Both rendering and hit testing use this same orientation.
            captureOrientationMenuHeading();
        }

        Matrix.setRotateM(
            yawMatrix,
            0,
            (float) Math.toDegrees(-yawOffsetRadians),
            0.0f,
            1.0f,
            0.0f
        );
        updateHeadInertialTranslation(
            frameNanos
        );
        updateNativeLocomotion(frameNanos);

        Matrix.multiplyMM(
            adjustedHeadView,
            0,
            yawMatrix,
            0,
            rawHeadView,
            0
        );
        Matrix.translateM(
            adjustedHeadView,
            0,
            -headInertialTranslation.x(),
            -headInertialTranslation.y(),
            -headInertialTranslation.z()
        );
        if (mode == MODE_NATIVE || mode == MODE_HOME) {
            // One world-space translation shared by both Cardboard eyes.
            Matrix.translateM(
                adjustedHeadView,
                0,
                -nativeLocomotion.x(),
                0.0f,
                -nativeLocomotion.z()
            );
        }
        // The shell owns one center-head rig, never a separate per-eye rig.
        // This only supplies collision probes and future mirror geometry;
        // it does not touch the Cardboard projection or IPD transforms.
        playerHeadRig.update(adjustedHeadView, userIpdMeters);
        if (controllerPositionRecenterRequested.getAndSet(false)) {
            requestSoftControllerRecenter();
        }
        updateControllerInertialTranslation(frameNanos);
        if (controllerRecenterFeedbackActive
            && !controllerInertialTranslation.isReturningToCenter()) {
            controllerRecenterFeedbackActive = false;
            ReverieLog.milestone(
                "VR_CONTROLLER",
                "Controller recenter ended; offset meters="
                    + controllerRecenterOffsetLength()
            );
        }

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

        if (orientationMenuVisible) {
            if (lastQuickMenuStatusRefreshNanos == 0L
                || frameNanos
                    - lastQuickMenuStatusRefreshNanos
                    >= 1000000000L) {
                lastQuickMenuStatusRefreshNanos =
                    frameNanos;
                textureDirty = true;
            }

            if (mode == MODE_VIDEO) {
                videoRenderer.updateFrame();
            } else if (
                mode == MODE_DOS
                    || mode == MODE_DOS_OVERLAY
                    || mode == MODE_DOS_BINDINGS
            ) {
                dosRenderer.updateFrame();
            } else if (mode == MODE_NATIVE) {
                if (nativeModuleRuntime == null
                    || !nativeModuleRuntime.isRunning()) {
                    nativeSurfaceReady = false;
                    closeOrientationMenu();
                    mode = MODE_HOME;
                } else {
                    if (!nativeSurfaceReady) {
                        nativeSurfaceReady =
                            nativeModuleRuntime.onSurfaceCreated();
                    }
                    if (nativeSurfaceReady) {
                        nativeModuleRuntime.update(
                            NativeModuleRuntime.POINTER_NONE,
                            null,
                            null
                        );
                    } else {
                        host.onNativeModuleStopRequested();
                        closeOrientationMenu();
                        mode = MODE_HOME;
                    }
                }
            }

            updateShellInteraction(
                frameNanos
            );
            return;
        }

        if (mode == MODE_MODEL_VIEWER && !orientationMenuVisible) {
            if (backRequested.getAndSet(false)) {
                mode = MODE_HOME;
                hoveredButton = -1;
                textureDirty = true;
                return;
            }
            if (selectRequested.getAndSet(false)) {
                modelViewer.rotate(30.0f);
                host.onUiFocusChanged();
            }
            ControllerSnapshot touch = locomotionControllerSnapshot;
            boolean freshTouch = touch != null
                && touch.receivedAtNanos > 0L
                && frameNanos >= touch.receivedAtNanos
                && frameNanos - touch.receivedAtNanos <= 250000000L
                && touch.touching && !touch.touchpadPressed;
            if (freshTouch) {
                float currentX = touch.touchX;
                float currentY = touch.touchY;
                if (viewerTouchActive) {
                    modelViewer.adjustView(
                        (currentX - viewerPreviousTouchX) * 0.5f,
                        (currentY - viewerPreviousTouchY) * 0.35f,
                        0.0f
                    );
                }
                viewerTouchActive = true;
                viewerPreviousTouchX = currentX;
                viewerPreviousTouchY = currentY;
            } else {
                viewerTouchActive = false;
            }
            // The rocker is free for hosted app controls (not Android volume).
            if (controllerVolumeUpPressed) {
                modelViewer.adjustView(0.0f, 0.0f, -0.04f);
            }
            if (controllerVolumeDownPressed) {
                modelViewer.adjustView(0.0f, 0.0f, 0.04f);
            }
            controllerPointerActive = updateActivePointer(frameNanos);
            pointerRenderer.hide();
            return;
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

            updateNativeModuleInteraction(
                frameNanos
            );
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

        updateShellInteraction(
            frameNanos
        );
    }

    private void updateNativeLocomotion(long frameNanos) {
        final boolean homeTravel = mode == MODE_HOME;
        final boolean activeNative = mode == MODE_NATIVE
            && nativeModuleRuntime != null
            && nativeModuleRuntime.isRunning();
        if (!homeTravel && !activeNative) {
            if (!nativeLocomotionModuleId.isEmpty()) {
                nativeLocomotion.reset();
                nativeLocomotionModuleId = "";
            }
            nativeLocomotionGate.reset();
            nativeLocomotionLastFrameNanos = 0L;
            return;
        }

        // The shell home is a real, bounded room-scale space, not a
        // decorative backdrop. Native experiences retain their own bounds.
        String moduleId = homeTravel
            ? "shell-home"
            : nativeModuleRuntime.getActiveModuleId();
        if (!moduleId.equals(nativeLocomotionModuleId)) {
            nativeLocomotion.reset();
            nativeLocomotionGate.reset();
            nativeLocomotionModuleId = moduleId;
            nativeLocomotionLastFrameNanos = 0L;
            ReverieLog.milestone(
                "VR_LOCOMOTION",
                "Headset-relative touchpad movement initialized: "
                    + moduleId
            );
        }

        if (!homeTravel && !nativeModuleRuntime
                .copyShellLocomotionBounds(
                    nativeLocomotionBounds
                )) {
            nativeLocomotionLastFrameNanos =
                frameNanos;
            return;
        }

        final float limitX = homeTravel ? 1.4f : nativeLocomotionBounds[0];
        final float limitZ = homeTravel ? 1.4f : nativeLocomotionBounds[1];

        float dt = nativeLocomotionLastFrameNanos == 0L
            ? 0.0f
            : (frameNanos - nativeLocomotionLastFrameNanos)
                / 1000000000.0f;
        nativeLocomotionLastFrameNanos = frameNanos;

        // Click remains Select. A modal menu, stale BLE sample or finger
        // release immediately stops artificial camera travel.
        ControllerSnapshot touch = locomotionControllerSnapshot;
        long poseAge = touch == null
            ? Long.MAX_VALUE
            : frameNanos - touch.receivedAtNanos;
        boolean blocked = orientationMenuVisible
            || !controllerConnected
            || !controllerPoseValid
            || touch == null
            || touch.receivedAtNanos <= 0L
            || poseAge < 0L
            || poseAge > 250000000L;
        if (!nativeLocomotionGate.allows(
                touch != null && touch.touching,
                touch != null && touch.touchpadPressed,
                blocked
            )) {
            return;
        }

        rotateYaw(
            headForward,
            -yawOffsetRadians,
            adjustedHeadForward
        );
        nativeLocomotion.update(
            (touch.touchX / 127.5f) - 1.0f,
            (touch.touchY / 127.5f) - 1.0f,
            adjustedHeadForward[0],
            adjustedHeadForward[2],
            dt,
            limitX,
            limitZ
        );
    }

    private void updateNativeModuleInteraction(
        long frameNanos
    ) {
        if (nativeModuleRuntime == null
            || !nativeModuleRuntime.isRunning()) {
            return;
        }

        rotateYaw(
            headForward,
            -yawOffsetRadians,
            adjustedHeadForward
        );

        boolean usingControllerPointer =
            updateActivePointer(frameNanos);
        controllerPointerActive =
            usingControllerPointer;
        controllerPointerDistance = 2.5f;
        controllerPointerHit = false;
        boolean pointerPressed =
            usingControllerPointer
                && activeNativePointerKind
                    == NativeModuleRuntime.POINTER_TRACKED_CONTROLLER
                && controllerTouchpadPressed;
        controllerModelRenderer.setInteractionState(
            false, pointerPressed
        );

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
                false,
                pointerPressed
            );
        } else {
            pointerRenderer.hide();
        }

        int feedbackFlags =
            nativeModuleRuntime.update(
                usingControllerPointer
                    ? activeNativePointerKind
                    : NativeModuleRuntime.POINTER_NONE,
                usingControllerPointer
                    ? activePointerOrigin
                    : null,
                usingControllerPointer
                    ? activePointerDirection
                    : null
            );
        if (feedbackFlags != 0) {
            host.onNativeModuleFeedbackRequested(
                feedbackFlags
            );
        }
    }

    private void updateShellInteraction(
        long frameNanos
    ) {
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
        int newHover =
            orientationMenuVisible
                    && keyboardQuickMenuFocusActive
                ? hoveredButton
                : hit.buttonIndex;

        controllerPointerActive =
            usingControllerPointer;
        controllerPointerDistance =
            hit.distance > 0.0f
                ? hit.distance
                : 6.0f;
        controllerPointerHit =
            newHover >= 0;
        boolean pointerPressed = false;
        if (usingControllerPointer) {
            pointerPressed = activeNativePointerKind
                    == NativeModuleRuntime.POINTER_TRACKED_CONTROLLER
                ? controllerTouchpadPressed
                : selectRequested.get();
        }
        controllerModelRenderer.setInteractionState(
            controllerPointerHit, pointerPressed
        );

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
                controllerPointerHit,
                pointerPressed
            );
        } else {
            pointerRenderer.hide();
        }

        if (newHover != hoveredButton) {
            hoveredButton =
                newHover;
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

        final boolean profileEye = ReverieLog.isDevelopment();
        final long eyeStartedNanos =
            profileEye ? System.nanoTime() : 0L;

        /*
         * Cardboard owns the physical per-eye viewport. Keep that raw rectangle
         * as the stereo isolation boundary, but render the VR world into a
         * centered 73.8% presentation rectangle inside it. This produces the
         * measured handset/headset inset without changing projection, IPD, or
         * the SDK's left/right eye ownership.
         *
         * The full eye is cleared first so the newly exposed margin is always
         * black rather than stale framebuffer content. The shell-global power
         * HUD is drawn after restoring the raw eye viewport so it remains a
         * stable calibration reference instead of shrinking with the world.
         */
        // Avoid six short-lived allocations per stereo frame.
        int[] eyeViewport = eyeViewportScratch;
        int[] previousScissor = eyeScissorScratch;
        int[] scissorEnabled = eyeScissorEnabledScratch;
        GLES20.glGetIntegerv(
            GLES20.GL_VIEWPORT,
            eyeViewport,
            0
        );
        GLES20.glGetIntegerv(
            GLES20.GL_SCISSOR_BOX,
            previousScissor,
            0
        );
        GLES20.glGetIntegerv(
            GLES20.GL_SCISSOR_TEST,
            scissorEnabled,
            0
        );

        if (eyeViewport[2] <= 0
            || eyeViewport[3] <= 0) {
            reportRendererFailure(
                "invalid-eye-viewport",
                new IllegalStateException(
                    "Empty eye viewport"
                )
            );
            return;
        }

        recordStereoEyeDiagnostic(eye, eyeViewport);

        // Hosted native applications own an immersive stereoscopic scene.
        // Do not letterbox them using the shell menu's optical comfort inset.
        final float contentScale = (mode == MODE_NATIVE
            || mode == MODE_MODEL_VIEWER)
            ? 1.0f : EYE_CONTENT_VIEWPORT_SCALE;
        int contentWidth =
            Math.max(
                1,
                Math.round(
                    eyeViewport[2] * contentScale
                )
            );
        int contentHeight =
            Math.max(
                1,
                Math.round(
                    eyeViewport[3] * contentScale
                )
            );
        int contentX =
            eyeViewport[0]
                + (eyeViewport[2] - contentWidth)
                    / 2;
        int contentY =
            eyeViewport[1]
                + (eyeViewport[3] - contentHeight)
                    / 2;

        GLES20.glEnable(
            GLES20.GL_SCISSOR_TEST
        );
        GLES20.glScissor(
            eyeViewport[0],
            eyeViewport[1],
            eyeViewport[2],
            eyeViewport[3]
        );
        GLES20.glClearColor(
            0.0f,
            0.0f,
            0.0f,
            1.0f
        );
        GLES20.glClear(
            GLES20.GL_COLOR_BUFFER_BIT
                | GLES20.GL_DEPTH_BUFFER_BIT
        );

        try {
            GLES20.glViewport(
                contentX,
                contentY,
                contentWidth,
                contentHeight
            );
            GLES20.glScissor(
                contentX,
                contentY,
                contentWidth,
                contentHeight
            );

            onDrawEyeInternal(eye);

            GLES20.glEnable(
                GLES20.GL_SCISSOR_TEST
            );
            GLES20.glViewport(
                eyeViewport[0],
                eyeViewport[1],
                eyeViewport[2],
                eyeViewport[3]
            );
            GLES20.glScissor(
                eyeViewport[0],
                eyeViewport[1],
                eyeViewport[2],
                eyeViewport[3]
            );
            drawPowerHudOverlay();
        } catch (RuntimeException | LinkageError failure) {
            reportRendererFailure(
                "draw-eye",
                failure
            );
        } finally {
            GLES20.glViewport(
                eyeViewport[0],
                eyeViewport[1],
                eyeViewport[2],
                eyeViewport[3]
            );
            GLES20.glScissor(
                previousScissor[0],
                previousScissor[1],
                previousScissor[2],
                previousScissor[3]
            );
            if (scissorEnabled[0] == 0) {
                GLES20.glDisable(
                    GLES20.GL_SCISSOR_TEST
                );
            }
            if (profileEye && !rendererFailed) {
                eyeRenderPerformanceTracker.recordEye(
                    mode,
                    eye.getEyeType() == CardboardView.Eye.LEFT ? 0 : 1,
                    System.nanoTime() - eyeStartedNanos
                );
            }
        }
    }

    private void onDrawEyeInternal(
        CardboardView.Eye eye
    ) {
        /*
         * onDrawEye has already established the centered content viewport.
         * Keep the SDK projection and head-view math unchanged so this remains
         * a presentation-size correction rather than an IPD/FOV rewrite.
         */
        eye.applyHeadView(adjustedHeadView);

        float correctionHalf =
            (userIpdMeters - viewerInterLensMeters) * 0.5f;
        float eyeCorrection = eye.getEyeType() == CardboardView.Eye.LEFT
            ? correctionHalf
            : -correctionHalf;

        if (mode != MODE_NATIVE && mode != MODE_MODEL_VIEWER) {
            HomeEnvironment homeEnvironment =
                preferences.getHomeEnvironment();
            homeEnvironmentRenderer.drawEye(
                eye,
                eyeCorrection,
                homeEnvironment,
                mode == MODE_HOME ? nativeLocomotion.x() : 0.0f,
                mode == MODE_HOME ? nativeLocomotion.z() : 0.0f
            );
            if (mode == MODE_HOME
                && homeEnvironment == HomeEnvironment.WHITE_ROOM
                && !orientationMenuVisible) {
                homeEnvironmentRenderer.drawMirrorEye(
                    playerHeadRig,
                    eye.getEyeType() == CardboardView.Eye.LEFT ? 0 : 1,
                    controllerPointerActive,
                    controllerAnchorWorld[0],
                    controllerAnchorWorld[1],
                    controllerAnchorWorld[2],
                    activePointerDirection[0],
                    activePointerDirection[1],
                    activePointerDirection[2]
                );
            }
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
            videoRenderer.drawEye(
                eye,
                eyeCorrection
            );
            if (orientationMenuVisible) {
                drawUiPanel(
                    eye,
                    eyeCorrection,
                    true
                );
                drawPointerOverlay(
                    eye,
                    eyeCorrection
                );
            }
            return;
        }

        if (mode == MODE_DOS) {
            dosRenderer.drawEye(
                eye,
                eyeCorrection
            );
            if (orientationMenuVisible) {
                drawUiPanel(
                    eye,
                    eyeCorrection,
                    true
                );
                drawPointerOverlay(
                    eye,
                    eyeCorrection
                );
            }
            return;
        }

        if (mode == MODE_DOS_OVERLAY
            || mode == MODE_DOS_BINDINGS) {
            dosRenderer.drawEye(eye, eyeCorrection);
            drawUiPanel(eye, eyeCorrection, true);
            drawPointerOverlay(
                eye,
                eyeCorrection
            );
            return;
        }

        if (mode == MODE_MODEL_VIEWER) {
            modelViewer.drawEye(eye, eyeCorrection);
            if (orientationMenuVisible) {
                drawUiPanel(eye, eyeCorrection, true);
            }
            drawPointerOverlay(eye, eyeCorrection);
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

            if (orientationMenuVisible) {
                drawUiPanel(
                    eye,
                    eyeCorrection,
                    true
                );
                drawPointerOverlay(
                    eye,
                    eyeCorrection
                );
            } else {
                drawPointerOverlay(
                    eye,
                    eyeCorrection
                );
            }
            return;
        }

        drawUiPanel(eye, eyeCorrection, false);
        drawPointerOverlay(
            eye,
            eyeCorrection
        );
    }

    private void recordStereoEyeDiagnostic(
        CardboardView.Eye eye,
        int[] eyeViewport
    ) {
        if (!ReverieLog.isDevelopment()) {
            return;
        }

        int eyeBit =
            eye.getEyeType() == CardboardView.Eye.LEFT
                ? 1
                : 2;
        int[] viewport =
            eyeBit == 1
                ? stereoLeftViewport
                : stereoRightViewport;

        // Reuse the Cardboard-owned viewport already queried by caller.
        System.arraycopy(eyeViewport, 0, viewport, 0, 4);
        stereoEyeMask |= eyeBit;
    }

    private void drawPointerOverlay(
        CardboardView.Eye eye,
        float eyeCorrection
    ) {
        if (!controllerPointerActive) {
            return;
        }

        controllerModelRenderer.drawEye(
            eye,
            eyeCorrection
        );
        pointerRenderer.drawEye(
            eye,
            eyeCorrection
        );
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
        // Render shell panels in the same heading-relative coordinates
        // used by the controller ray and home interaction hit tests.
        // Cardboard's eye view remains untouched for stereo calibration.
        Matrix.rotateM(
            tempMatrix,
            0,
            (float) Math.toDegrees(
                yawOffsetRadians
                    + (orientationMenuVisible
                        ? orientationMenuYawRadians : 0.0f)
            ),
            0.0f,
            1.0f,
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

        if (mode == MODE_HOME
            && !orientationMenuVisible) {
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
                mode == MODE_KEYBOARD && !orientationMenuVisible
                    ? keyboardSlabVertexBuffer : vertexBuffer,
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

        if (ReverieLog.isDevelopment()) {
            long now = System.nanoTime();
            if (lastStereoDiagnosticLogNanos == 0L
                || now - lastStereoDiagnosticLogNanos
                    >= 1000000000L) {
                ReverieLog.dev(
                    "VR_STEREO",
                    "eyes="
                        + stereoEyeMask
                        + " leftViewport="
                        + viewportString(stereoLeftViewport)
                        + " rightViewport="
                        + viewportString(stereoRightViewport)
                        + " finishViewport=("
                        + viewport.x
                        + ","
                        + viewport.y
                        + ","
                        + viewport.width
                        + ","
                        + viewport.height
                        + ") mode="
                        + mode
                );
                lastStereoDiagnosticLogNanos = now;
            }
            stereoEyeMask = 0;
        }
    }

    private static String viewportString(
        int[] viewport
    ) {
        return "("
            + viewport[0]
            + ","
            + viewport[1]
            + ","
            + viewport[2]
            + ","
            + viewport[3]
            + ")";
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
        modelViewer.onSurfaceCreated();
        pointerRenderer.onSurfaceCreated();
        controllerModelRenderer.onSurfaceCreated();
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
        controllerModelRenderer.shutdown();
        homeEnvironmentRenderer.shutdown();
        modelViewer.shutdown();

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

        boolean freshControllerPose =
            hasFreshControllerPose(
                frameNanos
            );

        boolean useTrackedController =
            freshControllerPose
                && (
                    pointerMode
                        == VrPointerMode.CONTROLLER
                    || pointerMode
                        == VrPointerMode.AUTO
                );

        if (useTrackedController) {
            quaternionForward(
                controllerOrientationX,
                controllerOrientationY,
                controllerOrientationZ,
                controllerOrientationW,
                controllerForward
            );
            ControllerOrientationCalibration.rotate(
                controllerOrientationCorrection,
                fullControllerOrientationCalibration,
                controllerForward,
                adjustedControllerForward
            );
            rotateYaw(
                adjustedControllerForward,
                -controllerYawCalibrationRadians,
                adjustedControllerForward
            );
            normalizeDirection(
                adjustedControllerForward
            );

            resolveControllerViewAnchor(frameNanos);
            float controllerAnchorX =
                controllerAnchorWorld[0];
            float controllerAnchorY =
                controllerAnchorWorld[1];
            float controllerAnchorZ =
                controllerAnchorWorld[2];

            controllerModelRenderer.setAnchor(
                controllerAnchorX,
                controllerAnchorY,
                controllerAnchorZ
            );
            updateHeadsetOverlap(
                controllerAnchorX, controllerAnchorY, controllerAnchorZ
            );
            if (controllerInertialTranslation.isReturningToCenter()) {
                controllerModelRenderer.setGhostTarget(
                    controllerGhostWorld[0],
                    controllerGhostWorld[1],
                    controllerGhostWorld[2]
                );
            } else {
                controllerModelRenderer.clearGhost();
            }

            activePointerOrigin[0] =
                controllerAnchorX
                    + adjustedControllerForward[0]
                        * CONTROLLER_EMITTER_FORWARD_METERS;
            activePointerOrigin[1] =
                controllerAnchorY
                    + adjustedControllerForward[1]
                        * CONTROLLER_EMITTER_FORWARD_METERS;
            activePointerOrigin[2] =
                controllerAnchorZ
                    + adjustedControllerForward[2]
                        * CONTROLLER_EMITTER_FORWARD_METERS;

            System.arraycopy(
                adjustedControllerForward,
                0,
                activePointerDirection,
                0,
                3
            );

            controllerModelRenderer
                .setYawCalibration(
                    controllerYawCalibrationRadians
                );
            controllerModelRenderer.setTrackedPose(
                controllerOrientationX,
                controllerOrientationY,
                controllerOrientationZ,
                controllerOrientationW
            );
            controllerModelRenderer.setButtonState(
                controllerTouchpadPressed,
                controllerHomePressed,
                controllerAppPressed,
                controllerVolumeUpPressed,
                controllerVolumeDownPressed
            );
            activeNativePointerKind =
                NativeModuleRuntime.POINTER_TRACKED_CONTROLLER;
            setActivePointerSource(
                "Tracked controller"
            );
            return true;
        }

        if (controllerInertialTranslation.isReturningToCenter()
            && !freshControllerPose) {
            controllerInertialTranslation.cancelReturnToCenter();
            controllerModelRenderer.clearGhost();
            ReverieLog.milestone(
                "VR_CONTROLLER",
                "Controller recenter interrupted by stale or invalid tracking."
            );
        }

        boolean useVirtualController =
            gamepadPointerAvailable
                && (
                    pointerMode
                        == VrPointerMode.CONTROLLER
                    || pointerMode
                        == VrPointerMode.AUTO
                );

        if (useVirtualController) {
            updateVirtualPointerAngles(
                frameNanos
            );

            float cosPitch =
                (float) Math.cos(
                    virtualPointerPitch
                );
            activePointerDirection[0] =
                (float) Math.sin(
                    virtualPointerYaw
                ) * cosPitch;
            activePointerDirection[1] =
                (float) Math.sin(
                    virtualPointerPitch
                );
            activePointerDirection[2] =
                -(float) Math.cos(
                    virtualPointerYaw
                ) * cosPitch;
            normalizeDirection(
                activePointerDirection
            );

            resolveControllerViewAnchor(frameNanos);
            float controllerAnchorX =
                controllerAnchorWorld[0];
            float controllerAnchorY =
                controllerAnchorWorld[1];
            float controllerAnchorZ =
                controllerAnchorWorld[2];

            controllerModelRenderer.setAnchor(
                controllerAnchorX,
                controllerAnchorY,
                controllerAnchorZ
            );
            controllerModelRenderer.clearGhost();

            activePointerOrigin[0] =
                controllerAnchorX
                    + activePointerDirection[0]
                        * CONTROLLER_EMITTER_FORWARD_METERS;
            activePointerOrigin[1] =
                controllerAnchorY
                    + activePointerDirection[1]
                        * CONTROLLER_EMITTER_FORWARD_METERS;
            activePointerOrigin[2] =
                controllerAnchorZ
                    + activePointerDirection[2]
                        * CONTROLLER_EMITTER_FORWARD_METERS;

            controllerModelRenderer.setVirtualAim(
                virtualPointerYaw,
                virtualPointerPitch
            );
            controllerModelRenderer.setButtonState(
                false,
                false,
                false,
                false,
                false
            );
            activeNativePointerKind =
                NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER;
            setActivePointerSource(
                "Virtual gamepad"
            );
            return true;
        }

        virtualPointerLastFrameNanos = 0L;
        controllerHeadsetOverlap = false;
        controllerModelRenderer.hide();
        controllerModelRenderer.clearGhost();

        if (pointerMode
            == VrPointerMode.CONTROLLER) {
            activeNativePointerKind =
                NativeModuleRuntime.POINTER_NONE;
            setActivePointerSource(
                "Controller unavailable"
            );
            activePointerOrigin[0] = 0.0f;
            activePointerOrigin[1] = 0.0f;
            activePointerOrigin[2] = 0.0f;
            activePointerDirection[0] = 0.0f;
            activePointerDirection[1] = 0.0f;
            activePointerDirection[2] = 0.0f;
            return false;
        }

        activeNativePointerKind =
            NativeModuleRuntime.POINTER_NONE;
        setActivePointerSource("Gaze");
        activePointerOrigin[0] =
            headInertialTranslation.x()
                + (mode == MODE_NATIVE ? nativeLocomotion.x() : 0.0f);
        activePointerOrigin[1] =
            headInertialTranslation.y();
        activePointerOrigin[2] =
            headInertialTranslation.z()
                + (mode == MODE_NATIVE ? nativeLocomotion.z() : 0.0f);
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

    private void updateHeadsetOverlap(float x, float y, float z) {
        // A diagnostic collision probe, not physical head/hand tracking.
        // No hidden motion correction or gameplay input suppression.
        boolean overlaps = playerHeadRig.intersectsSphere(
            x, y, z, 0.045f
        );
        if (overlaps != controllerHeadsetOverlap) {
            controllerHeadsetOverlap = overlaps;
            if (ReverieLog.isDevelopment()) {
                ReverieLog.dev(
                    "VR_PLAYER_RIG",
                    overlaps ? "Controller proxy entered headset collider."
                        : "Controller proxy left headset collider."
                );
            }
        }
    }

    private void resolveControllerViewAnchor(
        long frameNanos
    ) {
        /*
         * Daydream has rotation but no absolute hand position. Place the
         * model at a bounded torso-side proxy, not at an offset rotated by
         * headset pitch/roll. Follow sustained turns in yaw with a deadband;
         * looking around briefly must not drag the user's hand around.
         * The tracked quaternion still owns aim independently.
         */
        float handX =
            preferences.isControllerLeftHanded()
                ? -CONTROLLER_ANCHOR_X
                : CONTROLLER_ANCHOR_X;
        float offsetX =
            handX + controllerInertialTranslation.x();
        float offsetY =
            CONTROLLER_ANCHOR_Y
                + controllerInertialTranslation.y();
        float offsetZ =
            CONTROLLER_ANCHOR_Z
                + controllerInertialTranslation.z();

        controllerAnchorView[0] = 0.0f;
        controllerAnchorView[1] = 0.0f;
        controllerAnchorView[2] = 0.0f;
        controllerAnchorView[3] = 1.0f;

        if (!Matrix.invertM(
                inverseAdjustedHeadView,
                0,
                adjustedHeadView,
                0
            )) {
            controllerBodyAnchor.reset();
            controllerAnchorLastFrameNanos = 0L;
            controllerAnchorWorld[0] =
                offsetX + headInertialTranslation.x()
                    + (mode == MODE_NATIVE ? nativeLocomotion.x() : 0.0f);
            controllerAnchorWorld[1] =
                offsetY + headInertialTranslation.y();
            controllerAnchorWorld[2] =
                offsetZ + headInertialTranslation.z()
                    + (mode == MODE_NATIVE ? nativeLocomotion.z() : 0.0f);
            controllerAnchorWorld[3] = 1.0f;
            controllerGhostWorld[0] =
                handX + headInertialTranslation.x()
                    + (mode == MODE_NATIVE ? nativeLocomotion.x() : 0.0f);
            controllerGhostWorld[1] =
                CONTROLLER_ANCHOR_Y + headInertialTranslation.y();
            controllerGhostWorld[2] =
                CONTROLLER_ANCHOR_Z + headInertialTranslation.z()
                    + (mode == MODE_NATIVE ? nativeLocomotion.z() : 0.0f);
            controllerGhostWorld[3] = 1.0f;
            return;
        }

        Matrix.multiplyMV(
            controllerAnchorWorld,
            0,
            inverseAdjustedHeadView,
            0,
            controllerAnchorView,
            0
        );
        Matrix.multiplyMV(
            controllerAnchorForwardWorld,
            0,
            inverseAdjustedHeadView,
            0,
            controllerAnchorForwardView,
            0
        );

        float heading =
            VrHeadingMath.yawFromForward(
                controllerAnchorForwardWorld[0],
                controllerAnchorForwardWorld[2]
            );
        float dt = controllerAnchorLastFrameNanos <= 0L
            ? 0.0f
            : clamp(
                (frameNanos - controllerAnchorLastFrameNanos)
                    / 1000000000.0f,
                0.0f,
                0.05f
            );
        controllerAnchorLastFrameNanos = frameNanos;
        controllerBodyAnchor.update(heading, dt);
        controllerBodyAnchor.place(
            controllerAnchorWorld[0],
            controllerAnchorWorld[1],
            controllerAnchorWorld[2],
            handX,
            CONTROLLER_ANCHOR_Y,
            CONTROLLER_ANCHOR_Z,
            controllerGhostWorld
        );
        controllerBodyAnchor.place(
            controllerAnchorWorld[0],
            controllerAnchorWorld[1],
            controllerAnchorWorld[2],
            offsetX,
            offsetY,
            offsetZ,
            controllerAnchorWorld
        );
    }

    private void requestSoftControllerRecenter() {
        if (controllerInertialTranslation.isReturningToCenter()
            || !controllerConnected
            || !controllerPoseValid) {
            return;
        }
        // Full 3D shake target: the headset's current forward, pitch,
        // and roll, expressed in the same adjusted world as the pointer.
        // Reject stale controller pose; retain the existing calibration
        // if the headset transform or quaternion is invalid.
        long now = System.nanoTime();
        if (hasFreshControllerPose(now)
                && ControllerOrientationCalibration.compute(
                    adjustedHeadView,
                    controllerOrientationX,
                    controllerOrientationY,
                    controllerOrientationZ,
                    controllerOrientationW,
                    controllerOrientationCorrection
                )) {
            fullControllerOrientationCalibration = true;
            controllerYawCalibrationRadians = 0.0f;
            controllerModelRenderer.setYawCalibration(0.0f);
            controllerModelRenderer.setOrientationCorrection(
                controllerOrientationCorrection
            );
        }
        controllerInertialTranslation.beginReturnToCenter();
        controllerRecenterFeedbackActive = true;
        host.onControllerSpringRecenterRequested();
        // Preserve tracked quaternion, live motion and gravity reference.
        ReverieLog.milestone(
            "VR_CONTROLLER",
            "Controller soft recenter started; offset meters="
                + controllerRecenterOffsetLength()
        );
    }

    private float controllerRecenterOffsetLength() {
        float x = controllerInertialTranslation.x();
        float y = controllerInertialTranslation.y();
        float z = controllerInertialTranslation.z();
        return (float) Math.sqrt(x * x + y * y + z * z);
    }

    private void updateControllerInertialTranslation(
        long frameNanos
    ) {
        if (controllerInertialLastFrameNanos <= 0L) {
            controllerInertialLastFrameNanos =
                frameNanos;
            return;
        }

        float deltaSeconds =
            clamp(
                (
                    frameNanos
                        - controllerInertialLastFrameNanos
                ) / 1000000000.0f,
                0.0f,
                0.05f
            );
        controllerInertialLastFrameNanos =
            frameNanos;

        long sampleAge =
            frameNanos
                - controllerAccelerationAtNanos;
        boolean freshSample =
            controllerConnected
                && controllerPoseValid
                && controllerAccelerationAtNanos > 0L
                && sampleAge >= 0L
                && sampleAge <= 350000000L;

        if (!freshSample) {
            controllerInertialTranslation.update(
                0.0f,
                0.0f,
                0.0f,
                deltaSeconds
            );
            return;
        }

        float accelX = controllerAccelerationX;
        float accelY = controllerAccelerationY;
        float accelZ = controllerAccelerationZ;
        float magnitude =
            (float) Math.sqrt(
                accelX * accelX
                    + accelY * accelY
                    + accelZ * accelZ
            );
        if (!Float.isFinite(magnitude)
            || magnitude < 0.05f) {
            controllerInertialTranslation.update(
                0.0f,
                0.0f,
                0.0f,
                deltaSeconds
            );
            return;
        }

        /*
         * Physical Daydream packets report acceleration in g, while the
         * companion-phone sensor route reports Android m/s^2. Normalize both
         * without requiring provider-specific rendering code.
         */
        float accelerationScale =
            magnitude < 4.0f
                ? STANDARD_GRAVITY_METERS_PER_SECOND_SQUARED
                : 1.0f;

        quaternionRotateVector(
            controllerOrientationX,
            controllerOrientationY,
            controllerOrientationZ,
            controllerOrientationW,
            accelX * accelerationScale,
            accelY * accelerationScale,
            accelZ * accelerationScale,
            controllerAccelerationWorld
        );
        rotateYaw(
            controllerAccelerationWorld,
            -controllerYawCalibrationRadians,
            adjustedControllerAccelerationWorld
        );

        if (!controllerGravityInitialized) {
            System.arraycopy(
                adjustedControllerAccelerationWorld,
                0,
                controllerGravityWorld,
                0,
                3
            );
            controllerGravityInitialized = true;
            return;
        }

        float gravityBlend =
            clamp(
                deltaSeconds * 1.4f,
                0.01f,
                0.08f
            );
        for (int axis = 0; axis < 3; axis++) {
            controllerGravityWorld[axis] +=
                (
                    adjustedControllerAccelerationWorld[axis]
                        - controllerGravityWorld[axis]
                ) * gravityBlend;
            controllerAccelerationWorld4[axis] =
                adjustedControllerAccelerationWorld[axis]
                    - controllerGravityWorld[axis];
        }
        controllerAccelerationWorld4[3] = 0.0f;

        Matrix.multiplyMV(
            controllerAccelerationView4,
            0,
            adjustedHeadView,
            0,
            controllerAccelerationWorld4,
            0
        );

        controllerInertialTranslation.update(
            controllerAccelerationView4[0],
            controllerAccelerationView4[1],
            controllerAccelerationView4[2],
            deltaSeconds
        );
    }

    private boolean hasFreshControllerPose(
        long frameNanos
    ) {
        long poseAgeNanos =
            frameNanos
                - controllerPoseReceivedAtNanos;
        return controllerConnected
            && controllerPoseValid
            && controllerPoseReceivedAtNanos > 0L
            && poseAgeNanos >= 0L
            && poseAgeNanos <= 750000000L;
    }

    private boolean recenterOnHeadset(
        long frameNanos
    ) {
        float headsetYaw =
            VrHeadingMath.yawFromForward(
                headForward[0],
                headForward[2]
            );
        if (!Float.isFinite(headsetYaw)) {
            return false;
        }

        setShellHeading(
            headsetYaw,
            hasFreshControllerPose(
                frameNanos
            )
        );
        shellHeadingInitialized = true;
        initialHeadingStableFrames =
            INITIAL_HEADING_STABLE_FRAME_TARGET;
        ReverieLog.milestone(
            "VR_HEADING",
            "Shell forward centered on headset pointing direction."
        );
        return true;
    }

    private boolean recenterOnController(
        long frameNanos
    ) {
        if (!hasFreshControllerPose(
                frameNanos
            )) {
            return false;
        }

        quaternionForward(
            controllerOrientationX,
            controllerOrientationY,
            controllerOrientationZ,
            controllerOrientationW,
            controllerForward
        );
        ControllerOrientationCalibration.rotate(
            controllerOrientationCorrection,
            fullControllerOrientationCalibration,
            controllerForward,
            adjustedControllerForward
        );
        rotateYaw(
            adjustedControllerForward,
            -controllerYawCalibrationRadians,
            adjustedControllerForward
        );
        normalizeDirection(
            adjustedControllerForward
        );

        float controllerYaw =
            VrHeadingMath.yawFromForward(
                adjustedControllerForward[0],
                adjustedControllerForward[2]
            );
        if (!Float.isFinite(controllerYaw)) {
            return false;
        }

        setShellHeading(
            yawOffsetRadians
                + controllerYaw,
            true
        );
        ReverieLog.milestone(
            "VR_HEADING",
            "Shell forward centered on tracked controller pointing direction."
        );
        return true;
    }

    private void updateInitialShellHeading(
        long frameNanos
    ) {
        float currentYaw =
            VrHeadingMath.yawFromForward(
                headForward[0],
                headForward[2]
            );
        if (!Float.isFinite(currentYaw)) {
            return;
        }

        if (initialHeadingStartedNanos <= 0L) {
            initialHeadingStartedNanos =
                frameNanos;
        }

        if (Float.isFinite(
                initialHeadingCandidateYaw
            )
            && VrHeadingMath.angularDistance(
                    currentYaw,
                    initialHeadingCandidateYaw
                )
                <= INITIAL_HEADING_STABLE_DELTA_RADIANS) {
            initialHeadingStableFrames++;
        } else {
            initialHeadingStableFrames = 1;
        }
        initialHeadingCandidateYaw =
            currentYaw;

        setShellHeading(
            currentYaw,
            hasFreshControllerPose(
                frameNanos
            )
        );

        boolean stable =
            initialHeadingStableFrames
                >= INITIAL_HEADING_STABLE_FRAME_TARGET;
        boolean timedOut =
            frameNanos
                    - initialHeadingStartedNanos
                >= INITIAL_HEADING_SETTLE_TIMEOUT_NANOS;
        if (stable || timedOut) {
            shellHeadingInitialized = true;
            ReverieLog.milestone(
                "VR_HEADING",
                stable
                    ? "Initial shell forward locked after stable headset heading acquisition."
                    : "Initial shell forward locked after heading settle timeout."
            );
        }
    }

    private void captureOrientationMenuHeading() {
        float headsetYaw =
            VrHeadingMath.yawFromForward(
                headForward[0],
                headForward[2]
            );
        float relativeYaw =
            VrHeadingMath.relativeYaw(
                headsetYaw,
                yawOffsetRadians
            );
        orientationMenuYawRadians =
            Float.isFinite(relativeYaw)
                ? relativeYaw
                : 0.0f;

        // Per-frame HUD attachment must not emit a log every frame.
    }

    private void setShellHeading(
        float requestedYawRadians,
        boolean preserveControllerDirection
    ) {
        float nextYaw =
            wrapAngle(
                requestedYawRadians
            );
        float delta =
            wrapAngle(
                nextYaw
                    - yawOffsetRadians
            );

        yawOffsetRadians = nextYaw;
        controllerBodyAnchor.reset();
        controllerAnchorLastFrameNanos = 0L;
        if (preserveControllerDirection) {
            controllerYawCalibrationRadians =
                wrapAngle(
                    controllerYawCalibrationRadians
                        + delta
                );
        }

        controllerModelRenderer
            .setYawCalibration(
                controllerYawCalibrationRadians
            );

        virtualPointerYaw = 0.0f;
        virtualPointerPitch = 0.0f;
        virtualPointerLastFrameNanos = 0L;
        headInertialTranslation.reset();
        headInertialLastFrameNanos = 0L;
        textureDirty = true;
    }

    private void updateHeadInertialTranslation(
        long frameNanos
    ) {
        if (headInertialLastFrameNanos <= 0L) {
            headInertialLastFrameNanos =
                frameNanos;
            return;
        }

        float deltaSeconds =
            clamp(
                (
                    frameNanos
                        - headInertialLastFrameNanos
                ) / 1000000000.0f,
                0.0f,
                0.05f
            );
        headInertialLastFrameNanos =
            frameNanos;

        long sampleAge =
            frameNanos
                - headLinearAccelerationAtNanos;
        boolean freshSample =
            headLinearAccelerationAtNanos > 0L
                && sampleAge >= 0L
                && sampleAge
                    <= 250000000L;

        if (!freshSample) {
            headInertialTranslation.update(
                0.0f,
                0.0f,
                0.0f,
                deltaSeconds
            );
            return;
        }

        float localX =
            headLinearAccelerationX;
        float localY =
            headLinearAccelerationY;
        float localZ =
            headLinearAccelerationZ;

        headMotionWorld[0] =
            rawHeadView[0] * localX
                + rawHeadView[1] * localY
                + rawHeadView[2] * localZ;
        headMotionWorld[1] =
            rawHeadView[4] * localX
                + rawHeadView[5] * localY
                + rawHeadView[6] * localZ;
        headMotionWorld[2] =
            rawHeadView[8] * localX
                + rawHeadView[9] * localY
                + rawHeadView[10] * localZ;

        rotateYaw(
            headMotionWorld,
            -yawOffsetRadians,
            adjustedHeadMotionWorld
        );

        headInertialTranslation.update(
            adjustedHeadMotionWorld[0],
            adjustedHeadMotionWorld[1],
            adjustedHeadMotionWorld[2],
            deltaSeconds
        );
    }

    private void setActivePointerSource(
        String source
    ) {
        String safe =
            source == null
                || source.trim().isEmpty()
                ? "Unknown"
                : source.trim();
        if (safe.equals(activePointerSource)) {
            return;
        }

        activePointerSource = safe;
        textureDirty = true;
    }

    private void updateVirtualPointerAngles(
        long frameNanos
    ) {
        if (virtualPointerLastFrameNanos <= 0L) {
            virtualPointerLastFrameNanos = frameNanos;
            return;
        }

        float deltaSeconds =
            Math.max(
                0.0f,
                Math.min(
                    0.05f,
                    (frameNanos
                        - virtualPointerLastFrameNanos)
                        / 1000000000.0f
                )
            );
        virtualPointerLastFrameNanos = frameNanos;

        float horizontal =
            applyPointerDeadzone(
                virtualPointerAxisX
            );
        float vertical =
            applyPointerDeadzone(
                virtualPointerAxisY
            );
        float angularRate =
            (float) Math.toRadians(95.0);

        virtualPointerYaw +=
            horizontal
                * angularRate
                * deltaSeconds;
        virtualPointerPitch -=
            vertical
                * angularRate
                * deltaSeconds;
        clampVirtualPointerAngles();
    }

    private void clampVirtualPointerAngles() {
        float yawLimit =
            (float) Math.toRadians(65.0);
        float pitchLimit =
            (float) Math.toRadians(45.0);

        virtualPointerYaw =
            clamp(
                virtualPointerYaw,
                -yawLimit,
                yawLimit
            );
        virtualPointerPitch =
            clamp(
                virtualPointerPitch,
                -pitchLimit,
                pitchLimit
            );
    }

    private static float applyPointerDeadzone(
        float value
    ) {
        float magnitude =
            Math.abs(value);
        if (magnitude <= 0.16f) {
            return 0.0f;
        }

        float normalized =
            (magnitude - 0.16f)
                / 0.84f;
        return Math.copySign(
            normalized,
            value
        );
    }

    private UiRayHit calculateUiRayHit(
        float[] origin,
        float[] direction
    ) {
        if (origin == null
            || origin.length < 3
            || direction == null
            || direction.length < 3) {
            return UiRayHit.miss();
        }

        if (orientationMenuVisible) {
            rotateYaw(
                origin,
                -orientationMenuYawRadians,
                orientationMenuRayOrigin
            );
            rotateYaw(
                direction,
                -orientationMenuYawRadians,
                orientationMenuRayDirection
            );
            return hitPanel(
                orientationMenuRayOrigin,
                orientationMenuRayDirection,
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

        if (mode == MODE_KEYBOARD) {
            // The text-entry texture is laid flat on the existing dais;
            // selection must intersect the same horizontal geometry.
            final float dy = direction[1];
            if (Math.abs(dy) < 0.00001f) return UiRayHit.miss();
            final float distance = (-1.065f - origin[1]) / dy;
            if (!Float.isFinite(distance) || distance <= 0.0f)
                return UiRayHit.miss();
            final float x = origin[0] + distance * direction[0];
            final float z = origin[2] + distance * direction[2];
            if (x < -1.73f || x > 1.73f ||
                z < -4.08f || z > -1.36f)
                return UiRayHit.miss();
            final float px = (x + 1.73f) / 3.46f * TEXTURE_WIDTH;
            final float py = (z + 4.08f) / 2.72f * TEXTURE_HEIGHT;
            return new UiRayHit(buttonAtPixel(px, py), distance);
        }

        if (mode == MODE_HOME) {
            UiRayHit best =
                hitTiltedPanel(
                    origin,
                    direction,
                    HOME_LEFT_WORLD_LEFT,
                    HOME_LEFT_WORLD_RIGHT,
                    HOME_WORLD_BOTTOM,
                    HOME_WORLD_TOP,
                    HOME_LEFT_WORLD_Z_OUTER,
                    HOME_LEFT_WORLD_Z_INNER,
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
                hitTiltedPanel(
                    origin,
                    direction,
                    HOME_RIGHT_WORLD_LEFT,
                    HOME_RIGHT_WORLD_RIGHT,
                    HOME_WORLD_BOTTOM,
                    HOME_WORLD_TOP,
                    HOME_RIGHT_WORLD_Z_INNER,
                    HOME_RIGHT_WORLD_Z_OUTER,
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

    private UiRayHit hitTiltedPanel(
        float[] origin,
        float[] direction,
        float worldLeft,
        float worldRight,
        float worldBottom,
        float worldTop,
        float leftZ,
        float rightZ,
        int pixelLeft,
        int pixelRight,
        int pixelTop,
        int pixelBottom
    ) {
        float width =
            worldRight - worldLeft;
        if (Math.abs(width) < 0.0001f) {
            return UiRayHit.miss();
        }

        float zSlope =
            (rightZ - leftZ)
                / width;
        float denominator =
            direction[2]
                - zSlope
                    * direction[0];
        if (Math.abs(denominator)
            < 0.00001f) {
            return UiRayHit.miss();
        }

        float numerator =
            leftZ
                + zSlope
                    * (
                        origin[0]
                            - worldLeft
                    )
                - origin[2];
        float hitDistance =
            numerator
                / denominator;
        if (!Float.isFinite(hitDistance)
            || hitDistance <= 0.0f) {
            return UiRayHit.miss();
        }

        float hitX =
            origin[0]
                + direction[0]
                    * hitDistance;
        float hitY =
            origin[1]
                + direction[1]
                    * hitDistance;

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
                        / width
                ) * (
                    pixelRight
                        - pixelLeft
                );
        float py =
            pixelTop
                + (
                    (worldTop - hitY)
                        / (
                            worldTop
                                - worldBottom
                        )
                ) * (
                    pixelBottom
                        - pixelTop
                );

        return new UiRayHit(
            buttonAtPixel(
                px,
                py
            ),
            hitDistance
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
                && py <= rect[3]
                && isButtonEnabled(index)) {
                return index;
            }
        }
        return -1;
    }

    private boolean isButtonEnabled(
        int index
    ) {
        if (index < 0) {
            return false;
        }

        if (orientationMenuVisible) {
            if (quickMenuSettingsVisible) {
                return index
                    < QUICK_SETTINGS_BUTTONS.length;
            }
            if (index == 1) {
                return hasFreshControllerPose(
                    System.nanoTime()
                );
            }
            return index
                < ORIENTATION_MENU_BUTTONS.length;
        }

        if (mode == MODE_HOME) {
            if (index == 2) {
                return NativeModuleRuntime.isAvailable()
                    && !nativeModules.isEmpty();
            }
            if (!BuildConfig.UPDATE_CHANNEL_ENABLED
                && index == 12) {
                return false;
            }
            if (index == 13) {
                return hasResumeTarget();
            }
            if (index == 14 || index == 15 || index == 16) {
                return true;
            }
            return index < HOME_BUTTONS.length;
        }

        if (mode == MODE_KEYBOARD) {
            return index < KEYBOARD_BUTTONS.length;
        }

        if (mode == MODE_NATIVE_LIBRARY) {
            int count = nativeModules.size();
            if (index >= 0 && index <= 2) {
                int moduleIndex =
                    nativeLibraryPage * 3 + index;
                return moduleIndex < count;
            }

            if (index == 3) {
                int pageCount =
                    Math.max(
                        1,
                        (count + 2) / 3
                    );
                return pageCount > 1;
            }

            return index == 4;
        }

        if (mode == MODE_DOS_LIBRARY) {
            if (index >= 0 && index <= 2) {
                int moduleIndex =
                    dosLibraryPage * 3 + index;
                return moduleIndex < dosModuleIds.length
                    || (
                        dosModuleIds.length == 0
                            && index == 0
                    );
            }

            if (index == 3) {
                int pageCount =
                    Math.max(
                        1,
                        (dosModuleIds.length + 2) / 3
                    );
                return pageCount > 1;
            }

            return index == 4;
        }

        int[][] buttons = activeButtons();
        return index < buttons.length;
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

    private static void quaternionRotateVector(
        float x,
        float y,
        float z,
        float w,
        float vectorX,
        float vectorY,
        float vectorZ,
        float[] destination
    ) {
        float lengthSquared =
            x * x + y * y + z * z + w * w;
        if (!Float.isFinite(lengthSquared)
            || lengthSquared < 0.0001f) {
            destination[0] = vectorX;
            destination[1] = vectorY;
            destination[2] = vectorZ;
            return;
        }

        float inverseLength =
            1.0f
                / (float) Math.sqrt(lengthSquared);
        x *= inverseLength;
        y *= inverseLength;
        z *= inverseLength;
        w *= inverseLength;

        float xx = x * x;
        float yy = y * y;
        float zz = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        destination[0] =
            (1.0f - 2.0f * (yy + zz)) * vectorX
                + 2.0f * (xy - wz) * vectorY
                + 2.0f * (xz + wy) * vectorZ;
        destination[1] =
            2.0f * (xy + wz) * vectorX
                + (1.0f - 2.0f * (xx + zz)) * vectorY
                + 2.0f * (yz - wx) * vectorZ;
        destination[2] =
            2.0f * (xz - wy) * vectorX
                + 2.0f * (yz + wx) * vectorY
                + (1.0f - 2.0f * (xx + yy)) * vectorZ;
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

        if (!isButtonEnabled(hoveredButton)) {
            host.onUiActionRejected();
            return;
        }

        if (orientationMenuVisible) {
            handleOrientationMenuSelection(
                hoveredButton
            );
            hoveredButton = -1;
            textureDirty = true;
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
                        nativeLibraryPage = 0;
                        mode = MODE_NATIVE_LIBRARY;
                    } else {
                        host.onUiActionRejected();
                    }
                    break;
                case 3:
                    mode = MODE_ENVIRONMENT;
                    break;
                case 4:
                    controllerTrainingReturn = false;
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
                case 11:
                    if (BuildConfig.UPDATE_CHANNEL_ENABLED) {
                        host.onUpdateCheckRequested();
                    } else {
                        host.onBluetoothSettingsRequested();
                    }
                    return;
                case 12:
                    host.onBluetoothSettingsRequested();
                    return;
                case 13:
                    if (resumeLastActivity()) {
                        hoveredButton = -1;
                        return;
                    }
                    host.onUiActionRejected();
                    break;
                case 14:
                    mode = MODE_CONTROLLER;
                    break;
                case 15:
                    mode = MODE_KEYBOARD;
                    break;
                case 16:
                    if (modelViewer.hasModel()) {
                        if (modelViewer.load()) mode = MODE_MODEL_VIEWER;
                        else host.onUiActionRejected();
                    } else {
                        host.onModelImportRequested();
                        return;
                    }
                    break;
                default:
                    break;
            }
        } else if (mode == MODE_CONTROLLER) {
            handleControllerSelection(hoveredButton);
        } else if (mode == MODE_KEYBOARD) {
            int key = hoveredButton;
            if (key == 50) {
                host.onVrKeyboardTextCommitted(keyboardEditor.value());
                mode = MODE_HOME;
            } else if (!keyboardEditor.press(key / 10, key % 10)) {
                host.onUiActionRejected();
            }
        } else if (mode == MODE_NATIVE_LIBRARY) {
            handleNativeLibrarySelection(hoveredButton);
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

    private void handleOrientationMenuSelection(
        int button
    ) {
        if (quickMenuSettingsVisible) {
            handleQuickSettingsSelection(button);
            return;
        }

        switch (button) {
            case 0:
                if (recenterOnHeadset(
                        System.nanoTime()
                    )) {
                    closeOrientationMenu();
                } else {
                    host.onUiActionRejected();
                }
                break;

            case 1:
                if (recenterOnController(
                        System.nanoTime()
                    )) {
                    closeOrientationMenu();
                } else {
                    host.onUiActionRejected();
                }
                break;

            case 2:
                host.onBrightnessAdjustRequested(-1);
                break;

            case 3:
                host.onBrightnessAdjustRequested(1);
                break;

            case 4:
                closeOrientationMenu();
                backRequested.set(true);
                break;

            case 5:
                returnToHomeFromQuickMenu();
                break;

            case 6:
                returnToHomeFromQuickMenu();
                host.onExitToPhoneRequested();
                break;

            case 7:
                quickMenuSettingsVisible = true;
                hoveredButton = -1;
                textureDirty = true;
                break;

            default:
                host.onUiActionRejected();
                break;
        }
    }

    private void handleQuickSettingsSelection(
        int button
    ) {
        switch (button) {
            case 0:
                preferences.setBatteryHudEnabled(
                    !preferences.isBatteryHudEnabled()
                );
                hudTextureDirty = true;
                break;

            case 1:
                preferences.setShowPercentagesEnabled(
                    !preferences.isShowPercentagesEnabled()
                );
                hudTextureDirty = true;
                break;

            case 2:
                preferences.setLookUpRevealEnabled(
                    !preferences.isLookUpRevealEnabled()
                );
                break;

            case 3:
                preferences.setVrPointerMode(
                    preferences
                        .getVrPointerMode()
                        .next()
                );
                break;

            case 4:
                host.onVolumeAdjustRequested(-1);
                break;

            case 5:
                host.onVolumeAdjustRequested(1);
                break;

            case 6:
                uiScale = clamp(
                    uiScale - 0.05f,
                    0.75f,
                    1.50f
                );
                preferences.setUiScale(uiScale);
                break;

            case 7:
                uiScale = clamp(
                    uiScale + 0.05f,
                    0.75f,
                    1.50f
                );
                preferences.setUiScale(uiScale);
                break;

            case 8:
                quickMenuSettingsVisible = false;
                break;

            case 9:
                closeOrientationMenu();
                return;

            default:
                host.onUiActionRejected();
                return;
        }

        hoveredButton = -1;
        textureDirty = true;
    }

    private void returnToHomeFromQuickMenu() {
        if (mode == MODE_VIDEO) {
            host.onVideoStopRequested();
            videoSeekRequestedMillis.set(0);
        } else if (
            mode == MODE_DOS
                || mode == MODE_DOS_OVERLAY
                || mode == MODE_DOS_BINDINGS
        ) {
            activeDosModuleName = "";
            host.onDosStopRequested();
            dosExitRequested.set(false);
        } else if (mode == MODE_NATIVE) {
            if (nativeSurfaceReady
                && nativeModuleRuntime != null) {
                nativeModuleRuntime.releaseSurface();
            }
            nativeSurfaceReady = false;
            host.onNativeModuleStopRequested();
        }

        controllerTrainingReturn = false;
        backRequested.set(false);
        selectRequested.set(false);
        mode = MODE_HOME;
        closeOrientationMenu();
    }

    private void closeOrientationMenu() {
        orientationMenuVisible = false;
        quickMenuSettingsVisible = false;
        keyboardQuickMenuFocusActive = false;
        hoveredButton = -1;
        selectRequested.set(false);
        textureDirty = true;
    }

    private void handleNativeLibrarySelection(
        int button
    ) {
        int count = nativeModules.size();
        int pageCount =
            Math.max(
                1,
                (count + 2) / 3
            );

        if (button >= 0 && button <= 2) {
            int moduleIndex =
                nativeLibraryPage * 3 + button;

            if (moduleIndex < count) {
                NativeModuleRuntime.Descriptor module =
                    nativeModules.get(
                        moduleIndex
                    );

                if (module != null
                    && host.onNativeModulePlaybackRequested(
                        module.id
                    )) {
                    preferences.markLastActivityNative(
                        module.id,
                        module.displayName
                    );
                    nativeSurfaceReady = false;
                    mode = MODE_NATIVE;
                    hoveredButton = -1;
                    return;
                }
            }

            host.onUiActionRejected();
        } else if (button == 3) {
            if (pageCount > 1) {
                nativeLibraryPage =
                    (nativeLibraryPage + 1)
                        % pageCount;
            } else {
                host.onUiActionRejected();
            }
        } else if (button == 4) {
            mode = MODE_HOME;
        } else {
            host.onUiActionRejected();
        }

        hoveredButton = -1;
        textureDirty = true;
    }

    private void handleControllerSelection(
        int button
    ) {
        switch (button) {
            case 0:
                preferences.setVrPointerMode(
                    preferences
                        .getVrPointerMode()
                        .next()
                );
                break;

            case 1:
                requestRecenter();
                break;

            case 2:
                controllerTrainingReturn = true;
                mode = MODE_SETUP;
                setupStep = 1;
                preferences.setVrSetupStep(1);
                lastInputAction =
                    "Waiting for input";
                lastInputSource =
                    "Try your controller controls below";
                break;

            case 3:
                host.onControllerPairingRequested();
                return;

            case 4:
                host.onBluetoothSettingsRequested();
                return;

            case 5:
                preferences.setControllerLeftHanded(
                    !preferences.isControllerLeftHanded()
                );
                controllerBodyAnchor.reset();
                controllerAnchorLastFrameNanos = 0L;
                ReverieLog.milestone(
                    "VR_CONTROLLER",
                    preferences.isControllerLeftHanded()
                        ? "Controller anchor set to left hand."
                        : "Controller anchor set to right hand."
                );
                break;

            case 6:
                requestSoftControllerRecenter();
                break;

            case 7:
                mode = MODE_HOME;
                break;

            default:
                host.onUiActionRejected();
                return;
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
                preferences.markLastActivityMedia();
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
                preferences.markLastActivityDos(
                    ids[moduleIndex],
                    activeDosModuleName
                );
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
                if (controllerTrainingReturn) {
                    if (button == 0) {
                        controllerTrainingReturn = false;
                        mode = MODE_CONTROLLER;
                    } else if (button == 1) {
                        controllerTrainingReturn = false;
                        mode = MODE_HOME;
                    }
                } else if (button == 0 || button == 1) {
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
        if (orientationMenuVisible) {
            if (quickMenuSettingsVisible) {
                quickMenuSettingsVisible = false;
                hoveredButton = -1;
                textureDirty = true;
            } else {
                closeOrientationMenu();
            }
            return;
        }

        if (mode == MODE_HOME) {
            host.onExitToPhoneRequested();
            return;
        }

        if (mode == MODE_DOS_LIBRARY
            || mode == MODE_MEDIA_LIBRARY
            || mode == MODE_ENVIRONMENT
            || mode == MODE_CONTROLLER
            || mode == MODE_KEYBOARD
            || mode == MODE_NATIVE_LIBRARY) {
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

        if (controllerTrainingReturn
            && mode == MODE_SETUP
            && setupStep == 1) {
            controllerTrainingReturn = false;
            mode = MODE_CONTROLLER;
        } else if (setupStep > 0) {
            setupStep--;
            preferences.setVrSetupStep(setupStep);
        } else {
            mode = MODE_HOME;
        }

        hoveredButton = -1;
        textureDirty = true;
    }

    private int[][] activeButtons() {
        if (orientationMenuVisible) {
            return quickMenuSettingsVisible
                ? QUICK_SETTINGS_BUTTONS
                : ORIENTATION_MENU_BUTTONS;
        }

        if (mode == MODE_HOME) {
            return HOME_BUTTONS;
        }

        if (mode == MODE_CONTROLLER) {
            return CONTROLLER_BUTTONS;
        }
        if (mode == MODE_KEYBOARD) {
            return KEYBOARD_BUTTONS;
        }

        if (mode == MODE_NATIVE_LIBRARY) {
            return NATIVE_LIBRARY_BUTTONS;
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

        if (orientationMenuVisible) {
            canvas.drawColor(
                Color.TRANSPARENT,
                PorterDuff.Mode.CLEAR
            );
            paint.setColor(
                Color.argb(
                    238,
                    14,
                    19,
                    27
                )
            );
            canvas.drawRoundRect(
                190,
                116,
                834,
                650,
                30,
                30,
                paint
            );
            paint.setStyle(
                Paint.Style.STROKE
            );
            paint.setStrokeWidth(2.0f);
            paint.setColor(
                Color.argb(
                    180,
                    56,
                    214,
                    200
                )
            );
            canvas.drawRoundRect(
                192,
                118,
                832,
                648,
                28,
                28,
                paint
            );
            paint.setStyle(
                Paint.Style.FILL
            );
        } else if (mode == MODE_HOME) {
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

        if (orientationMenuVisible) {
            drawOrientationMenu(
                canvas,
                paint
            );
        } else if (mode == MODE_HOME) {
            drawHome(canvas, paint);
        } else if (mode == MODE_CONTROLLER) {
            drawControllerPanel(canvas, paint);
        } else if (mode == MODE_KEYBOARD) {
            drawKeyboard(canvas, paint);
        } else if (mode == MODE_NATIVE_LIBRARY) {
            drawNativeLibrary(canvas, paint);
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

    private void drawOrientationMenu(
        Canvas canvas,
        Paint paint
    ) {
        if (quickMenuSettingsVisible) {
            drawQuickSettingsMenu(canvas, paint);
            return;
        }

        paint.setFakeBoldText(true);
        paint.setColor(
            Color.rgb(
                56,
                214,
                200
            )
        );
        paint.setTextSize(
            38.0f * uiScale
        );
        canvas.drawText(
            "QUICK MENU",
            250,
            178,
            paint
        );
        paint.setFakeBoldText(false);

        paint.setColor(
            Color.rgb(
                210,
                220,
                230
            )
        );
        paint.setTextSize(
            20.0f * uiScale
        );
        canvas.drawText(
            "Forward, brightness and recovery stay shell-owned.",
            250,
            216,
            paint
        );
        canvas.drawText(
            "Use headset forward for desk or keyboard play.",
            250,
            242,
            paint
        );

        String controllerLabel =
            hasFreshControllerPose(
                System.nanoTime()
            )
                ? "CONTROLLER FWD"
                : "CONTROLLER N/A";

        drawButtons(
            canvas,
            paint,
            new String[] {
                "HEADSET FWD",
                controllerLabel,
                "BRIGHT -",
                "BRIGHT +",
                "BACK",
                "HOME",
                "EXIT PHONE",
                "SETTINGS"
            },
            ORIENTATION_MENU_BUTTONS
        );

        int phonePercent =
            phoneBattery.get();
        int controllerPercent =
            controllerBattery.get();
        long elapsedSeconds =
            Math.max(
                0L,
                (
                    System.nanoTime()
                        - vrSessionStartedNanos
                ) / 1000000000L
            );
        long elapsedMinutes =
            elapsedSeconds / 60L;
        long remainingSeconds =
            elapsedSeconds % 60L;

        PerformanceEnvironmentSnapshot environment =
            host.getPerformanceEnvironmentSnapshot();
        if (environment == null) {
            environment =
                PerformanceEnvironmentSnapshot
                    .unavailable();
        }

        FramePerformanceTracker.Snapshot performance =
            performanceTracker.snapshot();

        String phoneLabel =
            phonePercent >= 0
                ? phonePercent + "%"
                : "N/A";
        String controllerPowerLabel =
            controllerPercent >= 0
                ? controllerPercent + "%"
                : "N/A";
        String sessionLabel =
            String.format(
                Locale.US,
                "%d:%02d",
                elapsedMinutes,
                remainingSeconds
            );
        String batteryTempLabel =
            environment.batteryTemperatureTenthsC
                    == PerformanceEnvironmentSnapshot
                        .BATTERY_TEMPERATURE_UNAVAILABLE
                ? "N/A"
                : String.format(
                    Locale.US,
                    "%.1fC",
                    environment
                        .batteryTemperatureTenthsC
                        / 10.0
                );
        String performanceLabel =
            performance.sampleCount > 0
                ? String.format(
                    Locale.US,
                    "P95 %.1fms",
                    performance.p95Millis
                )
                : "P95 N/A";

        paint.setColor(
            Color.rgb(
                144,
                158,
                174
            )
        );
        paint.setTextSize(
            15.0f
                * Math.min(
                    uiScale,
                    1.2f
                )
        );
        canvas.drawText(
            "PHONE "
                + phoneLabel
                + " • CTRL "
                + controllerPowerLabel
                + " • SESSION "
                + sessionLabel,
            220,
            574,
            paint
        );
        canvas.drawText(
            "THERMAL "
                + PerformanceEnvironmentSnapshot
                    .thermalStatusLabel(
                        environment.thermalStatus
                    )
                    .toUpperCase(Locale.US)
                + " • BAT "
                + batteryTempLabel
                + " • "
                + performanceLabel,
            220,
            600,
            paint
        );
        canvas.drawText(
            "Menu/Start toggles this panel anywhere.",
            220,
            626,
            paint
        );
    }

    private void drawQuickSettingsMenu(
        Canvas canvas,
        Paint paint
    ) {
        paint.setFakeBoldText(true);
        paint.setColor(
            Color.rgb(
                56,
                214,
                200
            )
        );
        paint.setTextSize(
            36.0f * uiScale
        );
        canvas.drawText(
            "QUICK SETTINGS",
            250,
            178,
            paint
        );
        paint.setFakeBoldText(false);

        paint.setColor(
            Color.rgb(
                210,
                220,
                230
            )
        );
        paint.setTextSize(
            19.0f * uiScale
        );
        canvas.drawText(
            "Shell settings apply immediately; controller rocker stays game input.",
            220,
            218,
            paint
        );

        String hudLabel =
            preferences.isBatteryHudEnabled()
                ? "HUD: ON"
                : "HUD: OFF";
        String percentLabel =
            preferences.isShowPercentagesEnabled()
                ? "PERCENT: ON"
                : "PERCENT: OFF";
        String lookupLabel =
            preferences.isLookUpRevealEnabled()
                ? "LOOK-UP: ON"
                : "LOOK-UP: OFF";
        String pointerLabel =
            "PTR: "
                + preferences
                    .getVrPointerMode()
                    .displayName
                    .toUpperCase(Locale.US);

        drawButtons(
            canvas,
            paint,
            new String[] {
                hudLabel,
                percentLabel,
                lookupLabel,
                pointerLabel,
                "VOLUME -",
                "VOLUME +",
                "UI SCALE -",
                "UI SCALE +",
                "BACK QUICK",
                "CLOSE"
            },
            QUICK_SETTINGS_BUTTONS
        );

        paint.setColor(
            Color.rgb(
                144,
                158,
                174
            )
        );
        paint.setTextSize(
            16.0f
                * Math.min(
                    uiScale,
                    1.2f
                )
        );
        canvas.drawText(
            "UI SCALE "
                + Math.round(
                    uiScale * 100.0f
                )
                + "% • Back returns to Quick Menu.",
            220,
            570,
            paint
        );
        canvas.drawText(
            "Menu/Start closes the panel from either page.",
            220,
            604,
            paint
        );
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

        boolean controllerCardFocused =
            hoveredButton == 14;
        paint.setColor(
            controllerCardFocused
                ? Color.rgb(
                    25,
                    88,
                    116
                )
                : Color.rgb(
                    34,
                    45,
                    58
                )
        );
        canvas.drawRoundRect(
            292,
            220,
            718,
            326,
            18,
            18,
            paint
        );
        if (controllerCardFocused) {
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
                293,
                221,
                717,
                325,
                17,
                17,
                paint
            );
            paint.setStyle(
                Paint.Style.FILL
            );
        }
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

        paint.setColor(hoveredButton == 15
            ? Color.rgb(25, 88, 116) : Color.rgb(34, 45, 58));
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
            "Virtual Keyboard",
            314,
            383,
            paint
        );
        paint.setColor(Color.rgb(160, 176, 194));
        paint.setTextSize(17.0f * uiScale);
        canvas.drawText(
            "Type text, then copy it to Android clipboard",
            314,
            414,
            paint
        );

        paint.setColor(hoveredButton == 16 ? Color.rgb(25, 88, 116) : Color.rgb(34, 45, 58));
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
            "3D Model Viewer",
            314,
            511,
            paint
        );
        paint.setColor(Color.rgb(160, 176, 194));
        paint.setTextSize(17.0f * uiScale);
        canvas.drawText(
            modelViewer.hasModel() ? "Open imported OBJ in full VR"
                : "Select to import an OBJ model",
            314, 542, paint
        );
        paint.setColor(Color.rgb(126, 205, 221));
        paint.setTextSize(15.0f * uiScale);
        canvas.drawText(
            "Active: "
                + shorten(
                    activePointerSource,
                    28
                ),
            314,
            568,
            paint
        );

        if (hasResumeTarget()) {
            int[] resumeRect =
                HOME_BUTTONS[13];
            boolean resumeFocused =
                hoveredButton == 13;

            paint.setColor(
                resumeFocused
                    ? Color.rgb(
                        18,
                        126,
                        160
                    )
                    : Color.rgb(
                        21,
                        83,
                        112
                    )
            );
            canvas.drawRoundRect(
                resumeRect[0],
                resumeRect[1],
                resumeRect[2],
                resumeRect[3],
                16,
                16,
                paint
            );

            if (resumeFocused) {
                paint.setStyle(
                    Paint.Style.STROKE
                );
                paint.setStrokeWidth(3.0f);
                paint.setColor(
                    Color.rgb(
                        87,
                        229,
                        248
                    )
                );
                canvas.drawRoundRect(
                    resumeRect[0] + 1,
                    resumeRect[1] + 1,
                    resumeRect[2] - 1,
                    resumeRect[3] - 1,
                    15,
                    15,
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
            paint.setFakeBoldText(true);
            canvas.drawText(
                shorten(
                    resumeLabel(),
                    38
                ),
                resumeRect[0] + 18,
                resumeRect[1] + 33,
                paint
            );
            paint.setFakeBoldText(false);
        }

        paint.setColor(Color.rgb(126, 205, 221));
        paint.setTextSize(13.0f * uiScale);
        canvas.drawText(
            homeHelpText(hoveredButton),
            292,
            676,
            paint
        );

        paint.setColor(Color.rgb(143, 160, 179));
        paint.setTextSize(13.0f * uiScale);
        canvas.drawText(
            "Look or point • click to select",
            292,
            700,
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

        String[] rightLabels;
        int rightCount;
        if (BuildConfig.UPDATE_CHANNEL_ENABLED) {
            rightLabels =
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
                    "Recenter View",
                    "Check Updates",
                    "Android Bluetooth"
                };
            rightCount = 7;
        } else {
            rightLabels =
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
                    "Recenter View",
                    "Android Bluetooth"
                };
            rightCount = 6;
        }
        drawHomeButtons(
            canvas,
            paint,
            rightLabels,
            6,
            rightCount
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

    private String homeHelpText(
        int index
    ) {
        switch (index) {
            case 0:
                return "Open local video and floating-screen media controls.";
            case 1:
                return "Browse and launch imported DOS game modules.";
            case 2:
                return "Launch ReverieVR-native modules.";
            case 3:
                return "Change the persistent 3D home environment.";
            case 4:
                return "Comfort, calibration, and first-run setup controls.";
            case 5:
                return "Leave the headset shell and return to the phone interface.";
            case 6:
                return "Show or hide phone/controller battery bars in VR.";
            case 7:
                return "Show numeric battery percentages beside the HUD bars.";
            case 8:
                return "Reveal the HUD automatically when you look upward.";
            case 9:
                return "Cycle Auto, Gaze, and Controller pointer modes.";
            case 10:
                return "Recenter headset view and controller/virtual pointer aim.";
            case 11:
                return BuildConfig.UPDATE_CHANNEL_ENABLED
                    ? "Return to the phone and check GitHub for a signed ReverieVR update."
                    : "Return to the phone and open Android Bluetooth settings.";
            case 12:
                return "Return to the phone and open Android Bluetooth settings.";
            case 13:
                return "Resume the last valid media, DOS, or native activity.";
            case 14:
                return "Open controller status, pairing, pointer, and input tools.";
            default:
                return "Choose a destination or quick option.";
        }
    }

    private boolean hasResumeTarget() {
        String type =
            preferences.getLastActivityType();

        if (ReveriePreferences.LAST_ACTIVITY_MEDIA.equals(
                type
            )) {
            return preferences.hasSelectedVideo();
        }

        if (ReveriePreferences.LAST_ACTIVITY_DOS.equals(
                type
            )) {
            return findDosModuleIndex(
                preferences.getLastDosModuleId()
            ) >= 0;
        }

        if (ReveriePreferences.LAST_ACTIVITY_NATIVE.equals(
                type
            )) {
            return findNativeModule(
                preferences.getLastNativeModuleId()
            ) != null;
        }

        return false;
    }

    private String resumeLabel() {
        String type =
            preferences.getLastActivityType();

        if (ReveriePreferences.LAST_ACTIVITY_MEDIA.equals(
                type
            )
            && preferences.hasSelectedVideo()) {
            String name =
                preferences
                    .getSelectedVideoDisplayName();
            return "RESUME MEDIA"
                + (
                    name.trim().isEmpty()
                        ? ""
                        : " • " + shorten(name, 25)
                );
        }

        if (ReveriePreferences.LAST_ACTIVITY_DOS.equals(
                type
            )) {
            int index =
                findDosModuleIndex(
                    preferences.getLastDosModuleId()
                );
            if (index >= 0) {
                String name =
                    index < dosModuleNames.length
                        ? dosModuleNames[index]
                        : preferences
                            .getLastDosModuleName();
                return "RESUME DOS"
                    + (
                        name == null
                            || name.trim().isEmpty()
                            ? ""
                            : " • "
                                + shorten(
                                    name,
                                    27
                                )
                    );
            }
        }

        if (ReveriePreferences.LAST_ACTIVITY_NATIVE.equals(
                type
            )) {
            NativeModuleRuntime.Descriptor module =
                findNativeModule(
                    preferences
                        .getLastNativeModuleId()
                );
            if (module != null) {
                String name =
                    module.displayName == null
                        || module.displayName.trim().isEmpty()
                        ? preferences
                            .getLastNativeModuleName()
                        : module.displayName;
                return "RESUME NATIVE"
                    + (
                        name == null
                            || name.trim().isEmpty()
                            ? ""
                            : " • "
                                + shorten(
                                    name,
                                    23
                                )
                    );
            }
        }

        return "RESUME";
    }

    private boolean resumeLastActivity() {
        String type =
            preferences.getLastActivityType();

        if (ReveriePreferences.LAST_ACTIVITY_MEDIA.equals(
                type
            )
            && preferences.hasSelectedVideo()) {
            videoRenderer.setProjection(
                preferences.getVideoProjection()
            );
            videoSeekRequestedMillis.set(0);
            mode = MODE_VIDEO;
            host.onVideoPlaybackRequested();
            return true;
        }

        if (ReveriePreferences.LAST_ACTIVITY_DOS.equals(
                type
            )) {
            int index =
                findDosModuleIndex(
                    preferences.getLastDosModuleId()
                );
            if (index >= 0
                && host.onDosPlaybackRequested(
                    dosModuleIds[index]
                )) {
                activeDosModuleName =
                    index < dosModuleNames.length
                        ? dosModuleNames[index]
                        : "DOS session";
                dosExitRequested.set(false);
                mode = MODE_DOS;
                return true;
            }
            return false;
        }

        if (ReveriePreferences.LAST_ACTIVITY_NATIVE.equals(
                type
            )) {
            NativeModuleRuntime.Descriptor module =
                findNativeModule(
                    preferences
                        .getLastNativeModuleId()
                );
            if (module != null
                && host.onNativeModulePlaybackRequested(
                    module.id
                )) {
                nativeSurfaceReady = false;
                mode = MODE_NATIVE;
                return true;
            }
        }

        return false;
    }

    private int findDosModuleIndex(
        String moduleId
    ) {
        if (moduleId == null
            || moduleId.trim().isEmpty()) {
            return -1;
        }

        for (int index = 0;
             index < dosModuleIds.length;
             index++) {
            if (moduleId.equals(
                    dosModuleIds[index]
                )) {
                return index;
            }
        }

        return -1;
    }

    private NativeModuleRuntime.Descriptor
        findNativeModule(
            String moduleId
        ) {
        if (moduleId == null
            || moduleId.trim().isEmpty()) {
            return null;
        }

        for (
            NativeModuleRuntime.Descriptor module
            : nativeModules
        ) {
            if (module != null
                && moduleId.equals(module.id)) {
                return module;
            }
        }

        return null;
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
            boolean enabled =
                isButtonEnabled(index);
            boolean focused =
                enabled
                    && index == hoveredButton;

            paint.setColor(
                !enabled
                    ? Color.rgb(
                        28,
                        32,
                        38
                    )
                    : (
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

            paint.setColor(
                enabled
                    ? Color.WHITE
                    : Color.rgb(
                        103,
                        112,
                        123
                    )
            );
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

    private void drawKeyboard(Canvas canvas, Paint paint) {
        paint.setColor(Color.WHITE);
        paint.setTextSize(30.0f * uiScale);
        canvas.drawText("VIRTUAL KEYBOARD - COPY TEXT", 72, 110, paint);
        paint.setColor(Color.rgb(35, 45, 58));
        canvas.drawRoundRect(72, 133, 952, 211, 10, 10, paint);
        String value = keyboardEditor.value().replace("\n", " ↵ ").replace("\t", " ⇥ ");
        int caret = keyboardEditor.cursor();
        String visible = shorten(value, 85);
        paint.setColor(Color.WHITE);
        paint.setTextSize(23.0f * uiScale);
        canvas.drawText(visible, 90, 176, paint);
        paint.setTextSize(16.0f * uiScale);
        canvas.drawText("Cursor " + caret + " / " + keyboardEditor.value().length(),
            75, 224, paint);
        for (int key = 0; key < 50; key++) {
            int[] rect = KEYBOARD_BUTTONS[key];
            boolean highlighted = hoveredButton == key;
            paint.setColor(highlighted ? Color.rgb(40, 126, 125)
                : Color.rgb(42, 54, 66));
            canvas.drawRoundRect(rect[0], rect[1], rect[2], rect[3], 9, 9, paint);
            paint.setColor(Color.WHITE);
            String label = keyboardEditor.label(key / 10, key % 10);
            paint.setTextSize((label.length() > 4 ? 13 : 21) * uiScale);
            float textX = rect[0] + (rect[2]-rect[0]-paint.measureText(label))*0.5f;
            float baseline = rect[1] + (rect[3]-rect[1]
                - paint.ascent()-paint.descent())*0.5f;
            canvas.drawText(label, textX, baseline, paint);
        }
        int[] done = KEYBOARD_BUTTONS[50];
        paint.setColor(hoveredButton == 50 ? Color.rgb(40, 126, 125)
            : Color.rgb(45, 78, 72));
        canvas.drawRoundRect(done[0],done[1],done[2],done[3],8,8,paint);
        paint.setColor(Color.WHITE);
        paint.setTextSize(24.0f * uiScale);
        canvas.drawText("COPY + CLOSE", 429, 720, paint);
    }

    private void drawNativeLibrary(
        Canvas canvas,
        Paint paint
    ) {
        int count = nativeModules.size();
        int pageCount =
            Math.max(
                1,
                (count + 2) / 3
            );
        int page =
            Math.min(
                nativeLibraryPage,
                pageCount - 1
            );
        int start = page * 3;

        paint.setColor(Color.WHITE);
        paint.setTextSize(31.0f * uiScale);
        canvas.drawText(
            "NATIVE APPS",
            90,
            175,
            paint
        );

        paint.setColor(
            Color.rgb(
                150,
                162,
                177
            )
        );
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

        String[] labels =
            new String[5];

        for (int slot = 0;
             slot < 3;
             slot++) {
            int index = start + slot;
            labels[slot] =
                index < count
                    ? shorten(
                        nativeModules
                            .get(index)
                            .displayName,
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
            NATIVE_LIBRARY_BUTTONS
        );
    }

    private void drawControllerPanel(
        Canvas canvas,
        Paint paint
    ) {
        paint.setColor(Color.WHITE);
        paint.setTextSize(31.0f * uiScale);
        paint.setFakeBoldText(true);
        canvas.drawText(
            "CONTROLLER",
            90,
            160,
            paint
        );
        paint.setFakeBoldText(false);

        paint.setColor(
            controllerConnected
                ? Color.rgb(
                    93,
                    224,
                    177
                )
                : Color.rgb(
                    231,
                    174,
                    87
                )
        );
        paint.setTextSize(21.0f * uiScale);
        canvas.drawText(
            controllerConnected
                ? "Connected"
                : "Not connected",
            90,
            205,
            paint
        );

        paint.setColor(Color.rgb(184, 194, 207));
        paint.setTextSize(18.0f * uiScale);
        canvas.drawText(
            shorten(
                controllerMessage,
                54
            ),
            90,
            238,
            paint
        );

        int battery =
            controllerBattery.get();
        canvas.drawText(
            battery >= 0
                ? "Battery: "
                    + battery
                    + "%"
                : "Battery: unavailable",
            90,
            270,
            paint
        );

        paint.setColor(Color.rgb(126, 205, 221));
        canvas.drawText(
            "Pointer: "
                + preferences
                    .getVrPointerMode()
                    .displayName
                + " • Active: "
                + shorten(
                    activePointerSource,
                    28
                ),
            520,
            205,
            paint
        );

        paint.setColor(Color.rgb(184, 194, 207));
        canvas.drawText(
            "Last input: "
                + shorten(
                    lastInputAction,
                    28
                ),
            520,
            238,
            paint
        );
        canvas.drawText(
            "Source: "
                + shorten(
                    lastInputSource,
                    30
                ),
            520,
            270,
            paint
        );

        String[] labels =
            new String[] {
                "POINTER: "
                    + preferences
                        .getVrPointerMode()
                        .displayName
                        .toUpperCase(Locale.US),
                "RECENTER",
                "INPUT FAMILIARIZATION",
                controllerConnected
                    ? "PAIR / SWITCH CONTROLLER"
                    : "PAIR / SYNC CONTROLLER",
                "ANDROID BLUETOOTH",
                preferences.isControllerLeftHanded()
                    ? "HAND: LEFT"
                    : "HAND: RIGHT",
                "RESET HAND POSITION",
                "BACK TO HOME"
            };

        drawButtons(
            canvas,
            paint,
            labels,
            CONTROLLER_BUTTONS
        );
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
                labels =
                    controllerTrainingReturn
                        ? new String[] {
                            "BACK TO CONTROLLER",
                            "HOME"
                        }
                        : new String[] {
                            "CONTINUE",
                            "SKIP"
                        };
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
        float left =
            0.30f
                * SHELL_VIEW_CONTRACTION;
        float right =
            0.96f
                * SHELL_VIEW_CONTRACTION;
        float top =
            (
                hudDroppedDown
                    ? 0.60f
                    : 0.96f
            ) * SHELL_VIEW_CONTRACTION;
        float bottom =
            (
                hudDroppedDown
                    ? 0.32f
                    : 0.70f
            ) * SHELL_VIEW_CONTRACTION;

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
            boolean enabled =
                isButtonEnabled(index);
            boolean focused =
                enabled
                    && index == hoveredButton;

            paint.setColor(
                !enabled
                    ? Color.rgb(29, 33, 39)
                    : (
                        focused
                            ? Color.rgb(42, 126, 121)
                            : Color.rgb(38, 46, 57)
                    )
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

            paint.setColor(
                enabled
                    ? Color.WHITE
                    : Color.rgb(
                        103,
                        112,
                        123
                    )
            );
            paint.setTextSize(24.0f * uiScale);
            paint.setFakeBoldText(focused);
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

        // Source and destination may alias after full 3D calibration.
        float x = vector[0];
        float y = vector[1];
        float z = vector[2];
        destination[0] = cos * x + sin * z;
        destination[1] = y;
        destination[2] = -sin * x + cos * z;
    }

    private static float[] panelVertices(
        float left,
        float right,
        float bottom,
        float top,
        float z
    ) {
        return panelVertices(
            left,
            right,
            bottom,
            top,
            z,
            z
        );
    }

    private static float[] panelVertices(
        float left,
        float right,
        float bottom,
        float top,
        float leftZ,
        float rightZ
    ) {
        return new float[] {
            left * SHELL_VIEW_CONTRACTION,
            bottom * SHELL_VIEW_CONTRACTION,
            leftZ,
            right * SHELL_VIEW_CONTRACTION,
            bottom * SHELL_VIEW_CONTRACTION,
            rightZ,
            left * SHELL_VIEW_CONTRACTION,
            top * SHELL_VIEW_CONTRACTION,
            leftZ,
            right * SHELL_VIEW_CONTRACTION,
            top * SHELL_VIEW_CONTRACTION,
            rightZ
        };
    }

    private static float[] panelUvs(
        int pixelLeft,
        int pixelRight,
        int pixelTop,
        int pixelBottom
    ) {
        float u0 =
            pixelLeft
                / (float) TEXTURE_WIDTH;
        float u1 =
            pixelRight
                / (float) TEXTURE_WIDTH;
        float v0 =
            pixelTop
                / (float) TEXTURE_HEIGHT;
        float v1 =
            pixelBottom
                / (float) TEXTURE_HEIGHT;

        return new float[] {
            u0, v1,
            u1, v1,
            u0, v0,
            u1, v0
        };
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
