package io.github.mrcalzon02.reverievr;
import org.junit.Test;
import static org.junit.Assert.*;

public final class HomeKeyboardGeometryTest {
    private static int[][] keys() {
        int[][] all = new int[51][4];
        for (int row = 0; row < 5; row++)
            for (int col = 0; col < 10; col++) {
                int left = 72 + col * 88, top = 238 + row * 84;
                all[row*10+col] = new int[]{left, top, left+82, top+74};
            }
        all[50] = new int[]{340, 684, 684, 736};
        return all;
    }
    private static float[] pointForPixel(int x, int y) {
        return new float[]{
            HomeKeyboardGeometry.LEFT + x/1024f * 3.46f,
            HomeKeyboardGeometry.TOP_Y,
            HomeKeyboardGeometry.BACK_Z + y/768f * 2.72f
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
    @Test public void gazeAboveDeskCannotTriggerKeys() {
        assertEquals(-1, HomeKeyboardGeometry.keyAtRay(
            new float[]{0f, 0f, 0f}, new float[]{0f, 0.2f, -1f}, keys()));
    }
}
