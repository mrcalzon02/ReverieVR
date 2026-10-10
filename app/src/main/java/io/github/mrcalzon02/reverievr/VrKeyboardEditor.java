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
    private static final int MAX_LENGTH = 2048;
    private final StringBuilder buffer = new StringBuilder();
    private int caret;
    private boolean shift, caps, symbols;

    String value() { return buffer.toString(); }
    int cursor() { return caret; }
    boolean shifted() { return shift; }
    boolean capsLocked() { return caps; }
    boolean symbolsVisible() { return symbols; }

    String label(int row, int column) {
        if (row < 0 || row >= ROWS.length ||
            column < 0 || column >= ROWS[row].length) return "";
        if (symbols && row < 4) return SYMBOLS[row * 10 + column];
        String base = ROWS[row][column];
        if (base.length() == 1 && Character.isLetter(base.charAt(0))
            && (caps ^ shift)) return base.toUpperCase(java.util.Locale.US);
        return base;
    }
    boolean press(int row, int column) {
        if (row < 0 || row >= ROWS.length ||
            column < 0 || column >= ROWS[row].length) return false;
        String key = ROWS[row][column];
        switch (key) {
            case "SHIFT": shift = !shift; return true;
            case "CAPS": caps = !caps; return true;
            case "SYMS": symbols = !symbols; return true;
            case "LEFT": caret = Math.max(0, caret - 1); return true;
            case "RIGHT": caret = Math.min(buffer.length(), caret + 1); return true;
            case "BACK":
                if (caret > 0) buffer.deleteCharAt(--caret);
                return true;
            case "DEL":
                if (caret < buffer.length()) buffer.deleteCharAt(caret);
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
