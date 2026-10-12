package io.github.mrcalzon02.reverievr;

/** Controller-selectable text editor, independent of the Android IME. */
final class VrKeyboardEditor {
    static final String[][] ROWS = {
        {"1","2","3","4","5","6","7","8","9","0"},
        {"q","w","e","r","t","y","u","i","o","p"},
        {"a","s","d","f","g","h","j","k","l",";"},
        {"SHIFT","z","x","c","v","b","n","m",",","."},
        {"CAPS","SYMS","SPACE","TAB","LEFT","RIGHT","BACK","DEL","ENTER","CLEAR"}
    };
    private static final String[] SYMBOLS = {
        "!","@","#","$","%","^","&","*","(",")",
        "[","]","{","}","/","\\","|","~","=","+",
        "-","_","'","\"","<",">","?",":",";","\u0060",
        "!","@","SPACE","TAB","LEFT","RIGHT","BACK","DEL","ENTER","CLEAR"
    };
    static final int EXTERNAL_TEXT = 0;
    static final int EXTERNAL_BACKSPACE = 1;
    static final int EXTERNAL_DELETE = 2;
    static final int EXTERNAL_LEFT = 3;
    static final int EXTERNAL_RIGHT = 4;
    static final int EXTERNAL_TAB = 5;
    static final int EXTERNAL_ENTER = 6;
    static final int EXTERNAL_NO_TEXT = -1;
    static final int EXTERNAL_CAPS_TOGGLE = 7;
    private static final int MAX_LENGTH = 2048;
    private final StringBuilder buffer = new StringBuilder();
    private int caret;
    private boolean shift, caps, symbols, physicalShift;

    void setPhysicalShift(boolean down) {
        physicalShift = down;
    }

    String value() { return buffer.toString(); }
    int cursor() { return caret; }
    boolean shifted() { return shift; }
    boolean capsLocked() { return caps; }
    boolean symbolsVisible() { return symbols; }

    String label(int row, int column) {
        if (row < 0 || row >= ROWS.length ||
            column < 0 || column >= ROWS[row].length) return "";
        if (symbols && row < 4 && !(row == 3 && column == 0))
            return SYMBOLS[row * 10 + column];
        String base = ROWS[row][column];
        boolean uppercase = caps ^ shift ^ physicalShift;
        boolean shifted = shift ^ physicalShift;
        if (!symbols && shifted) {
            if (row == 0) return SYMBOLS[column];
            if (row == 2 && column == 9) return ":";
            if (row == 3 && column == 8) return "<";
            if (row == 3 && column == 9) return ">";
        }
        if (base.length() == 1 && Character.isLetter(base.charAt(0))
            && uppercase)
            return base.toUpperCase(java.util.Locale.US);
        return base;
    }
    /**
     * Real USB/Bluetooth/scrcpy keyboard text uses the same caret and editor
     * as the physical-looking Home desk keys. Do not route it to a guest app.
     * The text comes from Android's KeyEvent unicode conversion on the input
     * thread, never from hardcoded US-layout key codes.
     */
    boolean applyExternal(int action, String text) {
        switch (action) {
            case EXTERNAL_BACKSPACE: return press(4, 6);
            case EXTERNAL_DELETE: return press(4, 7);
            case EXTERNAL_LEFT: return press(4, 4);
            case EXTERNAL_RIGHT: return press(4, 5);
            case EXTERNAL_TAB: return insertExact("\t");
            case EXTERNAL_ENTER: return insertExact("\n");
            case EXTERNAL_CAPS_TOGGLE: return press(4, 0);
            case EXTERNAL_TEXT: return insertExact(text);
            default: return false;
        }
    }

    private boolean insertExact(String text) {
        if (text == null || text.isEmpty() ||
            buffer.length() + text.length() > MAX_LENGTH) return false;
        for (int i = 0; i < text.length();) {
            int codePoint = text.codePointAt(i);
            // Reject dangling UTF-16 surrogates rather than corrupting
            // the Home text editor when an external HID sends malformed text.
            if (codePoint >= Character.MIN_SURROGATE
                && codePoint <= Character.MAX_SURROGATE) return false;
            if (Character.isISOControl(codePoint)
                && codePoint != '\n' && codePoint != '\t') return false;
            i += Character.charCount(codePoint);
        }
        buffer.insert(caret, text);
        caret += text.length();
        shift = false;
        return true;
    }

    boolean press(int row, int column) {
        if (row < 0 || row >= ROWS.length ||
            column < 0 || column >= ROWS[row].length) return false;
        String key = ROWS[row][column];
        switch (key) {
            case "SHIFT": shift = !shift; return true;
            case "CAPS": caps = !caps; return true;
            case "SYMS": symbols = !symbols; return true;
            case "LEFT":
                if (caret > 0) caret = Character.offsetByCodePoints(buffer, caret, -1);
                return true;
            case "RIGHT":
                if (caret < buffer.length())
                    caret = Character.offsetByCodePoints(buffer, caret, 1);
                return true;
            case "BACK":
                if (caret > 0) {
                    int prior = Character.offsetByCodePoints(buffer, caret, -1);
                    buffer.delete(prior, caret);
                    caret = prior;
                }
                return true;
            case "DEL":
                if (caret < buffer.length()) {
                    int next = Character.offsetByCodePoints(buffer, caret, 1);
                    buffer.delete(caret, next);
                }
                return true;
            case "CLEAR": buffer.setLength(0); caret = 0; return true;
            default:
                String value = key.equals("SPACE") ? " " :
                    key.equals("TAB") ? "\t" :
                    key.equals("ENTER") ? "\n" : label(row, column);
                if (buffer.length() + value.length() > MAX_LENGTH) return false;
                buffer.insert(caret, value);
                caret += value.length();
                shift = false;
                return true;
        }
    }
}
