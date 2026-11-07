package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;
import java.util.List;
import org.junit.Test;

/**
 * A reproducible Daydream packet replay, not a mere profile-string check:
 * physical-button and touch packets -> actual VrInputRouter callbacks ->
 * InputBindingEngine -> VirtualInputBus -> a DOS frame input snapshot.
 *
 * This is not a claim that a physical BLE service or Doom.EXE ran on CI.
 */
public final class DoomDaydreamControllerSimulationTest {
    private static final class Harness {
        final VirtualInputBus bus = new VirtualInputBus();
        final InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE));
        final List<VrInputAction> shellActions = new ArrayList<>();
        final VrInputRouter router;

        Harness() {
            router = new VrInputRouter(new VrInputRouter.Listener() {
                @Override public void onInputAction(
                    VrInputAction action, String source) {
                    shellActions.add(action);
                }
                @Override public boolean usesDaydreamDosGameControls() {
                    return true;
                }
            });
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
        }

        void send(long millis, boolean touching, int x, int y,
                  boolean click, boolean app, boolean home,
                  boolean volUp, boolean volDown) {
            ControllerSnapshot packet = new ControllerSnapshot(
                millis * 1000000L, 0, (int)(millis % 512),
                0f, 0f, 0f, 1f, 0f, 0f, 0f,
                0f, 0f, 0f, touching, x, y,
                volUp, volDown, app, home, click, 0);
            router.onControllerSnapshot(packet, "Daydream BLE simulated");
        }
        VirtualInputSnapshot frame() {
            VirtualInputSnapshot snapshot = new VirtualInputSnapshot();
            bus.capture(snapshot);
            return snapshot;
        }
    }

    @Test public void noClickHeadMovementAndPadWalkingReachDosFrameTogether() {
        Harness h = new Harness();
        h.send(1000, true, 128, 15, false, false, false, false, false);
        h.engine.submitRelative(BindingInput.HEAD_YAW_DELTA, .021f);
        VirtualInputSnapshot frame = h.frame();
        assertEquals(13.65f, frame.mouseRelativeX, .002f);
        assertEquals(0f, frame.mouseRelativeY, .001f);
        assertTrue(frame.keys[VirtualKey.UP.ordinal()]);
        assertFalse(frame.mouseButtons[VirtualOutput.MOUSE_LEFT]);
        assertFalse(frame.keys[VirtualKey.CTRL.ordinal()]);
        assertFalse(frame.keys[VirtualKey.SPACE.ordinal()]);

        // Repeated idle pose packets never require a click to preserve walk.
        h.send(1033, true, 128, 15, false, false, false, false, false);
        assertTrue(h.bus.isKeyDown(VirtualKey.UP));
        h.engine.submitRelative(BindingInput.HEAD_YAW_DELTA, -.010f);
        assertEquals(-6.5f, h.frame().mouseRelativeX, .002f);

        // Looking sharply up/down has no walking side-effect in this profile.
        h.send(1066, false, 0, 0, false, false, false, false, false);
        h.engine.submitAxis(BindingInput.HEAD_PITCH_UP_AXIS, .90f);
        assertFalse(h.bus.isKeyDown(VirtualKey.UP));
        assertFalse(h.bus.isKeyDown(VirtualKey.DOWN));
        h.engine.submitAxis(BindingInput.HEAD_PITCH_UP_AXIS, -.90f);
        assertFalse(h.bus.isKeyDown(VirtualKey.DOWN));
    }

    @Test public void padReverseAndStrafeHaveIndependentHoldsAndReleases() {
        Harness h = new Harness();
        h.send(1000, true, 128, 245, false, false, false, false, false);
        assertTrue(h.bus.isKeyDown(VirtualKey.DOWN));
        assertFalse(h.bus.isKeyDown(VirtualKey.UP));
        h.send(1033, true, 10, 128, false, false, false, false, false);
        assertFalse(h.bus.isKeyDown(VirtualKey.DOWN));
        assertTrue(h.bus.isKeyDown(VirtualKey.COMMA));
        assertFalse(h.bus.isKeyDown(VirtualKey.LEFT));
        h.send(1066, true, 245, 128, false, false, false, false, false);
        assertFalse(h.bus.isKeyDown(VirtualKey.COMMA));
        assertTrue(h.bus.isKeyDown(VirtualKey.PERIOD));
        h.send(1100, false, 0, 0, false, false, false, false, false);
        assertFalse(h.bus.isKeyDown(VirtualKey.PERIOD));
        assertFalse(h.bus.isKeyDown(VirtualKey.UP));
        assertFalse(h.bus.isKeyDown(VirtualKey.DOWN));
    }

    @Test public void clickFireAndAppUseAreIndependentAndHomeIsShellOnly() {
        Harness h = new Harness();
        h.send(1000, false, 0, 0, true, true, false, false, false);
        assertTrue(h.bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
        assertTrue(h.bus.isKeyDown(VirtualKey.CTRL));
        assertTrue(h.bus.isKeyDown(VirtualKey.SPACE));
        assertFalse(h.bus.isMouseButtonDown(VirtualOutput.MOUSE_RIGHT));
        assertFalse(h.shellActions.contains(VrInputAction.MENU));
        h.send(1033, false, 0, 0, false, true, false, false, false);
        assertFalse(h.bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
        assertTrue(h.bus.isKeyDown(VirtualKey.SPACE));
        h.send(1066, false, 0, 0, false, false, true, false, false);
        assertFalse(h.bus.isKeyDown(VirtualKey.SPACE));
        assertTrue(h.shellActions.contains(VrInputAction.MENU));
        h.engine.releaseAll(); // The real Activity does this on shell focus.
        assertFalse(h.bus.isKeyDown(VirtualKey.CTRL));
        assertFalse(h.bus.isKeyDown(VirtualKey.SPACE));
        assertFalse(h.bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
    }

    @Test public void rockerCyclesWeaponSlotsOncePerPressAndNeverVolume() {
        Harness h = new Harness();
        h.send(1000, false, 0, 0, false, false, false, true, false);
        VirtualInputSnapshot frame = h.frame();
        assertTrue(frame.keys[VirtualKey.DIGIT_3.ordinal()]);
        assertFalse(frame.keys[VirtualKey.ENTER.ordinal()]);
        assertFalse(frame.keys[VirtualKey.ESCAPE.ordinal()]);
        h.send(1033, false, 0, 0, false, false, false, true, false);
        assertFalse(h.frame().keys[VirtualKey.DIGIT_4.ordinal()]);
        h.send(1066, false, 0, 0, false, false, false, false, false);
        h.send(1100, false, 0, 0, false, false, false, true, false);
        assertTrue(h.frame().keys[VirtualKey.DIGIT_4.ordinal()]);
        h.send(1133, false, 0, 0, false, false, false, false, false);
        h.send(1166, false, 0, 0, false, false, false, false, true);
        assertTrue(h.frame().keys[VirtualKey.DIGIT_3.ordinal()]);
    }
}
