package io.github.mrcalzon02.reverievr;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
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
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import javax.microedition.khronos.egl.EGLConfig;

final class VrShellRenderer implements CardboardView.Renderer {
    interface Host {
        void onExitToPhoneRequested();
        void onSetupCompleted();
        void onControllerRecenterRequested();
        void onVideoSurfaceTextureReady(SurfaceTexture surfaceTexture);
        void onVideoPlaybackRequested();
        void onVideoTogglePauseRequested();
        void onVideoStopRequested();
    }

    private static final int TEXTURE_WIDTH = 1024;
    private static final int TEXTURE_HEIGHT = 768;

    private static final float PANEL_HALF_WIDTH = 1.70f;
    private static final float PANEL_HALF_HEIGHT = 1.20f;
    private static final float PANEL_Z = -3.0f;

    private static final float Z_NEAR = 0.10f;
    private static final float Z_FAR = 30.0f;

    private static final int MODE_SETUP = 0;
    private static final int MODE_HOME = 1;
    private static final int MODE_VIDEO = 2;

    private static final int[][] HOME_BUTTONS = new int[][] {
        {140, 270, 884, 345},
        {140, 360, 884, 435},
        {140, 450, 884, 525},
        {140, 540, 884, 615}
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

    private final FloatBuffer vertexBuffer;
    private final FloatBuffer uvBuffer;

    private final float[] rawHeadView = new float[16];
    private final float[] adjustedHeadView = new float[16];
    private final float[] eyeView = new float[16];
    private final float[] modelViewProjection = new float[16];
    private final float[] tempMatrix = new float[16];
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

    private volatile boolean controllerConnected;
    private volatile String controllerMessage = "Controller";
    private volatile boolean textureDirty = true;

    private int program;
    private int texture;
    private Bitmap uiBitmap;
    private Canvas uiCanvas;
    private Paint uiPaint;
    private boolean textureStorageInitialized;
    private int positionHandle;
    private int uvHandle;
    private int matrixHandle;
    private int textureHandle;

    private int mode;
    private int setupStep;
    private int hoveredButton = -1;
    private float yawOffsetRadians;
    private float userIpdMeters;
    private float uiScale;

    VrShellRenderer(
        ReveriePreferences preferences,
        float viewerInterLensMeters,
        Host host
    ) {
        this.preferences = preferences;
        this.viewerInterLensMeters = clamp(viewerInterLensMeters, 0.050f, 0.080f);
        this.host = host;
        videoRenderer = new VideoSurfaceRenderer(host::onVideoSurfaceTextureReady);

        userIpdMeters = preferences.getUserIpdMeters(this.viewerInterLensMeters);
        uiScale = preferences.getUiScale();

        if (preferences.isVrSetupCurrent()) {
            mode = MODE_HOME;
            setupStep = 0;
        } else {
            mode = MODE_SETUP;
            setupStep = clampInt(preferences.getVrSetupStep(), 0, 4);
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
        Matrix.setIdentityM(adjustedHeadView, 0);
        Matrix.setIdentityM(yawMatrix, 0);
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
        textureDirty = true;
    }

    void setControllerBattery(int percentage) {
        controllerBattery.set(percentage);
        textureDirty = true;
    }

    void setControllerState(boolean connected, String message) {
        controllerConnected = connected;
        controllerMessage = message == null ? "Controller" : message;
        textureDirty = true;
    }

    void setVideoAspectRatio(float aspectRatio) {
        videoRenderer.setVideoAspectRatio(aspectRatio);
    }

    void requestVideoExit() {
        backRequested.set(true);
    }

    void requestVideoSeek(int deltaMillis) {
        if (deltaMillis == 0) {
            return;
        }
        videoSeekRequestedMillis.set(deltaMillis);
    }

    @Override
    public void onNewFrame(HeadTransform headTransform) {
        headTransform.getHeadView(rawHeadView, 0);
        headTransform.getEulerAngles(headEuler, 0);
        headTransform.getForwardVector(headForward, 0);

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

        if (mode == MODE_VIDEO) {
            videoRenderer.updateFrame();

            if (backRequested.getAndSet(false)) {
                host.onVideoStopRequested();
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
            return;
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

        program = buildProgram(VERTEX_SHADER, FRAGMENT_SHADER);
        positionHandle = GLES20.glGetAttribLocation(program, "a_Position");
        uvHandle = GLES20.glGetAttribLocation(program, "a_TexCoord");
        matrixHandle = GLES20.glGetUniformLocation(program, "u_Mvp");
        textureHandle = GLES20.glGetUniformLocation(program, "u_Texture");

        int[] textures = new int[1];
        GLES20.glGenTextures(1, textures, 0);
        texture = textures[0];
        textureStorageInitialized = false;
        ensureUiBitmap();
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
        textureDirty = true;
    }

    @Override
    public void onRendererShutdown() {
        videoRenderer.shutdown();

        if (texture != 0) {
            GLES20.glDeleteTextures(1, new int[] {texture}, 0);
            texture = 0;
        }
        if (program != 0) {
            GLES20.glDeleteProgram(program);
            program = 0;
        }

        textureStorageInitialized = false;
        if (uiBitmap != null && !uiBitmap.isRecycled()) {
            uiBitmap.recycle();
        }
        uiBitmap = null;
        uiCanvas = null;
        uiPaint = null;
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
                    setupStep = 1;
                    preferences.setVrSetupStep(1);
                    break;
                case 2:
                    if (!preferences.hasSelectedVideo()) {
                        host.onExitToPhoneRequested();
                        return;
                    }

                    videoRenderer.setProjection(
                        preferences.getVideoProjection()
                    );
                    mode = MODE_VIDEO;
                    hoveredButton = -1;
                    host.onVideoPlaybackRequested();
                    return;
                case 3:
                    host.onExitToPhoneRequested();
                    return;
                default:
                    break;
            }
        } else {
            handleSetupSelection(hoveredButton);
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
                if (button == 0) {
                    userIpdMeters = clamp(userIpdMeters - 0.001f, 0.050f, 0.080f);
                    preferences.setUserIpdMeters(userIpdMeters);
                } else if (button == 1) {
                    userIpdMeters = clamp(userIpdMeters + 0.001f, 0.050f, 0.080f);
                    preferences.setUserIpdMeters(userIpdMeters);
                } else if (button == 2) {
                    advanceSetup();
                }
                break;

            case 2:
                if (button == 0) {
                    uiScale = clamp(uiScale - 0.05f, 0.75f, 1.50f);
                    preferences.setUiScale(uiScale);
                } else if (button == 1) {
                    uiScale = clamp(uiScale + 0.05f, 0.75f, 1.50f);
                    preferences.setUiScale(uiScale);
                } else if (button == 2) {
                    advanceSetup();
                }
                break;

            case 3:
                if (button == 0) {
                    preferences.setBatteryHudEnabled(!preferences.isBatteryHudEnabled());
                } else if (button == 1) {
                    preferences.setLookUpRevealEnabled(!preferences.isLookUpRevealEnabled());
                } else if (button == 2) {
                    advanceSetup();
                }
                break;

            case 4:
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
        setupStep = Math.min(4, setupStep + 1);
        preferences.setVrSetupStep(setupStep);
        hoveredButton = -1;
        textureDirty = true;
    }

    private void handleBack() {
        if (mode == MODE_HOME) {
            host.onExitToPhoneRequested();
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

        if (setupStep == 1 || setupStep == 2 || setupStep == 3) {
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

        canvas.drawColor(Color.rgb(9, 12, 16));

        paint.setColor(Color.rgb(24, 29, 36));
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
        } else {
            drawSetup(canvas, paint);
        }

        drawPowerHud(canvas, paint);

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

        String[] labels = new String[] {
            "RUN VR SETUP",
            "OPTICAL / DISPLAY CALIBRATION",
            preferences.hasSelectedVideo()
                ? "PLAY SELECTED VIDEO"
                : "SELECT VIDEO ON PHONE",
            "EXIT TO PHONE"
        };
        drawButtons(canvas, paint, labels, activeButtons());
    }

    private void drawSetup(Canvas canvas, Paint paint) {
        paint.setColor(Color.WHITE);
        paint.setTextSize(31.0f * uiScale);
        canvas.drawText(
            String.format(Locale.US, "SETUP  %d / 5", setupStep + 1),
            90,
            175,
            paint
        );

        paint.setColor(Color.rgb(184, 194, 207));
        paint.setTextSize(23.0f * uiScale);

        String[] labels;
        switch (setupStep) {
            case 0:
                canvas.drawText("Face forward in a comfortable seated position.", 90, 255, paint);
                canvas.drawText("Set this as your neutral direction.", 90, 300, paint);
                labels = new String[] {"RECENTER + NEXT", "SKIP"};
                break;

            case 1:
                canvas.drawText("Virtual eye spacing", 90, 245, paint);
                paint.setColor(Color.WHITE);
                paint.setTextSize(58.0f * uiScale);
                canvas.drawText(
                    String.format(Locale.US, "%.0f mm", userIpdMeters * 1000.0f),
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
                canvas.drawText("Adjust for easiest binocular fusion; reset later in Settings.", 90, 430, paint);
                labels = new String[] {"NARROWER", "WIDER", "NEXT"};
                break;

            case 2:
                canvas.drawText("Readability", 90, 245, paint);
                paint.setColor(Color.WHITE);
                paint.setTextSize(48.0f * uiScale);
                canvas.drawText(
                    String.format(Locale.US, "UI scale  %.0f%%", uiScale * 100.0f),
                    90,
                    335,
                    paint
                );
                paint.setColor(Color.rgb(184, 194, 207));
                paint.setTextSize(22.0f * uiScale);
                canvas.drawText("This text should be easy to read without leaning.", 90, 405, paint);
                labels = new String[] {"SMALLER", "LARGER", "NEXT"};
                break;

            case 3:
                canvas.drawText("Status HUD", 90, 245, paint);
                canvas.drawText(
                    "Battery HUD: " + onOff(preferences.isBatteryHudEnabled()),
                    90,
                    315,
                    paint
                );
                canvas.drawText(
                    "Look-up reveal: " + onOff(preferences.isLookUpRevealEnabled()),
                    90,
                    365,
                    paint
                );
                labels = new String[] {"BATTERY HUD", "LOOK-UP MODE", "NEXT"};
                break;

            case 4:
            default:
                canvas.drawText("Core setup is ready.", 90, 255, paint);
                canvas.drawText("More calibration tools remain available from Settings.", 90, 305, paint);
                labels = new String[] {"SAVE + HOME", "HOME WITHOUT SAVING"};
                break;
        }

        drawButtons(canvas, paint, labels, activeButtons());
    }

    private void drawPowerHud(Canvas canvas, Paint paint) {
        if (!preferences.isBatteryHudEnabled()) {
            return;
        }

        paint.setTextSize(18.0f);
        paint.setColor(Color.rgb(154, 166, 180));

        String phone = phoneBattery.get() >= 0
            ? "P " + phoneBattery.get() + "%"
            : "P --";
        String controller = controllerBattery.get() >= 0
            ? "H " + controllerBattery.get() + "%"
            : "H --";

        canvas.drawText(phone, 820, 92, paint);
        canvas.drawText(controller, 900, 92, paint);
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
