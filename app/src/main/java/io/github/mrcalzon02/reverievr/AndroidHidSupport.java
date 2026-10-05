package io.github.mrcalzon02.reverievr;

import android.view.InputDevice;

final class AndroidHidSupport {
    private AndroidHidSupport() {
    }

    static String firstConnectedKeyboardName() {
        return firstConnectedDeviceName(
            InputDevice.SOURCE_KEYBOARD
        );
    }

    static String firstConnectedMouseName() {
        String relative =
            firstConnectedDeviceName(
                InputDevice.SOURCE_MOUSE_RELATIVE
            );
        if (relative != null) {
            return relative;
        }
        return firstConnectedDeviceName(
            InputDevice.SOURCE_MOUSE
        );
    }

    private static String firstConnectedDeviceName(
        int requiredSource
    ) {
        int[] ids = InputDevice.getDeviceIds();
        for (int id : ids) {
            InputDevice device = InputDevice.getDevice(id);
            if (device == null || device.isVirtual()) {
                continue;
            }

            int sources = device.getSources();
            if ((sources & requiredSource) != requiredSource) {
                continue;
            }

            String name = device.getName();
            return name == null || name.trim().isEmpty()
                ? "Android HID device"
                : name;
        }
        return null;
    }
}
