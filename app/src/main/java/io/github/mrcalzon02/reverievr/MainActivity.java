package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Locale;

public final class MainActivity extends Activity {
    private ReveriePreferences preferences;

    private TextView phoneBatteryText;
    private TextView controllerBatteryText;
    private TextView controllerStatusText;
    private TextView deviceStatusText;

    private ProgressBar phoneBatteryBar;
    private ProgressBar controllerBatteryBar;

    private Switch batteryHudSwitch;
    private Switch lookUpRevealSwitch;
    private Switch showPercentagesSwitch;
    private Switch retroModeSwitch;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        preferences = new ReveriePreferences(this);

        bindViews();
        configurePersistentControls();
        configureActions();
        refreshStaticStatus();
        refreshPhoneBattery();
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshStaticStatus();
        refreshPhoneBattery();
    }

    private void bindViews() {
        phoneBatteryText = findViewById(R.id.phone_battery_text);
        controllerBatteryText = findViewById(R.id.controller_battery_text);
        controllerStatusText = findViewById(R.id.controller_status);
        deviceStatusText = findViewById(R.id.device_status);

        phoneBatteryBar = findViewById(R.id.phone_battery_bar);
        controllerBatteryBar = findViewById(R.id.controller_battery_bar);

        batteryHudSwitch = findViewById(R.id.battery_hud_switch);
        lookUpRevealSwitch = findViewById(R.id.look_up_reveal_switch);
        showPercentagesSwitch = findViewById(R.id.show_percentages_switch);
        retroModeSwitch = findViewById(R.id.retro_mode_switch);
    }

    private void configurePersistentControls() {
        applyPreferencesToControls();

        batteryHudSwitch.setOnCheckedChangeListener(
            (button, checked) -> preferences.setBatteryHudEnabled(checked)
        );
        lookUpRevealSwitch.setOnCheckedChangeListener(
            (button, checked) -> preferences.setLookUpRevealEnabled(checked)
        );
        showPercentagesSwitch.setOnCheckedChangeListener(
            (button, checked) -> preferences.setShowPercentagesEnabled(checked)
        );
        retroModeSwitch.setOnCheckedChangeListener(
            (button, checked) -> preferences.setRetroModeEnabled(checked)
        );
    }

    private void configureActions() {
        Button pairControllerButton = findViewById(R.id.pair_controller_button);
        pairControllerButton.setEnabled(false);

        Button testControllerButton = findViewById(R.id.test_controller_button);
        testControllerButton.setEnabled(false);

        Button bluetoothButton = findViewById(R.id.bluetooth_settings_button);
        bluetoothButton.setOnClickListener(view -> openBluetoothSettings());

        Button resetButton = findViewById(R.id.reset_settings_button);
        resetButton.setOnClickListener(view -> confirmReset());

        Button enterVrButton = findViewById(R.id.enter_vr_button);
        enterVrButton.setEnabled(false);
    }

    private void applyPreferencesToControls() {
        batteryHudSwitch.setChecked(preferences.isBatteryHudEnabled());
        lookUpRevealSwitch.setChecked(preferences.isLookUpRevealEnabled());
        showPercentagesSwitch.setChecked(preferences.isShowPercentagesEnabled());
        retroModeSwitch.setChecked(preferences.isRetroModeEnabled());
    }

    private void refreshStaticStatus() {
        String deviceText = String.format(
            Locale.US,
            "%s %s  •  Android %s (API %d)",
            Build.MANUFACTURER,
            Build.MODEL,
            Build.VERSION.RELEASE,
            Build.VERSION.SDK_INT
        );
        deviceStatusText.setText(deviceText);

        controllerStatusText.setText(R.string.controller_status_unavailable);
        controllerBatteryText.setText(R.string.controller_battery_unknown);
        controllerBatteryBar.setProgress(0);
    }

    private void refreshPhoneBattery() {
        BatteryManager batteryManager =
            (BatteryManager) getSystemService(Context.BATTERY_SERVICE);

        int percentage = batteryManager == null
            ? -1
            : batteryManager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);

        if (percentage >= 0 && percentage <= 100) {
            phoneBatteryBar.setProgress(percentage);
            phoneBatteryText.setText(
                getString(R.string.phone_battery_format, percentage)
            );
        } else {
            phoneBatteryBar.setProgress(0);
            phoneBatteryText.setText(R.string.phone_battery_unknown);
        }
    }

    private void openBluetoothSettings() {
        try {
            startActivity(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS));
        } catch (ActivityNotFoundException exception) {
            Toast.makeText(
                this,
                R.string.bluetooth_settings_unavailable,
                Toast.LENGTH_LONG
            ).show();
        }
    }

    private void confirmReset() {
        new AlertDialog.Builder(this)
            .setTitle(R.string.reset_settings_title)
            .setMessage(R.string.reset_settings_message)
            .setNegativeButton(android.R.string.cancel, null)
            .setPositiveButton(
                R.string.reset_settings_confirm,
                (dialog, which) -> {
                    preferences.reset();
                    applyPreferencesToControls();
                    Toast.makeText(
                        this,
                        R.string.settings_reset_complete,
                        Toast.LENGTH_SHORT
                    ).show();
                }
            )
            .show();
    }
}
