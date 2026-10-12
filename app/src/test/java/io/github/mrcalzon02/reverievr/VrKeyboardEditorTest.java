package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public class VrKeyboardEditorTest {
    @Test public void insertsAndEditsWithCaret() {
        VrKeyboardEditor editor = new VrKeyboardEditor();
        assertTrue(editor.press(1,0)); // q
        assertTrue(editor.press(1,1)); // w
        assertEquals("qw", editor.value());
        assertTrue(editor.press(4,4)); // left
        assertTrue(editor.press(1,2)); // e
        assertEquals("qew",editor.value());
        assertEquals(2,editor.cursor());
        assertTrue(editor.press(4,6)); // backspace
        assertEquals("qw",editor.value());
        assertTrue(editor.press(4,7)); // delete
        assertEquals("q",editor.value());
    }

    @Test public void externalHidTypingSharesThePhysicalDeskEditorCaret() {
        VrKeyboardEditor editor = new VrKeyboardEditor();
        assertTrue(editor.press(1,0)); // q, entered by controller pointer
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TEXT, "A"));
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TEXT, "é"));
        assertEquals("qAé", editor.value());
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_LEFT, null));
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TEXT, "🙂"));
        assertEquals("qA🙂é", editor.value());
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_BACKSPACE, null));
        assertEquals("qAé", editor.value());
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_ENTER, null));
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TAB, null));
        assertEquals("qA\n\té", editor.value());
    }

    @Test public void caretAndDeleteAreUnicodeCodePointSafe() {
        VrKeyboardEditor editor = new VrKeyboardEditor();
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TEXT, "A🙂B"));
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_LEFT, null));
        assertEquals(3, editor.cursor());
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_LEFT, null));
        assertEquals(1, editor.cursor());
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_DELETE, null));
        assertEquals("AB", editor.value());
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_RIGHT, null));
        assertEquals(2, editor.cursor());
    }

    @Test public void rejectsMalformedOrOutOfBoundsExternalText() {
        VrKeyboardEditor editor = new VrKeyboardEditor();
        assertFalse(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TEXT, ""));
        assertFalse(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TEXT, null));
        assertFalse(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TEXT, "\u0000"));
        assertFalse(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TEXT,
            String.valueOf((char)0xD800)));
        assertFalse(editor.applyExternal(999, "bad"));
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TEXT, "x".repeat(2048)));
        assertFalse(editor.applyExternal(VrKeyboardEditor.EXTERNAL_TEXT, "y"));
        assertEquals(2048, editor.value().length());
    }

    @Test public void modifiersAndControlKeysHaveRealEffects() {
        VrKeyboardEditor editor = new VrKeyboardEditor();
        assertTrue(editor.press(3,0)); // shift
        assertEquals("Q",editor.label(1,0));
        assertTrue(editor.press(1,0));
        assertEquals("Q",editor.value());
        assertFalse(editor.shifted());
        editor.press(4,0); // caps
        editor.press(1,1);
        assertEquals("QW",editor.value());
        editor.press(4,1); // symbols
        assertEquals("!",editor.label(0,0));
        assertEquals("SHIFT",editor.label(3,0));
        editor.press(0,0);
        editor.press(4,2); // space
        editor.press(4,3); // tab
        editor.press(4,8); // enter
        assertEquals("QW! \t\n",editor.value());
        editor.press(4,9); // clear
        assertEquals("",editor.value());
        assertEquals(0,editor.cursor());
        assertFalse(editor.press(-1,0));
        assertFalse(editor.press(5,0));
    }
}
