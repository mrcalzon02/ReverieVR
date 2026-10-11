package io.github.mrcalzon02.reverievr;

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
        return null;
    }

    static boolean isOriginalSteamController(InputDevice device) {
        if (device == null) {
            return false;
        }
        String name = device.getName();
        if (name == null || !name.toLowerCase(java.util.Locale.ROOT)
                .contains("steam controller")) {
            return false;
        }
        int sources = device.getSources();
        boolean usableHid =
            (sources & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE
            || (sources & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD
            || isGamepadDevice(device);
        // Valve's vendor ID helps classify named devices, but different BLE
        // firmware/Android paths may not expose USB VID/PID reliably.
        return usableHid;
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
