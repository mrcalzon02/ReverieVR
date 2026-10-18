package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public class RvPackInstallerTest {
    private static final String ID="between-deliveries-red-ledger";
    private static File zip(File root,String name,String path,long revision) throws Exception {
        File file=new File(root,name);
        try(ZipOutputStream out=new ZipOutputStream(new FileOutputStream(file))){
            out.putNextEntry(new ZipEntry("module.json"));
            out.write(("{\"schema\":1,\"kind\":\"content\",\"id\":\""+ID+
                "\",\"version\":\"0.1.0\",\"revision\":"+revision+"}")
                .getBytes(StandardCharsets.UTF_8));
            out.closeEntry();
            out.putNextEntry(new ZipEntry(path));
            out.write(new byte[]{'R','L','B','P',1,0,0,0});
            out.closeEntry();
        }
        return file;
    }
    @Test public void installsAndRollsBackIndependentGameRevision() throws Exception{
        File dir=Files.createTempDirectory("rvpack-test").toFile();
        RvPackInstaller service=new RvPackInstaller(dir);
        File first=zip(dir,"a.rvpack","assets/native/red-ledger/beckenburg/wall.rlprop",1);
        File a=service.install(first,ID,1,RvPackInstaller.digest(first));
        assertTrue(new File(a,"native/red-ledger/beckenburg/wall.rlprop").isFile());
        assertEquals(a.getCanonicalFile(),service.activeAssets(ID).getCanonicalFile());
        File second=zip(dir,"b.rvpack","assets/native/red-ledger/beckenburg/chair.rlprop",2);
        File b=service.install(second,ID,2,RvPackInstaller.digest(second));
        assertEquals(b.getCanonicalFile(),service.activeAssets(ID).getCanonicalFile());
        assertTrue(service.rollback(ID));
        assertEquals(a.getCanonicalFile(),service.activeAssets(ID).getCanonicalFile());
        assertNull(service.activeAssets("omega-front"));
    }
    @Test public void badDigestDoesNotActivate() throws Exception {
        File dir=Files.createTempDirectory("rvpack-bad-hash").toFile();
        File pack=zip(dir,"bad.rvpack","assets/native/red-ledger/beckenburg/wall.rlprop",1);
        try{
            new RvPackInstaller(dir).install(pack,ID,1,
                "0000000000000000000000000000000000000000000000000000000000000000");
            fail("Digest mismatch accepted");
        }catch(IOException expected){assertTrue(expected.getMessage().contains("SHA-256"));}
        assertNull(new RvPackInstaller(dir).activeAssets(ID));
    }
    @Test public void rejectsTraversalAndCrossGameFiles() throws Exception {
        assertFalse(RvPackInstaller.allowedEntry(ID,"assets/native/red-ledger/../other.png",false));
        assertFalse(RvPackInstaller.allowedEntry(ID,"assets/native/omega-front/tank.png",false));
        assertFalse(RvPackInstaller.allowedEntry(ID,"assets/native/red-ledger/payload.so",false));
        assertFalse(RvPackInstaller.allowedEntry(ID,"assets/native/red-ledger/a.jar",false));
        assertTrue(RvPackInstaller.allowedEntry(ID,"assets/native/red-ledger/beckenburg/door.rlprop",false));
        File dir=Files.createTempDirectory("rvpack-traversal").toFile();
        File pack=zip(dir,"evil.rvpack","assets/native/omega-front/tank.png",1);
        try{
            new RvPackInstaller(dir).install(pack,ID,1,RvPackInstaller.digest(pack));
            fail("Cross game asset accepted");
        }catch(IOException expected){assertTrue(expected.getMessage().contains("Rejected ZIP"));}
    }
    @Test public void rejectsPackWithDifferentDeclaredRevision() throws Exception {
        File dir=Files.createTempDirectory("rvpack-identity").toFile();
        File first=zip(dir,"wrong.rev.rvpack",
            "assets/native/red-ledger/beckenburg/wall.rlprop",2);
        RvPackInstaller store=new RvPackInstaller(dir);
        try{
            store.install(first,ID,1,RvPackInstaller.digest(first));
            fail("Mismatched release revision accepted");
        }catch(IOException expected){
            assertTrue(expected.getMessage().contains("identity/revision"));
        }
        assertNull(store.activeAssets(ID));
    }
    @Test public void rejectsModuleIdAndRevisionCollision() throws Exception {
        File dir=Files.createTempDirectory("rvpack-collision").toFile();
        File first=zip(dir,"a.rvpack","assets/native/red-ledger/beckenburg/wall.rlprop",1);
        RvPackInstaller store=new RvPackInstaller(dir);
        store.install(first,ID,1,RvPackInstaller.digest(first));
        File second=zip(dir,"b.rvpack","assets/native/red-ledger/beckenburg/chair.rlprop",1);
        try{
            store.install(second,ID,1,RvPackInstaller.digest(second));
            fail("Replacing immutable revision accepted");
        }catch(IOException expected){assertTrue(expected.getMessage().contains("collision"));}
        assertNotNull(store.activeAssets(ID));
        try{
            store.install(first,"unapproved-game",1,RvPackInstaller.digest(first));
            fail("Unapproved game accepted");
        }catch(IOException expected){assertTrue(expected.getMessage().contains("descriptor"));}
    }
}
