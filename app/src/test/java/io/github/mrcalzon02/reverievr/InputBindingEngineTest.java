package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import java.util.Arrays;

public final class InputBindingEngineTest {
    @Test
    public void digitalBindingKeepsKeyHeldUntilRelease() {
        VirtualInputBus bus = new VirtualInputBus();
        BindingProfile profile = new BindingProfile(
            "test",
            "Test",
            Arrays.asList(
                InputBinding.digital(
                    BindingInput.SELECT,
                    VirtualOutput.key(VirtualKey.CTRL)
                )
            )
        );
        InputBindingEngine engine =
            new InputBindingEngine(bus, profile);

        engine.submitDigital(BindingInput.SELECT, true);
        assertTrue(bus.isKeyDown(VirtualKey.CTRL));

        engine.submitDigital(BindingInput.SELECT, false);
        assertFalse(bus.isKeyDown(VirtualKey.CTRL));
    }

    @Test
    public void axisThresholdPressesAndReleases() {
        VirtualInputBus bus = new VirtualInputBus();
        BindingProfile profile = new BindingProfile(
            "test",
            "Test",
            Arrays.asList(
                InputBinding.negativeThreshold(
                    BindingInput.TOUCHPAD_Y,
                    VirtualOutput.key(VirtualKey.W),
                    0.35f
                )
            )
        );
        InputBindingEngine engine =
            new InputBindingEngine(bus, profile);

        engine.submitAxis(BindingInput.TOUCHPAD_Y, -0.9f);
        assertTrue(bus.isKeyDown(VirtualKey.W));

        engine.submitAxis(BindingInput.TOUCHPAD_Y, 0.0f);
        assertFalse(bus.isKeyDown(VirtualKey.W));
    }

    @Test
    public void relativeHeadMotionAccumulatesUntilConsumed() {
        VirtualInputBus bus = new VirtualInputBus();
        BindingProfile profile = new BindingProfile(
            "test",
            "Test",
            Arrays.asList(
                InputBinding.analog(
                    BindingInput.HEAD_YAW_DELTA,
                    VirtualOutput.mouseRelativeX(),
                    100.0f,
                    0.0f
                )
            )
        );
        InputBindingEngine engine =
            new InputBindingEngine(bus, profile);

        engine.submitRelative(
            BindingInput.HEAD_YAW_DELTA,
            0.02f
        );
        engine.submitRelative(
            BindingInput.HEAD_YAW_DELTA,
            0.03f
        );

        assertEquals(
            5.0f,
            bus.consumeMouseRelativeX(),
            0.0001f
        );
        assertEquals(
            0.0f,
            bus.consumeMouseRelativeX(),
            0.0001f
        );
    }

    @Test
    public void profileChangeReleasesOldOutputs() {
        VirtualInputBus bus = new VirtualInputBus();
        BindingProfile first = new BindingProfile(
            "first",
            "First",
            Arrays.asList(
                InputBinding.digital(
                    BindingInput.SELECT,
                    VirtualOutput.key(VirtualKey.CTRL)
                )
            )
        );
        BindingProfile second = new BindingProfile(
            "second",
            "Second",
            Arrays.asList()
        );

        InputBindingEngine engine =
            new InputBindingEngine(bus, first);
        engine.submitDigital(BindingInput.SELECT, true);
        assertTrue(bus.isKeyDown(VirtualKey.CTRL));

        engine.setProfile(second);
        assertFalse(bus.isKeyDown(VirtualKey.CTRL));
    }
    @Test
    public void dosPointerRelativeTouchAndHeadMotionReachRealMouse() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(BuiltInBindingProfiles.ID_DOS_CURSOR));
        engine.submitRelative(BindingInput.TOUCHPAD_REL_X, 10f);
        engine.submitRelative(BindingInput.TOUCHPAD_REL_Y, -5f);
        engine.submitRelative(BindingInput.HEAD_YAW_DELTA, 0.01f);
        // Corrected DOS headset yaw is negative: 10*.70 + .01*(-650) = .5.
        assertEquals(0.5f, bus.consumeMouseRelativeX(), 0.0001f);
        assertEquals(-3.5f, bus.consumeMouseRelativeY(), 0.0001f);
        assertEquals(0f, bus.consumeMouseRelativeX(), 0.0001f);
    }

}
