package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class MirrorLineRibbonsTest {
    @Test public void visibleRibbonsHaveFiniteBoundedTriangles() {
        float[] lines = {HeadsetMirrorProjection.LINE_X,-0.2f,0,
            HeadsetMirrorProjection.LINE_X,0.2f,0};
        float[] triangles = new float[18];
        assertEquals(6,MirrorLineRibbons.write(lines,2,triangles));
        assertEquals(MirrorLineRibbons.HALF_THICKNESS,
            triangles[2],0.00001f);
        assertEquals(-MirrorLineRibbons.HALF_THICKNESS,
            triangles[5],0.00001f);
        for (int i=0;i<triangles.length;i+=3) {
            assertEquals(HeadsetMirrorProjection.LINE_X,triangles[i],0);
            assertTrue(Float.isFinite(triangles[i+1]));
            assertTrue(Float.isFinite(triangles[i+2]));
            assertTrue(Math.abs(triangles[i+1])
                <= HeadsetMirrorProjection.HALF_HEIGHT);
            assertTrue(Math.abs(triangles[i+2])
                <= HeadsetMirrorProjection.HALF_WIDTH);
        }
    }

    @Test public void degenerateAndInvalidSegmentsAreRejected() {
        float[] lines = {HeadsetMirrorProjection.LINE_X,0,0,
            HeadsetMirrorProjection.LINE_X,0,0,
            HeadsetMirrorProjection.LINE_X,0,0,
            HeadsetMirrorProjection.LINE_X,Float.NaN,0};
        assertEquals(0,MirrorLineRibbons.write(lines,4,new float[36]));
        assertEquals(0,MirrorLineRibbons.write(lines,3,new float[36]));
        assertEquals(0,MirrorLineRibbons.write(lines,4,new float[6]));
    }

    @Test public void thicknessClipsAtPanelEdge() {
        float[] lines = {HeadsetMirrorProjection.LINE_X,
            HeadsetMirrorProjection.HALF_HEIGHT,0,
            HeadsetMirrorProjection.LINE_X,
            HeadsetMirrorProjection.HALF_HEIGHT,0.4f};
        float[] triangles = new float[18];
        assertEquals(6,MirrorLineRibbons.write(lines,2,triangles));
        for (int i=0;i<triangles.length;i+=3) {
            assertTrue(triangles[i+1] <= HeadsetMirrorProjection.HALF_HEIGHT);
            assertTrue(triangles[i+1] >= -HeadsetMirrorProjection.HALF_HEIGHT);
        }
    }
}
