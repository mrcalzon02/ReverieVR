package io.github.mrcalzon02.reverievr;

import java.io.IOException;
import java.nio.file.Files;

/** Dependency-free, command-line codec acceptance test. */
public final class SceneDocumentTest {
    static void rejects(String value) {
        try { SceneDocument.decode(value); throw new AssertionError("Accepted malformed scene: " + value); }
        catch (IOException expected) { }
    }
    public static void main(String[] args) throws Exception {
        SceneDocument d=new SceneDocument();
        d.environment="forest_glade";
        d.pieces.add(new SceneDocument.Piece("rock-1","box","-",1,2,3,45,1.5));
        d.pieces.add(new SceneDocument.Piece("prop","obj","app/src/main/assets/models/daydream/example.obj",-3,1,2,0,2));
        String encoded=d.encode();
        SceneDocument decoded=SceneDocument.decode(encoded);
        if(!decoded.encode().equals(encoded) || decoded.pieces.size()!=2) throw new AssertionError("Round trip");
        var root=Files.createTempDirectory("rv-desktop-scene-");
        try {
            var file=root.resolve("scenes/test.rvscene");decoded.save(file);
            if(!SceneDocument.load(file).encode().equals(encoded))throw new AssertionError("Disk persistence");
        } finally {
            Files.walk(root).sorted(java.util.Comparator.reverseOrder()).forEach(p -> {try{Files.delete(p);}catch(IOException e){throw new RuntimeException(e);}});
        }
        rejects(encoded.replace("REVSCENE\t1", "REVSCENE\t8"));
        rejects(encoded.replace("forest_glade", "wrong_env"));
        rejects(encoded.replace("rock-1", "prop"));
        rejects(encoded.replace("example.obj", "../../passwords.obj"));
        rejects(encoded.replace("45.0", "NaN"));
        rejects(encoded.replace("\t1.5\n", "\t-1\n"));
        rejects(encoded+"garbage\n");
        if(SceneDocument.safeAsset("C:\\stuff.obj") || SceneDocument.safeAsset("../stuff.obj") || SceneDocument.safeAsset("/etc/passwd.obj"))
            throw new AssertionError("Asset path escape");
        System.out.println("SceneDocument: PASS encode/decode, atomic save, bounded records, invalid inputs");
    }
}
