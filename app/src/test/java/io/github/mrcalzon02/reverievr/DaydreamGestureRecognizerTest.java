package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import java.util.ArrayList;
import java.util.List;
import static org.junit.Assert.*;

public final class DaydreamGestureRecognizerTest {
    private static final long M = 1000000L;
    private final DaydreamGestureRecognizer detector =
        new DaydreamGestureRecognizer();
    private final List<String> events = new ArrayList<>();
    private final DaydreamGestureRecognizer.Sink sink =
        (input, down) -> events.add(input.name() + (down ? "+" : "-"));

    private void feed(long ms, boolean click, float accel) {
        detector.accept(ms * M, click, accel, 0f, 0f, sink);
    }

    @Test public void oneTapIsDeferredAndDoubleDoesNotAlsoFireTap() {
        feed(100, true, 1f); feed(180, false, 1f);
        feed(350, true, 1f); feed(420, false, 1f);
        feed(1000, false, 1f);
        assertEquals(2, events.size());
        assertEquals("DAYDREAM_DOUBLE_TAP+", events.get(0));
        assertEquals("DAYDREAM_DOUBLE_TAP-", events.get(1));
    }

    @Test public void pressAndHoldStaysDownUntilReleased() {
        feed(100, true, 1f); feed(800, true, 1f);
        assertEquals("DAYDREAM_PRESS_HOLD+", events.get(0));
        feed(840, false, 1f);
        assertEquals("DAYDREAM_PRESS_HOLD-", events.get(1));
        assertEquals(2, events.size());
    }

    @Test public void longPressOnReleaseSurvivesMissingSamples() {
        feed(100, true, 1f);
        feed(950, false, 1f);
        assertEquals("DAYDREAM_PRESS_HOLD+", events.get(0));
        assertEquals("DAYDREAM_PRESS_HOLD-", events.get(1));
        assertEquals(2, events.size());
    }

    @Test public void shakeRequiresStrongImpulseAndRearm() {
        feed(100, false, 1f); feed(200, false, 2.7f);
        feed(500, false, 3f);
        feed(1500, false, 1f); feed(1600, false, 2.7f);
        assertEquals(4, events.size());
    }

    @Test public void singleTapWaitsForSecondTapWindow() {
        feed(100, true, 1f); feed(180, false, 1f);
        feed(600, false, 1f);
        assertEquals("DAYDREAM_TAP+", events.get(0));
        assertEquals("DAYDREAM_TAP-", events.get(1));
    }
}
