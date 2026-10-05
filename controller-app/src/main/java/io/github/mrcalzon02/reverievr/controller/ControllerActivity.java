package io.github.mrcalzon02.reverievr.controller;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Locale;

public final class ControllerActivity extends Activity
    implements SensorEventListener,
        ControllerServer.Listener {

    private static final int BLUETOOTH_PERMISSION_REQUEST = 2301;

    private ControllerServer server;
    private SensorManager sensorManager;
    private Sensor rotationVector;
    private Sensor gyroscope;
    private Sensor accelerometer;

    private TextView connectionStatus;
    private TextView sensorStatus;
    private TextView batteryStatus;
    private Button startButton;
    private Button stopButton;
    private ControllerTouchpadView touchpad;

    private boolean resumed;
    private boolean sensorsRegistered;
    private int batteryPercentage = -1;

    private final float[] quaternion = new float[4];

    private final BroadcastReceiver batteryReceiver =
        new BroadcastReceiver() {
            @Override
            public void onReceive(
                Context context,
                Intent intent
            ) {
                updateBattery(intent);
            }
        };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
        );
        setContentView(R.layout.activity_controller);

        connectionStatus =
            findViewById(R.id.connection_status);
        sensorStatus =
            findViewById(R.id.sensor_status);
        batteryStatus =
            findViewById(R.id.battery_status);
        startButton =
            findViewById(R.id.start_server_button);
        stopButton =
            findViewById(R.id.stop_server_button);
        touchpad =
            findViewById(R.id.controller_touchpad);

        server = new ControllerServer(this, this);

        sensorManager =
            (SensorManager) getSystemService(
                Context.SENSOR_SERVICE
            );
        if (sensorManager != null) {
            rotationVector = sensorManager.getDefaultSensor(
                Sensor.TYPE_ROTATION_VECTOR
            );
            if (rotationVector == null) {
                rotationVector = sensorManager.getDefaultSensor(
                    Sensor.TYPE_GAME_ROTATION_VECTOR
                );
            }
            gyroscope = sensorManager.getDefaultSensor(
                Sensor.TYPE_GYROSCOPE
            );
            accelerometer = sensorManager.getDefaultSensor(
                Sensor.TYPE_ACCELEROMETER
            );
        }

        configureUi();
        updateSensorStatus();
        updateControls();

        IntentFilter batteryFilter =
            new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        Intent battery;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            battery = registerReceiver(
                batteryReceiver,
                batteryFilter,
                Context.RECEIVER_NOT_EXPORTED
            );
        } else {
            battery = registerReceiver(
                batteryReceiver,
                batteryFilter
            );
        }
        if (battery != null) {
            updateBattery(battery);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        resumed = true;
        updateSensorRegistration();
    }

    @Override
    protected void onPause() {
        resumed = false;
        updateSensorRegistration();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        try {
            unregisterReceiver(batteryReceiver);
        } catch (IllegalArgumentException ignored) {
            // Receiver already gone.
        }

        if (server != null) {
            server.close();
        }

        super.onDestroy();
    }

    private void configureUi() {
        startButton.setOnClickListener(
            view -> startControllerServer()
        );
        stopButton.setOnClickListener(
            view -> server.stop()
        );

        Button bluetoothButton =
            findViewById(R.id.bluetooth_settings_button);
        bluetoothButton.setOnClickListener(
            view -> openBluetoothSettings()
        );

        touchpad.setListener(
            (action, x, y) -> {
                if (server == null || !server.isConnected()) {
                    return;
                }

                server.sendControl(
                    ControllerProtocolWriter.motion(
                        System.nanoTime(),
                        action,
                        x,
                        y
                    )
                );
            }
        );

        bindMomentaryButton(
            findViewById(R.id.select_button),
            ControllerProtocolWriter.KEY_CLICK
        );
        bindMomentaryButton(
            findViewById(R.id.app_button),
            ControllerProtocolWriter.KEY_APP
        );
        bindMomentaryButton(
            findViewById(R.id.home_button),
            ControllerProtocolWriter.KEY_HOME
        );
    }

    private void bindMomentaryButton(
        Button button,
        int keyCode
    ) {
        button.setOnTouchListener((view, event) -> {
            if (server == null || !server.isConnected()) {
                return false;
            }

            int action = event.getActionMasked();
            if (action == MotionEvent.ACTION_DOWN) {
                view.setPressed(true);
                server.sendControl(
                    ControllerProtocolWriter.key(
                        ControllerProtocolWriter.ACTION_DOWN,
                        keyCode
                    )
                );
                return true;
            }

            if (action == MotionEvent.ACTION_UP
                || action == MotionEvent.ACTION_CANCEL) {
                view.setPressed(false);
                server.sendControl(
                    ControllerProtocolWriter.key(
                        ControllerProtocolWriter.ACTION_UP,
                        keyCode
                    )
                );

                if (action == MotionEvent.ACTION_UP) {
                    view.performClick();
                }
                return true;
            }

            return true;
        });
    }

    private void startControllerServer() {
        if (rotationVector == null) {
            connectionStatus.setText(
                R.string.orientation_sensor_required
            );
            return;
        }

        if (!server.hasConnectPermission()) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                requestPermissions(
                    new String[] {
                        Manifest.permission.BLUETOOTH_CONNECT
                    },
                    BLUETOOTH_PERMISSION_REQUEST
                );
            }
            return;
        }

        server.start();
    }

    @Override
    public void onRequestPermissionsResult(
        int requestCode,
        String[] permissions,
        int[] grantResults
    ) {
        super.onRequestPermissionsResult(
            requestCode,
            permissions,
            grantResults
        );

        if (requestCode != BLUETOOTH_PERMISSION_REQUEST) {
            return;
        }

        boolean granted =
            grantResults.length > 0
                && grantResults[0]
                    == PackageManager.PERMISSION_GRANTED;

        if (granted) {
            server.start();
        } else {
            Toast.makeText(
                this,
                R.string.bluetooth_permission_denied,
                Toast.LENGTH_LONG
            ).show();
        }
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (server != null && server.isConnected()) {
            int code = event.getKeyCode();
            int protocolCode = 0;

            if (code == KeyEvent.KEYCODE_VOLUME_UP) {
                protocolCode =
                    ControllerProtocolWriter.KEY_VOLUME_UP;
            } else if (code == KeyEvent.KEYCODE_VOLUME_DOWN) {
                protocolCode =
                    ControllerProtocolWriter.KEY_VOLUME_DOWN;
            }

            if (protocolCode != 0) {
                int action =
                    event.getAction() == KeyEvent.ACTION_DOWN
                        ? ControllerProtocolWriter.ACTION_DOWN
                        : ControllerProtocolWriter.ACTION_UP;
                server.sendControl(
                    ControllerProtocolWriter.key(
                        action,
                        protocolCode
                    )
                );
                return true;
            }
        }

        return super.dispatchKeyEvent(event);
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        if (event == null
            || server == null
            || !server.isConnected()) {
            return;
        }

        long timestamp = event.timestamp;

        if (event.sensor.getType()
            == Sensor.TYPE_ROTATION_VECTOR) {
            SensorManager.getQuaternionFromVector(
                quaternion,
                event.values
            );

            server.sendOrientation(
                ControllerProtocolWriter.orientation(
                    timestamp,
                    quaternion[1],
                    quaternion[2],
                    quaternion[3],
                    quaternion[0]
                )
            );
        } else if (event.sensor.getType()
            == Sensor.TYPE_GYROSCOPE) {
            server.sendGyroscope(
                ControllerProtocolWriter.gyroscope(
                    timestamp,
                    event.values[0],
                    event.values[1],
                    event.values[2]
                )
            );
        } else if (event.sensor.getType()
            == Sensor.TYPE_ACCELEROMETER) {
            server.sendAccelerometer(
                ControllerProtocolWriter.accelerometer(
                    timestamp,
                    event.values[0],
                    event.values[1],
                    event.values[2]
                )
            );
        }
    }

    @Override
    public void onAccuracyChanged(
        Sensor sensor,
        int accuracy
    ) {
        // Sensor accuracy changes are diagnostic-only for now.
    }

    @Override
    public void onServerStateChanged(
        ControllerServer.State state,
        String message
    ) {
        runOnUiThread(() -> {
            connectionStatus.setText(message);
            updateControls();
            updateSensorRegistration();

            if (state == ControllerServer.State.CONNECTED
                && batteryPercentage >= 0) {
                server.sendControl(
                    ControllerProtocolWriter.batteryStatus(
                        batteryPercentage
                    )
                );
            }
        });
    }

    private void updateSensorRegistration() {
        boolean shouldRegister =
            resumed
                && server != null
                && server.isConnected()
                && sensorManager != null;

        if (shouldRegister == sensorsRegistered) {
            return;
        }

        if (shouldRegister) {
            if (rotationVector != null) {
                sensorManager.registerListener(
                    this,
                    rotationVector,
                    SensorManager.SENSOR_DELAY_GAME
                );
            }
            if (gyroscope != null) {
                sensorManager.registerListener(
                    this,
                    gyroscope,
                    SensorManager.SENSOR_DELAY_GAME
                );
            }
            if (accelerometer != null) {
                sensorManager.registerListener(
                    this,
                    accelerometer,
                    SensorManager.SENSOR_DELAY_GAME
                );
            }
            sensorsRegistered = true;
        } else {
            sensorManager.unregisterListener(this);
            sensorsRegistered = false;
        }
    }

    private void updateSensorStatus() {
        String text = String.format(
            Locale.US,
            "Orientation: %s  •  Gyro: %s  •  Accelerometer: %s",
            rotationVector == null ? "missing" : "ready",
            gyroscope == null ? "missing" : "ready",
            accelerometer == null ? "missing" : "ready"
        );
        sensorStatus.setText(text);
    }

    private void updateControls() {
        boolean connected =
            server != null && server.isConnected();

        startButton.setEnabled(!connected && rotationVector != null);
        stopButton.setEnabled(
            server != null
        );
        touchpad.setEnabled(connected);

        findViewById(R.id.select_button)
            .setEnabled(connected);
        findViewById(R.id.app_button)
            .setEnabled(connected);
        findViewById(R.id.home_button)
            .setEnabled(connected);
    }

    private void updateBattery(Intent intent) {
        if (intent == null) {
            return;
        }

        int level = intent.getIntExtra(
            BatteryManager.EXTRA_LEVEL,
            -1
        );
        int scale = intent.getIntExtra(
            BatteryManager.EXTRA_SCALE,
            -1
        );

        if (level >= 0 && scale > 0) {
            batteryPercentage =
                Math.max(
                    0,
                    Math.min(
                        100,
                        Math.round(
                            level * 100.0f / scale
                        )
                    )
                );

            batteryStatus.setText(
                getString(
                    R.string.battery_format,
                    batteryPercentage
                )
            );

            if (server != null && server.isConnected()) {
                server.sendControl(
                    ControllerProtocolWriter.batteryStatus(
                        batteryPercentage
                    )
                );
            }
        } else {
            batteryPercentage = -1;
            batteryStatus.setText(
                R.string.battery_unknown
            );
        }
    }

    private void openBluetoothSettings() {
        try {
            startActivity(
                new Intent(
                    Settings.ACTION_BLUETOOTH_SETTINGS
                )
            );
        } catch (ActivityNotFoundException exception) {
            Toast.makeText(
                this,
                R.string.bluetooth_settings_unavailable,
                Toast.LENGTH_LONG
            ).show();
        }
    }
}
