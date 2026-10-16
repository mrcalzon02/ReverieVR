package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

/** DOS guest control edges do not require physical touchpad click. */
public final class VrInputRouterDosNoClickTest {
    private static ControllerSnapshot touch(boolean touching, boolean click,
                                            int x, int y, long time) {
        return new ControllerSnapshot(
            time, 0, 0,
            0f, 0f, 0f, 1f,
            0f, 0f, 0f,
            0f, 0f, 0f,
            touching, x, y,
            false, false, false, false, click, 0);
    }

    @Test public void holdTouchAloneMovesDosArrowsWhileMouseRemainsUnpressed() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_HEAD_MOUSE));
        VrInputRouter.Listener listener = new VrInputRouter.Listener() {
            @Override public void onInputAction(VrInputAction action, String source) {}
            @Override public boolean usesDaydreamDosGameControls() { return true; }
        };
        VrInputRouter router = new VrInputRouter(listener);
        router.setBindingListener(new VrInputRouter.BindingListener() {
            @Override public void onBindingDigital(
                    BindingInput input, boolean down, String source) {
                engine.submitDigital(input, down);
            }
            @Override public void onBindingAxis(
                    BindingInput input, float value, String source) {
                engine.submitAxis(input, value);
            }
            @Override public void onBindingRelative(
                    BindingInput input, float delta, String source) {
                engine.submitRelative(input, delta);
            }
        });

        // Finger presses the TOP zone but never clicks the mouse button.
        router.onControllerSnapshot(touch(true, false, 128, 15, 1000000000L),
            "Daydream");
        assertTrue(bus.isKeyDown(VirtualKey.UP));
        assertFalse(bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));

        // The held key survives subsequent pose packets while not clicking.
        router.onControllerSnapshot(touch(true, false, 128, 15, 1050000000L),
            "Daydream");
        assertTrue(bus.isKeyDown(VirtualKey.UP));
        assertFalse(bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));

        // Move the finger to the BOTTOM zone; the old direction releases.
        router.onControllerSnapshot(touch(true, false, 128, 245, 1100000000L),
            "Daydream");
        assertFalse(bus.isKeyDown(VirtualKey.UP));
        assertTrue(bus.isKeyDown(VirtualKey.DOWN));

        router.onControllerSnapshot(touch(false, false, 0, 0, 1150000000L),
            "Daydream");
        assertFalse(bus.isKeyDown(VirtualKey.DOWN));

        // Head mouse remains independent of both pad click and held keys.
        engine.submitRelative(BindingInput.HEAD_YAW_DELTA, 0.02f);
        engine.submitRelative(BindingInput.HEAD_PITCH_DELTA, -0.01f);
        assertTrue(Math.abs(bus.consumeMouseRelativeX()) > 0f);
        assertTrue(Math.abs(bus.consumeMouseRelativeY()) > 0f);
        assertFalse(bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
    }
}
