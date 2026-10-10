package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public class PhoneScreenTouchMapperTest {
    @Test public void mapsToRealScreenResolutionWithClamping() {
        PhoneScreenTouchMapper mapper =
            new PhoneScreenTouchMapper(10, 20, 200, 400, 1440, 2960);
        PhoneScreenTouchMapper.Point start = mapper.map(10, 20);
        assertEquals(0f, start.x, 0.001f);
        assertEquals(0f, start.y, 0.001f);
        PhoneScreenTouchMapper.Point end = mapper.map(210, 420);
        assertEquals(1439f, end.x, 0.001f);
        assertEquals(2959f, end.y, 0.001f);
        assertNull(mapper.map(211, 420));
        assertNull(mapper.map(Float.NaN, 20));
    }
    @Test public void deniesAllGesturesWithoutPermissionAndCancelsOnRevoke() {
        PhoneScreenTouchMapper mapper =
            new PhoneScreenTouchMapper(0, 0, 1, 1, 1080, 1920);
        assertNull(mapper.begin(0.5f, 0.5f));
        mapper.setAuthorized(true);
        assertNotNull(mapper.begin(0.5f, 0.5f));
        assertTrue(mapper.isPressed());
        assertNotNull(mapper.move(0.8f, 0.7f));
        mapper.setAuthorized(false);
        assertFalse(mapper.isPressed());
        assertNull(mapper.end());
        assertNull(mapper.begin(0.5f, 0.5f));
    }
    @Test public void boundaryExitCancelsGestures() {
        PhoneScreenTouchMapper mapper =
            new PhoneScreenTouchMapper(0, 0, 1, 1, 1080, 1920);
        mapper.setAuthorized(true);
        assertNotNull(mapper.begin(0.3f, 0.3f));
        assertNull(mapper.move(-0.1f, 0.3f));
        assertFalse(mapper.isPressed());
        assertNull(mapper.end());
    }
    @Test public void rejectsInvalidScreenGeometry() {
        try {
            new PhoneScreenTouchMapper(0, 0, 0, 1, 1080, 1920);
            fail("invalid width was accepted");
        } catch (IllegalArgumentException expected) { }
    }
}
