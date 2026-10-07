package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class ControllerGhostTetherTest {
    @Test
    public void coilConnectsLiveAndNeutralHand() {
        float[] v = new float[ControllerGhostTether.VERTEX_COUNT * 3];
        int count = ControllerGhostTether.write(
            v, 0.35f, -0.30f, -0.60f, 0.24f, -0.32f, -0.46f
        );
        assertEquals(ControllerGhostTether.VERTEX_COUNT, count);
        assertEquals(0.35f, v[0], 0.000001f);
        assertEquals(-0.30f, v[1], 0.000001f);
        assertEquals(-0.60f, v[2], 0.000001f);
        int end = (count - 1) * 3;
        assertEquals(0.24f, v[end], 0.000001f);
        assertEquals(-0.32f, v[end + 1], 0.000001f);
        assertEquals(-0.46f, v[end + 2], 0.000001f);
        for (float x : v) assertTrue(Float.isFinite(x));
    }

    @Test
    public void axialAndDegenerateCasesStayFinite() {
        float[] v = new float[ControllerGhostTether.VERTEX_COUNT * 3];
        for (int axis = 0; axis < 3; axis++) {
            assertEquals(ControllerGhostTether.VERTEX_COUNT,
                ControllerGhostTether.write(v, 0, 0, 0,
                    axis == 0 ? 0.15f : 0,
                    axis == 1 ? 0.15f : 0,
                    axis == 2 ? 0.15f : 0));
            for (float x : v) assertTrue(Float.isFinite(x));
        }
        assertEquals(0, ControllerGhostTether.write(
            v, 0, 0, 0, 0.001f, 0, 0));
        assertEquals(0, ControllerGhostTether.write(
            v, 0, 0, 0, Float.NaN, 0, 0));
    }

    @Test
    public void opacityFadesAtTarget() {
        assertEquals(0.0f, ControllerGhostTether.opacity(0), 0);
        assertTrue(ControllerGhostTether.opacity(0.01f)
            < ControllerGhostTether.opacity(0.04f));
        assertEquals(0.27f, ControllerGhostTether.opacity(0.2f), 0.00001f);
    }
}
