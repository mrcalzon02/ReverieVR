package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.util.Properties;

public final class EmulatorSourceRepositoryTest {
    @Rule public TemporaryFolder temporary=new TemporaryFolder();

    private static String digest(byte[] data)throws Exception{
        byte[] result=MessageDigest.getInstance("SHA-256").digest(data);
        StringBuilder sb=new StringBuilder();
        for(byte b:result)sb.append(String.format(java.util.Locale.US,"%02x",b&0xff));
        return sb.toString();
    }
    @Test public void onlyCompletePinnedAndIntegrityCheckedSourceShowsAsSaved()
        throws Exception {
        File parent=temporary.newFolder("data");
        EmulatorSourceRepository repo=new EmulatorSourceRepository(parent);
        EmulatorModuleCatalog.Entry entry=EmulatorModuleCatalog.find("mgba");
        assertEquals("SOURCE NOT DOWNLOADED",repo.status(entry));
        File dir=new File(parent,"emulator-sources/mgba");
        assertTrue(dir.mkdirs());
        File archive=new File(dir,"upstream.zip");
        byte[] fakeZip=new byte[]{'P','K',3,4,1,2,3,4,5};
        try(FileOutputStream out=new FileOutputStream(archive)){out.write(fakeZip);}
        assertEquals("SOURCE NOT DOWNLOADED",repo.status(entry));
        Properties record=new Properties();
        record.setProperty("id","mgba");
        record.setProperty("repository",entry.repository);
        record.setProperty("license",entry.license);
        record.setProperty("commit","0123456789abcdef0123456789abcdef01234567");
        record.setProperty("archive_sha256",digest(fakeZip));
        record.setProperty("bytes",String.valueOf(fakeZip.length));
        File manifest=new File(dir,"upstream.properties");
        try(FileOutputStream out=new FileOutputStream(manifest)){
            record.store(out,"test");
        }
        assertEquals("SOURCE SAVED - NO PLAYABLE CORE",repo.status(entry));
        try(FileOutputStream out=new FileOutputStream(archive,true)){out.write(42);}
        assertEquals("SOURCE INTEGRITY FAILED",repo.status(entry));
    }
    @Test public void rejectsNonCatalogEntriesBeforeMakingAnyNetworkRequest()
        throws Exception {
        EmulatorSourceRepository repo=new EmulatorSourceRepository(
            temporary.newFolder("storage"));
        try{
            repo.download(null);
            fail("Null module approved.");
        } catch(IOException expected){
            assertTrue(expected.getMessage().contains("Unapproved"));
        }
        EmulatorModuleCatalog.Entry real=EmulatorModuleCatalog.find("mgba");
        EmulatorModuleCatalog.Entry counterfeit=new EmulatorModuleCatalog.Entry(
            real.id,real.system,real.name,"evil/pretend",real.license,real.coreFile,false);
        try{
            repo.download(counterfeit);
            fail("Counterfeit repository approved.");
        }catch(IOException expected){
            assertTrue(expected.getMessage().contains("Unapproved"));
        }
    }
}
