package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class ControllerBodyAnchorTest {
    private static final float EPSILON = 0.0001f;

    @Test
    public void handSideMirrorsAroundTheBody() {
        ControllerBodyAnchor anchor = new ControllerBodyAnchor();
        anchor.update(0.0f, 0.016f);
        float[] right = new float[4];
        float[] left = new float[4];
        anchor.place(1.0f, 2.0f, 3.0f,
            0.28f, -0.34f, -0.48f, right);
        anchor.place(1.0f, 2.0f, 3.0f,
            -0.28f, -0.34f, -0.48f, left);
        assertEquals(1.28f, right[0], EPSILON);
        assertEquals(0.72f, left[0], EPSILON);
        assertEquals(1.66f, right[1], EPSILON);
        assertEquals(right[1], left[1], EPSILON);
        assertEquals(right[2], left[2], EPSILON);
    }

    @Test
    public void shortHeadLookDoesNotDragTorsoAnchor() {
        ControllerBodyAnchor anchor = new ControllerBodyAnchor();
        anchor.update(0.0f, 0.016f);
        anchor.update((float) Math.toRadians(7.0), 0.016f);
        assertEquals(0.0f, anchor.bodyYaw(), EPSILON);
    }

    @Test
    public void sustainedTurnFollowsWithoutInstantSnap() {
        ControllerBodyAnchor anchor = new ControllerBodyAnchor();
        anchor.update(0.0f, 0.016f);
        float turn = (float) (Math.PI * 0.5);
        anchor.update(turn, 0.016f);
        assertTrue(anchor.bodyYaw() > 0.0f);
        assertTrue(anchor.bodyYaw() < turn);
        for (int i = 0; i < 90; i++) {
            anchor.update(turn, 0.016f);
        }
        assertTrue(VrHeadingMath.angularDistance(
            anchor.bodyYaw(), turn
        ) < (float) Math.toRadians(11.0));
    }

    @Test
    public void yawRotatesPositionWithoutPitchInfluence() {
        ControllerBodyAnchor anchor = new ControllerBodyAnchor();
        anchor.update((float) (Math.PI * 0.5), 0.016f);
        float[] position = new float[4];
        anchor.place(0.0f, 0.0f, 0.0f,
            0.28f, -0.34f, -0.48f, position);
        assertEquals(-0.48f, position[0], EPSILON);
        assertEquals(-0.34f, position[1], EPSILON);
        assertEquals(-0.28f, position[2], EPSILON);
    }

    @Test
    public void wrapAndRecenterAreBounded() {
        ControllerBodyAnchor anchor = new ControllerBodyAnchor();
        anchor.update((float) Math.toRadians(179.0), 0.016f);
        anchor.update((float) Math.toRadians(-179.0), 0.016f);
        assertTrue(VrHeadingMath.angularDistance(
            anchor.bodyYaw(), (float) Math.toRadians(179.0)
        ) < (float) Math.toRadians(4.0));
        anchor.reset();
        anchor.update((float) Math.toRadians(-90.0), 0.016f);
        assertEquals(
            (float) Math.toRadians(-90.0),
            anchor.bodyYaw(),
            EPSILON
        );
    }
}
