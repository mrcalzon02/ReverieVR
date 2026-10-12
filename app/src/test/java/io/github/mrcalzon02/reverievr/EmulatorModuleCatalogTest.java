package io.github.mrcalzon02.reverievr;
import org.junit.Test;
import static org.junit.Assert.*;
public final class EmulatorModuleCatalogTest {
    @Test public void coversGbaToPlaystationWithoutPaidOrUnknownLicenses(){
        assertNotNull(EmulatorModuleCatalog.find("mgba"));
        assertNotNull(EmulatorModuleCatalog.find("pcsx"));
        assertEquals(6,EmulatorModuleCatalog.entries().size());
        for(EmulatorModuleCatalog.Entry e:EmulatorModuleCatalog.entries()){
            assertTrue(e.sourceUrl().startsWith("https://github.com/"));
            assertTrue(e.githubApi().startsWith("https://api.github.com/repos/"));
            if(e.trustedLicenseBlob==null) {
                assertTrue(EmulatorModuleCatalog.approvedLicense(e,e.license));
            } else {
                assertFalse(EmulatorModuleCatalog.approvedLicense(e,e.license));
                assertTrue(EmulatorModuleCatalog.approvedLicenseFile(
                    e,"NOASSERTION",e.trustedLicenseBlob));
                assertFalse(EmulatorModuleCatalog.approvedLicenseFile(
                    e,"NOASSERTION","0000000000000000000000000000000000000000"));
                assertFalse(EmulatorModuleCatalog.approvedLicenseFile(
                    e,"MIT",e.trustedLicenseBlob));
            }
            assertFalse(EmulatorModuleCatalog.approvedLicense(e,"NOASSERTION"));
            assertNull(EmulatorModuleCatalog.find("unknown-emu"));
        }
        EmulatorModuleCatalog.Entry play=EmulatorModuleCatalog.find("play-ps2");
        assertNotNull(play);
        assertEquals("PlayStation 2",play.system);
        assertEquals("Play!",play.name);
        assertEquals("jpd002/Play-",play.repository);
        assertFalse(play.biosMayBeRequired);
        assertEquals("play_libretro_android.so",play.coreFile);
        assertFalse(EmulatorModuleCatalog.approvedLicenseFile(
            EmulatorModuleCatalog.find("pcsx"),"NOASSERTION",play.trustedLicenseBlob));
    }
    @Test public void sourceZipRequiresImmutableCommitNotBranchOrUserUrl(){
        EmulatorModuleCatalog.Entry e=EmulatorModuleCatalog.find("mgba");
        String sha="0123456789abcdef0123456789abcdef01234567";
        assertTrue(EmulatorModuleCatalog.isCommitId(sha));
        assertEquals("https://codeload.github.com/libretro/mgba/zip/"+sha,
                     e.sourceArchive(sha));
        for(String invalid:new String[]{"main","../latest",
             "https://evil.com/a","0123456789ABCDEF0123456789ABCDEF01234567"}) {
            assertFalse(EmulatorModuleCatalog.isCommitId(invalid));
            try{e.sourceArchive(invalid);fail("Accepted movable or unsafe ref");}
            catch(IllegalArgumentException expected){}
        }
    }
}
