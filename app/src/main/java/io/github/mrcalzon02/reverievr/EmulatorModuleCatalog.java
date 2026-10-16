package io.github.mrcalzon02.reverievr;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Allowlisted, license-reviewed emulator cores and independent game-engine sources.
 * Source browsing never installs or executes the downloaded engine/core.
 */
final class EmulatorModuleCatalog {
    enum HostType { LIBRETRO, OPENRA_ANDROID }
    static final class Entry {
        final String id, system, name, repository, license, coreFile;
        final HostType hostType;
        final String trustedLicenseBlob;
        final boolean biosMayBeRequired;
        Entry(String id,String system,String name,String repository,
              String license,String coreFile,boolean biosMayBeRequired) {
            this(id,system,name,repository,license,coreFile,biosMayBeRequired,null);
        }
        Entry(String id,String system,String name,String repository,
              String license,String coreFile,boolean biosMayBeRequired,
              String trustedLicenseBlob) {
            this(id,system,name,repository,license,coreFile,biosMayBeRequired,
                trustedLicenseBlob,HostType.LIBRETRO);
        }
        Entry(String id,String system,String name,String repository,
              String license,String coreFile,boolean biosMayBeRequired,
              String trustedLicenseBlob,HostType hostType) {
            this.id=id;this.system=system;this.name=name;
            this.repository=repository;this.license=license;
            this.coreFile=coreFile;this.biosMayBeRequired=biosMayBeRequired;
            this.trustedLicenseBlob=trustedLicenseBlob;
            this.hostType=hostType;
            if((hostType==HostType.LIBRETRO)!=(coreFile!=null))
                throw new IllegalArgumentException("Host type and core file disagree");
        }
        String sourceUrl(){return "https://github.com/"+repository;}
        String githubApi(){return "https://api.github.com/repos/"+repository;}
        String sourceArchive(String sha){
            if(!isCommitId(sha))throw new IllegalArgumentException("Unpinned commit");
            return "https://codeload.github.com/"+repository+"/zip/"+sha;
        }
    }
    private static final List<Entry> ENTRIES=Collections.unmodifiableList(
        Arrays.asList(
            new Entry("gambatte","GB / GBC","Gambatte","libretro/gambatte-libretro",
                "GPL-2.0","gambatte_libretro_android.so",false),
            new Entry("mgba","GBA","mGBA","libretro/mgba",
                "MPL-2.0","mgba_libretro_android.so",false),
            new Entry("nestopia","NES","Nestopia UE","libretro/nestopia",
                "GPL-2.0","nestopia_libretro_android.so",false),
            new Entry("melonds","Nintendo DS","melonDS","libretro/melonDS",
                "GPL-3.0","melonds_libretro_android.so",true),
            new Entry("pcsx","PlayStation 1","PCSX ReARMed","libretro/pcsx_rearmed",
                "GPL-2.0","pcsx_rearmed_libretro_android.so",true),
            // Play! is PS2, NOT PS1. Official GitHub cannot infer an SPDX
            // identifier for its BSD-style License.txt. Check that exact
            // license file in the resolved immutable commit before download.
            new Entry("play-ps2","PlayStation 2","Play!","jpd002/Play-",
                "BSD-style (verified)","play_libretro_android.so",false,
                "59e926c6b41eb76c874550d2896ef0d540a785e4"),
            // This is a full Android game engine, not a Libretro core. The
            // current catalog action downloads a pinned source snapshot only.
            // No APK, proprietary game assets, or playable game is bundled.
            new Entry("openra-android","Classic RTS","OpenRA - Red Alert",
                "franktfye/OpenRA-android","GPL-3.0",null,false,null,
                HostType.OPENRA_ANDROID)
        )
    );
    static List<Entry> entries(){return ENTRIES;}
    static Entry find(String id){
        if(id==null)return null;
        for(Entry entry:ENTRIES)if(entry.id.equals(id))return entry;
        return null;
    }
    static boolean isCommitId(String value){
        if(value==null||value.length()!=40)return false;
        for(int i=0;i<40;i++){
            char c=value.charAt(i);
            if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
        }
        return true;
    }
    static boolean approvedLicense(Entry entry,String spdx){
        return entry!=null && entry.trustedLicenseBlob==null &&
            entry.license.equals(spdx);
    }
    static boolean approvedLicenseFile(Entry entry,String spdx,String blobSha){
        return entry!=null && entry.trustedLicenseBlob!=null &&
            "NOASSERTION".equals(spdx) &&
            entry.trustedLicenseBlob.equals(blobSha);
    }
    private EmulatorModuleCatalog(){}
}
