package io.github.mrcalzon02.reverievr;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.IOException;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.security.PublicKey;
import java.security.Signature;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Authenticated per-game update metadata, independent of the hub APK feed.
 * A Release cannot be created from an arbitrary unverified HTTP response.
 */
final class GameModuleCatalog {
    static final int MAX_CATALOG_BYTES=256*1024;
    static final String APPROVED_RELEASE_PREFIX=
        "/mrcalzon02/ReverieVR/releases/download/";
    static final String APPROVED_CATALOG_BASE=
        "https://raw.githubusercontent.com/mrcalzon02/ReverieVR/main/modules/";

    static final class Release {
        final String id,version,kind,url,sha256,notes,channel,minHub;
        final long revision,byteCount;
        final int minAndroidSdk,nativeAbiMin,nativeAbiMax,requiredGlesMajor;
        final String[] supportedAbis;
        Release(String id,String version,String kind,String url,String sha256,
                String notes,String channel,long revision,long byteCount,
                String minHub,int minSdk,int abiMin,int abiMax,int gles,
                String[] abis){
            this.id=id;this.version=version;this.kind=kind;this.url=url;
            this.sha256=sha256;this.notes=notes;this.channel=channel;
            this.revision=revision;this.byteCount=byteCount;
            this.minHub=minHub;this.minAndroidSdk=minSdk;
            this.nativeAbiMin=abiMin;this.nativeAbiMax=abiMax;
            this.requiredGlesMajor=gles;this.supportedAbis=abis.clone();
        }
    }
    static final class Verified {
        final long catalogRevision;
        final List<Release> releases;
        Verified(long revision,List<Release> entries){
            catalogRevision=revision;
            releases=Collections.unmodifiableList(entries);
        }
        Release find(String id,String kind,String channel){
            Release best=null;
            for(Release r:releases)if(r.id.equals(id) && r.kind.equals(kind)
                && r.channel.equals(channel) &&
                (best==null||r.revision>best.revision))best=r;
            return best;
        }
    }
    static Verified verify(byte[] raw,byte[] signed,PublicKey key,long minimumRevision)
        throws IOException {
        if(raw==null||raw.length==0||raw.length>MAX_CATALOG_BYTES ||
           signed==null||signed.length<48||signed.length>128||key==null)
            throw new IOException("Signed catalog unavailable");
        try{
            Signature verifier=Signature.getInstance("SHA256withECDSA");
            verifier.initVerify(key);
            verifier.update(raw);
            if(!verifier.verify(signed))
                throw new IOException("Rejected publisher catalog signature");
            JSONObject root=new JSONObject(new String(raw,StandardCharsets.UTF_8));
            if(root.getInt("schema")!=1)throw new IOException("Unknown catalog schema");
            long revision=root.getLong("catalog_revision");
            if(revision<=0||revision<minimumRevision)
                throw new IOException("Rejected rolled-back catalog");
            JSONArray modules=root.getJSONArray("modules");
            if(modules.length()>128)throw new IOException("Excessive module entries");
            ArrayList<Release> all=new ArrayList<>();
            Set<String> keys=new HashSet<>();
            for(int i=0;i<modules.length();i++){
                JSONObject v=modules.getJSONObject(i);
                String id=v.getString("id");
                if(RvPackInstaller.namespace(id)==null)
                    throw new IOException("Unknown game id: "+id);
                String kind=v.getString("kind");
                if(!"content".equals(kind)&&!"companion-apk".equals(kind))
                    throw new IOException("Unsupported release type");
                String channel=v.getString("channel");
                if(!"stable".equals(channel)&&!"testing".equals(channel)&&
                   !"development".equals(channel))
                    throw new IOException("Invalid release channel");
                String version=v.getString("version");
                if(!version.matches("[0-9]+\\.[0-9]+\\.[0-9]+(-[a-z0-9.-]+)?"))
                    throw new IOException("Invalid semantic version");
                long rel=v.getLong("revision");
                long size=v.getLong("size_bytes");
                if(rel<1||rel>Integer.MAX_VALUE||size<22||
                   size>RvPackInstaller.MAX_ARCHIVE && "content".equals(kind))
                    throw new IOException("Unreasonable module package size");
                if("companion-apk".equals(kind)&&size>512L*1024L*1024L)
                    throw new IOException("Oversized companion APK");
                String url=v.getString("url");
                if(!approvedReleaseUrl(url,kind))throw new IOException("Untrusted release URL");
                String sha=v.getString("sha256");
                if(!sha.matches("[0-9a-f]{64}"))throw new IOException("Invalid SHA-256");
                String notes=v.optString("notes","");
                if(notes.length()>2048)throw new IOException("Oversized notes");
                String minHub=v.getString("min_hub");
                if(!minHub.matches("[0-9]+\\.[0-9]+\\.[0-9]+(-[a-z0-9.-]+)?"))
                    throw new IOException("Invalid minimum hub version");
                int minSdk=v.getInt("min_android_sdk");
                int abiMin=v.getInt("native_abi_min");
                int abiMax=v.getInt("native_abi_max");
                int gles=v.getInt("gles_major");
                if(minSdk<26||minSdk>100||abiMin<1||abiMax<abiMin||
                   abiMax>32||gles<2||gles>4)
                    throw new IOException("Invalid native device compatibility");
                JSONArray supported=v.getJSONArray("abis");
                if(supported.length()==0||supported.length()>2)
                    throw new IOException("Empty or excessive device ABI list");
                Set<String> seenAbis=new HashSet<>();
                for(int j=0;j<supported.length();j++){
                    String abi=supported.getString(j);
                    if((!"armeabi-v7a".equals(abi)&&!"arm64-v8a".equals(abi))||
                       !seenAbis.add(abi))
                        throw new IOException("Unknown/duplicate native device ABI");
                }
                String identity=id+"/"+kind+"/"+channel+"/"+rel;
                if(!keys.add(identity))throw new IOException("Duplicated module revision");
                all.add(new Release(id,version,kind,url,sha,notes,channel,rel,size,
                    minHub,minSdk,abiMin,abiMax,gles,
                    seenAbis.toArray(new String[0])));
            }
            return new Verified(revision,all);
        }catch(IOException e){throw e;}
        catch(Exception e){throw new IOException("Malformed module catalog",e);}
    }
    static boolean compatibleWith(Release r,String hubVersion,int sdk,
                                  int abiVersion,int glesMajor,String[] deviceAbis){
        if(r==null || hubVersion==null || deviceAbis==null ||
           VersionUtils.isNewer(r.minHub,hubVersion) ||
           sdk<r.minAndroidSdk ||
           abiVersion<r.nativeAbiMin||abiVersion>r.nativeAbiMax ||
           glesMajor<r.requiredGlesMajor)return false;
        for(String supported:r.supportedAbis)
            for(String available:deviceAbis)
                if(supported.equals(available))return true;
        return false;
    }
    static boolean compatibleWithThisDevice(Release r){
        return compatibleWith(r,BuildConfig.VERSION_NAME,
            android.os.Build.VERSION.SDK_INT,1,2,android.os.Build.SUPPORTED_ABIS);
    }
    static boolean approvedReleaseUrl(String value,String kind) {
        if(value==null||value.length()>1024)return false;
        try{
            URI u=URI.create(value);
            if(!"https".equals(u.getScheme()) ||
               !"github.com".equalsIgnoreCase(u.getHost()) ||
               u.getPort()!=-1 || u.getUserInfo()!=null ||
               u.getRawQuery()!=null || u.getRawFragment()!=null)return false;
            String path=u.getPath();
            if(path==null||!path.startsWith(APPROVED_RELEASE_PREFIX) ||
               path.contains("..") || path.contains("//"))return false;
            String remaining=path.substring(APPROVED_RELEASE_PREFIX.length());
            int slash=remaining.indexOf('/');
            if(slash<=0||slash==remaining.length()-1||remaining.indexOf('/',slash+1)!=-1)
                return false;
            return "content".equals(kind)?
                remaining.endsWith(".rvpack"):remaining.endsWith(".apk");
        }catch(IllegalArgumentException e){return false;}
    }
}
