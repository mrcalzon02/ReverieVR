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
