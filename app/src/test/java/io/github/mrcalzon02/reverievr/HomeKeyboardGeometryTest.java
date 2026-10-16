package io.github.mrcalzon02.reverievr;
import org.junit.Test;
import static org.junit.Assert.*;

public final class HomeKeyboardGeometryTest {
    private static int[][] keys() {
        return HomeKeyboardGeometry.keyRects();
    }
    private static float[] pointForPixel(int x, int y) {
        return new float[]{
            HomeKeyboardGeometry.worldX(x),
            HomeKeyboardGeometry.legendAt(HomeKeyboardGeometry.worldZ(y)),
            HomeKeyboardGeometry.worldZ(y)
        };
    }
    @Test public void deskIntersectionsMapToRealKeysAndCopyButton() {
        float[] origin = new float[]{0f, 0f, 0f};
        assertEquals(0, HomeKeyboardGeometry.keyAtRay(
            origin, pointForPixel(100, 275), keys()));
        assertEquals(49, HomeKeyboardGeometry.keyAtRay(
            origin, pointForPixel(875, 630), keys()));
        assertEquals(50, HomeKeyboardGeometry.keyAtRay(
            origin, pointForPixel(510, 710), keys()));
    }
    @Test public void independentDeskRayCanHitKeysWithScreenPanelOpen() {
        // Keyboard ray calculation is independent of any vertical phone
        // panel. Home must use this same selection path in both states.
        float[] origin = {0f, 0f, 0f};
        assertEquals(0, HomeKeyboardGeometry.keyAtRay(
            origin, pointForPixel(105, 280), keys()));
        assertEquals(50, HomeKeyboardGeometry.keyAtRay(
            origin, pointForPixel(515, 707), keys()));
    }
    @Test public void missesDeskEdgesAndBackwardRays() {
        float[] origin = {0f, 0f, 0f};
        assertEquals(-1, HomeKeyboardGeometry.keyAtRay(
            origin, new float[]{5f, -1f, -2f}, keys()));
        assertEquals(-1, HomeKeyboardGeometry.keyAtRay(
            origin, new float[]{0f, -1f, 2f}, keys()));
    }
    @Test public void allPhysicalKeycapsShareTheirInputRectangles() {
        int[][] buttons = keys();
        assertEquals(51, buttons.length);
        float[] origin = {0f, 0f, 0f};
        for (int i = 0; i < buttons.length; i++) {
            int[] rect = buttons[i];
            float centerX = HomeKeyboardGeometry.worldX(
                (rect[0] + rect[2]) * 0.5f);
            float centerZ = HomeKeyboardGeometry.worldZ(
                (rect[1] + rect[3]) * 0.5f);
            assertEquals("Keycap and ray hit differ for key " + i,
                i, HomeKeyboardGeometry.keyAtRay(
                    origin,
                    new float[]{centerX, HomeKeyboardGeometry.legendAt(centerZ),
                        centerZ},
                    buttons));
            assertTrue(centerX > HomeKeyboardGeometry.LEFT);
            assertTrue(centerX < HomeKeyboardGeometry.RIGHT);
            assertTrue(centerZ > HomeKeyboardGeometry.BACK_Z);
            assertTrue(centerZ < HomeKeyboardGeometry.FRONT_Z);
        }
    }

    @Test public void raisedCapsAndLegendsSitAboveGrayDais() {
        assertTrue(HomeKeyboardGeometry.KEYCAP_BASE_Y
            > HomeKeyboardGeometry.DESK_TOP_Y);
        assertTrue(HomeKeyboardGeometry.KEYCAP_TOP_Y
            > HomeKeyboardGeometry.KEYCAP_BASE_Y);
        assertTrue(HomeKeyboardGeometry.LEGEND_Y
            > HomeKeyboardGeometry.KEYCAP_TOP_Y);
        assertTrue(HomeKeyboardGeometry.legendAt(HomeKeyboardGeometry.BACK_Z)
            > HomeKeyboardGeometry.legendAt(HomeKeyboardGeometry.FRONT_Z));
        assertEquals(-1.45f, HomeKeyboardGeometry.worldX(0f), 0.0001f);
        assertEquals(1.45f,
            HomeKeyboardGeometry.worldX(
                HomeKeyboardGeometry.WIDTH_PIXELS), 0.0001f);
        assertEquals(-3.65f, HomeKeyboardGeometry.worldZ(0f), 0.0001f);
        assertEquals(-1.42f,
            HomeKeyboardGeometry.worldZ(
                HomeKeyboardGeometry.HEIGHT_PIXELS), 0.0001f);
    }

    @Test public void menuAndKeyboardDoNotOverlapFromSeatedHeadset() {
        // Check angular separation; planes can avoid intersection and yet
        // still overlap in the player's view at their different depths.
        float menuBottomY = VrShellRenderer.HOME_WORLD_BOTTOM
            * VrShellRenderer.SHELL_VIEW_CONTRACTION;
        float menuLowerAngle = -menuBottomY
            / -VrShellRenderer.HOME_CENTER_WORLD_Z;
        float rearLegendAngle = -HomeKeyboardGeometry.legendAt(
            HomeKeyboardGeometry.BACK_Z) / -HomeKeyboardGeometry.BACK_Z;
        assertTrue("Desk rear must project below menu lower edge",
            rearLegendAngle - menuLowerAngle > 0.015f);
        assertEquals("Dais and keycap rise must stay coupled", 0.011f,
            HomeKeyboardGeometry.KEYCAP_BASE_Y
                - HomeKeyboardGeometry.DESK_TOP_Y, 0.0001f);
    }

    @Test public void gazeAboveDeskCannotTriggerKeys() {
        assertEquals(-1, HomeKeyboardGeometry.keyAtRay(
            new float[]{0f, 0f, 0f}, new float[]{0f, 0.2f, -1f}, keys()));
    }
}
