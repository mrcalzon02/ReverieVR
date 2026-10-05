package io.github.mrcalzon02.reverievr;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothStatusCodes;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.Queue;
import java.util.UUID;

final class DaydreamControllerProvider implements ControllerProvider {
    private static final UUID DAYDREAM_SERVICE =
        UUID.fromString("0000fe55-0000-1000-8000-00805f9b34fb");
    private static final UUID POSE_CHARACTERISTIC =
        UUID.fromString("00000001-1000-1000-8000-00805f9b34fb");
    private static final UUID CONTROL_CHARACTERISTIC =
        UUID.fromString("00000002-1000-1000-8000-00805f9b34fb");
    private static final UUID VOLTAGE_CHARACTERISTIC =
        UUID.fromString("00000003-1000-1000-8000-00805f9b34fb");

    private static final UUID BATTERY_SERVICE =
        UUID.fromString("0000180f-0000-1000-8000-00805f9b34fb");
    private static final UUID BATTERY_LEVEL_CHARACTERISTIC =
        UUID.fromString("00002a19-0000-1000-8000-00805f9b34fb");
    private static final UUID CLIENT_CONFIG_DESCRIPTOR =
        UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    private static final long SCAN_TIMEOUT_MILLIS = 15000L;
    private static final long POSE_STREAM_TIMEOUT_MILLIS = 8000L;

    private final Context context;
    private final BluetoothAdapter adapter;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Queue<GattOperation> operationQueue = new ArrayDeque<>();

    private Listener listener;
    private BluetoothLeScanner scanner;
    private BluetoothDevice pendingDevice;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic controlCharacteristic;
    private BluetoothGattCharacteristic batteryCharacteristic;
    private BluetoothGattCharacteristic voltageCharacteristic;

    private boolean operationInFlight;
    private volatile boolean ready;
    private volatile boolean setupFailed;
    private volatile boolean poseNotificationsConfigured;
    private int batteryPercentage = -1;
    private int batteryMillivolts = -1;
    private int scanGeneration;
    private int poseWaitGeneration;

    private final BroadcastReceiver bondReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context receiverContext, Intent intent) {
            if (!BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(intent.getAction())) {
                return;
            }

            BluetoothDevice device;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                device = intent.getParcelableExtra(
                    BluetoothDevice.EXTRA_DEVICE,
                    BluetoothDevice.class
                );
            } else {
                device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            }

            if (device == null || pendingDevice == null
                || !device.equals(pendingDevice)) {
                return;
            }

            int state = intent.getIntExtra(
                BluetoothDevice.EXTRA_BOND_STATE,
                BluetoothDevice.BOND_NONE
            );

            if (state == BluetoothDevice.BOND_BONDED) {
                emitConnection(ConnectionState.CONNECTING, "Bonded. Connecting…");
                connect(device);
            } else if (state == BluetoothDevice.BOND_NONE) {
                emitConnection(
                    ConnectionState.ERROR,
                    "Controller bonding was cancelled or failed."
                );
            }
        }
    };

    private final ScanCallback scanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            BluetoothDevice device = result == null ? null : result.getDevice();
            if (device == null || !isDaydreamCandidate(result)) {
                return;
            }

            stopScan();
            pendingDevice = device;

            if (!hasConnectPermission()) {
                emitConnection(
                    ConnectionState.PERMISSION_REQUIRED,
                    "Bluetooth connect permission is required."
                );
                return;
            }

            try {
                int bondState = device.getBondState();
                if (bondState == BluetoothDevice.BOND_BONDED) {
                    emitConnection(
                        ConnectionState.CONNECTING,
                        "Controller found. Connecting…"
                    );
                    connect(device);
                } else if (bondState == BluetoothDevice.BOND_BONDING) {
                    emitConnection(
                        ConnectionState.BONDING,
                        "Waiting for Android pairing…"
                    );
                } else {
                    emitConnection(
                        ConnectionState.BONDING,
                        "Controller found. Confirm Android's pairing prompt if shown."
                    );
                    if (!device.createBond()) {
                        emitConnection(
                            ConnectionState.ERROR,
                            "Android could not start controller bonding."
                        );
                    }
                }
            } catch (SecurityException exception) {
                handleBluetoothPermissionLoss();
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            emitConnection(
                ConnectionState.ERROR,
                "Bluetooth LE scan failed (" + errorCode + ")."
            );
        }
    };

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(
            BluetoothGatt callbackGatt,
            int status,
            int newState
        ) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                ready = false;
                emitConnection(
                    ConnectionState.ERROR,
                    "Controller GATT connection failed (" + status + ")."
                );
                closeGatt();
                return;
            }

            if (newState == BluetoothProfile.STATE_CONNECTED) {
                emitConnection(
                    ConnectionState.DISCOVERING,
                    "Connected. Reading controller services…"
                );
                try {
                    if (!callbackGatt.discoverServices()) {
                        failSetup("Android could not start controller service discovery.");
                    }
                } catch (SecurityException exception) {
                    handleBluetoothPermissionLoss();
                }
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                ready = false;
                emitConnection(
                    ConnectionState.DISCONNECTED,
                    "Daydream controller disconnected."
                );
                closeGatt();
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt callbackGatt, int status) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                emitConnection(
                    ConnectionState.ERROR,
                    "Controller service discovery failed (" + status + ")."
                );
                return;
            }

            BluetoothGattService daydreamService =
                callbackGatt.getService(DAYDREAM_SERVICE);
            if (daydreamService == null) {
                emitConnection(
                    ConnectionState.ERROR,
                    "Connected device does not expose the Daydream controller service."
                );
                return;
            }

            BluetoothGattCharacteristic pose =
                daydreamService.getCharacteristic(POSE_CHARACTERISTIC);
            controlCharacteristic =
                daydreamService.getCharacteristic(CONTROL_CHARACTERISTIC);
            voltageCharacteristic =
                daydreamService.getCharacteristic(VOLTAGE_CHARACTERISTIC);

            if (pose == null) {
                emitConnection(
                    ConnectionState.ERROR,
                    "Daydream pose characteristic is missing."
                );
                return;
            }

            BluetoothGattService batteryService =
                callbackGatt.getService(BATTERY_SERVICE);
            batteryCharacteristic = batteryService == null
                ? null
                : batteryService.getCharacteristic(BATTERY_LEVEL_CHARACTERISTIC);

            ready = false;
            setupFailed = false;
            poseNotificationsConfigured = false;
            poseWaitGeneration++;
            clearGattQueue();

            if (!queueEnableNotifications(callbackGatt, pose, true)) {
                return;
            }

            if (batteryCharacteristic != null) {
                queueEnableNotifications(callbackGatt, batteryCharacteristic, false);
                queueRead(callbackGatt, batteryCharacteristic);
            }

            if (voltageCharacteristic != null) {
                queueRead(callbackGatt, voltageCharacteristic);
            }
        }

        @Override
        public void onDescriptorWrite(
            BluetoothGatt callbackGatt,
            BluetoothGattDescriptor descriptor,
            int status
        ) {
            boolean poseDescriptor =
                descriptor != null
                    && descriptor.getCharacteristic() != null
                    && POSE_CHARACTERISTIC.equals(
                        descriptor.getCharacteristic().getUuid()
                    );

            if (poseDescriptor) {
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    poseNotificationsConfigured = true;
                    emitConnection(
                        ConnectionState.DISCOVERING,
                        "Pose channel subscribed. Waiting for controller data…"
                    );
                    schedulePoseStreamTimeout();
                } else {
                    failSetup(
                        "Controller pose notification setup failed ("
                            + status
                            + ")."
                    );
                }
            }
            completeGattOperation();
        }

        @Override
        public void onCharacteristicRead(
            BluetoothGatt callbackGatt,
            BluetoothGattCharacteristic characteristic,
            int status
        ) {
            byte[] value = characteristic == null ? null : characteristic.getValue();
            handleCharacteristicValue(characteristic, value, status);
            completeGattOperation();
        }

        @Override
        public void onCharacteristicRead(
            BluetoothGatt callbackGatt,
            BluetoothGattCharacteristic characteristic,
            byte[] value,
            int status
        ) {
            handleCharacteristicValue(characteristic, value, status);
            completeGattOperation();
        }

        @Override
        public void onCharacteristicChanged(
            BluetoothGatt callbackGatt,
            BluetoothGattCharacteristic characteristic
        ) {
            handleNotification(characteristic, characteristic.getValue());
        }

        @Override
        public void onCharacteristicChanged(
            BluetoothGatt callbackGatt,
            BluetoothGattCharacteristic characteristic,
            byte[] value
        ) {
            handleNotification(characteristic, value);
        }

        @Override
        public void onCharacteristicWrite(
            BluetoothGatt callbackGatt,
            BluetoothGattCharacteristic characteristic,
            int status
        ) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                emitConnection(
                    ConnectionState.ERROR,
                    "Controller command failed (" + status + ")."
                );
            }
            completeGattOperation();
        }
    };

    DaydreamControllerProvider(Context context) {
        this.context = context.getApplicationContext();

        BluetoothManager manager =
            (BluetoothManager) this.context.getSystemService(Context.BLUETOOTH_SERVICE);
        adapter = manager == null ? null : manager.getAdapter();

        IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            this.context.registerReceiver(
                bondReceiver,
                filter,
                Context.RECEIVER_NOT_EXPORTED
            );
        } else {
            this.context.registerReceiver(bondReceiver, filter);
        }
    }

    @Override
    public String getProviderId() {
        return "daydream_ble";
    }

    @Override
    public String getDisplayName() {
        return "Daydream controller";
    }

    @Override
    public String[] getMissingRuntimePermissions() {
        List<String> missing = new ArrayList<>();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            addIfMissing(missing, Manifest.permission.BLUETOOTH_SCAN);
            addIfMissing(missing, Manifest.permission.BLUETOOTH_CONNECT);
        } else {
            addIfMissing(missing, Manifest.permission.ACCESS_FINE_LOCATION);
        }

        return missing.toArray(new String[0]);
    }

    private void addIfMissing(List<String> missing, String permission) {
        if (context.checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
            missing.add(permission);
        }
    }

    @Override
    public void setListener(Listener listener) {
        this.listener = listener;
    }

    @Override
    public void startPairing() {
        String[] missing = getMissingRuntimePermissions();
        if (missing.length > 0) {
            emitConnection(
                ConnectionState.PERMISSION_REQUIRED,
                "Bluetooth permission is required before pairing."
            );
            return;
        }

        if (adapter == null) {
            emitConnection(
                ConnectionState.ERROR,
                "This phone does not expose a Bluetooth adapter."
            );
            return;
        }

        try {
            if (!adapter.isEnabled()) {
                emitConnection(
                    ConnectionState.BLUETOOTH_DISABLED,
                    "Bluetooth is turned off."
                );
                return;
            }
        } catch (SecurityException exception) {
            handleBluetoothPermissionLoss();
            return;
        }

        disconnect();
        scanner = adapter.getBluetoothLeScanner();
        if (scanner == null) {
            emitConnection(
                ConnectionState.ERROR,
                "Android Bluetooth LE scanning is unavailable."
            );
            return;
        }

        ScanSettings settings = new ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build();

        emitConnection(
            ConnectionState.SCANNING,
            "Scanning for the Daydream controller…"
        );

        final int generation = ++scanGeneration;
        try {
            scanner.startScan(
                new ArrayList<>(),
                settings,
                scanCallback
            );
        } catch (SecurityException exception) {
            scanner = null;
            handleBluetoothPermissionLoss();
            return;
        }

        mainHandler.postDelayed(
            () -> {
                if (scanner != null && generation == scanGeneration) {
                    stopScan();
                    emitConnection(
                        ConnectionState.ERROR,
                        "No Daydream controller was found. Wake it and try again."
                    );
                }
            },
            SCAN_TIMEOUT_MILLIS
        );
    }

    @Override
    public void disconnect() {
        stopScan();
        ready = false;
        clearGattQueue();

        BluetoothGatt activeGatt = gatt;
        if (activeGatt != null) {
            try {
                activeGatt.disconnect();
            } catch (SecurityException ignored) {
                // Permission may have been revoked while the connection was active.
            } finally {
                activeGatt.close();
                gatt = null;
            }
        }

        setupFailed = false;
        poseNotificationsConfigured = false;
        poseWaitGeneration++;

        controlCharacteristic = null;
        batteryCharacteristic = null;
        voltageCharacteristic = null;
    }

    @Override
    public void recenter() {
        if (!ready || gatt == null || controlCharacteristic == null) {
            return;
        }

        queueWrite(gatt, controlCharacteristic, new byte[] {0x00});
    }

    @Override
    public boolean isReady() {
        return ready;
    }

    private void connect(BluetoothDevice device) {
        if (!hasConnectPermission()) {
            handleBluetoothPermissionLoss();
            return;
        }

        closeGatt();
        try {
            gatt = device.connectGatt(
                context,
                false,
                gattCallback,
                BluetoothDevice.TRANSPORT_LE
            );
            if (gatt == null) {
                emitConnection(
                    ConnectionState.ERROR,
                    "Android did not create a controller GATT connection."
                );
            }
        } catch (SecurityException exception) {
            handleBluetoothPermissionLoss();
        }
    }

    private boolean isDaydreamCandidate(ScanResult result) {
        if (result == null) {
            return false;
        }

        if (result.getScanRecord() != null) {
            List<ParcelUuid> advertisedServices = result.getScanRecord().getServiceUuids();
            if (advertisedServices != null
                && advertisedServices.contains(new ParcelUuid(DAYDREAM_SERVICE))) {
                return true;
            }

            String advertisedName = result.getScanRecord().getDeviceName();
            if (advertisedName != null
                && advertisedName.toLowerCase().contains("daydream controller")) {
                return true;
            }
        }

        if (hasConnectPermission()) {
            try {
                String name = result.getDevice().getName();
                return name != null
                    && name.toLowerCase().contains("daydream controller");
            } catch (SecurityException ignored) {
                return false;
            }
        }

        return false;
    }

    private void stopScan() {
        scanGeneration++;
        if (scanner != null && hasScanPermission()) {
            try {
                scanner.stopScan(scanCallback);
            } catch (SecurityException ignored) {
                // Runtime permission changed while the scan was active.
            }
        }
        scanner = null;
    }

    private boolean hasScanPermission() {
        return Build.VERSION.SDK_INT < Build.VERSION_CODES.S
            || context.checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN)
                == PackageManager.PERMISSION_GRANTED;
    }

    private boolean hasConnectPermission() {
        return Build.VERSION.SDK_INT < Build.VERSION_CODES.S
            || context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                == PackageManager.PERMISSION_GRANTED;
    }

    private boolean queueEnableNotifications(
        BluetoothGatt callbackGatt,
        BluetoothGattCharacteristic characteristic,
        boolean requiredForReady
    ) {
        try {
            if (!callbackGatt.setCharacteristicNotification(characteristic, true)) {
                if (requiredForReady) {
                    failSetup("Android rejected controller pose notification setup.");
                }
                return false;
            }
        } catch (SecurityException exception) {
            handleBluetoothPermissionLoss();
            return false;
        }

        BluetoothGattDescriptor descriptor =
            characteristic.getDescriptor(CLIENT_CONFIG_DESCRIPTOR);
        if (descriptor == null) {
            if (requiredForReady) {
                failSetup("Controller pose notification descriptor is missing.");
            }
            return false;
        }

        queueOperation(() -> {
            try {
                boolean started = writeDescriptor(
                    callbackGatt,
                    descriptor,
                    BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                );
                if (!started && requiredForReady) {
                    failSetup(
                        "Android could not start controller pose notification setup."
                    );
                }
                return started;
            } catch (SecurityException exception) {
                handleBluetoothPermissionLoss();
                return false;
            }
        });
        return true;
    }

    private void queueRead(
        BluetoothGatt callbackGatt,
        BluetoothGattCharacteristic characteristic
    ) {
        queueOperation(() -> {
            try {
                return callbackGatt.readCharacteristic(characteristic);
            } catch (SecurityException exception) {
                handleBluetoothPermissionLoss();
                return false;
            }
        });
    }

    private void queueWrite(
        BluetoothGatt callbackGatt,
        BluetoothGattCharacteristic characteristic,
        byte[] value
    ) {
        queueOperation(() -> {
            try {
                return writeCharacteristic(callbackGatt, characteristic, value);
            } catch (SecurityException exception) {
                handleBluetoothPermissionLoss();
                return false;
            }
        });
    }

    private boolean writeDescriptor(
        BluetoothGatt callbackGatt,
        BluetoothGattDescriptor descriptor,
        byte[] value
    ) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            return callbackGatt.writeDescriptor(descriptor, value)
                == BluetoothStatusCodes.SUCCESS;
        }

        descriptor.setValue(value);
        return callbackGatt.writeDescriptor(descriptor);
    }

    private boolean writeCharacteristic(
        BluetoothGatt callbackGatt,
        BluetoothGattCharacteristic characteristic,
        byte[] value
    ) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            return callbackGatt.writeCharacteristic(
                characteristic,
                value,
                BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            ) == BluetoothStatusCodes.SUCCESS;
        }

        characteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
        characteristic.setValue(value);
        return callbackGatt.writeCharacteristic(characteristic);
    }

    private synchronized void queueOperation(GattOperation operation) {
        operationQueue.add(operation);
        startNextGattOperationLocked();
    }

    private synchronized void completeGattOperation() {
        operationInFlight = false;
        startNextGattOperationLocked();
    }

    private synchronized void clearGattQueue() {
        operationQueue.clear();
        operationInFlight = false;
    }

    private void startNextGattOperationLocked() {
        if (operationInFlight) {
            return;
        }

        while (!operationQueue.isEmpty()) {
            GattOperation operation = operationQueue.remove();
            boolean started;
            try {
                started = operation.start();
            } catch (RuntimeException exception) {
                started = false;
            }

            if (started) {
                operationInFlight = true;
                return;
            }
        }
    }

    private void handleCharacteristicValue(
        BluetoothGattCharacteristic characteristic,
        byte[] value,
        int status
    ) {
        if (status != BluetoothGatt.GATT_SUCCESS
            || characteristic == null
            || value == null) {
            return;
        }

        UUID uuid = characteristic.getUuid();
        if (BATTERY_LEVEL_CHARACTERISTIC.equals(uuid) && value.length >= 1) {
            batteryPercentage = Math.max(0, Math.min(100, value[0] & 0xff));
            emitBattery();
        } else if (VOLTAGE_CHARACTERISTIC.equals(uuid) && value.length >= 2) {
            batteryMillivolts = (value[0] & 0xff) | ((value[1] & 0xff) << 8);
            emitBattery();
        }
    }

    private void handleNotification(
        BluetoothGattCharacteristic characteristic,
        byte[] value
    ) {
        if (characteristic == null || value == null) {
            return;
        }

        UUID uuid = characteristic.getUuid();
        if (POSE_CHARACTERISTIC.equals(uuid)) {
            if (value.length < DaydreamControllerPacket.PACKET_BYTES) {
                return;
            }

            try {
                ControllerSnapshot snapshot = DaydreamControllerPacket.parse(value);

                if (!ready && poseNotificationsConfigured && !setupFailed) {
                    ready = true;
                    poseWaitGeneration++;
                    emitConnection(
                        ConnectionState.READY,
                        "Daydream controller connected and streaming."
                    );
                    emitBattery();
                }

                Listener target = listener;
                if (target != null) {
                    target.onControllerStateChanged(snapshot);
                }
            } catch (IllegalArgumentException ignored) {
                // Ignore a malformed notification rather than dropping a healthy link.
            }
        } else if (BATTERY_LEVEL_CHARACTERISTIC.equals(uuid) && value.length >= 1) {
            batteryPercentage = Math.max(0, Math.min(100, value[0] & 0xff));
            emitBattery();
        }
    }

    private void schedulePoseStreamTimeout() {
        final int generation = ++poseWaitGeneration;
        mainHandler.postDelayed(
            () -> {
                if (!ready
                    && poseNotificationsConfigured
                    && !setupFailed
                    && generation == poseWaitGeneration) {
                    failSetup(
                        "Controller pose channel was enabled, but no pose packets arrived."
                    );
                }
            },
            POSE_STREAM_TIMEOUT_MILLIS
        );
    }

    private void failSetup(String message) {
        ready = false;
        setupFailed = true;
        poseNotificationsConfigured = false;
        poseWaitGeneration++;
        clearGattQueue();
        emitConnection(ConnectionState.ERROR, message);
    }

    private void handleBluetoothPermissionLoss() {
        ready = false;
        setupFailed = true;
        poseNotificationsConfigured = false;
        poseWaitGeneration++;
        clearGattQueue();
        emitConnection(
            ConnectionState.PERMISSION_REQUIRED,
            "Bluetooth permission was removed while the controller was active."
        );
        closeGatt();
    }

    private void emitBattery() {
        Listener target = listener;
        if (target != null) {
            target.onBatteryChanged(batteryPercentage, batteryMillivolts);
        }
    }

    private void emitConnection(ConnectionState state, String message) {
        Listener target = listener;
        if (target != null) {
            target.onConnectionStateChanged(state, message);
        }
    }

    private void closeGatt() {
        BluetoothGatt activeGatt = gatt;
        gatt = null;
        if (activeGatt != null) {
            try {
                activeGatt.close();
            } catch (RuntimeException ignored) {
                // Bluetooth stack is already tearing down.
            }
        }
    }

    @Override
    public void close() {
        disconnect();
        try {
            context.unregisterReceiver(bondReceiver);
        } catch (IllegalArgumentException ignored) {
            // Receiver was already removed during teardown.
        }
    }

    private interface GattOperation {
        boolean start();
    }
}
