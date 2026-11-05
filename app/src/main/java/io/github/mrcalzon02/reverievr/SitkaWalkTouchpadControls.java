package io.github.mrcalzon02.reverievr;

/** Sitka-only safe touchpad-to-native-walk adapter.
 *
 * The native Sitka WalkSession owns collision and grade; the shell must NOT
 * apply its own room movement to this gesture. Click remains selection.
 * This pure helper never grants any walkable geography.
 */
final class SitkaWalkTouchpadControls {
    private SitkaWalkTouchpadControls() {}

    static float axis(float raw) {
        if (!Float.isFinite(raw)) return 0.0f;
        return Math.max(-1.0f, Math.min(1.0f, raw/127.5f - 1.0f));
    }

    static boolean useTouchpad(float joystickX,float joystickY,
                                boolean touchActive) {
        return touchActive
            && Float.isFinite(joystickX)
            && Float.isFinite(joystickY)
            && Math.hypot(joystickX,joystickY)<=0.2;
    }
}
