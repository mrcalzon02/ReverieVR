package io.github.mrcalzon02.reverievr.input;

import android.os.Build;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import java.util.ArrayList;
import java.util.List;

/**
 * Read-only Android input report inspector for physical and virtual controllers.
 * Does not request USB/BLE raw-device ownership or interfere with the system mouse.
 * Never logs typed characters or unhandled keyboard key events by default.
 */
public final class HidInputInspector {
    private HidInputInspector() {}

    public static List<String> enumerate() {
        List<String> result = new ArrayList<>();
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(id);
            if (device == null) continue;
            int sources = device.getSources();
            if ((sources & (InputDevice.SOURCE_GAMEPAD | InputDevice.SOURCE_JOYSTICK
                    | InputDevice.SOURCE_MOUSE | InputDevice.SOURCE_KEYBOARD
                    | InputDevice.SOURCE_TOUCHPAD)) == 0) continue;
            StringBuilder line = new StringBuilder();
            line.append("id=").append(id).append(" name=").append(device.getName());
            line.append(" vid=").append(device.getVendorId());
            line.append(" pid=").append(device.getProductId());
            line.append(" sources=0x").append(Integer.toHexString(sources));
            line.append(" keyboardType=").append(device.getKeyboardType());
            if (Build.VERSION.SDK_INT >= 29) {
                line.append(" external=").append(device.isExternal());
            }
            line.append(" axes=[");
            boolean first = true;
            for (InputDevice.MotionRange range : device.getMotionRanges()) {
                if (!first) line.append(", ");
                first = false;
                line.append(MotionEvent.axisToString(range.getAxis()))
                    .append(":").append(range.getMin()).append("..").append(range.getMax())
                    .append("@").append(range.getSource());
            }
            line.append("]");
            result.add(line.toString());
        }
        return result;
    }

    /** Button-only diagnostic: prevent recording passwords or other typed text. */
    public static String controllerKey(KeyEvent event) {
        if (event == null) return null;
        int source = event.getSource();
        if ((source & (InputDevice.SOURCE_GAMEPAD | InputDevice.SOURCE_JOYSTICK)) == 0
                && !KeyEvent.isGamepadButton(event.getKeyCode())) return null;
        return "device=" + event.getDeviceId()
                + " key=" + KeyEvent.keyCodeToString(event.getKeyCode())
                + " action=" + event.getAction()
                + " repeat=" + event.getRepeatCount();
    }

    /** Reports the axes actually provided by Android for this event's device. */
    public static String controllerMotion(MotionEvent event) {
        if (event == null) return null;
        int source = event.getSource();
        if ((source & (InputDevice.SOURCE_GAMEPAD | InputDevice.SOURCE_JOYSTICK
                | InputDevice.SOURCE_MOUSE | InputDevice.SOURCE_TOUCHPAD)) == 0) return null;
        InputDevice device = event.getDevice();
        StringBuilder out = new StringBuilder("device=" + event.getDeviceId()
                + " source=0x" + Integer.toHexString(source)
                + " action=" + event.getActionMasked()
                + " buttons=0x" + Integer.toHexString(event.getButtonState()));
        if (device != null) {
            for (InputDevice.MotionRange range : device.getMotionRanges()) {
                if ((range.getSource() & source) != source) continue;
                int axis = range.getAxis();
                out.append(" ").append(MotionEvent.axisToString(axis))
                    .append("=").append(event.getAxisValue(axis));
            }
        }
        return out.toString();
    }
}
