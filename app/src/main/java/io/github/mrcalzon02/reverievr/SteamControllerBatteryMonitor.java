package io.github.mrcalzon02.reverievr;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.view.InputDevice;

import java.util.Set;
import java.util.UUID;

/**
 * Read-only, scan-free Battery Service probe for an already bonded 2015 Steam
 * Controller selected as Android HID. Does not switch Valve mode or take HID.
 * Unsupported firmware exposes unknown, never an estimated battery percentage.
 */
final class SteamControllerBatteryMonitor implements AutoCloseable {
    interface Listener { void onChanged(); }
    private static final UUID SERVICE = UUID.fromString(
        "0000180f-0000-1000-8000-00805f9b34fb");
    private static final UUID LEVEL = UUID.fromString(
        "00002a19-0000-1000-8000-00805f9b34fb");
    private final Context context;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Listener listener;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic characteristic;
    private int inputDeviceId = -1;
    private int percentage = -1;

    private final Runnable timeout = () -> {
        if (gatt != null && characteristic == null) {
            ReverieLog.incident("STEAM_BATTERY", "Bonded GATT service query timed out.");
            disconnect();
        }
    };
    private final Runnable poll = new Runnable() {
        @Override public void run() {
            if (gatt == null || characteristic == null) return;
            try {
                if (!gatt.readCharacteristic(characteristic)) {
                    ReverieLog.incident("STEAM_BATTERY", "GATT battery read refused.");
                    disconnect();
                }
            } catch (SecurityException e) {
                ReverieLog.error("STEAM_BATTERY", "GATT battery read rejected.", e);
                disconnect();
            }
        }
    };

    private final BluetoothGattCallback callback = new BluetoothGattCallback() {
        @Override public void onConnectionStateChange(BluetoothGatt source,
            int status, int state) {
            handler.post(() -> {
                if (source != gatt) return;
                if (status != BluetoothGatt.GATT_SUCCESS
                    || state == BluetoothProfile.STATE_DISCONNECTED) {
                    disconnect();
                } else if (state == BluetoothProfile.STATE_CONNECTED) {
                    try {
                        if (!source.discoverServices()) disconnect();
                    } catch (SecurityException e) {
                        ReverieLog.error("STEAM_BATTERY", "GATT discovery rejected.", e);
                        disconnect();
                    }
                }
            });
        }

        @Override public void onServicesDiscovered(BluetoothGatt source, int status) {
            handler.post(() -> {
                if (source != gatt) return;
                if (status != BluetoothGatt.GATT_SUCCESS) {
                    disconnect();
                    return;
                }
                BluetoothGattService service = source.getService(SERVICE);
                BluetoothGattCharacteristic target =
                    service == null ? null : service.getCharacteristic(LEVEL);
                if (target == null || (target.getProperties()
                    & BluetoothGattCharacteristic.PROPERTY_READ) == 0) {
                    ReverieLog.dev("STEAM_BATTERY",
                        "Controller does not expose readable standard 0x2A19 battery.");
                    disconnect();
                    return;
                }
                characteristic = target;
                handler.removeCallbacks(timeout);
                handler.post(poll);
            });
        }

        @Override public void onCharacteristicRead(BluetoothGatt source,
            BluetoothGattCharacteristic target, int status) {
            acceptRead(source, target, target.getValue(), status);
        }

        @Override public void onCharacteristicRead(BluetoothGatt source,
            BluetoothGattCharacteristic target, byte[] value, int status) {
            acceptRead(source, target, value, status);
        }
    };

    SteamControllerBatteryMonitor(Context context, Listener listener) {
        this.context = context.getApplicationContext();
        this.listener = listener;
    }

    int getPercentage() { return percentage; }

    /** Invoked on the foreground Android UI thread after selection/hotplug. */
    void refresh(boolean selected) {
        InputDevice input = selected
            ? AndroidGamepadSupport.firstConnectedSteamController() : null;
        int id = input == null ? -1 : input.getId();
        if (id == inputDeviceId) return;
        disconnect();
        inputDeviceId = id;
        if (id < 0) return;
        try {
            BluetoothManager manager = (BluetoothManager)
                context.getSystemService(Context.BLUETOOTH_SERVICE);
            BluetoothAdapter adapter = manager == null ? null : manager.getAdapter();
            if (adapter == null || !adapter.isEnabled()) return;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
                && context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) return;
            Set<BluetoothDevice> bonded = adapter.getBondedDevices();
            for (BluetoothDevice device : bonded) {
                if ((device.getType() == BluetoothDevice.DEVICE_TYPE_LE
                    || device.getType() == BluetoothDevice.DEVICE_TYPE_DUAL)
                    && SteamControllerIdentity.matchesAndroidHid(0, device.getName())) {
                    gatt = device.connectGatt(context, false, callback,
                        BluetoothDevice.TRANSPORT_LE);
                    if (gatt != null) handler.postDelayed(timeout, 12000L);
                    return;
                }
            }
            ReverieLog.dev("STEAM_BATTERY", "No bonded original Steam BLE target.");
        } catch (SecurityException | IllegalArgumentException e) {
            ReverieLog.error("STEAM_BATTERY", "Bonded BLE battery query failed.", e);
            disconnect();
        }
    }

    private void acceptRead(BluetoothGatt source, BluetoothGattCharacteristic target,
        byte[] value, int status) {
        final byte[] copy = value == null ? null : value.clone();
        handler.post(() -> {
            if (source != gatt || target != characteristic) return;
            int next = status == BluetoothGatt.GATT_SUCCESS
                ? SteamControllerBatteryLevel.decode(copy) : -1;
            if (next < 0) {
                ReverieLog.incident("STEAM_BATTERY", "Battery response invalid/unavailable.");
                disconnect();
                return;
            }
            if (percentage != next) {
                percentage = next;
                listener.onChanged();
            }
            handler.removeCallbacks(poll);
            handler.postDelayed(poll, 120000L);
        });
    }

    private void disconnect() {
        handler.removeCallbacks(timeout);
        handler.removeCallbacks(poll);
        BluetoothGatt previous = gatt;
        gatt = null;
        characteristic = null;
        if (previous != null) {
            try { previous.disconnect(); }
            catch (SecurityException e) {
                ReverieLog.error("STEAM_BATTERY", "GATT disconnect rejected.", e);
            } finally { previous.close(); }
        }
        if (percentage != -1) {
            percentage = -1;
            listener.onChanged();
        }
    }

    @Override public void close() {
        inputDeviceId = -1;
        disconnect();
    }
}
