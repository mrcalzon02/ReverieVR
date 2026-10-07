package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class PlayerHeadRigTest {
    private static final float EPS=0.00001f;

    private static float[] viewAt(float x,float y,float z) {
        return new float[] {1,0,0,0,0,1,0,0,0,0,1,0,-x,-y,-z,1};
    }

    @Test public void eyesUseCenterHeadAndCalibratedIpd() {
        PlayerHeadRig rig=new PlayerHeadRig();
        float[] view=viewAt(2.0f,1.6f,3.0f);
        assertTrue(rig.update(view,0.064f));
        float[] center=new float[3],eyes=new float[6];
        assertTrue(rig.copyHeadCenter(center));
        assertTrue(rig.copyEyeCenters(eyes));
        assertEquals(2.0f,center[0],EPS);
        assertEquals(1.6f,center[1],EPS);
        assertEquals(3.0f,center[2],EPS);
        assertEquals(1.968f,eyes[0],EPS);
        assertEquals(2.032f,eyes[3],EPS);
        assertEquals(1.6f,eyes[1],EPS);
        assertEquals(3.0f,eyes[2],EPS);
        assertEquals(-2.0f,view[12],EPS);
    }

    @Test public void yawRotatesEyesAndColliderWithoutMovingHeadCenter() {
        PlayerHeadRig rig=new PlayerHeadRig();
        // Inverse of +90-degree headset yaw at world (2,1.6,3).
        float[] view={0,0,1,0,0,1,0,0,-1,0,0,0,3,-1.6f,-2,1};
        assertTrue(rig.update(view,0.060f));
        float[] eyes=new float[6],head=new float[3];
        assertTrue(rig.copyEyeCenters(eyes));
        assertTrue(rig.copyHeadCenter(head));
        assertEquals(2.0f,head[0],EPS);
        assertEquals(3.0f,head[2],EPS);
        assertEquals(2.0f,eyes[0],EPS);
        assertEquals(2.0f,eyes[3],EPS);
        assertEquals(3.030f,eyes[2],EPS);
        assertEquals(2.970f,eyes[5],EPS);
        assertTrue(rig.intersectsSphere(2,1.6f,3,0));
        assertFalse(rig.intersectsSphere(2,1.6f,3.3f,0.01f));
    }

    @Test public void sphereAgainstOrientedBoxIsExactAndBounded() {
        PlayerHeadRig rig=new PlayerHeadRig();
        assertTrue(rig.update(viewAt(0,0,0),0.063f));
        assertTrue(rig.intersectsSphere(0.12f,0,0,0.02f));
        assertFalse(rig.intersectsSphere(0.16f,0,0,0.02f));
        assertTrue(rig.intersectsSphere(0,0,-0.066f,0.002f));
        assertFalse(rig.intersectsSphere(0,0,-0.09f,0.01f));
        assertFalse(rig.intersectsSphere(0,0,0,Float.NaN));
        assertFalse(rig.intersectsSphere(0,0,0,-0.01f));
    }

    @Test public void geometryHasBoxEdgesAndEyeCrosses() {
        PlayerHeadRig rig=new PlayerHeadRig();
        assertTrue(rig.update(viewAt(0,0,0),0.064f));
        float[] wire=new float[PlayerHeadRig.WIREFRAME_FLOATS];
        assertEquals(PlayerHeadRig.WIREFRAME_VERTICES,rig.writeWireframe(wire));
        assertEquals(-PlayerHeadRig.HALF_WIDTH,wire[0],EPS);
        assertEquals(-PlayerHeadRig.HALF_HEIGHT,wire[1],EPS);
        assertEquals(PlayerHeadRig.CENTER_BACK-PlayerHeadRig.HALF_DEPTH,wire[2],EPS);
        assertEquals(-0.032f-PlayerHeadRig.EYE_MARKER_RADIUS,wire[72],EPS);
        assertEquals(0.032f-PlayerHeadRig.EYE_MARKER_RADIUS,wire[90],EPS);
        for (float f:wire) assertTrue(Float.isFinite(f));
    }

    @Test public void invalidPoseClearsColliderAndGeometry() {
        PlayerHeadRig rig=new PlayerHeadRig();
        assertTrue(rig.update(viewAt(0,0,0),0.063f));
        float[] bad=viewAt(0,0,0);
        bad[15]=0;
        assertFalse(rig.update(bad,0.063f));
        assertFalse(rig.isValid());
        assertFalse(rig.intersectsSphere(0,0,0,0.1f));
        assertEquals(0,rig.writeWireframe(new float[PlayerHeadRig.WIREFRAME_FLOATS]));
        assertFalse(rig.update(viewAt(0,0,0),0.1f));
        assertFalse(rig.update(null,0.063f));
        bad=viewAt(0,0,0);
        bad[4]=0.4f;
        assertFalse(rig.update(bad,0.063f));
        assertTrue(rig.update(viewAt(0,0,0),0.063f));
        rig.reset();
        assertFalse(rig.copyEyeCenters(new float[6]));
    }
}
