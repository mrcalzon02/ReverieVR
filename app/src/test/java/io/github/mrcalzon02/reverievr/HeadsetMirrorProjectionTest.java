package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Test;
import java.nio.FloatBuffer;

public final class HeadsetMirrorProjectionTest {
    private static float[] at(float x,float y,float z) {
        return new float[] {1,0,0,0,0,1,0,0,0,0,1,0,-x,-y,-z,1};
    }

    @Test public void bothEyesSeeClippedFiniteHeadsetAndController() {
        PlayerHeadRig rig=new PlayerHeadRig();
        assertTrue(rig.update(at(0,0,0),0.064f));
        HeadsetMirrorProjection projection=new HeadsetMirrorProjection();
        float[] headset=new float[HeadsetMirrorProjection.MAX_VERTICES*3];
        float[] controller=new float[HeadsetMirrorProjection.CONTROLLER_VERTICES*3];
        for (int eye=0; eye<2; eye++) {
            assertTrue(projection.prepare(rig,eye));
            assertEquals(36,projection.writeHeadset(headset));
            assertEquals(12,projection.writeController(0.28f,-0.34f,-0.48f,
                0,0,-1,controller));
            for (float[] buffer:new float[][]{headset,controller}) {
                for (int i=0;i<buffer.length;i+=3) {
                    assertEquals(HeadsetMirrorProjection.LINE_X,buffer[i],0.00001f);
                    assertTrue(Float.isFinite(buffer[i+1]));
                    assertTrue(Float.isFinite(buffer[i+2]));
                    assertTrue(Math.abs(buffer[i+1])
                        <= HeadsetMirrorProjection.HALF_HEIGHT+0.00001f);
                    assertTrue(Math.abs(buffer[i+2])
                        <= HeadsetMirrorProjection.HALF_WIDTH+0.00001f);
                }
            }
        }
    }

    @Test public void eachEyeGetsDistinctStereoReflection() {
        PlayerHeadRig rig=new PlayerHeadRig();
        assertTrue(rig.update(at(0,0,0),0.064f));
        HeadsetMirrorProjection projection=new HeadsetMirrorProjection();
        float[] left=new float[HeadsetMirrorProjection.MAX_VERTICES*3];
        float[] right=new float[left.length];
        assertTrue(projection.prepare(rig,0));
        assertEquals(36,projection.writeHeadset(left));
        assertTrue(projection.prepare(rig,1));
        assertEquals(36,projection.writeHeadset(right));
        boolean differs=false;
        for (int i=0;i<left.length;i++) {
            if (Math.abs(left[i]-right[i]) > 0.000001f) differs=true;
        }
        assertTrue(differs);
    }

    @Test public void mirrorRejectsBacksideAndOutsidePanel() {
        PlayerHeadRig rig=new PlayerHeadRig();
        HeadsetMirrorProjection projection=new HeadsetMirrorProjection();
        float[] buffer=new float[HeadsetMirrorProjection.MAX_VERTICES*3];
        assertTrue(rig.update(at(3.0f,0,0),0.064f));
        assertFalse(projection.prepare(rig,0));
        assertEquals(0,projection.writeHeadset(buffer));
        assertTrue(rig.update(at(0,0,5.0f),0.064f));
        assertTrue(projection.prepare(rig,0));
        assertEquals(0,projection.writeHeadset(buffer));
        assertTrue(rig.update(at(0,0,0),0.064f));
        assertTrue(projection.prepare(rig,0));
        assertEquals(0,projection.writeController(Float.NaN,0,0,0,0,-1,
            new float[HeadsetMirrorProjection.CONTROLLER_VERTICES*3]));
    }

    @Test public void controllerAimChangesMirroredArrowWithoutChangingMarker() {
        PlayerHeadRig rig = new PlayerHeadRig();
        assertTrue(rig.update(at(0,0,0),0.064f));
        HeadsetMirrorProjection projection = new HeadsetMirrorProjection();
        assertTrue(projection.prepare(rig,0));
        float[] forward = new float[HeadsetMirrorProjection.CONTROLLER_VERTICES*3];
        float[] upward = new float[forward.length];
        assertEquals(12,projection.writeController(0.28f,-0.34f,-0.48f,
            0,0,-1,forward));
        assertEquals(12,projection.writeController(0.28f,-0.34f,-0.48f,
            0,1,0,upward));
        for (int i=0;i<18;i++) assertEquals(forward[i],upward[i],0.00001f);
        assertTrue(Math.abs(forward[22]-upward[22])>0.01f
            || Math.abs(forward[23]-upward[23])>0.01f);
        assertEquals(0,projection.writeController(0,0,0,0,0,0,forward));
        assertEquals(0,projection.writeController(0,0,0,Float.NaN,0,1,forward));
        assertEquals(12,projection.writeController(0,0,0,0,1,0,forward));
        for (float value:forward) assertTrue(Float.isFinite(value));
    }

    @Test public void invalidHeadPoseNeverRendersStaleReflection() {
        PlayerHeadRig rig=new PlayerHeadRig();
        HeadsetMirrorProjection projection=new HeadsetMirrorProjection();
        assertTrue(rig.update(at(0,0,0),0.064f));
        assertTrue(projection.prepare(rig,0));
        rig.reset();
        assertFalse(projection.prepare(rig,0));
        assertEquals(0,projection.writeHeadset(
            new float[HeadsetMirrorProjection.MAX_VERTICES*3]));
    }

    @Test public void avatarReflectionTracksWorldPositionForBothEyes() {
        PlayerHeadRig rig = new PlayerHeadRig();
        assertTrue(rig.update(at(0f, 0f, 0f), 0.064f));
        HeadsetMirrorProjection projection = new HeadsetMirrorProjection();
        FloatBuffer body = FloatBuffer.wrap(new float[] {
            HomeAvatarWardrobeGeometry.MIRROR_X, 0.95f, -0.25f,
            HomeAvatarWardrobeGeometry.MIRROR_X, 0.65f,  0.25f,
            HomeAvatarWardrobeGeometry.MIRROR_X, 0.35f, -0.25f
        });
        float[] left = new float[45], right = new float[45];
        float[] moved = new float[45];
        assertTrue(projection.prepare(rig, 0));
        assertEquals(3, projection.writeAvatarTriangles(body, 3,
            0f, 0f, 0f, 1f, left));
        assertEquals(3, projection.writeAvatarTriangles(body, 3,
            0f, 0f, 0.5f, 1f, moved));
        // The figure follows the player in WORLD space, rather than
        // staying at an immutable offset on the mirror.
        assertTrue(Math.abs(left[2] - moved[2]) > 0.15f);
        assertEquals(0, body.position());
        assertTrue(projection.prepare(rig, 1));
        assertEquals(3, projection.writeAvatarTriangles(body, 3,
            0f, 0f, 0f, 1f, right));
        assertTrue(Math.abs(left[2] - right[2]) > 0.0001f);
    }

    @Test public void avatarTrianglesAreClippedToPhysicalGlass() {
        PlayerHeadRig rig = new PlayerHeadRig();
        assertTrue(rig.update(at(0f, 0f, 0f), 0.064f));
        HeadsetMirrorProjection projection = new HeadsetMirrorProjection();
        assertTrue(projection.prepare(rig, 0));
        FloatBuffer wideBody = FloatBuffer.wrap(new float[] {
            HomeAvatarWardrobeGeometry.MIRROR_X, 0.0f, -5.0f,
            HomeAvatarWardrobeGeometry.MIRROR_X, 1.8f,  0.0f,
            HomeAvatarWardrobeGeometry.MIRROR_X, 0.0f,  5.0f
        });
        float[] triangles = new float[45];
        int count = projection.writeAvatarTriangles(wideBody, 3,
            0f, 0f, 0f, 1f, triangles);
        assertTrue(count >= 3);
        assertTrue(count <= 15);
        for (int i = 0; i < count * 3; i += 3) {
            assertEquals(HeadsetMirrorProjection.LINE_X,
                triangles[i], 0.00001f);
            assertTrue(Float.isFinite(triangles[i + 1]));
            assertTrue(Float.isFinite(triangles[i + 2]));
            assertTrue(Math.abs(triangles[i + 1])
                <= HeadsetMirrorProjection.HALF_HEIGHT + 0.00001f);
            assertTrue(Math.abs(triangles[i + 2])
                <= HeadsetMirrorProjection.HALF_WIDTH + 0.00001f);
        }
        assertEquals(0, projection.writeAvatarTriangles(wideBody, 3,
            Float.NaN, 0f, 0f, 1f, triangles));
        assertEquals(0, projection.writeAvatarTriangles(wideBody, 3,
            0f, 0f, 0f, 1f, new float[10]));
        assertTrue(rig.update(at(3f, 0f, 0f), 0.064f));
        assertFalse(projection.prepare(rig, 0));
        assertEquals(0, projection.writeAvatarTriangles(wideBody, 3,
            0f, 0f, 0f, 1f, triangles));
    }
}
