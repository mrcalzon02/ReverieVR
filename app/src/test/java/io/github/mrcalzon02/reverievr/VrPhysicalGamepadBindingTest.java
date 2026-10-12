package io.github.mrcalzon02.reverievr;

import android.view.KeyEvent;
import org.junit.Test;
import static org.junit.Assert.*;

public final class VrPhysicalGamepadBindingTest {
    @Test public void faceAndShoulderKeysRetainTheirIdentity() {
        assertEquals(BindingInput.GAMEPAD_BUTTON_A,
            VrInputRouter.mapPhysicalGamepadKey(KeyEvent.KEYCODE_BUTTON_A));
        assertEquals(BindingInput.GAMEPAD_BUTTON_B,
            VrInputRouter.mapPhysicalGamepadKey(KeyEvent.KEYCODE_BUTTON_B));
        assertEquals(BindingInput.GAMEPAD_BUTTON_X,
            VrInputRouter.mapPhysicalGamepadKey(KeyEvent.KEYCODE_BUTTON_X));
        assertEquals(BindingInput.GAMEPAD_BUTTON_Y,
            VrInputRouter.mapPhysicalGamepadKey(KeyEvent.KEYCODE_BUTTON_Y));
        assertEquals(BindingInput.GAMEPAD_BUTTON_L2,
            VrInputRouter.mapPhysicalGamepadKey(KeyEvent.KEYCODE_BUTTON_L2));
        assertEquals(BindingInput.GAMEPAD_BUTTON_R2,
            VrInputRouter.mapPhysicalGamepadKey(KeyEvent.KEYCODE_BUTTON_R2));
    }

    @Test public void platformReservedKeyCanBeObservedButNotImplicitlyGuestBound() {
        assertEquals(BindingInput.GAMEPAD_BUTTON_MODE,
            VrInputRouter.mapPhysicalGamepadKey(KeyEvent.KEYCODE_BUTTON_MODE));
        assertNull(VrInputRouter.mapPhysicalGamepadKey(
            KeyEvent.KEYCODE_VOLUME_DOWN));
        assertNull(VrInputRouter.mapPhysicalGamepadKey(KeyEvent.KEYCODE_UNKNOWN));
    }
}
