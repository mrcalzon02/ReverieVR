package io.github.mrcalzon02.reverievr;

/**
 * Compact home-center hit rectangles. Kept independent of Android graphics so
 * the interaction targets can be checked without an Android SDK.
 *
 * Screen source: 1024px UI atlas, center VR panel covers x 266..744,
 * y 60..710. These are the same coordinates used to draw and raycast buttons.
 */
final class HomeMainMenuLayout {
    static final int RESUME = 13;
    static final int CONTROLLER = 14;
    static final int KEYBOARD = 15;
    static final int MODEL = 16;
    static final int IMPORT = 17;
    static final int EXIT_TO_PHONE = 26;

    private HomeMainMenuLayout() {}

    static int[] centerRect(int action) {
        switch (action) {
            case CONTROLLER: return new int[] {292, 212, 718, 278};
            case KEYBOARD:   return new int[] {292, 290, 718, 356};
            case MODEL:      return new int[] {292, 368, 718, 434};
            case IMPORT:     return new int[] {292, 446, 718, 512};
            case RESUME:     return new int[] {292, 527, 718, 593};
            default: throw new IllegalArgumentException("Not a home center action: " + action);
        }
    }

    static boolean contains(int[] rect, int x, int y) {
        return x >= rect[0] && x <= rect[2] && y >= rect[1] && y <= rect[3];
    }

    static boolean intersects(int[] a, int[] b) {
        return a[0] < b[2] && a[2] > b[0] &&
               a[1] < b[3] && a[3] > b[1];
    }
}
