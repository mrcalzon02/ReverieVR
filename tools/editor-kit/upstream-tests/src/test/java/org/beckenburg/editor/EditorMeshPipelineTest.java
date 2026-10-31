package org.beckenburg.editor;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;

/** Standalone import, reduction, source-preservation and export smoke proof. */
public final class EditorMeshPipelineTest {
    private static void check(boolean b,String why){if(!b)throw new AssertionError(why);}
    public static void main(String[] args)throws Exception{
        Path root=Files.createTempDirectory("editor-mesh-lab-");
        try{
            Path obj=root.resolve("figure.obj");
            String geometry="""
                o Head
                v 0 1.7 0
                v .2 1.4 0
                v -.2 1.4 0
                v 0 1.4 .2
                f 1 2 4
                f 1 4 3
                f 1 3 2
                f 2 3 4
                o Torso
                v -.6 0 -.25
                v .6 0 -.25
                v .6 1.2 -.25
                v -.6 1.2 -.25
                v -.6 0 .25
                v .6 0 .25
                v .6 1.2 .25
                v -.6 1.2 .25
                f 5 6 7 8
                f 9 10 11 12
                f 5 9 12 8
                f 6 10 11 7
                f 8 7 11 12
                f 5 6 10 9
                """;
            Files.writeString(obj,geometry);
            byte[] unchanged=Files.readAllBytes(obj);
            var model=EditorMeshPipeline.read(obj);
            check(model.triangles().size()==16,"OBJ triangulation");
            check(model.triangles().stream().anyMatch(t->t.part().equals("Head")),"Head identity");
            var exact=EditorMeshPipeline.regress(model,0);
            check(exact.parts().stream().allMatch(p->p.before()==p.after()),"Level zero no loss");
            var face=model.triangles().get(0);
            var repeated=new EditorMeshPipeline.Mesh("two-sided-face",
                java.util.List.of(face,face),10);
            check(EditorMeshPipeline.regress(repeated,0).optimized().triangles().size()==2,
                "Level 0 must not silently deduplicate UV/sidedness-sensitive geometry");
            byte[] glb=EditorGlbWriter.encode(exact.optimized());
            check(ByteBuffer.wrap(glb).order(ByteOrder.LITTLE_ENDIAN).getInt(0)==0x46546c67,
                  "Export GLB magic");
            check(glb.length<=4_000_000,"Game decoder size budget");
            var extreme=EditorMeshPipeline.regress(model,12);
            check(extreme.overRegressed(),"Warn before a head or torso collapses");
            check(extreme.warnings().stream().anyMatch(w->w.contains("OVER-REGRESSION")&&w.contains("Head")),
                  "Head-specific over-regression warning");
            check(Files.readAllBytes(obj).length==unchanged.length &&
                  java.util.Arrays.equals(Files.readAllBytes(obj),unchanged),"Original unchanged");
            Path stl=root.resolve("triangles.stl");
            Files.writeString(stl,"""
                solid Test
                facet normal 0 0 1
                outer loop
                vertex 0 0 0
                vertex 1 0 0
                vertex 0 1 0
                endloop
                endfacet
                endsolid Test
                """);
            check(EditorMeshPipeline.read(stl).triangles().size()==1,"ASCII STL");
            ByteBuffer b=ByteBuffer.allocate(134).order(ByteOrder.LITTLE_ENDIAN);
            b.putInt(80,1);b.putFloat(84+12+12,1);b.putFloat(84+12+24+4,1);
            Path binary=root.resolve("binary.stl");Files.write(binary,b.array());
            check(EditorMeshPipeline.read(binary).triangles().size()==1,"Binary STL");
            boolean refused=false;
            try{EditorMeshPipeline.read(root.resolve("missing.gltf"));}
            catch(java.io.IOException expected){refused=true;}
            check(refused,"Missing/unimplemented formats must fail, not silently import");
            System.out.println("PASS OBJ, ASCII/binary STL, GLB export and head-specific over-regression");
        }finally{try(var walk=Files.walk(root)){
            walk.sorted(java.util.Comparator.reverseOrder()).forEach(p->{
                try{Files.deleteIfExists(p);}catch(Exception ignored){}
            });
        }}
    }
}
