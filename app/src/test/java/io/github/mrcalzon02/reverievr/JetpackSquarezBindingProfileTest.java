package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class JetpackSquarezBindingProfileTest {
    private static void assertProfile(String id, VirtualKey click,
                                      VirtualKey app) {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(id));
        assertTrue(BuiltInBindingProfiles.isDosProfileId(id));
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0.9f);
        engine.submitAxis(BindingInput.TOUCHPAD_Y, -0.9f);
        assertTrue(bus.isKeyDown(VirtualKey.RIGHT));
        assertTrue(bus.isKeyDown(VirtualKey.UP));
        engine.submitDigital(BindingInput.SELECT, true);
        engine.submitDigital(BindingInput.DAYDREAM_APP_BUTTON, true);
        assertTrue(bus.isKeyDown(click));
        assertTrue(bus.isKeyDown(app));
        engine.submitDigital(BindingInput.SELECT, false);
        engine.submitDigital(BindingInput.DAYDREAM_APP_BUTTON, false);
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0f);
        engine.submitAxis(BindingInput.TOUCHPAD_Y, 0f);
        assertFalse(bus.isKeyDown(click));
        assertFalse(bus.isKeyDown(app));
        assertFalse(bus.isKeyDown(VirtualKey.RIGHT));
        assertFalse(bus.isKeyDown(VirtualKey.UP));
    }

    @Test public void jetpackHasHeldThrustAndPhase() {
        assertProfile(BuiltInBindingProfiles.ID_DOS_JETPACK,
            VirtualKey.ALT, VirtualKey.CTRL);
    }

    @Test public void squarezHasSeparateRotateAndPlace() {
        assertProfile(BuiltInBindingProfiles.ID_DOS_SQUAREZ,
            VirtualKey.CTRL, VirtualKey.ALT);
    }
}
