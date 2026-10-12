package io.github.mrcalzon02.reverievr;

/** Deterministic non-color-only shell button colors.
 * The contrast mode uses near-black and near-white luminance separation,
 * plus persistent border/focus rails/disabled symbol geometry from the
 * shell renderer. No GPU shader, screen analysis, OCR or media changes.
 */
final class ShellContrastPalette {
    static final int ENABLED_BG = 0xff101010;
    static final int FOCUSED_BG = 0xfff5f5f5;
    static final int DISABLED_BG = 0xff333333;
    static final int ENABLED_TEXT = 0xffffffff;
    static final int FOCUSED_TEXT = 0xff090909;
    static final int DISABLED_TEXT = 0xffdedede;
    static final int ENABLED_BORDER = 0xffeeeeee;
    static final int FOCUSED_RAIL = 0xff090909;
    static final int DISABLED_BORDER = 0xff999999;

    private ShellContrastPalette() {}

    static int background(boolean enabled, boolean focused) {
        return !enabled ? DISABLED_BG : focused ? FOCUSED_BG : ENABLED_BG;
    }

    static int foreground(boolean enabled, boolean focused) {
        return !enabled ? DISABLED_TEXT : focused ? FOCUSED_TEXT : ENABLED_TEXT;
    }

    static int border(boolean enabled) {
        return enabled ? ENABLED_BORDER : DISABLED_BORDER;
    }

    // Relative luminance for sRGB contrast ratio regression checks.
    private static double channel(int c) {
        double s = c / 255.0;
        return s <= 0.04045 ? s / 12.92
            : Math.pow((s + 0.055) / 1.055, 2.4);
    }

    static double contrastRatio(int a, int b) {
        double x = .2126 * channel((a >>> 16) & 255)
            + .7152 * channel((a >>> 8) & 255)
            + .0722 * channel(a & 255);
        double y = .2126 * channel((b >>> 16) & 255)
            + .7152 * channel((b >>> 8) & 255)
            + .0722 * channel(b & 255);
        return (Math.max(x, y) + .05) / (Math.min(x, y) + .05);
    }
}
