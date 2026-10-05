package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.graphics.SurfaceTexture;
import android.os.BatteryManager;
import android.os.Bundle;
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
        LocalVideoPlayer.Listener {

    private static final float SAFE_VIEWER_FALLBACK_IPD_METERS = 0.060f;

    private CardboardView cardboardView;
    private VrShellRenderer renderer;
    private ControllerManager controllerManager;
    private ReveriePreferences preferences;
    private LocalVideoPlayer videoPlayer;

    private static final int VIDEO_SWIPE_THRESHOLD = 48;

    private boolean previousTouchpadPressed;
    private boolean previousMenuPressed;
    private boolean previousHomePressed;
    private boolean previousTouching;
    private int touchStartX;
    private int touchStartY;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        enterImmersiveMode();

        controllerManager =
            ((ReverieApplication) getApplication()).getControllerManager();

        preferences = new ReveriePreferences(this);
        videoPlayer = new LocalVideoPlayer(this, this);

        CardboardView.setUseCardboardGlSurfaceView(true);
        cardboardView = new CardboardView(this);
        cardboardView.setStereoRenderMode(true);

        float viewerIpd = readViewerInterLensDistance();

        renderer = new VrShellRenderer(preferences, viewerIpd, this);
        renderer.setPhoneBattery(readPhoneBattery());
        renderer.setControllerBattery(controllerManager.getBatteryPercentage());
        renderer.setControllerState(
            controllerManager.isReady(),
            controllerManager.isReady()
                ? "Controller ready"
                : "Controller disconnected"
        );

        cardboardView.setRenderer(renderer);
        cardboardView.setOnBackButtonClick(this::finish);
        cardboardView.setOnSettingsButtonClick(renderer::requestBack);
        cardboardView.setOnTriggerEvent(renderer::requestSelect);

        setContentView(cardboardView);
        controllerManager.addListener(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
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
    }

    @Override
    protected void onPause() {
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
        if (renderer == null) {
            return;
        }

        boolean ready = state == ControllerProvider.ConnectionState.READY;
        renderer.setControllerState(ready, message);
    }

    @Override
    public void onBatteryChanged(int percentage, int millivolts) {
        if (renderer != null) {
            renderer.setControllerBattery(percentage);
        }
    }

    @Override
    public void onControllerStateChanged(ControllerSnapshot snapshot) {
        if (renderer == null || snapshot == null) {
            return;
        }

        boolean selectEdge =
            snapshot.touchpadPressed && !previousTouchpadPressed;
        boolean menuEdge =
            snapshot.menuPressed && !previousMenuPressed;
        boolean homeEdge =
            snapshot.homePressed && !previousHomePressed;

        if (snapshot.touching && !previousTouching) {
            touchStartX = snapshot.touchX;
            touchStartY = snapshot.touchY;
        } else if (!snapshot.touching && previousTouching) {
            int deltaX = snapshot.touchX - touchStartX;
            int deltaY = snapshot.touchY - touchStartY;
            if (Math.abs(deltaX) >= VIDEO_SWIPE_THRESHOLD
                && Math.abs(deltaX) > Math.abs(deltaY)) {
                renderer.requestVideoSeek(deltaX > 0 ? 10000 : -10000);
            }
        }

        previousTouching = snapshot.touching;
        previousTouchpadPressed = snapshot.touchpadPressed;
        previousMenuPressed = snapshot.menuPressed;
        previousHomePressed = snapshot.homePressed;

        if (selectEdge) {
            renderer.requestSelect();
        }
        if (menuEdge) {
            renderer.requestBack();
        }
        if (homeEdge) {
            renderer.requestRecenter();
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
    public void onVideoSurfaceTextureReady(SurfaceTexture surfaceTexture) {
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
            Toast.makeText(this, message, Toast.LENGTH_LONG).show();
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
                DeviceParamsUtils.parseCardboardDeviceParams(savedParams);
            if (params == null || !params.hasInterLensDistance()) {
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
            (BatteryManager) getSystemService(Context.BATTERY_SERVICE);
        if (batteryManager == null) {
            return -1;
        }

        int percentage =
            batteryManager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);
        return percentage >= 0 && percentage <= 100 ? percentage : -1;
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
