package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.graphics.SurfaceTexture;
import android.hardware.input.InputManager;
import android.media.AudioManager;
import android.os.BatteryManager;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

import com.google.cardboard.proto.CardboardDevice;
import com.google.cardboard.sdk.CardboardView;
import com.google.cardboard.sdk.QrCode;
import com.google.cardboard.sdk.deviceparams.DeviceParamsUtils;

public final class VrActivity extends Activity
    implements ControllerManager.Listener,
        VrShellRenderer.Host,
        LocalVideoPlayer.Listener,
        InputManager.InputDeviceListener,
        VrInputRouter.Listener,
        VrInputRouter.BindingListener,
        StandardHidInputRouter.Listener {

    private static final float SAFE_VIEWER_FALLBACK_IPD_METERS = 0.060f;

    private CardboardView cardboardView;
    private VrShellRenderer renderer;
    private ControllerManager controllerManager;
    private ReveriePreferences preferences;
    private LocalVideoPlayer videoPlayer;
    private InputManager inputManager;
    private VrInputRouter inputRouter;
    private InputBindingManager inputBindingManager;
    private InputBindingEngine inputBindingEngine;
    private VirtualInputBus virtualInputBus;
    private StandardHidInputRouter standardHidInputRouter;

    private boolean inputDeviceListenerRegistered;
    private volatile String controllerConnectionMessage =
        "Controller is not connected.";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        enterImmersiveMode();

        ReverieApplication application =
            (ReverieApplication) getApplication();
        controllerManager =
            application.getControllerManager();
        inputBindingManager =
            application.getInputBindingManager();
        inputBindingEngine =
            inputBindingManager.getEngine();
        virtualInputBus =
            inputBindingManager.getBus();
        standardHidInputRouter =
            new StandardHidInputRouter(this);
        preferences = new ReveriePreferences(this);
        videoPlayer = new LocalVideoPlayer(this, this);
        inputManager =
            (InputManager) getSystemService(Context.INPUT_SERVICE);
        inputRouter = new VrInputRouter(this);
        inputRouter.setBindingListener(this);

        CardboardView.setUseCardboardGlSurfaceView(true);
        cardboardView = new CardboardView(this);
        cardboardView.setStereoRenderMode(true);

        float viewerIpd = readViewerInterLensDistance();

        renderer = new VrShellRenderer(preferences, viewerIpd, this);
        renderer.setPhoneBattery(readPhoneBattery());

        cardboardView.setRenderer(renderer);
        cardboardView.setOnBackButtonClick(this::finish);
        cardboardView.setOnSettingsButtonClick(
            () -> inputRouter.submitAction(
                VrInputAction.BACK,
                "Cardboard system control"
            )
        );
        cardboardView.setOnTriggerEvent(
            () -> inputRouter.submitAction(
                VrInputAction.SELECT,
                "Cardboard trigger"
            )
        );

        setContentView(cardboardView);
        controllerManager.addListener(this);
        refreshInputSourceStatus();
    }

    @Override
    protected void onResume() {
        super.onResume();
        registerInputDeviceListener();
        enterImmersiveMode();

        if (cardboardView != null) {
            cardboardView.onResume();
        }
        if (renderer != null) {
            renderer.setPhoneBattery(readPhoneBattery());
        }
        if (videoPlayer != null) {
            videoPlayer.resumeForLifecycle();
        }

        refreshInputSourceStatus();
    }

    @Override
    protected void onPause() {
        unregisterInputDeviceListener();

        if (inputBindingEngine != null) {
            inputBindingEngine.releaseAll();
        }
        if (standardHidInputRouter != null) {
            standardHidInputRouter.reset();
        }
        if (videoPlayer != null) {
            videoPlayer.pauseForLifecycle();
        }
        if (cardboardView != null) {
            cardboardView.onPause();
        }
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        unregisterInputDeviceListener();

        if (controllerManager != null) {
            controllerManager.removeListener(this);
        }
        if (videoPlayer != null) {
            videoPlayer.release();
        }
        if (cardboardView != null) {
            cardboardView.onDestroy();
        }
        super.onDestroy();
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (inputRouter != null && inputRouter.onKeyEvent(event)) {
            return true;
        }
        if (standardHidInputRouter != null
            && standardHidInputRouter.onKeyboardEvent(event)) {
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        if (inputRouter != null
            && inputRouter.onGenericMotionEvent(event)) {
            return true;
        }

        int width = cardboardView == null
            ? 0
            : cardboardView.getWidth();
        int height = cardboardView == null
            ? 0
            : cardboardView.getHeight();

        if (standardHidInputRouter != null
            && standardHidInputRouter.onMouseEvent(
                event,
                width,
                height
            )) {
            return true;
        }

        return super.onGenericMotionEvent(event);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            enterImmersiveMode();
        }
    }

    @Override
    public void onConnectionStateChanged(
        ControllerProvider.ConnectionState state,
        String message
    ) {
        controllerConnectionMessage =
            message == null ? "" : message;

        if (state != ControllerProvider.ConnectionState.READY
            && inputBindingEngine != null) {
            inputBindingEngine.releaseAll();
        }

        refreshInputSourceStatus();
    }

    @Override
    public void onBatteryChanged(int percentage, int millivolts) {
        if (renderer != null && controllerManager.isReady()) {
            renderer.setControllerBattery(percentage);
        }
    }

    @Override
    public void onControllerStateChanged(
        ControllerSnapshot snapshot
    ) {
        if (inputRouter == null || snapshot == null) {
            return;
        }

        inputRouter.onControllerSnapshot(
            snapshot,
            controllerManager.getActiveProviderDisplayName()
        );
    }

    @Override
    public void onInputAction(
        VrInputAction action,
        String source
    ) {
        if (renderer == null || action == null) {
            return;
        }

        renderer.noteInputAction(action, source);

        switch (action) {
            case SELECT:
                renderer.requestSelect();
                break;

            case BACK:
                if (!renderer.consumeBackDuringInputTraining()) {
                    renderer.requestBack();
                }
                break;

            case RECENTER:
                renderer.requestRecenter();
                break;

            case NAV_LEFT:
                renderer.requestVideoSeek(-10000);
                break;

            case NAV_RIGHT:
                renderer.requestVideoSeek(10000);
                break;

            case VOLUME_UP:
                adjustMediaVolume(AudioManager.ADJUST_RAISE);
                break;

            case VOLUME_DOWN:
                adjustMediaVolume(AudioManager.ADJUST_LOWER);
                break;

            case NAV_UP:
            case NAV_DOWN:
            default:
                break;
        }
    }

    @Override
    public void onBindingDigital(
        BindingInput input,
        boolean down,
        String source
    ) {
        if (inputBindingEngine != null) {
            inputBindingEngine.submitDigital(
                input,
                down
            );
        }
    }

    @Override
    public void onBindingAxis(
        BindingInput input,
        float value,
        String source
    ) {
        if (inputBindingEngine != null) {
            inputBindingEngine.submitAxis(
                input,
                value
            );
        }
    }

    @Override
    public void onHeadBindingDelta(
        float yawDeltaRadians,
        float pitchDeltaRadians
    ) {
        if (inputBindingEngine == null) {
            return;
        }

        inputBindingEngine.submitRelative(
            BindingInput.HEAD_YAW_DELTA,
            yawDeltaRadians
        );
        inputBindingEngine.submitRelative(
            BindingInput.HEAD_PITCH_DELTA,
            pitchDeltaRadians
        );
    }

    @Override
    public void onVirtualKey(
        VirtualKey key,
        boolean down,
        String source
    ) {
        if (virtualInputBus != null && key != null) {
            virtualInputBus.applyDigital(
                VirtualOutput.key(key),
                down
            );
        }
    }

    @Override
    public void onVirtualMouseButton(
        int button,
        boolean down,
        String source
    ) {
        if (virtualInputBus != null) {
            virtualInputBus.applyDigital(
                VirtualOutput.mouseButton(button),
                down
            );
        }
    }

    @Override
    public void onVirtualMouseRelative(
        float deltaX,
        float deltaY,
        String source
    ) {
        if (virtualInputBus == null) {
            return;
        }

        virtualInputBus.applyAnalog(
            VirtualOutput.mouseRelativeX(),
            deltaX
        );
        virtualInputBus.applyAnalog(
            VirtualOutput.mouseRelativeY(),
            deltaY
        );
    }

    @Override
    public void onVirtualMouseAbsolute(
        float normalizedX,
        float normalizedY,
        String source
    ) {
        if (virtualInputBus == null) {
            return;
        }

        virtualInputBus.applyAnalog(
            VirtualOutput.mouseAbsoluteX(),
            (normalizedX * 2.0f) - 1.0f
        );
        virtualInputBus.applyAnalog(
            VirtualOutput.mouseAbsoluteY(),
            (normalizedY * 2.0f) - 1.0f
        );
    }

    @Override
    public void onVirtualMouseWheel(
        float delta,
        String source
    ) {
        if (virtualInputBus != null) {
            virtualInputBus.applyMouseWheel(delta);
        }
    }

    private void adjustMediaVolume(int direction) {
        AudioManager audio =
            (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        if (audio != null) {
            audio.adjustStreamVolume(
                AudioManager.STREAM_MUSIC,
                direction,
                AudioManager.FLAG_SHOW_UI
            );
        }
    }

    private void registerInputDeviceListener() {
        if (inputManager != null && !inputDeviceListenerRegistered) {
            inputManager.registerInputDeviceListener(this, null);
            inputDeviceListenerRegistered = true;
        }
    }

    private void unregisterInputDeviceListener() {
        if (inputManager != null && inputDeviceListenerRegistered) {
            inputManager.unregisterInputDeviceListener(this);
            inputDeviceListenerRegistered = false;
        }
    }

    @Override
    public void onInputDeviceAdded(int deviceId) {
        refreshInputSourceStatus();
    }

    @Override
    public void onInputDeviceRemoved(int deviceId) {
        if (inputBindingEngine != null) {
            inputBindingEngine.releaseAll();
        }
        if (standardHidInputRouter != null) {
            standardHidInputRouter.reset();
        }
        refreshInputSourceStatus();
    }

    @Override
    public void onInputDeviceChanged(int deviceId) {
        refreshInputSourceStatus();
    }

    private void refreshInputSourceStatus() {
        if (renderer == null || controllerManager == null) {
            return;
        }

        if (controllerManager.isReady()) {
            String provider =
                controllerManager.getActiveProviderDisplayName();
            renderer.setControllerState(
                true,
                provider == null || provider.trim().isEmpty()
                    ? controllerConnectionMessage
                    : provider + " ready"
            );
            renderer.setControllerBattery(
                controllerManager.getBatteryPercentage()
            );
            return;
        }

        String gamepad =
            AndroidGamepadSupport.firstConnectedGamepadName();
        if (gamepad != null) {
            renderer.setControllerState(
                true,
                "Android gamepad ready: " + gamepad
            );
            renderer.setControllerBattery(-1);
        } else {
            renderer.setControllerState(
                false,
                controllerConnectionMessage
            );
            renderer.setControllerBattery(-1);
        }
    }

    @Override
    public void onExitToPhoneRequested() {
        runOnUiThread(this::finish);
    }

    @Override
    public void onSetupCompleted() {
        // The renderer persists the setup profile before this callback.
    }

    @Override
    public void onControllerRecenterRequested() {
        if (controllerManager != null) {
            controllerManager.recenterController();
        }
    }

    @Override
    public void onVideoSurfaceTextureReady(
        SurfaceTexture surfaceTexture
    ) {
        runOnUiThread(() -> {
            if (videoPlayer != null) {
                videoPlayer.attachSurfaceTexture(surfaceTexture);
            }
        });
    }

    @Override
    public void onVideoPlaybackRequested() {
        runOnUiThread(() -> {
            if (videoPlayer != null && preferences != null) {
                videoPlayer.play(preferences.getSelectedVideoUri());
            }
        });
    }

    @Override
    public void onVideoTogglePauseRequested() {
        runOnUiThread(() -> {
            if (videoPlayer != null) {
                videoPlayer.togglePause();
            }
        });
    }

    @Override
    public void onVideoSeekRequested(int deltaMillis) {
        runOnUiThread(() -> {
            if (videoPlayer != null) {
                videoPlayer.seekRelative(deltaMillis);
            }
        });
    }

    @Override
    public void onVideoStopRequested() {
        runOnUiThread(() -> {
            if (videoPlayer != null) {
                videoPlayer.stop();
            }
        });
    }

    @Override
    public void onVideoPrepared(float aspectRatio) {
        if (renderer != null) {
            renderer.setVideoAspectRatio(aspectRatio);
        }
    }

    @Override
    public void onVideoStateChanged(String message) {
        // Playback state is intentionally quiet inside the headset for now.
    }

    @Override
    public void onVideoError(String message) {
        runOnUiThread(() -> {
            Toast.makeText(
                this,
                message,
                Toast.LENGTH_LONG
            ).show();
            if (renderer != null) {
                renderer.requestVideoExit();
            }
        });
    }

    private float readViewerInterLensDistance() {
        try {
            byte[] savedParams = QrCode.getSavedDeviceParams();
            if (savedParams == null) {
                return SAFE_VIEWER_FALLBACK_IPD_METERS;
            }

            CardboardDevice.DeviceParams params =
                DeviceParamsUtils.parseCardboardDeviceParams(
                    savedParams
                );
            if (params == null
                || !params.hasInterLensDistance()) {
                return SAFE_VIEWER_FALLBACK_IPD_METERS;
            }

            float value = params.getInterLensDistance();
            if (value < 0.050f || value > 0.080f) {
                return SAFE_VIEWER_FALLBACK_IPD_METERS;
            }
            return value;
        } catch (RuntimeException exception) {
            return SAFE_VIEWER_FALLBACK_IPD_METERS;
        }
    }

    private int readPhoneBattery() {
        BatteryManager batteryManager =
            (BatteryManager) getSystemService(
                Context.BATTERY_SERVICE
            );
        if (batteryManager == null) {
            return -1;
        }

        int percentage =
            batteryManager.getIntProperty(
                BatteryManager.BATTERY_PROPERTY_CAPACITY
            );
        return percentage >= 0 && percentage <= 100
            ? percentage
            : -1;
    }

    private void enterImmersiveMode() {
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );
    }
}
