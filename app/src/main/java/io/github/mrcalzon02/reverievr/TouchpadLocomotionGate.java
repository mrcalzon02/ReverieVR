package io.github.mrcalzon02.reverievr;

/**
 * Prevents an already-held touchpad or an interrupted gesture from
 * unexpectedly moving the camera on scene entry, menu close or BLE recovery.
 * A full finger release is required to arm a fresh movement gesture.
 */
final class TouchpadLocomotionGate {
    private boolean armed;

    void reset() { armed = false; }

    boolean allows(boolean touching, boolean clicked, boolean blocked) {
        if (blocked || clicked) {
            armed = false;
            return false;
        }
        if (!touching) {
            armed = true;
            return false;
        }
        return armed;
    }
}
