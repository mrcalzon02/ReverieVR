package io.github.mrcalzon02.reverievr;

import org.json.JSONObject;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Locale;
import java.util.Properties;

/**
 * Source snapshot download ONLY: a source archive is not an installed
 * executable core. All requests must originate from the explicit VR confirm.
 */
final class EmulatorSourceRepository {
    private static final long MAX_SOURCE_ZIP=96L*1024L*1024L;
    private static final int TIMEOUT_MS=15000;
    private final File root;
    EmulatorSourceRepository(File filesRoot){
        root=new File(filesRoot,"emulator-sources");
    }
    static final class Result {
        final String message,commit,sha256;
        Result(String message,String commit,String sha256){
            this.message=message;this.commit=commit;this.sha256=sha256;
        }
    }
    String inspectUpstream(EmulatorModuleCatalog.Entry entry)
        throws IOException {
        if(entry==null||EmulatorModuleCatalog.find(entry.id)!=entry)
            throw new IOException("Unapproved emulator.");
        JSONObject remote=getJson(entry.githubApi());
        JSONObject license=remote.optJSONObject("license");
        if(remote.optBoolean("private",true) ||
           remote.optBoolean("archived",true) ||
           !entry.repository.equalsIgnoreCase(remote.optString("full_name")) ||
           license==null ||
           !EmulatorModuleCatalog.approvedLicense(entry,
                                                  license.optString("spdx_id")))
            throw new IOException("Repository or license no longer approved.");
        String locallyStored=status(entry);
        return locallyStored.startsWith("SOURCE SAVED")
            ? locallyStored : "PUBLIC "+entry.license+" - SOURCE AVAILABLE";
    }

    Result download(EmulatorModuleCatalog.Entry entry) throws IOException {
        if(entry==null||EmulatorModuleCatalog.find(entry.id)!=entry)
            throw new IOException("Unapproved emulator source module.");
        JSONObject metadata=getJson(entry.githubApi());
        if(metadata.optBoolean("private",true) || metadata.optBoolean("archived",true))
            throw new IOException("The upstream emulator must remain public and maintained.");
        if(!entry.repository.equalsIgnoreCase(metadata.optString("full_name")))
            throw new IOException("Unexpected GitHub repository identity.");
        JSONObject license=metadata.optJSONObject("license");
        if(license==null ||
           !EmulatorModuleCatalog.approvedLicense(entry,license.optString("spdx_id")))
            throw new IOException("Upstream license changed or could not be verified.");
        final String branch=metadata.optString("default_branch");
        if(!branch.matches("[A-Za-z0-9._-]{1,80}"))
            throw new IOException("Unsafe or unsupported upstream default branch.");
        JSONObject commit=getJson(entry.githubApi()+"/commits/"+branch);
        final String sha=commit.optString("sha");
        if(!EmulatorModuleCatalog.isCommitId(sha))
            throw new IOException("GitHub did not return an immutable commit id.");
        if(!root.isDirectory()&&!root.mkdirs())
            throw new IOException("Could not create the emulator source directory.");
        final File directory=new File(root,entry.id);
        if(!directory.isDirectory()&&!directory.mkdirs())
            throw new IOException("Could not prepare emulator source directory.");
        final File part=new File(directory,"upstream.part");
        final File archive=new File(directory,"upstream.zip");
        final File manifest=new File(directory,"upstream.properties");
        final File manifestPart=new File(directory,"upstream.properties.part");
        part.delete();
        manifestPart.delete();
        try {
            MessageDigest digester=sha256();
            long size=0;
            try(InputStream source=openHttps(entry.sourceArchive(sha));
                FileOutputStream sink=new FileOutputStream(part)){
                byte[] buffer=new byte[16*1024];
                int read;
                while((read=source.read(buffer))!=-1){
                    size+=read;
                    if(size>MAX_SOURCE_ZIP)
                        throw new IOException("Upstream source archive exceeds size limit.");
                    digester.update(buffer,0,read);
                    sink.write(buffer,0,read);
                }
                sink.getFD().sync();
            }
            // Keep upstream ZIP as inert source material: never dlopen, extract,
            // install an APK, import content, or execute arbitrary repo scripts.
            if(size<4 || !zipSignature(part))
                throw new IOException("Upstream source is not a ZIP archive.");
            String digest=hex(digester.digest());
            Properties info=new Properties();
            info.setProperty("id",entry.id);
            info.setProperty("repository",entry.repository);
            info.setProperty("license",entry.license);
            info.setProperty("commit",sha);
            info.setProperty("archive_sha256",digest);
            info.setProperty("bytes",Long.toString(size));
            try(FileOutputStream out=new FileOutputStream(manifestPart)){
                info.store(out,"Source snapshot only; not an executable emulation core");
                out.getFD().sync();
            }
            if(archive.exists()&&!archive.delete())
                throw new IOException("Could not replace old emulator source.");
            if(!part.renameTo(archive))
                throw new IOException("Could not commit emulator source archive.");
            if(manifest.exists()&&!manifest.delete())
                throw new IOException("Could not replace emulator source manifest.");
            if(!manifestPart.renameTo(manifest))
                throw new IOException("Could not commit emulator source metadata.");
            return new Result("SOURCE SAVED - EMULATION HOST NOT YET INSTALLED",
                sha,digest);
        } finally {
            if(part.exists())part.delete();
            if(manifestPart.exists())manifestPart.delete();
        }
    }
    String status(EmulatorModuleCatalog.Entry entry){
        if(entry==null)return "UNKNOWN";
        File d=new File(root,entry.id),a=new File(d,"upstream.zip"),
            m=new File(d,"upstream.properties");
        if(!a.isFile()||!m.isFile())return "SOURCE NOT DOWNLOADED";
        try(FileInputStream in=new FileInputStream(m)){
            Properties p=new Properties();p.load(in);
            if(!entry.id.equals(p.getProperty("id")) ||
               !entry.repository.equals(p.getProperty("repository")) ||
               !entry.license.equals(p.getProperty("license")) ||
               !EmulatorModuleCatalog.isCommitId(p.getProperty("commit")) ||
               !String.valueOf(a.length()).equals(p.getProperty("bytes")) ||
               !p.getProperty("archive_sha256","").equals(sha256File(a)))
                return "SOURCE INTEGRITY FAILED";
            return "SOURCE SAVED - NO PLAYABLE CORE";
        }catch(IOException e){return "SOURCE INTEGRITY FAILED";}
    }
    private static String sha256File(File file)throws IOException{
        MessageDigest digester=sha256();
        try(FileInputStream in=new FileInputStream(file)){
            byte[] b=new byte[16384];int n;
            while((n=in.read(b))!=-1)digester.update(b,0,n);
        }
        return hex(digester.digest());
    }
    private static JSONObject getJson(String address)throws IOException{
        try(InputStream in=openHttps(address)){
            byte[] bytes=readBounded(in,64*1024);
            try {return new JSONObject(new String(bytes,StandardCharsets.UTF_8));}
            catch(Exception e){throw new IOException("Invalid upstream GitHub metadata",e);}
        }
    }
    private static byte[] readBounded(InputStream in,int max)throws IOException{
        java.io.ByteArrayOutputStream buffer=new java.io.ByteArrayOutputStream();
        byte[] b=new byte[4096];int n,total=0;
        while((n=in.read(b))!=-1){
            total+=n;if(total>max)throw new IOException("Metadata too large");
            buffer.write(b,0,n);
        }
        return buffer.toByteArray();
    }
    private static InputStream openHttps(String address)throws IOException {
        URL u=new URL(address);
        if(!"https".equals(u.getProtocol()) ||
            !("api.github.com".equals(u.getHost()) ||
              "codeload.github.com".equals(u.getHost())))
            throw new IOException("Untrusted module download host.");
        HttpURLConnection c=(HttpURLConnection)u.openConnection();
        c.setConnectTimeout(TIMEOUT_MS);c.setReadTimeout(TIMEOUT_MS);
        c.setInstanceFollowRedirects(false);
        c.setRequestProperty("Accept","application/vnd.github+json");
        c.setRequestProperty("User-Agent","ReverieVR-Emulator-Catalog");
        if(c.getResponseCode()!=200) {
            int status=c.getResponseCode();c.disconnect();
            throw new IOException("Upstream download returned HTTP "+status);
        }
        InputStream upstream=c.getInputStream();
        return new java.io.FilterInputStream(upstream){
            @Override public void close()throws IOException{
                try{super.close();}finally{c.disconnect();}
            }
        };
    }
    private static boolean zipSignature(File archive)throws IOException{
        try(FileInputStream in=new FileInputStream(archive)){
            return in.read()==0x50 && in.read()==0x4b
                && in.read()==0x03 && in.read()==0x04;
        }
    }
    private static MessageDigest sha256(){
        try{return MessageDigest.getInstance("SHA-256");}
        catch(NoSuchAlgorithmException e){throw new IllegalStateException(e);}
    }
    private static String hex(byte[] bytes){
        StringBuilder sb=new StringBuilder(bytes.length*2);
        for(byte b:bytes)sb.append(String.format(Locale.US,"%02x",b&0xff));
        return sb.toString();
    }
}
