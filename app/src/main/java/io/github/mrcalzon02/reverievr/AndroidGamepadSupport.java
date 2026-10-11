package io.github.mrcalzon02.reverievr;

import android.os.Build;
import android.view.InputDevice;
import android.view.InputEvent;

final class AndroidGamepadSupport {
    private AndroidGamepadSupport() {
    }

    static boolean hasConnectedGamepad() {
        return firstConnectedGamepadName() != null;
    }

    static String firstConnectedGamepadName() {
        int[] ids = InputDevice.getDeviceIds();
        for (int id : ids) {
            InputDevice device = InputDevice.getDevice(id);
            if (isGamepadDevice(device)) {
                String name = device.getName();
                return name == null || name.trim().isEmpty()
                    ? "Android gamepad"
                    : name;
            }
        }
        return null;
    }

    /**
     * Stage A accepts the original Steam Controller in its BLE keyboard/mouse
     * compatibility mode. Generic mice/keyboards must never unlock VR entry.
     */
    static String firstConnectedVrControllerName() {
        String gamepad = firstConnectedGamepadName();
        if (gamepad != null) {
            return gamepad;
        }
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(id);
            if (isOriginalSteamController(device)) {
                return device.getName();
            }
        }
        InputDevice scrcpy = firstConnectedScrcpyHid();
        return scrcpy == null ? null : scrcpy.getName();
    }

    static InputDevice firstConnectedScrcpyHid() {
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(id);
            if (isScrcpyHid(device)) return device;
        }
        return null;
    }

    /** Explicit scrcpy mode may select its Xbox-360-compatible UHID gamepad. */
    static InputDevice firstConnectedScrcpySelectableInput() {
        InputDevice labelled = firstConnectedScrcpyHid();
        if (labelled != null) return labelled;
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(id);
            if (isScrcpyCompatibleGamepad(device)) return device;
        }
        return null;
    }

    static boolean isScrcpyCompatibleGamepad(InputDevice device) {
        return device != null && !device.isVirtual()
            && isGamepadDevice(device)
            && ScrcpyHidIdentity.matchesCompatibleGamepad(
                device.getVendorId(), device.getProductId(), device.getName());
    }

    static boolean isScrcpyHid(InputDevice device) {
        if (device == null || device.isVirtual()
            || !ScrcpyHidIdentity.matchesName(device.getName())) return false;
        int sources = device.getSources();
        return (sources & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD
            || (sources & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE
            || (sources & InputDevice.SOURCE_MOUSE_RELATIVE)
                == InputDevice.SOURCE_MOUSE_RELATIVE
            || isGamepadDevice(device);
    }

    static InputDevice firstConnectedSteamController() {
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice d = InputDevice.getDevice(id);
            if (isOriginalSteamController(d)) return d;
        }
        return null;
    }

    static boolean isOriginalSteamController(InputDevice d) {
        if (d == null || d.isVirtual() || !SteamControllerIdentity.matchesAndroidHid(
                d.getVendorId(), d.getName())) return false;
        int source = d.getSources();
        return (source & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE
            || (source & InputDevice.SOURCE_MOUSE_RELATIVE) == InputDevice.SOURCE_MOUSE_RELATIVE
            || (source & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD
            || isGamepadDevice(d);
    }

    static int steamBatteryPercentage() {
        InputDevice d = firstConnectedSteamController();
        // Reference Galaxy S9 runs Android 10 (API 29): no public InputDevice battery API.
        if (d == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.S) return -1;
        android.hardware.BatteryState state = d.getBatteryState();
        if (!state.isPresent()) return -1;
        float capacity = state.getCapacity();
        return Float.isFinite(capacity) && capacity >= 0f && capacity <= 1f
            ? Math.round(capacity * 100f) : -1;
    }

    static boolean isGamepadEvent(InputEvent event) {
        return event != null && isGamepadDevice(event.getDevice());
    }

    static boolean isGamepadDevice(InputDevice device) {
        if (device == null) {
            return false;
        }

        int sources = device.getSources();
        return (sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
            || (sources & InputDevice.SOURCE_JOYSTICK)
                == InputDevice.SOURCE_JOYSTICK;
    }
}
