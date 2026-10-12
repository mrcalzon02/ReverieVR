package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public class ShellContrastPaletteTest {
    @Test public void textMeetsEnhancedLargeAndNormalContrastTargets() {
        for (boolean enabled : new boolean[]{true, false})
            for (boolean focused : new boolean[]{true, false}) {
                double ratio = ShellContrastPalette.contrastRatio(
                    ShellContrastPalette.background(enabled, focused),
                    ShellContrastPalette.foreground(enabled, focused));
                assertTrue("Accessible label contrast " + ratio, ratio >= 7.0);
            }
    }

    @Test public void allThreeStatesHaveDistinctSurfacesAndBorders() {
        assertNotEquals(ShellContrastPalette.background(true, false),
            ShellContrastPalette.background(true, true));
        assertNotEquals(ShellContrastPalette.background(true, true),
            ShellContrastPalette.background(false, false));
        assertNotEquals(ShellContrastPalette.ENABLED_BORDER,
            ShellContrastPalette.DISABLED_BORDER);
        assertTrue(ShellContrastPalette.contrastRatio(
            ShellContrastPalette.ENABLED_BG,
            ShellContrastPalette.ENABLED_BORDER) >= 7.0);
        assertTrue(ShellContrastPalette.contrastRatio(
            ShellContrastPalette.FOCUSED_BG,
            ShellContrastPalette.FOCUSED_RAIL) >= 7.0);
    }

    @Test public void contrastPaletteIsMonochromatic() {
        int[] values = {
            ShellContrastPalette.ENABLED_BG, ShellContrastPalette.FOCUSED_BG,
            ShellContrastPalette.DISABLED_BG, ShellContrastPalette.ENABLED_TEXT,
            ShellContrastPalette.FOCUSED_TEXT, ShellContrastPalette.DISABLED_TEXT,
            ShellContrastPalette.ENABLED_BORDER, ShellContrastPalette.FOCUSED_RAIL,
            ShellContrastPalette.DISABLED_BORDER
        };
        for (int color : values) {
            int red=(color >>> 16)&255, green=(color >>> 8)&255,
                blue=color&255;
            assertEquals(red,green);
            assertEquals(green,blue);
        }
    }
}
