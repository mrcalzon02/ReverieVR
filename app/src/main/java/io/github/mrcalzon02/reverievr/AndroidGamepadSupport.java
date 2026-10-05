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
