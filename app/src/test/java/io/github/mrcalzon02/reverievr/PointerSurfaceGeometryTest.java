package io.github.mrcalzon02.reverievr;
import java.nio.FloatBuffer;
import static org.junit.Assert.*;
import org.junit.Test;

public final class PointerSurfaceGeometryTest {
    @Test public void wallMarkerLiesOnWallPlaneAndBeamStopsAtHit() {
        FloatBuffer out=FloatBuffer.allocate(54);
        PointerRibbonGeometry.fillSurface(out,0,0,0,0,0,-3,
            .024f,0,0,1);
        assertEquals(54,out.position());
        // First 6 vertices are the ray beam; final 12 lie 12mm camera-side
        // of the impacted triangle at z=-3.
        for(int i=18;i<54;i+=3)
            assertEquals(-2.988f,out.get(i+2),.00001f);
    }
    @Test public void floorMarkerRotatesIntoFloorPlane() {
        FloatBuffer out=FloatBuffer.allocate(54);
        PointerRibbonGeometry.fillSurface(out,0,0,0,0,-1,0,
            .030f,0,1,0);
        for(int i=18;i<54;i+=3)
            assertEquals(-.988f,out.get(i+1),.00001f);
        for(int i=0;i<54;i++)assertTrue(Float.isFinite(out.get(i)));
    }
    @Test public void rejectsShortOutputWithoutPartialWrite() {
        FloatBuffer out=FloatBuffer.allocate(53);
        assertThrows(IllegalArgumentException.class,
            ()->PointerRibbonGeometry.fillSurface(out,0,0,0,0,-1,0,
                .024f,0,1,0));
        assertEquals(0,out.position());
    }
}
