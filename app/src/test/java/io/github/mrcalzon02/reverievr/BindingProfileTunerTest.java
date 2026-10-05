package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;

import java.util.ArrayList;
import java.util.List;

import org.junit.Test;

public final class BindingProfileTunerTest {
    @Test
    public void scaleAdjustmentPreservesDirectionAndDigitalBindings() {
        List<InputBinding> bindings = new ArrayList<>();
        InputBinding digital = InputBinding.digital(
            BindingInput.SELECT,
            VirtualOutput.key(VirtualKey.CTRL)
        );
        bindings.add(digital);
        bindings.add(
            InputBinding.analog(
                BindingInput.HEAD_YAW_DELTA,
                VirtualOutput.mouseRelativeX(),
                650.0f,
                0.0f
            )
        );
        bindings.add(
            InputBinding.analog(
                BindingInput.HEAD_PITCH_DELTA,
                VirtualOutput.mouseRelativeY(),
                -650.0f,
                0.0f
            )
        );

        BindingProfile tuned =
            BindingProfileTuner.adjustAnalogScale(
                new BindingProfile(
                    "test",
                    "Test",
                    bindings
                ),
                1.1f
            );

        assertSame(digital, tuned.bindings.get(0));
        assertEquals(
            715.0f,
            tuned.bindings.get(1).scale,
            0.001f
        );
        assertEquals(
            -715.0f,
            tuned.bindings.get(2).scale,
            0.001f
        );
    }

    @Test
    public void deadzoneAdjustmentClampsAtValidBounds() {
        BindingProfile base =
            new BindingProfile(
                "test",
                "Test",
                java.util.Collections.singletonList(
                    InputBinding.analog(
                        BindingInput.TOUCHPAD_X,
                        VirtualOutput.mouseAbsoluteX(),
                        1.0f,
                        0.94f
                    )
                )
            );

        BindingProfile raised =
            BindingProfileTuner.adjustAnalogDeadzone(
                base,
                0.10f
            );
        assertEquals(
            0.95f,
            raised.bindings.get(0).deadzone,
            0.0001f
        );

        BindingProfile lowered =
            BindingProfileTuner.adjustAnalogDeadzone(
                raised,
                -2.0f
            );
        assertEquals(
            0.0f,
            lowered.bindings.get(0).deadzone,
            0.0001f
        );
    }

    @Test
    public void profileWithoutAnalogBindingsIsReturnedUnchanged() {
        BindingProfile profile =
            new BindingProfile(
                "digital",
                "Digital",
                java.util.Collections.singletonList(
                    InputBinding.digital(
                        BindingInput.SELECT,
                        VirtualOutput.key(VirtualKey.ENTER)
                    )
                )
            );

        assertSame(
            profile,
            BindingProfileTuner.adjustAnalogScale(
                profile,
                1.1f
            )
        );
        assertSame(
            profile,
            BindingProfileTuner.adjustAnalogDeadzone(
                profile,
                0.02f
            )
        );
    }
}
