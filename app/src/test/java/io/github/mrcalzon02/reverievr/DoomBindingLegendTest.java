package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;
import java.util.List;
import org.junit.Test;

public final class DoomBindingLegendTest {
    @Test
    public void rightSideLegendReflectsStockDoomBindings() {
        String[] lines = DoomBindingLegend.lines(BuiltInBindingProfiles.byId(
            BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE));
        assertEquals(8, lines.length);
        assertEquals("HEAD TURN : MOUSE AIM", lines[0]);
        assertEquals("PAD UP/DN : WALK", lines[1]);
        assertEquals("PAD L/R  : STRAFE", lines[2]);
        assertEquals("PAD CLICK: FIRE", lines[3]);
        assertEquals("APP / -  : USE / OPEN", lines[4]);
        assertEquals("VOL +/-  : WEAPON +/-", lines[5]);
        assertTrue(lines[6].contains("SHELL MENU"));
        assertTrue(lines[7].contains("RESERVED"));
    }

    @Test
    public void customRebindingIsNotMisreportedAsStock() {
        BindingProfile stock = BuiltInBindingProfiles.byId(
            BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE);
        List<InputBinding> custom = new ArrayList<>();
        for (InputBinding b : stock.bindings) {
            if (b.input == BindingInput.DAYDREAM_APP_BUTTON) {
                custom.add(InputBinding.digital(b.input,
                    VirtualOutput.key(VirtualKey.E)));
            } else if (b.input == BindingInput.TOUCHPAD_X) {
                // Leave horizontal directions unbound.
            } else {
                custom.add(b);
            }
        }
        String[] lines = DoomBindingLegend.lines(stock.withBindings(custom));
        assertEquals("PAD L/R  : CUSTOM / OFF", lines[2]);
        assertEquals("APP / -  : CUSTOM / OFF", lines[4]);
    }

    @Test
    public void noActiveProfileIsClearlyReported() {
        assertEquals("NO ACTIVE INPUT PROFILE", DoomBindingLegend.lines(null)[0]);
    }
}
