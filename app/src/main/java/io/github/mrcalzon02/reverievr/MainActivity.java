package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
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
    private UpdateChecker updateChecker;
    private UpdateInstaller updateInstaller;
    private UpdateChecker.Release availableUpdate;

    private TextView phoneBatteryText;
    private TextView controllerBatteryText;
    private TextView controllerStatusText;
    private TextView deviceStatusText;
    private TextView updateStatusText;

    private ProgressBar phoneBatteryBar;
    private ProgressBar controllerBatteryBar;

    private Switch batteryHudSwitch;
    private Switch lookUpRevealSwitch;
    private Switch showPercentagesSwitch;
    private Switch retroModeSwitch;
    private Switch autoUpdateCheckSwitch;

    private Button checkUpdateButton;
    private Button installUpdateButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        preferences = new ReveriePreferences(this);
        updateChecker = new UpdateChecker();
        updateInstaller = new UpdateInstaller(this);

        bindViews();
        configurePersistentControls();
        configureActions();
        refreshStaticStatus();
        refreshPhoneBattery();

        if (preferences.isAutoUpdateCheckEnabled()) {
            checkForUpdates(false);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshStaticStatus();
        refreshPhoneBattery();
    }

    @Override
    protected void onDestroy() {
        if (updateChecker != null) {
            updateChecker.close();
        }
        if (updateInstaller != null) {
            updateInstaller.close();
        }
        super.onDestroy();
    }

    private void bindViews() {
        phoneBatteryText = findViewById(R.id.phone_battery_text);
        controllerBatteryText = findViewById(R.id.controller_battery_text);
        controllerStatusText = findViewById(R.id.controller_status);
        deviceStatusText = findViewById(R.id.device_status);
        updateStatusText = findViewById(R.id.update_status);

        phoneBatteryBar = findViewById(R.id.phone_battery_bar);
        controllerBatteryBar = findViewById(R.id.controller_battery_bar);

        batteryHudSwitch = findViewById(R.id.battery_hud_switch);
        lookUpRevealSwitch = findViewById(R.id.look_up_reveal_switch);
        showPercentagesSwitch = findViewById(R.id.show_percentages_switch);
        retroModeSwitch = findViewById(R.id.retro_mode_switch);
        autoUpdateCheckSwitch = findViewById(R.id.auto_update_check_switch);

        checkUpdateButton = findViewById(R.id.check_update_button);
        installUpdateButton = findViewById(R.id.install_update_button);
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
        autoUpdateCheckSwitch.setOnCheckedChangeListener(
            (button, checked) -> preferences.setAutoUpdateCheckEnabled(checked)
        );
    }

    private void configureActions() {
        Button pairControllerButton = findViewById(R.id.pair_controller_button);
        pairControllerButton.setEnabled(false);

        Button testControllerButton = findViewById(R.id.test_controller_button);
        testControllerButton.setEnabled(false);

        Button bluetoothButton = findViewById(R.id.bluetooth_settings_button);
        bluetoothButton.setOnClickListener(view -> openBluetoothSettings());

        checkUpdateButton.setOnClickListener(view -> checkForUpdates(true));
        installUpdateButton.setEnabled(false);
        installUpdateButton.setOnClickListener(view -> confirmInstallAvailableUpdate());

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
        autoUpdateCheckSwitch.setChecked(preferences.isAutoUpdateCheckEnabled());
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

    private void checkForUpdates(boolean userInitiated) {
        checkUpdateButton.setEnabled(false);
        updateStatusText.setText(R.string.update_checking);

        updateChecker.check(BuildConfig.VERSION_NAME, result ->
            runOnUiThread(() -> {
                checkUpdateButton.setEnabled(true);
                handleUpdateResult(result, userInitiated);
            })
        );
    }

    private void handleUpdateResult(UpdateChecker.Result result, boolean userInitiated) {
        availableUpdate = null;
        installUpdateButton.setEnabled(false);

        switch (result.state) {
            case UPDATE_AVAILABLE:
                availableUpdate = result.release;
                updateStatusText.setText(
                    getString(
                        R.string.update_available_format,
                        BuildConfig.VERSION_NAME,
                        result.release.version
                    )
                );
                installUpdateButton.setEnabled(true);
                showUpdateAvailableDialog(result.release);
                break;

            case UP_TO_DATE:
                updateStatusText.setText(
                    getString(
                        R.string.update_current_format,
                        BuildConfig.VERSION_NAME
                    )
                );
                if (userInitiated) {
                    Toast.makeText(
                        this,
                        R.string.update_already_current,
                        Toast.LENGTH_SHORT
                    ).show();
                }
                break;

            case NO_RELEASES:
                updateStatusText.setText(R.string.update_no_releases);
                break;

            case RELEASE_WITHOUT_APK:
                updateStatusText.setText(
                    getString(
                        R.string.update_release_without_apk_format,
                        result.release == null ? "?" : result.release.version
                    )
                );
                break;

            case ERROR:
            default:
                updateStatusText.setText(
                    getString(R.string.update_error_format, result.message)
                );
                break;
        }
    }

    private void showUpdateAvailableDialog(UpdateChecker.Release release) {
        String title = getString(
            R.string.update_dialog_title,
            release.version
        );
        String message = buildUpdateDialogMessage(release);

        AlertDialog dialog = new AlertDialog.Builder(this)
            .setTitle(title)
            .setMessage(message)
            .setPositiveButton(
                R.string.update_now,
                (whichDialog, which) -> updateInstaller.downloadAndInstall(release)
            )
            .setNegativeButton(R.string.update_not_now, null)
            .setNeutralButton(
                R.string.update_view_release,
                (whichDialog, which) -> openReleasePage(release)
            )
            .create();

        dialog.show();
    }

    private String buildUpdateDialogMessage(UpdateChecker.Release release) {
        StringBuilder builder = new StringBuilder();
        builder.append(
            getString(
                R.string.update_dialog_versions,
                BuildConfig.VERSION_NAME,
                release.version
            )
        );

        if (release.notes != null && !release.notes.trim().isEmpty()) {
            builder.append("\n\n");
            String notes = release.notes.trim();
            if (notes.length() > 1200) {
                notes = notes.substring(0, 1200) + "…";
            }
            builder.append(notes);
        }

        return builder.toString();
    }

    private void confirmInstallAvailableUpdate() {
        if (availableUpdate == null) {
            return;
        }
        showUpdateAvailableDialog(availableUpdate);
    }

    private void openReleasePage(UpdateChecker.Release release) {
        if (release == null || release.releasePageUrl == null
            || release.releasePageUrl.trim().isEmpty()) {
            return;
        }

        try {
            startActivity(
                new Intent(Intent.ACTION_VIEW, Uri.parse(release.releasePageUrl))
            );
        } catch (ActivityNotFoundException exception) {
            Toast.makeText(
                this,
                R.string.update_release_page_unavailable,
                Toast.LENGTH_LONG
            ).show();
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
