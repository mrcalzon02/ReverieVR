package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

import java.util.Arrays;

public final class BindingProfileCodecTest {
    @Test
    public void roundTripsCustomBindingProfile() {
        BindingProfile original = new BindingProfile(
            "doom-custom",
            "Doom custom profile",
            Arrays.asList(
                InputBinding.digital(
                    BindingInput.SELECT,
                    VirtualOutput.key(VirtualKey.CTRL)
                ),
                InputBinding.analog(
                    BindingInput.HEAD_YAW_DELTA,
                    VirtualOutput.mouseRelativeX(),
                    725.0f,
                    0.01f
                )
            )
        );

        BindingProfile decoded =
            BindingProfileCodec.decode(
                BindingProfileCodec.encode(original)
            );

        assertEquals(original.id, decoded.id);
        assertEquals(original.displayName, decoded.displayName);
        assertEquals(2, decoded.bindings.size());
        assertEquals(
            BindingInput.HEAD_YAW_DELTA,
            decoded.bindings.get(1).input
        );
        assertEquals(
            VirtualOutputKind.MOUSE_REL_X,
            decoded.bindings.get(1).output.kind
        );
        assertEquals(
            725.0f,
            decoded.bindings.get(1).scale,
            0.0001f
        );
    }
}
