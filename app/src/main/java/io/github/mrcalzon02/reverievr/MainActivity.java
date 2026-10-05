package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.hardware.input.InputManager;
import android.net.Uri;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.provider.Settings;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.RadioGroup;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class MainActivity extends Activity
    implements ControllerManager.Listener,
        InputManager.InputDeviceListener,
        VrInputRouter.Listener {

    private static final int CONTROLLER_PERMISSION_REQUEST = 1201;
    private static final int MEDIA_PICK_REQUEST = 1202;
    private static final int DOS_PICK_REQUEST = 1203;

    private static final int PENDING_CONTROLLER_NONE = 0;
    private static final int PENDING_CONTROLLER_DAYDREAM = 1;
    private static final int PENDING_CONTROLLER_PHONE = 2;

    private ReveriePreferences preferences;
    private UpdateChecker updateChecker;
    private UpdateInstaller updateInstaller;
    private UpdateChecker.Release availableUpdate;
    private ControllerManager controllerManager;
    private InputManager inputManager;
    private VrInputRouter inputRouter;
    private DosModuleRepository dosModuleRepository;
    private DosModuleImporter dosModuleImporter;
    private final ExecutorService dosImportExecutor =
        Executors.newSingleThreadExecutor();

    private TextView phoneBatteryText;
    private TextView controllerBatteryText;
    private TextView controllerStatusText;
    private TextView controllerInputTestText;
    private TextView deviceStatusText;
    private TextView updateStatusText;
    private TextView selectedVideoText;
    private TextView dosModuleStatusText;

    private ProgressBar phoneBatteryBar;
    private ProgressBar controllerBatteryBar;

    private Switch batteryHudSwitch;
    private Switch lookUpRevealSwitch;
    private Switch showPercentagesSwitch;
    private Switch retroModeSwitch;
    private Switch autoUpdateCheckSwitch;

    private Button pairControllerButton;
    private Button phoneEmulatorButton;
    private Button testControllerButton;
    private Button checkUpdateButton;
    private Button installUpdateButton;
    private Button enterVrButton;
    private Button chooseVideoButton;
    private Button clearVideoButton;
    private Button importDosButton;
    private Button clearDosModulesButton;
    private RadioGroup videoProjectionGroup;

    private boolean controllerTestEnabled;
    private boolean inputDeviceListenerRegistered;
    private int pendingControllerPermissionAction = PENDING_CONTROLLER_NONE;

    private volatile ControllerProvider.ConnectionState controllerConnectionState =
        ControllerProvider.ConnectionState.IDLE;
    private volatile String controllerConnectionMessage =
        "Controller is not connected.";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        preferences = new ReveriePreferences(this);
        updateChecker = new UpdateChecker();
        updateInstaller = new UpdateInstaller(this);
        controllerManager =
            ((ReverieApplication) getApplication()).getControllerManager();
        inputManager =
            (InputManager) getSystemService(Context.INPUT_SERVICE);
        inputRouter = new VrInputRouter(this);
        dosModuleRepository = new DosModuleRepository(this);
        dosModuleImporter =
            new DosModuleImporter(
                this,
                dosModuleRepository
            );

        bindViews();
        configurePersistentControls();
        configureActions();
        refreshStaticStatus();
        refreshPhoneBattery();
        refreshControllerPermissionState();
        refreshMediaStatus();
        refreshDosModuleStatus();
        controllerManager.addListener(this);
        refreshInputReadiness();

        if (preferences.isAutoUpdateCheckEnabled()) {
            checkForUpdates(false);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        registerInputDeviceListener();
        refreshStaticStatus();
        refreshPhoneBattery();
        refreshControllerPermissionState();
        refreshMediaStatus();
        refreshDosModuleStatus();
        refreshInputReadiness();
    }

    @Override
    protected void onPause() {
        unregisterInputDeviceListener();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        unregisterInputDeviceListener();
        if (controllerManager != null) {
            controllerManager.removeListener(this);
        }
        if (updateChecker != null) {
            updateChecker.close();
        }
        if (updateInstaller != null) {
            updateInstaller.close();
        }
        dosImportExecutor.shutdownNow();
        super.onDestroy();
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (controllerTestEnabled
            && inputRouter != null
            && inputRouter.onKeyEvent(event)) {
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        if (controllerTestEnabled
            && inputRouter != null
            && inputRouter.onGenericMotionEvent(event)) {
            return true;
        }
        return super.onGenericMotionEvent(event);
    }

    private void bindViews() {
        phoneBatteryText = findViewById(R.id.phone_battery_text);
        controllerBatteryText = findViewById(R.id.controller_battery_text);
        controllerStatusText = findViewById(R.id.controller_status);
        controllerInputTestText = findViewById(R.id.controller_input_test);
        deviceStatusText = findViewById(R.id.device_status);
        updateStatusText = findViewById(R.id.update_status);
        selectedVideoText = findViewById(R.id.selected_video_status);
        dosModuleStatusText = findViewById(R.id.dos_module_status);

        phoneBatteryBar = findViewById(R.id.phone_battery_bar);
        controllerBatteryBar = findViewById(R.id.controller_battery_bar);

        batteryHudSwitch = findViewById(R.id.battery_hud_switch);
        lookUpRevealSwitch = findViewById(R.id.look_up_reveal_switch);
        showPercentagesSwitch = findViewById(R.id.show_percentages_switch);
        retroModeSwitch = findViewById(R.id.retro_mode_switch);
        autoUpdateCheckSwitch = findViewById(R.id.auto_update_check_switch);

        pairControllerButton = findViewById(R.id.pair_controller_button);
        phoneEmulatorButton = findViewById(R.id.phone_emulator_button);
        testControllerButton = findViewById(R.id.test_controller_button);
        checkUpdateButton = findViewById(R.id.check_update_button);
        installUpdateButton = findViewById(R.id.install_update_button);
        enterVrButton = findViewById(R.id.enter_vr_button);
        chooseVideoButton = findViewById(R.id.choose_video_button);
        clearVideoButton = findViewById(R.id.clear_video_button);
        importDosButton = findViewById(R.id.import_dos_button);
        clearDosModulesButton =
            findViewById(R.id.clear_dos_modules_button);
        videoProjectionGroup = findViewById(R.id.video_projection_group);
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

        videoProjectionGroup.setOnCheckedChangeListener((group, checkedId) -> {
            if (checkedId == R.id.video_projection_360) {
                preferences.setVideoProjection(VideoProjection.MONO_EQUIRECTANGULAR_360);
            } else if (checkedId == R.id.video_projection_flat) {
                preferences.setVideoProjection(VideoProjection.FLAT_CINEMA);
            }
        });
    }

    private void configureActions() {
        pairControllerButton.setOnClickListener(
            view -> beginControllerPairing()
        );
        phoneEmulatorButton.setOnClickListener(
            view -> beginPhoneController()
        );

        testControllerButton.setEnabled(false);
        testControllerButton.setOnClickListener(view -> {
            controllerTestEnabled = !controllerTestEnabled;
            testControllerButton.setText(
                controllerTestEnabled
                    ? R.string.stop_controller_test
                    : R.string.test_controller
            );
            controllerInputTestText.setText(
                controllerTestEnabled
                    ? R.string.controller_test_waiting
                    : R.string.controller_test_inactive
            );
        });

        Button bluetoothButton = findViewById(R.id.bluetooth_settings_button);
        bluetoothButton.setOnClickListener(view -> openBluetoothSettings());

        checkUpdateButton.setOnClickListener(view -> checkForUpdates(true));
        installUpdateButton.setEnabled(false);
        installUpdateButton.setOnClickListener(
            view -> confirmInstallAvailableUpdate()
        );

        chooseVideoButton.setOnClickListener(view -> chooseLocalVideo());
        clearVideoButton.setOnClickListener(view -> clearSelectedVideo());

        importDosButton.setOnClickListener(
            view -> chooseDosContent()
        );
        clearDosModulesButton.setOnClickListener(
            view -> clearDosModules()
        );

        Button resetButton = findViewById(R.id.reset_settings_button);
        resetButton.setOnClickListener(view -> confirmReset());

        enterVrButton.setOnClickListener(
            view -> startActivity(new Intent(this, VrActivity.class))
        );
    }

    private void beginControllerPairing() {
        pendingControllerPermissionAction = PENDING_CONTROLLER_DAYDREAM;
        String[] missing = controllerManager.getMissingRuntimePermissions();
        if (missing.length > 0) {
            requestPermissions(missing, CONTROLLER_PERMISSION_REQUEST);
            return;
        }

        pendingControllerPermissionAction = PENDING_CONTROLLER_NONE;
        controllerManager.pairDaydreamController();
    }

    private void beginPhoneController() {
        pendingControllerPermissionAction = PENDING_CONTROLLER_PHONE;
        String[] missing =
            controllerManager.getMissingPhoneEmulatorPermissions();

        if (missing.length > 0) {
            requestPermissions(missing, CONTROLLER_PERMISSION_REQUEST);
            return;
        }

        pendingControllerPermissionAction = PENDING_CONTROLLER_NONE;
        showPhoneControllerChooser();
    }

    private void showPhoneControllerChooser() {
        List<PhoneControllerTarget> targets =
            controllerManager.getPairedPhoneTargets();

        if (targets.isEmpty()) {
            new AlertDialog.Builder(this)
                .setTitle(R.string.phone_controller_no_devices_title)
                .setMessage(R.string.phone_controller_no_devices_message)
                .setNegativeButton(android.R.string.cancel, null)
                .setPositiveButton(
                    R.string.open_bluetooth_settings,
                    (dialog, which) -> openBluetoothSettings()
                )
                .show();
            return;
        }

        String[] labels = new String[targets.size()];
        for (int index = 0; index < targets.size(); index++) {
            labels[index] = targets.get(index).label();
        }

        new AlertDialog.Builder(this)
            .setTitle(R.string.phone_controller_choose_title)
            .setItems(labels, (dialog, which) -> {
                if (which >= 0 && which < targets.size()) {
                    controllerManager.connectPhoneEmulator(
                        targets.get(which)
                    );
                }
            })
            .setNegativeButton(android.R.string.cancel, null)
            .show();
    }

    private void refreshControllerPermissionState() {
        String[] missing = controllerManager.getMissingRuntimePermissions();
        pairControllerButton.setText(
            missing.length > 0
                ? R.string.grant_and_pair_controller
                : R.string.pair_controller
        );
    }

    private void refreshInputReadiness() {
        boolean providerReady =
            controllerManager != null && controllerManager.isReady();
        String gamepadName =
            AndroidGamepadSupport.firstConnectedGamepadName();
        boolean gamepadReady = gamepadName != null;

        if (enterVrButton != null) {
            enterVrButton.setEnabled(providerReady || gamepadReady);
        }
        if (testControllerButton != null) {
            testControllerButton.setEnabled(providerReady || gamepadReady);
        }

        if (controllerStatusText == null) {
            return;
        }

        if (providerReady) {
            controllerStatusText.setText(controllerConnectionMessage);
        } else if (gamepadReady) {
            controllerStatusText.setText(
                getString(
                    R.string.generic_gamepad_ready_format,
                    gamepadName
                )
            );
        } else {
            controllerStatusText.setText(controllerConnectionMessage);
        }

        if (!providerReady && !gamepadReady && controllerTestEnabled) {
            controllerTestEnabled = false;
            testControllerButton.setText(R.string.test_controller);
            controllerInputTestText.setText(
                R.string.controller_test_inactive
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
        runOnUiThread(this::refreshInputReadiness);
    }

    @Override
    public void onInputDeviceRemoved(int deviceId) {
        runOnUiThread(this::refreshInputReadiness);
    }

    @Override
    public void onInputDeviceChanged(int deviceId) {
        runOnUiThread(this::refreshInputReadiness);
    }

    @Override
    public void onInputAction(VrInputAction action, String source) {
        if (!controllerTestEnabled || action == null) {
            return;
        }

        runOnUiThread(() ->
            controllerInputTestText.setText(
                getString(
                    R.string.controller_action_test_format,
                    action.name(),
                    source
                )
            )
        );
    }

    private void chooseLocalVideo() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("video/*");
        intent.addFlags(
            Intent.FLAG_GRANT_READ_URI_PERMISSION
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
        );

        try {
            startActivityForResult(intent, MEDIA_PICK_REQUEST);
        } catch (ActivityNotFoundException exception) {
            Toast.makeText(
                this,
                R.string.video_picker_unavailable,
                Toast.LENGTH_LONG
            ).show();
        }
    }

    @Override
    protected void onActivityResult(
        int requestCode,
        int resultCode,
        Intent data
    ) {
        super.onActivityResult(requestCode, resultCode, data);

        if (resultCode != RESULT_OK
            || data == null
            || data.getData() == null) {
            return;
        }

        if (requestCode == MEDIA_PICK_REQUEST) {
            handleSelectedVideo(data);
            return;
        }

        if (requestCode == DOS_PICK_REQUEST) {
            handleSelectedDosContent(data);
        }
    }

    private void handleSelectedVideo(Intent data) {
        Uri uri = data.getData();
        if (uri == null) {
            return;
        }

        int offeredFlags = data.getFlags();
        int persistFlags =
            offeredFlags & Intent.FLAG_GRANT_READ_URI_PERMISSION;

        String previousUri = preferences.getSelectedVideoUri();

        try {
            getContentResolver().takePersistableUriPermission(
                uri,
                persistFlags
            );
        } catch (SecurityException exception) {
            Toast.makeText(
                this,
                R.string.video_permission_persist_failed,
                Toast.LENGTH_LONG
            ).show();
            return;
        }

        if (!previousUri.equals(uri.toString())) {
            releaseSelectedVideoPermission();
        }
        preferences.setSelectedVideo(
            uri.toString(),
            resolveDisplayName(uri)
        );
        refreshMediaStatus();
    }

    private void handleSelectedDosContent(Intent data) {
        Uri uri = data.getData();
        if (uri == null) {
            return;
        }

        String displayName = resolveDisplayName(uri);
        importDosButton.setEnabled(false);
        dosModuleStatusText.setText(
            getString(
                R.string.dos_import_copying_format,
                displayName
            )
        );

        dosImportExecutor.execute(() -> {
            try {
                DosGameModule module =
                    dosModuleImporter.importDocument(
                        uri,
                        displayName
                    );

                runOnUiThread(() -> {
                    importDosButton.setEnabled(true);
                    refreshDosModuleStatus();
                    Toast.makeText(
                        this,
                        getString(
                            R.string.dos_import_complete_format,
                            module.displayName
                        ),
                        Toast.LENGTH_SHORT
                    ).show();
                });
            } catch (Exception exception) {
                String message =
                    exception.getMessage() == null
                        ? exception.getClass().getSimpleName()
                        : exception.getMessage();

                runOnUiThread(() -> {
                    importDosButton.setEnabled(true);
                    refreshDosModuleStatus();
                    Toast.makeText(
                        this,
                        getString(
                            R.string.dos_import_failed_format,
                            message
                        ),
                        Toast.LENGTH_LONG
                    ).show();
                });
            }
        });
    }

    private String resolveDisplayName(Uri uri) {
        if (uri == null) {
            return getString(R.string.selected_video_unknown_name);
        }

        try (Cursor cursor = getContentResolver().query(
            uri,
            new String[] {OpenableColumns.DISPLAY_NAME},
            null,
            null,
            null
        )) {
            if (cursor != null && cursor.moveToFirst()) {
                int column =
                    cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (column >= 0) {
                    String value = cursor.getString(column);
                    if (value != null && !value.trim().isEmpty()) {
                        return value;
                    }
                }
            }
        } catch (RuntimeException ignored) {
            // The persisted URI remains usable even if metadata is hidden.
        }

        String last = uri.getLastPathSegment();
        return last == null || last.trim().isEmpty()
            ? getString(R.string.selected_video_unknown_name)
            : last;
    }

    private void chooseDosContent() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.addFlags(
            Intent.FLAG_GRANT_READ_URI_PERMISSION
        );

        try {
            startActivityForResult(
                intent,
                DOS_PICK_REQUEST
            );
        } catch (ActivityNotFoundException exception) {
            Toast.makeText(
                this,
                R.string.dos_picker_unavailable,
                Toast.LENGTH_LONG
            ).show();
        }
    }

    private void refreshDosModuleStatus() {
        if (dosModuleStatusText == null
            || dosModuleRepository == null) {
            return;
        }

        List<DosGameModule> modules =
            dosModuleRepository.list();

        if (modules.isEmpty()) {
            dosModuleStatusText.setText(
                R.string.dos_modules_none
            );
            clearDosModulesButton.setEnabled(false);
            return;
        }

        DosGameModule latest = modules.get(0);
        dosModuleStatusText.setText(
            getString(
                R.string.dos_modules_status_format,
                modules.size(),
                latest.displayName
            )
        );
        clearDosModulesButton.setEnabled(true);
    }

    private void clearDosModules() {
        if (dosModuleRepository == null) {
            return;
        }

        new AlertDialog.Builder(this)
            .setTitle(R.string.clear_dos_modules_title)
            .setMessage(R.string.clear_dos_modules_message)
            .setNegativeButton(android.R.string.cancel, null)
            .setPositiveButton(
                R.string.clear_dos_modules_confirm,
                (dialog, which) -> {
                    dosModuleRepository.clearAll();
                    refreshDosModuleStatus();
                }
            )
            .show();
    }

    private void refreshMediaStatus() {
        if (!preferences.hasSelectedVideo()) {
            selectedVideoText.setText(R.string.selected_video_none);
            clearVideoButton.setEnabled(false);
            return;
        }

        String name = preferences.getSelectedVideoDisplayName();
        if (name.trim().isEmpty()) {
            name = getString(R.string.selected_video_unknown_name);
        }

        selectedVideoText.setText(
            getString(R.string.selected_video_format, name)
        );
        clearVideoButton.setEnabled(true);
    }

    private void clearSelectedVideo() {
        releaseSelectedVideoPermission();
        preferences.clearSelectedVideo();
        refreshMediaStatus();
    }

    private void releaseSelectedVideoPermission() {
        String value = preferences.getSelectedVideoUri();
        if (value.trim().isEmpty()) {
            return;
        }

        try {
            getContentResolver().releasePersistableUriPermission(
                Uri.parse(value),
                Intent.FLAG_GRANT_READ_URI_PERMISSION
            );
        } catch (SecurityException ignored) {
            // The provider may already have revoked the grant.
        }
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

        if (requestCode != CONTROLLER_PERMISSION_REQUEST) {
            return;
        }

        boolean granted = grantResults.length > 0;
        for (int result : grantResults) {
            granted &= result == PackageManager.PERMISSION_GRANTED;
        }

        int pending = pendingControllerPermissionAction;
        pendingControllerPermissionAction = PENDING_CONTROLLER_NONE;
        refreshControllerPermissionState();

        if (!granted) {
            controllerStatusText.setText(
                R.string.controller_permission_denied
            );
            return;
        }

        if (pending == PENDING_CONTROLLER_DAYDREAM) {
            controllerManager.pairDaydreamController();
        } else if (pending == PENDING_CONTROLLER_PHONE) {
            showPhoneControllerChooser();
        }
    }

    private void applyPreferencesToControls() {
        batteryHudSwitch.setChecked(preferences.isBatteryHudEnabled());
        lookUpRevealSwitch.setChecked(preferences.isLookUpRevealEnabled());
        showPercentagesSwitch.setChecked(
            preferences.isShowPercentagesEnabled()
        );
        retroModeSwitch.setChecked(preferences.isRetroModeEnabled());
        autoUpdateCheckSwitch.setChecked(
            preferences.isAutoUpdateCheckEnabled()
        );

        VideoProjection projection = preferences.getVideoProjection();
        videoProjectionGroup.check(
            projection == VideoProjection.MONO_EQUIRECTANGULAR_360
                ? R.id.video_projection_360
                : R.id.video_projection_flat
        );
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
    }

    private void refreshPhoneBattery() {
        BatteryManager batteryManager =
            (BatteryManager) getSystemService(Context.BATTERY_SERVICE);

        int percentage = batteryManager == null
            ? -1
            : batteryManager.getIntProperty(
                BatteryManager.BATTERY_PROPERTY_CAPACITY
            );

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

    @Override
    public void onConnectionStateChanged(
        ControllerProvider.ConnectionState state,
        String message
    ) {
        controllerConnectionState = state;
        controllerConnectionMessage =
            message == null ? "" : message;
        runOnUiThread(this::refreshInputReadiness);
    }

    @Override
    public void onBatteryChanged(int percentage, int millivolts) {
        runOnUiThread(() -> {
            if (percentage >= 0) {
                controllerBatteryBar.setProgress(percentage);
                if (millivolts > 0) {
                    controllerBatteryText.setText(
                        getString(
                            R.string.controller_battery_voltage_format,
                            percentage,
                            millivolts
                        )
                    );
                } else {
                    controllerBatteryText.setText(
                        getString(
                            R.string.controller_battery_format,
                            percentage
                        )
                    );
                }
            } else if (millivolts > 0) {
                controllerBatteryBar.setProgress(0);
                controllerBatteryText.setText(
                    getString(
                        R.string.controller_voltage_format,
                        millivolts
                    )
                );
            } else {
                controllerBatteryBar.setProgress(0);
                controllerBatteryText.setText(
                    R.string.controller_battery_unknown
                );
            }
        });
    }

    @Override
    public void onControllerStateChanged(ControllerSnapshot snapshot) {
        if (snapshot == null) {
            return;
        }

        if (inputRouter != null) {
            inputRouter.onControllerSnapshot(
                snapshot,
                controllerManager.getActiveProviderDisplayName()
            );
        }

        if (controllerTestEnabled) {
            runOnUiThread(() ->
                controllerInputTestText.setText(
                    snapshot.toDiagnosticString()
                )
            );
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

    private void handleUpdateResult(
        UpdateChecker.Result result,
        boolean userInitiated
    ) {
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
                        result.release == null
                            ? "?"
                            : result.release.version
                    )
                );
                break;

            case ERROR:
            default:
                updateStatusText.setText(
                    getString(
                        R.string.update_error_format,
                        result.message
                    )
                );
                break;
        }
    }

    private void showUpdateAvailableDialog(
        UpdateChecker.Release release
    ) {
        String title = getString(
            R.string.update_dialog_title,
            release.version
        );
        String message = buildUpdateDialogMessage(release);

        new AlertDialog.Builder(this)
            .setTitle(title)
            .setMessage(message)
            .setPositiveButton(
                R.string.update_now,
                (dialog, which) ->
                    updateInstaller.downloadAndInstall(release)
            )
            .setNegativeButton(R.string.update_not_now, null)
            .setNeutralButton(
                R.string.update_view_release,
                (dialog, which) -> openReleasePage(release)
            )
            .show();
    }

    private String buildUpdateDialogMessage(
        UpdateChecker.Release release
    ) {
        StringBuilder builder = new StringBuilder();
        builder.append(
            getString(
                R.string.update_dialog_versions,
                BuildConfig.VERSION_NAME,
                release.version
            )
        );

        if (release.notes != null
            && !release.notes.trim().isEmpty()) {
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
        if (availableUpdate != null) {
            showUpdateAvailableDialog(availableUpdate);
        }
    }

    private void openReleasePage(UpdateChecker.Release release) {
        if (release == null
            || release.releasePageUrl == null
            || release.releasePageUrl.trim().isEmpty()) {
            return;
        }

        try {
            startActivity(
                new Intent(
                    Intent.ACTION_VIEW,
                    Uri.parse(release.releasePageUrl)
                )
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
            startActivity(
                new Intent(Settings.ACTION_BLUETOOTH_SETTINGS)
            );
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
                    releaseSelectedVideoPermission();
                    preferences.reset();
                    applyPreferencesToControls();
                    refreshMediaStatus();
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
