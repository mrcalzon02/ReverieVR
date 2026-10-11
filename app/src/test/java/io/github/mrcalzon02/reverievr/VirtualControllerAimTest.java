package io.github.mrcalzon02.reverievr;
import org.junit.Test;
import static org.junit.Assert.*;

public final class VirtualControllerAimTest {
    @Test public void modelAndSelectionRayBothFaceForwardThroughoutSweep() {
        float[] d = new float[3];
        for (int degrees = -65; degrees <= 65; degrees += 5) {
            float yaw = (float) Math.toRadians(degrees);
            VirtualControllerAim.writeDirection(yaw, 0f, d);
            double modelYaw = Math.toRadians(VirtualControllerAim.modelYawDegrees(yaw));
            assertEquals(-Math.sin(modelYaw), d[0], 0.0001f);
            assertEquals(-Math.cos(modelYaw), d[2], 0.0001f);
            assertTrue(d[2] < 0f);
        }
    }
    @Test public void pitchedControllerStillFacesTheMenu() {
        float[] d = new float[3];
        VirtualControllerAim.writeDirection(0f, (float)Math.toRadians(30), d);
        assertEquals(0.5f, d[1], 0.0001f);
        assertTrue(d[2] < 0f);
    }
}
