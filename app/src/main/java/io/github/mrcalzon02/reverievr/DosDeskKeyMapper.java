package io.github.mrcalzon02.reverievr;

/** Translate floor-keyboard labels to the existing DOS virtual-key bus. */
final class DosDeskKeyMapper {
    static final class Stroke {
        final VirtualKey key;
        final boolean shift;
        Stroke(VirtualKey key, boolean shift) {
            this.key = key;
            this.shift = shift;
        }
    }
    static Stroke map(String label) {
        if (label == null || label.isEmpty()) return null;
        switch (label) {
            case "SPACE": return new Stroke(VirtualKey.SPACE, false);
            case "TAB": return new Stroke(VirtualKey.TAB, false);
            case "ENTER": return new Stroke(VirtualKey.ENTER, false);
            case "LEFT": return new Stroke(VirtualKey.LEFT, false);
            case "RIGHT": return new Stroke(VirtualKey.RIGHT, false);
            case "BACK": return new Stroke(VirtualKey.BACKSPACE, false);
            case "DEL": return new Stroke(VirtualKey.DELETE, false);
            default: break;
        }
        if (label.length() != 1) return null;
        char ch = label.charAt(0);
        if (ch >= 'a' && ch <= 'z')
            return new Stroke(VirtualKey.valueOf(String.valueOf(
                Character.toUpperCase(ch))), false);
        if (ch >= 'A' && ch <= 'Z')
            return new Stroke(VirtualKey.valueOf(String.valueOf(ch)), true);
        if (ch >= '0' && ch <= '9')
            return new Stroke(VirtualKey.valueOf("DIGIT_" + ch), false);
        String unshifted = "-=[]\\;',./`";
        String shifted = "_+{}|:\"<>?~";
        String digits = "!@#$%^&*()";
        int digit = digits.indexOf(ch);
        if (digit >= 0) return new Stroke(VirtualKey.valueOf(
            "DIGIT_" + ((digit + 1) % 10)), true);
        int i = unshifted.indexOf(ch);
        boolean shift = false;
        if (i < 0) { i = shifted.indexOf(ch); shift = true; }
        if (i < 0) return null;
        VirtualKey[] keys = {
            VirtualKey.MINUS, VirtualKey.EQUALS,
            VirtualKey.LEFT_BRACKET, VirtualKey.RIGHT_BRACKET,
            VirtualKey.BACKSLASH, VirtualKey.SEMICOLON,
            VirtualKey.APOSTROPHE, VirtualKey.COMMA,
            VirtualKey.PERIOD, VirtualKey.SLASH, VirtualKey.GRAVE
        };
        return new Stroke(keys[i], shift);
    }
    private DosDeskKeyMapper() {}
}
