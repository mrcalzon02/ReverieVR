package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class KensLabyrinthBindingProfileTest {
    @Test
    public void freewareGameUsesActualPhysicalDaydreamControls() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_KENS_LABYRINTH));
        assertTrue(BuiltInBindingProfiles.isDosProfileId(
            BuiltInBindingProfiles.ID_DOS_KENS_LABYRINTH));
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0.8f);
        engine.submitAxis(BindingInput.TOUCHPAD_Y, -0.8f);
        assertTrue(bus.isKeyDown(VirtualKey.RIGHT));
        assertTrue(bus.isKeyDown(VirtualKey.UP));
        engine.submitDigital(BindingInput.SELECT, true);
        engine.submitDigital(BindingInput.DAYDREAM_APP_BUTTON, true);
        assertTrue(bus.isKeyDown(VirtualKey.CTRL));
        assertTrue(bus.isKeyDown(VirtualKey.SPACE));
        engine.submitDigital(BindingInput.VOLUME_UP, true);
        engine.submitDigital(BindingInput.VOLUME_DOWN, true);
        assertTrue(bus.isKeyDown(VirtualKey.ENTER));
        assertTrue(bus.isKeyDown(VirtualKey.ESCAPE));
        assertFalse(bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
        engine.submitDigital(BindingInput.SELECT, false);
        engine.submitDigital(BindingInput.DAYDREAM_APP_BUTTON, false);
        engine.submitDigital(BindingInput.VOLUME_UP, false);
        engine.submitDigital(BindingInput.VOLUME_DOWN, false);
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0f);
        engine.submitAxis(BindingInput.TOUCHPAD_Y, 0f);
        assertFalse(bus.isKeyDown(VirtualKey.CTRL));
        assertFalse(bus.isKeyDown(VirtualKey.SPACE));
        assertFalse(bus.isKeyDown(VirtualKey.RIGHT));
        assertFalse(bus.isKeyDown(VirtualKey.UP));
    }
}
