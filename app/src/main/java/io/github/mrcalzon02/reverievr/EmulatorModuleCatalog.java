package io.github.mrcalzon02.reverievr;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Allowlisted, license-reviewed upstream Libretro source projects.
 * Browsing never installs, imports a ROM, or executes downloaded code.
 */
final class EmulatorModuleCatalog {
    static final class Entry {
        final String id, system, name, repository, license, coreFile;
        final boolean biosMayBeRequired;
        Entry(String id,String system,String name,String repository,
              String license,String coreFile,boolean biosMayBeRequired) {
            this.id=id;this.system=system;this.name=name;
            this.repository=repository;this.license=license;
            this.coreFile=coreFile;this.biosMayBeRequired=biosMayBeRequired;
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
                "GPL-2.0","pcsx_rearmed_libretro_android.so",true)
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
        return entry!=null && entry.license.equals(spdx);
    }
    private EmulatorModuleCatalog(){}
}
