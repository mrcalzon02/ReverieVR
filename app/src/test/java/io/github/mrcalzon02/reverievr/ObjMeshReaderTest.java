package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import java.io.IOException;
import java.io.StringReader;
import static org.junit.Assert.*;

public class ObjMeshReaderTest {
    @Test public void trianglesAndPolygonsHaveCorrectBounds() throws Exception {
        ObjMeshReader.Mesh mesh = ObjMeshReader.read(new StringReader(
            "# square\nv 0 0 0\nv 2 0 0\nv 2 3 0\nv 0 3 0\nf 1 2 3 4\n"));
        assertEquals(2, mesh.triangleCount());
        assertArrayEquals(new float[]{0,0,0,2,3,0}, mesh.bounds, 0.0f);
    }
    @Test public void negativeIndicesWork() throws Exception {
        ObjMeshReader.Mesh mesh = ObjMeshReader.read(new StringReader(
            "v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3/-1 -2/-1 -1/-1\n"));
        assertEquals(1, mesh.triangleCount());
    }
    @Test public void malformedAndNonfiniteGeometryFailClosed() throws Exception {
        assertRejected("v NaN 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3");
        assertRejected("v 0 0 0\nf 1 2 3");
        assertRejected("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 0 2 3");
        assertRejected("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2");
    }
    private static void assertRejected(String source) throws Exception {
        try { ObjMeshReader.read(new StringReader(source)); fail("accepted malformed OBJ"); }
        catch (IOException expected) { /* bounded parser rejection */ }
    }
}
