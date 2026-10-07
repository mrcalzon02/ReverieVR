package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class BoundedViewRelativeLocomotionTest {
    @Test public void forwardTracksHorizontalHeadYaw() {
        BoundedViewRelativeLocomotion m = new BoundedViewRelativeLocomotion();
        m.update(0f, -1f, 0f, -1f, 0.05f, 2f, 2f);
        assertEquals(0f, m.x(), 0.0001f);
        assertTrue(m.z() < 0f);
        m.reset();
        m.update(0f, -1f, 1f, 0f, 0.05f, 2f, 2f);
        assertTrue(m.x() > 0f);
        assertEquals(0f, m.z(), 0.0001f);
    }
    @Test public void strafeAndReverseFollowHeadDirection() {
        BoundedViewRelativeLocomotion m = new BoundedViewRelativeLocomotion();
        m.update(1f, 0f, 0f, -1f, 0.05f, 2f, 2f);
        assertTrue(m.x() > 0f);
        m.reset();
        m.update(0f, 1f, 0f, -1f, 0.05f, 2f, 2f);
        assertTrue(m.z() > 0f);
    }
    @Test public void deadzoneAndReleaseStopWithoutCoasting() {
        BoundedViewRelativeLocomotion m = new BoundedViewRelativeLocomotion();
        m.update(0.12f, -0.12f, 0f, -1f, 0.05f, 2f, 2f);
        assertEquals(0f, m.z(), 0f);
        m.update(0f, -1f, 0f, -1f, 0.05f, 2f, 2f);
        float z = m.z();
        m.update(0f, 0f, 0f, -1f, 0.05f, 2f, 2f);
        assertEquals(z, m.z(), 0f);
    }
    @Test public void boundsAndFrameGapAreClamped() {
        BoundedViewRelativeLocomotion m = new BoundedViewRelativeLocomotion();
        for (int i = 0; i < 2000; i++)
            m.update(1f, -1f, 0f, -1f, 1f, 0.7f, 0.3f);
        assertEquals(0.7f, m.x(), 0.0001f);
        assertEquals(-0.3f, m.z(), 0.0001f);
    }
    @Test public void diagonalsAreNotFaster() {
        BoundedViewRelativeLocomotion a = new BoundedViewRelativeLocomotion();
        BoundedViewRelativeLocomotion b = new BoundedViewRelativeLocomotion();
        a.update(0f, -1f, 0f, -1f, 0.05f, 2f, 2f);
        b.update(1f, -1f, 0f, -1f, 0.05f, 2f, 2f);
        assertEquals(-a.z(), Math.hypot(b.x(), b.z()), 0.0001);
    }
    @Test public void invalidAndVerticalHeadingsDoNotMove() {
        BoundedViewRelativeLocomotion m = new BoundedViewRelativeLocomotion();
        m.update(Float.NaN, -1f, 0f, -1f, 0.05f, 2f, 2f);
        m.update(0f, -1f, 0f, 0f, 0.05f, 2f, 2f);
        m.update(0f, -1f, 0f, -1f, -1f, 2f, 2f);
        m.update(0f, -1f, 0f, -1f, 0.05f, 0f, 2f);
        assertEquals(0f, m.x(), 0f);
        assertEquals(0f, m.z(), 0f);
    }
}
