package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.hardware.input.InputManager;
import android.net.Uri;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.provider.Settings;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RadioGroup;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
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
    private static final int LOG_EXPORT_REQUEST = 1204;
    private static final int VR_LAUNCH_REQUEST = 1205;

    private static final String DIAGNOSTIC_PENDING_PREFS =
        "reverie-diagnostic-pending";
    private static final String DIAGNOSTIC_PENDING_ID =
        "diagnostic_id";
    private static final String DIAGNOSTIC_PENDING_SHA256 =
        "sha256";
    private static final String DIAGNOSTIC_PENDING_RECEIPT =
        "receipt";

    private static final int PENDING_CONTROLLER_NONE = 0;
    private static final int PENDING_CONTROLLER_DAYDREAM = 1;
    private static final int PENDING_CONTROLLER_PHONE = 2;

    private ReveriePreferences preferences;
    private UpdateChecker updateChecker;
    private UpdateInstaller updateInstaller;
    private UiFeedback uiFeedback;
    private UpdateChecker.Release availableUpdate;
    private ControllerManager controllerManager;
    private InputManager inputManager;
    private VrInputRouter inputRouter;
    private DosModuleRepository dosModuleRepository;
    private DosModuleImporter dosModuleImporter;
    private DiagnosticBundleExporter diagnosticBundleExporter;
    private DiagnosticSubmissionClient diagnosticSubmissionClient;
    private final ExecutorService dosImportExecutor =
        Executors.newSingleThreadExecutor();
    private final ExecutorService diagnosticExecutor =
        Executors.newSingleThreadExecutor();

    private TextView appVersionText;
    private TextView phoneBatteryText;
    private TextView controllerBatteryText;
    private TextView controllerStatusText;
    private TextView controllerInputTestText;
    private TextView deviceStatusText;
    private TextView updateStatusText;
    private TextView selectedVideoText;
    private TextView dosModuleStatusText;
    private TextView loggingStatusText;
    private TextView vrRuntimeStatusText;

    private ProgressBar phoneBatteryBar;
    private ProgressBar controllerBatteryBar;

    private Switch batteryHudSwitch;
    private Switch lookUpRevealSwitch;
    private Switch showPercentagesSwitch;
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
    private Button exportLogsButton;
    private Button submitDiagnosticsButton;
    private Button clearLogsButton;
    private Button controllerActionButton;
    private View updatePanel;
    private VrRuntimePreflight.Result vrRuntimePreflight;
    private RadioGroup videoProjectionGroup;
    private RadioGroup loggingModeGroup;

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
        uiFeedback = new UiFeedback(this);
        updateChecker = new UpdateChecker();
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
        installBundledDosContent();
        diagnosticBundleExporter =
            new DiagnosticBundleExporter(this);
        diagnosticSubmissionClient =
            new DiagnosticSubmissionClient();

        bindViews();
        vrRuntimePreflight =
            VrRuntimePreflight.check();
        if (!vrRuntimePreflight.available) {
            ReverieLog.incident(
                "VR_STARTUP",
                "Cardboard native runtime preflight failed: "
                    + vrRuntimePreflight.detail
            );
        }
        updateInstaller =
            new UpdateInstaller(
                this,
                () -> uiFeedback.failure(installUpdateButton)
            );
        configurePersistentControls();
        configureActions();
        refreshStaticStatus();
        refreshPhoneBattery();
        refreshControllerPermissionState();
        refreshMediaStatus();
        refreshDosModuleStatus();
        controllerManager.addListener(this);
        refreshInputReadiness();
        showPreviousVrStartupFailureIfNeeded();

        ReverieLog.milestone(
            "STAGE_A",
            "Stage A setup screen ready."
        );

        if (BuildConfig.UPDATE_CHANNEL_ENABLED
            && preferences.isAutoUpdateCheckEnabled()) {
            checkForUpdates(false);
        }
    }

    private void launchVr() {
        if (vrRuntimePreflight == null) {
            vrRuntimePreflight =
                VrRuntimePreflight.check();
        }

        if (!vrRuntimePreflight.available) {
            ReverieLog.incident(
                "VR_STARTUP",
                "Blocked VR launch because Cardboard native runtime is unavailable: "
                    + vrRuntimePreflight.detail
            );
            uiFeedback.failure(enterVrButton);
            refreshVrRuntimeStatus();
            showVrStartupFailure(
                getString(
                    R.string.vr_runtime_unavailable_format,
                    vrRuntimePreflight.detail
                )
            );
            return;
        }

        VrStartupGuard.begin(this);

        try {
            startActivityForResult(
                new Intent(this, VrActivity.class),
                VR_LAUNCH_REQUEST
            );
        } catch (RuntimeException exception) {
            VrStartupGuard.recordFailure(
                this,
                "activity-launch",
                exception
            );
            ReverieLog.error(
                "VR_STARTUP",
                "Android rejected the VR activity launch.",
                exception
            );
            uiFeedback.failure(enterVrButton);
            showPreviousVrStartupFailureIfNeeded();
        }
    }

    private void showPreviousVrStartupFailureIfNeeded() {
        String failure =
            VrStartupGuard.consumePendingFailure(this);
        if (failure == null
            || failure.trim().isEmpty()) {
            return;
        }

        showVrStartupFailure(failure);
    }

    private void showVrStartupFailure(String message) {
        new AlertDialog.Builder(this)
            .setTitle(R.string.vr_startup_failure_title)
            .setMessage(message)
            .setPositiveButton(
                android.R.string.ok,
                null
            )
            .show();
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
        if (uiFeedback != null) {
            uiFeedback.close();
        }
        dosImportExecutor.shutdownNow();
        diagnosticExecutor.shutdownNow();
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
        appVersionText = findViewById(R.id.app_version);
        phoneBatteryText = findViewById(R.id.phone_battery_text);
        controllerBatteryText = findViewById(R.id.controller_battery_text);
        controllerStatusText = findViewById(R.id.controller_status);
        controllerInputTestText = findViewById(R.id.controller_input_test);
        deviceStatusText = findViewById(R.id.device_status);
        updateStatusText = findViewById(R.id.update_status);
        selectedVideoText = findViewById(R.id.selected_video_status);
        dosModuleStatusText = findViewById(R.id.dos_module_status);
        loggingStatusText = findViewById(R.id.logging_status);
        vrRuntimeStatusText = findViewById(R.id.vr_runtime_status);
        updatePanel = findViewById(R.id.update_panel);
        updatePanel.setVisibility(
            BuildConfig.UPDATE_CHANNEL_ENABLED
                ? View.VISIBLE
                : View.GONE
        );

        phoneBatteryBar = findViewById(R.id.phone_battery_bar);
        controllerBatteryBar = findViewById(R.id.controller_battery_bar);

        batteryHudSwitch = findViewById(R.id.battery_hud_switch);
        lookUpRevealSwitch = findViewById(R.id.look_up_reveal_switch);
        showPercentagesSwitch = findViewById(R.id.show_percentages_switch);
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
        exportLogsButton = findViewById(R.id.export_logs_button);
        submitDiagnosticsButton =
            findViewById(R.id.submit_diagnostics_button);
        clearLogsButton = findViewById(R.id.clear_logs_button);
        videoProjectionGroup = findViewById(R.id.video_projection_group);
        loggingModeGroup = findViewById(R.id.logging_mode_group);
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
        autoUpdateCheckSwitch.setOnCheckedChangeListener(
            (button, checked) -> preferences.setAutoUpdateCheckEnabled(checked)
        );

        loggingModeGroup.setOnCheckedChangeListener(
            (group, checkedId) -> {
                LoggingMode mode =
                    checkedId == R.id.logging_development
                        ? LoggingMode.DEVELOPMENT
                        : LoggingMode.STANDARD;
                preferences.setLoggingMode(mode);
                ReverieLog.setMode(mode);
                refreshLoggingStatus();
            }
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
        uiFeedback.bind(
            pairControllerButton,
            this::beginControllerPairing
        );
        uiFeedback.bind(
            phoneEmulatorButton,
            this::beginPhoneController
        );

        testControllerButton.setEnabled(false);
        uiFeedback.bind(
            testControllerButton,
            () -> {
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
            }
        );

        Button bluetoothButton =
            findViewById(R.id.bluetooth_settings_button);
        uiFeedback.bind(
            bluetoothButton,
            this::openBluetoothSettings
        );

        uiFeedback.bind(
            checkUpdateButton,
            () -> checkForUpdates(true)
        );
        installUpdateButton.setEnabled(false);
        uiFeedback.bind(
            installUpdateButton,
            this::confirmInstallAvailableUpdate
        );

        uiFeedback.bind(
            chooseVideoButton,
            this::chooseLocalVideo
        );
        uiFeedback.bind(
            clearVideoButton,
            this::clearSelectedVideo
        );

        uiFeedback.bind(
            importDosButton,
            this::chooseDosContent
        );
        uiFeedback.bind(
            clearDosModulesButton,
            this::clearDosModules
        );

        uiFeedback.bind(
            exportLogsButton,
            this::chooseDiagnosticExportDestination
        );

        boolean diagnosticSubmissionConfigured =
            diagnosticSubmissionClient != null
                && diagnosticSubmissionClient.isConfigured();
        submitDiagnosticsButton.setVisibility(
            diagnosticSubmissionConfigured
                ? View.VISIBLE
                : View.GONE
        );
        if (diagnosticSubmissionConfigured) {
            uiFeedback.bind(
                submitDiagnosticsButton,
                this::beginDiagnosticSubmission
            );
        }

        uiFeedback.bind(
            clearLogsButton,
            this::confirmClearLogs
        );

        Button resetButton =
            findViewById(R.id.reset_settings_button);
        uiFeedback.bind(
            resetButton,
            this::confirmReset
        );

        uiFeedback.bind(
            enterVrButton,
            this::launchVr
        );
    }

    private void beginControllerPairing() {
        controllerActionButton = pairControllerButton;
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
        controllerActionButton = phoneEmulatorButton;
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
            uiFeedback.failure(phoneEmulatorButton);
            controllerActionButton = null;
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

        controllerActionButton = null;
        new AlertDialog.Builder(this)
            .setTitle(R.string.phone_controller_choose_title)
            .setItems(labels, (dialog, which) -> {
                if (which >= 0 && which < targets.size()) {
                    controllerActionButton = phoneEmulatorButton;
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
        boolean vrRuntimeReady =
            vrRuntimePreflight != null
                && vrRuntimePreflight.available;

        if (enterVrButton != null) {
            enterVrButton.setEnabled(
                vrRuntimeReady
                    && (providerReady || gamepadReady)
            );
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
    public void onPointerAxis(
        float horizontal,
        float vertical,
        String source
    ) {
        // Stage A does not render a spatial pointer.
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
            uiFeedback.failure(chooseVideoButton);
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

        if (requestCode == VR_LAUNCH_REQUEST) {
            String failure =
                data == null
                    ? ""
                    : data.getStringExtra(
                        VrActivity.EXTRA_STARTUP_ERROR
                    );

            if (failure != null
                && !failure.trim().isEmpty()) {
                VrStartupGuard.clear(this);
                uiFeedback.failure(enterVrButton);
                showVrStartupFailure(failure);
                return;
            }

            if (resultCode == RESULT_OK
                && data != null) {
                if (data.getBooleanExtra(
                        VrActivity.EXTRA_REQUEST_MEDIA_PICKER,
                        false
                    )) {
                    chooseLocalVideo();
                    return;
                }

                if (data.getBooleanExtra(
                        VrActivity.EXTRA_REQUEST_DOS_PICKER,
                        false
                    )) {
                    chooseDosContent();
                    return;
                }
            }
            return;
        }

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
            return;
        }

        if (requestCode == LOG_EXPORT_REQUEST) {
            handleDiagnosticExport(data.getData());
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
            uiFeedback.failure(chooseVideoButton);
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
            resolveDisplayName(
                uri,
                getString(
                    R.string.selected_video_unknown_name
                )
            )
        );
        refreshMediaStatus();
    }

    private void handleSelectedDosContent(Intent data) {
        Uri uri = data.getData();
        if (uri == null) {
            return;
        }

        String displayName = resolveDisplayName(
            uri,
            "dos-content"
        );
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

                ReverieLog.milestone(
                    "DOS_IMPORT",
                    "Imported module "
                        + module.displayName
                        + " from "
                        + module.originalFileName
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
                ReverieLog.error(
                    "DOS_IMPORT",
                    "DOS content import failed.",
                    exception
                );

                String message =
                    exception.getMessage() == null
                        ? exception.getClass().getSimpleName()
                        : exception.getMessage();

                runOnUiThread(() -> {
                    importDosButton.setEnabled(true);
                    refreshDosModuleStatus();
                    uiFeedback.failure(importDosButton);
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

    private String resolveDisplayName(
        Uri uri,
        String fallback
    ) {
        String safeFallback =
            fallback == null || fallback.trim().isEmpty()
                ? "selected-content"
                : fallback.trim();

        if (uri == null) {
            return safeFallback;
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
            // The selected document may still be readable without metadata.
        }

        String last = uri.getLastPathSegment();
        return last == null || last.trim().isEmpty()
            ? safeFallback
            : last;
    }

    private void chooseDiagnosticExportDestination() {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/zip");

        String stamp =
            LocalDateTime.now().format(
                DateTimeFormatter.ofPattern(
                    "yyyyMMdd-HHmmss",
                    Locale.US
                )
            );
        intent.putExtra(
            Intent.EXTRA_TITLE,
            "ReverieVR-diagnostics-"
                + stamp
                + ".zip"
        );

        try {
            startActivityForResult(
                intent,
                LOG_EXPORT_REQUEST
            );
        } catch (ActivityNotFoundException exception) {
            ReverieLog.error(
                "DIAGNOSTICS",
                "No document provider available for log export.",
                exception
            );
            uiFeedback.failure(exportLogsButton);
            Toast.makeText(
                this,
                R.string.log_export_unavailable,
                Toast.LENGTH_LONG
            ).show();
        }
    }

    private void handleDiagnosticExport(Uri destination) {
        if (destination == null) {
            return;
        }

        exportLogsButton.setEnabled(false);
        loggingStatusText.setText(
            R.string.log_export_in_progress
        );

        diagnosticExecutor.execute(() -> {
            try {
                ReverieLog.milestone(
                    "DIAGNOSTICS",
                    "Manual diagnostic export started."
                );
                diagnosticBundleExporter.export(
                    destination
                );
                ReverieLog.milestone(
                    "DIAGNOSTICS",
                    "Manual diagnostic export completed."
                );

                runOnUiThread(() -> {
                    exportLogsButton.setEnabled(true);
                    refreshLoggingStatus();
                    Toast.makeText(
                        this,
                        R.string.log_export_complete,
                        Toast.LENGTH_SHORT
                    ).show();
                });
            } catch (Exception exception) {
                ReverieLog.error(
                    "DIAGNOSTICS",
                    "Manual diagnostic export failed.",
                    exception
                );

                runOnUiThread(() -> {
                    exportLogsButton.setEnabled(true);
                    refreshLoggingStatus();
                    uiFeedback.failure(exportLogsButton);
                    Toast.makeText(
                        this,
                        getString(
                            R.string.log_export_failed_format,
                            exception.getMessage() == null
                                ? exception.getClass().getSimpleName()
                                : exception.getMessage()
                        ),
                        Toast.LENGTH_LONG
                    ).show();
                });
            }
        });
    }

    private void beginDiagnosticSubmission() {
        if (diagnosticSubmissionClient == null
            || !diagnosticSubmissionClient.isConfigured()) {
            return;
        }

        PendingDiagnostic pending =
            loadPendingDiagnostic();
        if (pending != null) {
            showPendingDiagnosticChoice(
                pending
            );
            return;
        }

        int padding =
            Math.round(
                16f
                    * getResources()
                        .getDisplayMetrics()
                        .density
            );

        LinearLayout form = new LinearLayout(this);
        form.setOrientation(LinearLayout.VERTICAL);
        form.setPadding(
            padding,
            padding / 2,
            padding,
            0
        );

        TextView privacy = new TextView(this);
        privacy.setText(
            R.string.diagnostic_submit_privacy
        );
        privacy.setTextSize(14f);
        form.addView(privacy);

        EditText summary = new EditText(this);
        summary.setHint(
            R.string.diagnostic_summary_hint
        );
        summary.setInputType(
            InputType.TYPE_CLASS_TEXT
                | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES
                | InputType.TYPE_TEXT_FLAG_MULTI_LINE
        );
        summary.setMinLines(3);
        summary.setMaxLines(8);
        form.addView(summary);

        EditText expected = new EditText(this);
        expected.setHint(
            R.string.diagnostic_expected_hint
        );
        expected.setInputType(
            InputType.TYPE_CLASS_TEXT
                | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES
                | InputType.TYPE_TEXT_FLAG_MULTI_LINE
        );
        expected.setMinLines(2);
        expected.setMaxLines(6);
        form.addView(expected);

        CheckBox includeLogs = new CheckBox(this);
        includeLogs.setText(
            R.string.diagnostic_include_logs
        );
        includeLogs.setChecked(true);
        form.addView(includeLogs);

        AlertDialog dialog =
            new AlertDialog.Builder(this)
                .setTitle(
                    R.string.diagnostic_submit_title
                )
                .setView(form)
                .setNegativeButton(
                    android.R.string.cancel,
                    null
                )
                .setPositiveButton(
                    R.string.diagnostic_submit_confirm,
                    null
                )
                .create();

        dialog.setOnShowListener(
            ignored ->
                dialog.getButton(
                    AlertDialog.BUTTON_POSITIVE
                ).setOnClickListener(view -> {
                    String summaryText =
                        summary.getText()
                            .toString()
                            .trim();
                    if (summaryText.isEmpty()) {
                        uiFeedback.failure(view);
                        summary.setError(
                            getString(
                                R.string.diagnostic_summary_required
                            )
                        );
                        return;
                    }

                    String expectedText =
                        expected.getText()
                            .toString()
                            .trim();
                    boolean logs =
                        includeLogs.isChecked();

                    dialog.dismiss();
                    prepareDiagnosticSubmission(
                        summaryText,
                        expectedText,
                        logs
                    );
                })
        );
        dialog.show();
    }

    private void prepareDiagnosticSubmission(
        String summary,
        String expected,
        boolean includeLogs
    ) {
        if (diagnosticSubmissionClient == null
            || diagnosticBundleExporter == null
            || submitDiagnosticsButton == null) {
            return;
        }

        final String diagnosticId =
            DiagnosticSubmissionClient
                .newDiagnosticId();

        submitDiagnosticsButton.setEnabled(false);
        loggingStatusText.setText(
            R.string.diagnostic_preparing_preview
        );

        diagnosticExecutor.execute(() -> {
            File bundle = null;
            try {
                ReverieLog.milestone(
                    "DIAGNOSTICS",
                    "Preparing diagnostic submission preview: "
                        + diagnosticId
                        + ", includeLogs="
                        + includeLogs
                );

                bundle =
                    diagnosticBundleExporter
                        .createSubmissionBundle(
                            diagnosticId,
                            includeLogs
                        );

                String sha256 =
                    DiagnosticSubmissionClient
                        .sha256(bundle);
                long byteLength =
                    bundle.length();

                ReverieLog.milestone(
                    "DIAGNOSTICS",
                    "Diagnostic submission preview prepared: "
                        + diagnosticId
                        + ", bytes="
                        + byteLength
                        + ", sha256="
                        + sha256
                );

                File preparedBundle = bundle;
                runOnUiThread(() -> {
                    refreshLoggingStatus();
                    showDiagnosticSubmissionPreview(
                        preparedBundle,
                        diagnosticId,
                        sha256,
                        summary,
                        expected,
                        includeLogs
                    );
                });
            } catch (Exception exception) {
                boolean retained =
                    bundle != null
                        && bundle.isFile();

                ReverieLog.error(
                    "DIAGNOSTICS",
                    "Diagnostic submission preview preparation failed: "
                        + diagnosticId
                        + ", retained="
                        + retained,
                    exception
                );

                String message =
                    exception.getMessage() == null
                        ? exception.getClass()
                            .getSimpleName()
                        : exception.getMessage();

                runOnUiThread(() -> {
                    submitDiagnosticsButton.setEnabled(true);
                    refreshLoggingStatus();
                    uiFeedback.failure(
                        submitDiagnosticsButton
                    );
                    showDiagnosticSubmissionFailure(
                        message,
                        retained
                    );
                });
            }
        });
    }

    private void showDiagnosticSubmissionPreview(
        File bundle,
        String diagnosticId,
        String sha256,
        String summary,
        String expected,
        boolean includeLogs
    ) {
        if (submitDiagnosticsButton == null) {
            return;
        }
        if (bundle == null
            || !bundle.isFile()) {
            submitDiagnosticsButton.setEnabled(true);
            refreshLoggingStatus();
            return;
        }

        String expectedText =
            expected == null
                    || expected.trim().isEmpty()
                ? getString(
                    R.string.diagnostic_expected_not_supplied
                )
                : expected.trim();

        String logsText =
            getString(
                includeLogs
                    ? R.string.diagnostic_logs_yes
                    : R.string.diagnostic_logs_no
            );

        String device =
            Build.MANUFACTURER
                + " "
                + Build.MODEL
                + " ("
                + Build.DEVICE
                + ")";
        String androidVersion =
            "Android "
                + Build.VERSION.RELEASE
                + " / API "
                + Build.VERSION.SDK_INT;

        AlertDialog preview =
            new AlertDialog.Builder(this)
                .setTitle(
                    R.string.diagnostic_preview_title
                )
                .setMessage(
                    getString(
                        R.string.diagnostic_preview_format,
                        diagnosticId,
                        bundle.length(),
                        sha256,
                        logsText,
                        BuildConfig.VERSION_NAME,
                        BuildConfig.BUILD_TYPE,
                        device,
                        androidVersion,
                        summary,
                        expectedText
                    )
                )
                .setNegativeButton(
                    android.R.string.cancel,
                    (dialog, which) ->
                        cancelPreparedDiagnostic(
                            bundle,
                            diagnosticId
                        )
                )
                .setPositiveButton(
                    R.string.diagnostic_preview_submit,
                    (dialog, which) ->
                        uploadPreparedDiagnostic(
                            bundle,
                            diagnosticId,
                            sha256,
                            summary,
                            expectedText
                        )
                )
                .create();

        preview.setOnCancelListener(
            dialog ->
                cancelPreparedDiagnostic(
                    bundle,
                    diagnosticId
                )
        );
        preview.show();
    }

    private void cancelPreparedDiagnostic(
        File bundle,
        String diagnosticId
    ) {
        boolean removed =
            bundle == null
                || !bundle.isFile()
                || bundle.delete();

        if (!removed) {
            ReverieLog.incident(
                "DIAGNOSTICS",
                "Cancelled diagnostic preview bundle could not be "
                    + "removed from private outbox: "
                    + bundle.getAbsolutePath()
            );
        } else {
            ReverieLog.milestone(
                "DIAGNOSTICS",
                "Diagnostic submission cancelled before upload: "
                    + diagnosticId
            );
        }

        submitDiagnosticsButton.setEnabled(true);
        refreshLoggingStatus();
        Toast.makeText(
            this,
            R.string.diagnostic_preview_cancelled,
            Toast.LENGTH_SHORT
        ).show();
    }

    private void uploadPreparedDiagnostic(
        File bundle,
        String diagnosticId,
        String preparedSha256,
        String summary,
        String expected
    ) {
        if (submitDiagnosticsButton == null) {
            return;
        }
        if (diagnosticSubmissionClient == null
            || bundle == null
            || !bundle.isFile()) {
            submitDiagnosticsButton.setEnabled(true);
            refreshLoggingStatus();
            return;
        }

        submitDiagnosticsButton.setEnabled(false);
        loggingStatusText.setText(
            R.string.diagnostic_submission_in_progress
        );

        diagnosticExecutor.execute(() -> {
            try {
                String currentSha256 =
                    DiagnosticSubmissionClient
                        .sha256(bundle);
                if (!preparedSha256.equals(
                        currentSha256
                    )) {
                    throw new java.io.IOException(
                        "Prepared diagnostic bundle changed "
                            + "after preview."
                    );
                }

                ReverieLog.milestone(
                    "DIAGNOSTICS",
                    "Secure diagnostic submission started: "
                        + diagnosticId
                        + ", bytes="
                        + bundle.length()
                        + ", sha256="
                        + preparedSha256
                );

                DiagnosticSubmissionClient.Result result =
                    diagnosticSubmissionClient.submit(
                        bundle,
                        diagnosticId,
                        summary,
                        expected
                    );

                if (!preparedSha256.equals(
                        result.sha256
                    )) {
                    throw new java.io.IOException(
                        "Diagnostic intake returned a different "
                            + "hash than the reviewed bundle."
                    );
                }

                if (bundle.isFile()
                    && !bundle.delete()) {
                    ReverieLog.dev(
                        "DIAGNOSTICS",
                        "Submitted diagnostic bundle could not be "
                            + "removed from private outbox: "
                            + bundle.getAbsolutePath()
                    );
                }

                ReverieLog.milestone(
                    "DIAGNOSTICS",
                    "Secure diagnostic submission completed: "
                        + result.diagnosticId
                        + " issue="
                        + result.issueUrl
                );

                clearPendingDiagnostic();

                runOnUiThread(() -> {
                    submitDiagnosticsButton.setEnabled(true);
                    refreshLoggingStatus();
                    showDiagnosticSubmissionSuccess(
                        result
                    );
                });
            } catch (Exception exception) {
                boolean retained =
                    bundle.isFile();

                ReverieLog.error(
                    "DIAGNOSTICS",
                    "Secure diagnostic submission failed: "
                        + diagnosticId
                        + ", retained="
                        + retained,
                    exception
                );

                if (exception
                    instanceof DiagnosticSubmissionClient.SubmissionException) {
                    DiagnosticSubmissionClient.SubmissionException
                        submissionException =
                            (DiagnosticSubmissionClient.SubmissionException)
                                exception;

                    if (submissionException.storedRemotely
                        && submissionException.canFinalize) {
                        rememberPendingDiagnostic(
                            submissionException
                        );

                        runOnUiThread(() -> {
                            submitDiagnosticsButton.setEnabled(true);
                            refreshLoggingStatus();
                            uiFeedback.failure(
                                submitDiagnosticsButton
                            );
                            showStoredDiagnosticPending(
                                submissionException
                            );
                        });
                        return;
                    }
                }

                String message =
                    exception.getMessage() == null
                        ? exception.getClass()
                            .getSimpleName()
                        : exception.getMessage();

                runOnUiThread(() -> {
                    submitDiagnosticsButton.setEnabled(true);
                    refreshLoggingStatus();
                    uiFeedback.failure(
                        submitDiagnosticsButton
                    );
                    showDiagnosticSubmissionFailure(
                        message,
                        retained
                    );
                });
            }
        });
    }

    private void showStoredDiagnosticPending(
        DiagnosticSubmissionClient.SubmissionException
            failure
    ) {
        if (failure == null) {
            return;
        }

        String receipt =
            failure.receiptReference == null
                    || failure.receiptReference.trim().isEmpty()
                ? failure.diagnosticId
                : failure.receiptReference;

        new AlertDialog.Builder(this)
            .setTitle(
                R.string.diagnostic_stored_pending_title
            )
            .setMessage(
                getString(
                    R.string.diagnostic_stored_pending_format,
                    failure.getMessage() == null
                        ? "GitHub issue creation failed."
                        : failure.getMessage(),
                    receipt
                )
            )
            .setNegativeButton(
                android.R.string.ok,
                null
            )
            .setPositiveButton(
                R.string.diagnostic_retry_issue,
                (dialog, which) ->
                    retryStoredDiagnostic(
                        new PendingDiagnostic(
                            failure.diagnosticId,
                            failure.sha256,
                            receipt
                        )
                    )
            )
            .show();
    }

    private void showPendingDiagnosticChoice(
        PendingDiagnostic pending
    ) {
        new AlertDialog.Builder(this)
            .setTitle(
                R.string.diagnostic_pending_found_title
            )
            .setMessage(
                getString(
                    R.string.diagnostic_pending_found_format,
                    pending.diagnosticId
                )
            )
            .setNegativeButton(
                R.string.diagnostic_start_new_report,
                (dialog, which) -> {
                    ReverieLog.milestone(
                        "DIAGNOSTICS",
                        "User abandoned pending diagnostic receipt "
                            + pending.receiptReference
                            + " to start a new report."
                    );
                    deleteLocalDiagnosticBundle(
                        pending.diagnosticId
                    );
                    clearPendingDiagnostic();
                    beginDiagnosticSubmission();
                }
            )
            .setPositiveButton(
                R.string.diagnostic_retry_issue,
                (dialog, which) ->
                    retryStoredDiagnostic(
                        pending
                    )
            )
            .show();
    }

    private void retryStoredDiagnostic(
        PendingDiagnostic pending
    ) {
        if (pending == null
            || diagnosticSubmissionClient == null
            || submitDiagnosticsButton == null) {
            return;
        }

        submitDiagnosticsButton.setEnabled(false);
        loggingStatusText.setText(
            R.string.diagnostic_retrying_issue
        );

        diagnosticExecutor.execute(() -> {
            try {
                ReverieLog.milestone(
                    "DIAGNOSTICS",
                    "Retrying GitHub issue creation from stored receipt "
                        + pending.receiptReference
                );

                DiagnosticSubmissionClient.Result result =
                    diagnosticSubmissionClient
                        .finalizeStored(
                            pending.diagnosticId,
                            pending.sha256
                        );

                if (!pending.sha256.equals(
                        result.sha256
                    )) {
                    throw new java.io.IOException(
                        "Stored diagnostic finalize returned "
                            + "a different hash."
                    );
                }

                deleteLocalDiagnosticBundle(
                    pending.diagnosticId
                );
                clearPendingDiagnostic();

                ReverieLog.milestone(
                    "DIAGNOSTICS",
                    "Stored diagnostic receipt finalized: "
                        + result.diagnosticId
                        + " issue="
                        + result.issueUrl
                );

                runOnUiThread(() -> {
                    submitDiagnosticsButton.setEnabled(true);
                    refreshLoggingStatus();
                    showDiagnosticSubmissionSuccess(
                        result
                    );
                });
            } catch (Exception exception) {
                ReverieLog.error(
                    "DIAGNOSTICS",
                    "Stored diagnostic receipt finalize failed: "
                        + pending.receiptReference,
                    exception
                );

                if (exception
                    instanceof DiagnosticSubmissionClient.SubmissionException) {
                    DiagnosticSubmissionClient.SubmissionException
                        submissionException =
                            (DiagnosticSubmissionClient.SubmissionException)
                                exception;

                    if (submissionException.storedRemotely
                        && submissionException.canFinalize) {
                        rememberPendingDiagnostic(
                            submissionException
                        );

                        runOnUiThread(() -> {
                            submitDiagnosticsButton.setEnabled(true);
                            refreshLoggingStatus();
                            uiFeedback.failure(
                                submitDiagnosticsButton
                            );
                            showStoredDiagnosticPending(
                                submissionException
                            );
                        });
                        return;
                    }

                    // The server gave a definitive response that this
                    // receipt can no longer be finalized. Do not trap
                    // future Submit actions behind a stale marker.
                    clearPendingDiagnostic();
                }

                boolean retained =
                    localDiagnosticBundle(
                        pending.diagnosticId
                    ).isFile();
                String message =
                    exception.getMessage() == null
                        ? exception.getClass()
                            .getSimpleName()
                        : exception.getMessage();

                runOnUiThread(() -> {
                    submitDiagnosticsButton.setEnabled(true);
                    refreshLoggingStatus();
                    uiFeedback.failure(
                        submitDiagnosticsButton
                    );
                    showDiagnosticSubmissionFailure(
                        message,
                        retained
                    );
                });
            }
        });
    }

    private void rememberPendingDiagnostic(
        DiagnosticSubmissionClient.SubmissionException
            failure
    ) {
        if (failure == null
            || failure.diagnosticId == null
            || failure.diagnosticId.trim().isEmpty()
            || failure.sha256 == null
            || failure.sha256.trim().isEmpty()) {
            return;
        }

        getSharedPreferences(
            DIAGNOSTIC_PENDING_PREFS,
            MODE_PRIVATE
        )
            .edit()
            .putString(
                DIAGNOSTIC_PENDING_ID,
                failure.diagnosticId
            )
            .putString(
                DIAGNOSTIC_PENDING_SHA256,
                failure.sha256
            )
            .putString(
                DIAGNOSTIC_PENDING_RECEIPT,
                failure.receiptReference
            )
            .apply();
    }

    private PendingDiagnostic loadPendingDiagnostic() {
        SharedPreferences pending =
            getSharedPreferences(
                DIAGNOSTIC_PENDING_PREFS,
                MODE_PRIVATE
            );

        String diagnosticId =
            pending.getString(
                DIAGNOSTIC_PENDING_ID,
                ""
            );
        String sha256 =
            pending.getString(
                DIAGNOSTIC_PENDING_SHA256,
                ""
            );
        String receipt =
            pending.getString(
                DIAGNOSTIC_PENDING_RECEIPT,
                ""
            );

        if (diagnosticId == null
            || diagnosticId.trim().isEmpty()
            || sha256 == null
            || sha256.trim().isEmpty()) {
            return null;
        }

        return new PendingDiagnostic(
            diagnosticId.trim(),
            sha256.trim(),
            receipt == null
                || receipt.trim().isEmpty()
                    ? diagnosticId.trim()
                    : receipt.trim()
        );
    }

    private void clearPendingDiagnostic() {
        getSharedPreferences(
            DIAGNOSTIC_PENDING_PREFS,
            MODE_PRIVATE
        )
            .edit()
            .clear()
            .apply();
    }

    private File localDiagnosticBundle(
        String diagnosticId
    ) {
        return new File(
            new File(
                getFilesDir(),
                "diagnostic-outbox"
            ),
            diagnosticId + ".zip"
        );
    }

    private void deleteLocalDiagnosticBundle(
        String diagnosticId
    ) {
        File bundle =
            localDiagnosticBundle(
                diagnosticId
            );
        if (bundle.isFile()
            && !bundle.delete()) {
            ReverieLog.dev(
                "DIAGNOSTICS",
                "Finalized diagnostic bundle could not be "
                    + "removed from private outbox: "
                    + bundle.getAbsolutePath()
            );
        }
    }

    private void showDiagnosticSubmissionSuccess(
        DiagnosticSubmissionClient.Result result
    ) {
        if (result == null) {
            return;
        }

        new AlertDialog.Builder(this)
            .setTitle(
                R.string.diagnostic_submission_success_title
            )
            .setMessage(
                getString(
                    R.string.diagnostic_submission_success_format,
                    result.diagnosticId,
                    result.receiptReference
                )
            )
            .setNegativeButton(
                android.R.string.ok,
                null
            )
            .setPositiveButton(
                R.string.diagnostic_open_issue,
                (dialog, which) ->
                    openDiagnosticIssue(
                        result.issueUrl
                    )
            )
            .show();
    }

    private void showDiagnosticSubmissionFailure(
        String failure,
        boolean retained
    ) {
        int format =
            retained
                ? R.string.diagnostic_submission_failed_format
                : R.string.diagnostic_submission_failed_no_bundle_format;

        new AlertDialog.Builder(this)
            .setTitle(
                R.string.diagnostic_submission_failed_title
            )
            .setMessage(
                getString(
                    format,
                    failure == null
                        ? "Unknown error"
                        : failure
                )
            )
            .setPositiveButton(
                android.R.string.ok,
                null
            )
            .show();
    }

    private void openDiagnosticIssue(
        String issueUrl
    ) {
        if (issueUrl == null
            || !issueUrl.startsWith(
                "https://github.com/mrcalzon02/ReverieVR/issues/"
            )) {
            return;
        }

        try {
            startActivity(
                new Intent(
                    Intent.ACTION_VIEW,
                    Uri.parse(issueUrl)
                )
            );
        } catch (ActivityNotFoundException exception) {
            Toast.makeText(
                this,
                issueUrl,
                Toast.LENGTH_LONG
            ).show();
        }
    }

    private void confirmClearLogs() {
        new AlertDialog.Builder(this)
            .setTitle(R.string.clear_logs_title)
            .setMessage(R.string.clear_logs_message)
            .setNegativeButton(android.R.string.cancel, null)
            .setPositiveButton(
                R.string.clear_logs_confirm,
                (dialog, which) -> {
                    ReverieLog.clearLogs();
                    refreshLoggingStatus();
                }
            )
            .show();
    }

    private void refreshLoggingStatus() {
        if (loggingStatusText == null) {
            return;
        }

        LoggingMode mode =
            preferences == null
                ? ReverieLog.getMode()
                : preferences.getLoggingMode();

        loggingStatusText.setText(
            mode == LoggingMode.DEVELOPMENT
                ? R.string.logging_status_development
                : R.string.logging_status_standard
        );
    }

    private void installBundledDosContent() {
        if (!BuildConfig.DOOM_SHAREWARE_BUNDLED) {
            ReverieLog.milestone(
                "BUNDLED_CONTENT",
                "DOOM Shareware is not bundled in this build."
            );
            return;
        }

        dosImportExecutor.execute(() -> {
            try {
                DosGameModule module =
                    new BundledDosContentInstaller(
                        this,
                        dosModuleRepository
                    ).ensureBundledDoom();

                if (module != null) {
                    runOnUiThread(this::refreshDosModuleStatus);
                }
            } catch (Exception exception) {
                ReverieLog.error(
                    "BUNDLED_CONTENT",
                    "Default DOOM Shareware module could not be installed.",
                    exception
                );
            }
        });
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
            uiFeedback.failure(importDosButton);
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
            Button failedButton =
                pending == PENDING_CONTROLLER_PHONE
                    ? phoneEmulatorButton
                    : pairControllerButton;
            uiFeedback.failure(failedButton);
            controllerActionButton = null;
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
        autoUpdateCheckSwitch.setChecked(
            preferences.isAutoUpdateCheckEnabled()
        );

        LoggingMode loggingMode =
            preferences.getLoggingMode();
        loggingModeGroup.check(
            loggingMode == LoggingMode.DEVELOPMENT
                ? R.id.logging_development
                : R.id.logging_standard
        );
        refreshLoggingStatus();

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
        refreshVrRuntimeStatus();

        appVersionText.setText(
            getString(
                R.string.app_version_format,
                BuildConfig.VERSION_NAME,
                BuildConfig.VERSION_CODE,
                buildIdentityLabel()
            )
        );
    }

    private void refreshVrRuntimeStatus() {
        if (vrRuntimePreflight == null) {
            vrRuntimePreflight =
                VrRuntimePreflight.check();
        }

        if (vrRuntimeStatusText == null) {
            return;
        }

        if (vrRuntimePreflight.available) {
            vrRuntimeStatusText.setText(
                R.string.vr_runtime_ready
            );
        } else {
            vrRuntimeStatusText.setText(
                getString(
                    R.string.vr_runtime_unavailable_format,
                    vrRuntimePreflight.detail
                )
            );
        }
    }

    private String buildIdentityLabel() {
        String revision =
            BuildConfig.SOURCE_REVISION == null
                ? ""
                : BuildConfig.SOURCE_REVISION.trim();
        if (revision.length() > 7) {
            revision = revision.substring(0, 7);
        }
        if (revision.isEmpty()) {
            revision = "unknown";
        }

        if (BuildConfig.PHONE_TEST_RUN_NUMBER > 0) {
            return String.format(
                Locale.US,
                "phone-test-%d-%d • %s",
                BuildConfig.PHONE_TEST_RUN_NUMBER,
                BuildConfig.PHONE_TEST_RUN_ATTEMPT,
                revision
            );
        }

        return "local/dev • " + revision;
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

        ReverieLog.milestone(
            "CONTROLLER",
            "State="
                + state
                + " message="
                + controllerConnectionMessage
        );

        runOnUiThread(() -> {
            if (state == ControllerProvider.ConnectionState.READY) {
                controllerActionButton = null;
            } else if (
                state == ControllerProvider.ConnectionState.ERROR
                    || state
                        == ControllerProvider.ConnectionState.BLUETOOTH_DISABLED
            ) {
                if (controllerActionButton != null) {
                    uiFeedback.failure(controllerActionButton);
                    controllerActionButton = null;
                }
            }
            refreshInputReadiness();
        });
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

        updateChecker.check(
            BuildConfig.VERSION_NAME,
            BuildConfig.PHONE_TEST_RUN_NUMBER,
            BuildConfig.PHONE_TEST_RUN_ATTEMPT,
            result ->
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
                if (userInitiated) {
                    uiFeedback.failure(checkUpdateButton);
                }
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
                if (userInitiated) {
                    uiFeedback.failure(checkUpdateButton);
                }
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
            uiFeedback.failure();
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
            uiFeedback.failure();
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
    private static final class PendingDiagnostic {
        final String diagnosticId;
        final String sha256;
        final String receiptReference;

        PendingDiagnostic(
            String diagnosticId,
            String sha256,
            String receiptReference
        ) {
            this.diagnosticId = diagnosticId;
            this.sha256 = sha256;
            this.receiptReference =
                receiptReference;
        }
    }

}
