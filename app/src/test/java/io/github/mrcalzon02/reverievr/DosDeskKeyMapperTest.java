package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class DosDeskKeyMapperTest {
    @Test public void mapsAllLetterDirectionsAndGuestControlKeys() {
        assertEquals(VirtualKey.W, DosDeskKeyMapper.map("w").key);
        assertFalse(DosDeskKeyMapper.map("w").shift);
        assertTrue(DosDeskKeyMapper.map("W").shift);
        assertEquals(VirtualKey.LEFT, DosDeskKeyMapper.map("LEFT").key);
        assertEquals(VirtualKey.RIGHT, DosDeskKeyMapper.map("RIGHT").key);
        assertEquals(VirtualKey.BACKSPACE, DosDeskKeyMapper.map("BACK").key);
        assertEquals(VirtualKey.ENTER, DosDeskKeyMapper.map("ENTER").key);
        assertEquals(VirtualKey.TAB, DosDeskKeyMapper.map("TAB").key);
        assertEquals(VirtualKey.SPACE, DosDeskKeyMapper.map("SPACE").key);
        assertEquals(VirtualKey.DIGIT_1, DosDeskKeyMapper.map("!").key);
        assertTrue(DosDeskKeyMapper.map("!").shift);
        assertEquals(VirtualKey.SLASH, DosDeskKeyMapper.map("?").key);
        assertTrue(DosDeskKeyMapper.map("?").shift);
    }
    @Test public void refusesUnknownKeys() {
        assertNull(DosDeskKeyMapper.map(null));
        assertNull(DosDeskKeyMapper.map(""));
        assertNull(DosDeskKeyMapper.map("SYMS"));
        assertNull(DosDeskKeyMapper.map("UNKNOWN"));
    }
}
