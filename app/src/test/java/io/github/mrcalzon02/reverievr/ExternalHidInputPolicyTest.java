package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public class ExternalHidInputPolicyTest {
    @Test public void enabledDoesNotBlockKeyboardOrMouse() {
        assertFalse(ExternalHidInputPolicy.shouldConsume(true,true,false,false,false));
        assertFalse(ExternalHidInputPolicy.shouldConsume(true,false,true,false,false));
    }
    @Test public void disabledBlocksOnlyExternalHidTraffic() {
        assertTrue(ExternalHidInputPolicy.shouldConsume(false,true,false,false,false));
        assertTrue(ExternalHidInputPolicy.shouldConsume(false,false,true,false,false));
        assertFalse(ExternalHidInputPolicy.shouldConsume(false,false,false,false,false));
        assertFalse(ExternalHidInputPolicy.shouldConsume(false,true,true,true,false));
    }
    @Test public void shellRecoveryStaysAvailableEvenWhenDisabled() {
        assertFalse(ExternalHidInputPolicy.shouldConsume(false,true,false,false,true));
        assertFalse(ExternalHidInputPolicy.shouldConsume(false,false,true,false,true));
    }
}
