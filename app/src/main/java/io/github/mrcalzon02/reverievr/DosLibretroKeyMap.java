package io.github.mrcalzon02.reverievr;

final class DosLibretroKeyMap {
    private DosLibretroKeyMap() {
    }

    static int[] create() {
        VirtualKey[] keys = VirtualKey.values();
        int[] result = new int[keys.length];

        for (int index = 0; index < keys.length; index++) {
            result[index] = toRetroKey(keys[index]);
        }
        return result;
    }

    private static int toRetroKey(VirtualKey key) {
        switch (key) {
            case BACKSPACE: return 8;
            case TAB: return 9;
            case ENTER: return 13;
            case PAUSE: return 19;
            case ESCAPE: return 27;
            case SPACE: return 32;

            case APOSTROPHE: return 39;
            case COMMA: return 44;
            case MINUS: return 45;
            case PERIOD: return 46;
            case SLASH: return 47;

            case DIGIT_0: return 48;
            case DIGIT_1: return 49;
            case DIGIT_2: return 50;
            case DIGIT_3: return 51;
            case DIGIT_4: return 52;
            case DIGIT_5: return 53;
            case DIGIT_6: return 54;
            case DIGIT_7: return 55;
            case DIGIT_8: return 56;
            case DIGIT_9: return 57;

            case SEMICOLON: return 59;
            case EQUALS: return 61;
            case LEFT_BRACKET: return 91;
            case BACKSLASH: return 92;
            case RIGHT_BRACKET: return 93;
            case GRAVE: return 96;

            case A: return 97;
            case B: return 98;
            case C: return 99;
            case D: return 100;
            case E: return 101;
            case F: return 102;
            case G: return 103;
            case H: return 104;
            case I: return 105;
            case J: return 106;
            case K: return 107;
            case L: return 108;
            case M: return 109;
            case N: return 110;
            case O: return 111;
            case P: return 112;
            case Q: return 113;
            case R: return 114;
            case S: return 115;
            case T: return 116;
            case U: return 117;
            case V: return 118;
            case W: return 119;
            case X: return 120;
            case Y: return 121;
            case Z: return 122;

            case DELETE: return 127;

            case NUMPAD_0: return 256;
            case NUMPAD_1: return 257;
            case NUMPAD_2: return 258;
            case NUMPAD_3: return 259;
            case NUMPAD_4: return 260;
            case NUMPAD_5: return 261;
            case NUMPAD_6: return 262;
            case NUMPAD_7: return 263;
            case NUMPAD_8: return 264;
            case NUMPAD_9: return 265;
            case NUMPAD_DOT: return 266;
            case NUMPAD_DIVIDE: return 267;
            case NUMPAD_MULTIPLY: return 268;
            case NUMPAD_SUBTRACT: return 269;
            case NUMPAD_ADD: return 270;

            case UP: return 273;
            case DOWN: return 274;
            case RIGHT: return 275;
            case LEFT: return 276;
            case INSERT: return 277;
            case HOME: return 278;
            case END: return 279;
            case PAGE_UP: return 280;
            case PAGE_DOWN: return 281;

            case F1: return 282;
            case F2: return 283;
            case F3: return 284;
            case F4: return 285;
            case F5: return 286;
            case F6: return 287;
            case F7: return 288;
            case F8: return 289;
            case F9: return 290;
            case F10: return 291;
            case F11: return 292;
            case F12: return 293;

            case NUM_LOCK: return 300;
            case CAPS_LOCK: return 301;
            case SCROLL_LOCK: return 302;
            case SHIFT: return 304;
            case CTRL: return 306;
            case ALT: return 308;
            case PRINT_SCREEN: return 316;

            default:
                return 0;
        }
    }
}
