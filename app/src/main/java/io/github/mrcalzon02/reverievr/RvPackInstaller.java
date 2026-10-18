package io.github.mrcalzon02.reverievr;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HashSet;
import java.util.Properties;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Transactional, data-only game update installer. No downloaded code is run.
 * expectedSha256 MUST originate from a trusted, signature-verified catalog.
 * This class deliberately has no Android dependency for JVM regression tests.
 */
final class RvPackInstaller {
    static final long MAX_ARCHIVE = 64L * 1024L * 1024L;
    static final long MAX_EXPANDED = 96L * 1024L * 1024L;
    static final long MAX_ENTRY = 4L * 1024L * 1024L;
    static final int MAX_ENTRIES = 512;
    private final File root;

    RvPackInstaller(File filesRoot) {
        if (filesRoot == null) throw new IllegalArgumentException("Missing root");
        this.root = new File(filesRoot,"module-packs");
    }

    static String namespace(String id) {
        GameModuleRegistry.Entry entry=GameModuleRegistry.find(id);
        return entry==null?null:entry.contentNamespace;
    }

    static boolean allowedEntry(String id, String name, boolean directory) {
        String ns=namespace(id);
        if(ns==null||name==null||name.isEmpty()||name.length()>160 ||
           name.startsWith("/")||name.contains("\\")||name.contains("..") ||
           name.contains("//")||name.indexOf(':')>=0 ||
           !name.matches("[A-Za-z0-9_.\\-/]+")) return false;
        if(directory && name.endsWith("/"))name=name.substring(0,name.length()-1);
        if("module.json".equals(name))return !directory;
        if(name.startsWith("licenses/"))
            return (directory || name.endsWith(".json") || name.endsWith(".txt"));
        String p="assets/native/"+ns+"/";
        if (!name.startsWith(p)) return false;
        if(directory)return true;
        return name.endsWith(".rlprop")||name.endsWith(".png")||
            name.endsWith(".ogg")||name.endsWith(".wav")||
            name.endsWith(".mp3")||name.endsWith(".json")||
            name.endsWith(".rvdata")||name.endsWith(".atlas");
    }

    static String digest(File file) throws IOException {
        try {
            MessageDigest md=MessageDigest.getInstance("SHA-256");
            try(InputStream in=new FileInputStream(file)){
                byte[] buf=new byte[16384];int n;
                while((n=in.read(buf))!=-1)md.update(buf,0,n);
            }
            byte[] b=md.digest();StringBuilder hex=new StringBuilder(64);
            for(byte v:b)hex.append(String.format(java.util.Locale.ROOT,"%02x",v&255));
            return hex.toString();
        }catch(NoSuchAlgorithmException e){throw new IOException(e);}
    }

    synchronized File install(File pack,String id,long revision,
                              String expectedSha256)throws IOException {
        String ns=namespace(id);
        if(ns==null||revision<=0||revision>Integer.MAX_VALUE ||
           expectedSha256==null||!expectedSha256.matches("[0-9a-f]{64}"))
            throw new IOException("Invalid signed release descriptor");
        if(pack==null||!pack.isFile()||pack.length()<22||pack.length()>MAX_ARCHIVE)
            throw new IOException("Invalid content archive size");
        if(!MessageDigest.isEqual(digest(pack).getBytes(StandardCharsets.US_ASCII),
             expectedSha256.getBytes(StandardCharsets.US_ASCII)))
            throw new IOException("Content archive SHA-256 mismatch");
        File mod=new File(root,id),releases=new File(mod,"releases");
        if(!releases.isDirectory()&&!releases.mkdirs())
            throw new IOException("Cannot create module release directory");
        String ver=Long.toString(revision);
        File finalDir=new File(releases,ver);
        File stage=new File(releases,".pending-"+ver);
        if(stage.exists())deleteTree(stage);
        if(!stage.mkdirs())throw new IOException("Cannot create staging directory");
        int count=0;long total=0;boolean manifest=false;
        int payloadCount=0;
        Set<String> names=new HashSet<>();
        try {
            try(ZipInputStream in=new ZipInputStream(new FileInputStream(pack))){
                ZipEntry entry;byte[] buf=new byte[16384];
                while((entry=in.getNextEntry())!=null){
                    String name=entry.getName();
                    if(++count>MAX_ENTRIES || !allowedEntry(id,name,entry.isDirectory()) ||
                       !names.add(name))
                        throw new IOException("Rejected ZIP entry: "+name);
                    File target=new File(stage,name).getCanonicalFile();
                    if(!target.getPath().startsWith(stage.getCanonicalPath()+File.separator))
                        throw new IOException("ZIP traversal");
                    if(entry.isDirectory()){
                        if(!target.isDirectory()&&!target.mkdirs())
                            throw new IOException("Cannot create ZIP folder");
                    }else{
                        File parent=target.getParentFile();
                        if(!parent.isDirectory()&&!parent.mkdirs())
                            throw new IOException("Cannot create entry folder");
                        long fileBytes=0;
                        try(FileOutputStream out=new FileOutputStream(target)){
                            int n;
                            while((n=in.read(buf))!=-1){
                                fileBytes+=n;total+=n;
                                if(fileBytes>MAX_ENTRY||total>MAX_EXPANDED)
                                    throw new IOException("Uncompressed content limit");
                                out.write(buf,0,n);
                            }
                            out.getFD().sync();
                        }
                        if("module.json".equals(name)){
                            manifest=true;
                            if(fileBytes>16384)throw new IOException("Oversized manifest");
                        }
                        // Validate actual game mesh bytes before activation,
                        // not merely the archive filename or hash.
                        RvPackAssetValidator.verify(name,target);
                        if(name.startsWith("assets/"))payloadCount++;
                    }
                    in.closeEntry();
                }
            }
            if(!manifest || payloadCount==0)
                throw new IOException("Missing actual game content assets");
            // A signed archive hash alone does not establish which game or
            // revision the contained manifest actually declares. Bind both
            // identities BEFORE publishing or changing the active pointer.
            RvPackManifest metadata=RvPackManifest.parse(
                Files.readAllBytes(new File(stage,"module.json").toPath()));
            if(!id.equals(metadata.id) || revision!=metadata.revision)
                throw new IOException("Package module identity/revision mismatch");
            // Zip contents are bound to a signed catalog by the checked pack hash.
            Properties receipt=new Properties();
            receipt.setProperty("id",id);
            receipt.setProperty("revision",ver);
            receipt.setProperty("sha256",expectedSha256);
            receipt.setProperty("entries",Integer.toString(count));
            writeProperties(new File(stage,"receipt.properties"),receipt);
            if(finalDir.exists()){
                Properties old=readProperties(new File(finalDir,"receipt.properties"));
                if(!expectedSha256.equals(old.getProperty("sha256")))
                    throw new IOException("Revision collision; refusing replacement");
                deleteTree(stage);
            }else if(!stage.renameTo(finalDir))
                throw new IOException("Cannot publish verified content directory");
            File active=new File(mod,"active.properties");
            File previous=new File(mod,"previous.properties");
            if(active.isFile()){
                Properties prior=readProperties(active);
                if(validInstalled(mod,prior))writePropertiesAtomically(previous,prior);
            }
            writePropertiesAtomically(active,receipt);
            return new File(finalDir,"assets");
        } finally {
            if(stage.exists())deleteTree(stage);
        }
    }

    synchronized File activeAssets(String id){
        if(namespace(id)==null)return null;
        File mod=new File(root,id);
        try {
            Properties p=readProperties(new File(mod,"active.properties"));
            if(!validInstalled(mod,p))return null;
            return new File(new File(new File(mod,"releases"),
                p.getProperty("revision")),"assets");
        }catch(IOException e){return null;}
    }

    synchronized boolean rollback(String id)throws IOException{
        if(namespace(id)==null)return false;
        File mod=new File(root,id);
        File prev=new File(mod,"previous.properties");
        if(!prev.isFile())return false;
        Properties p=readProperties(prev);
        if(!validInstalled(mod,p))return false;
        Properties now=readProperties(new File(mod,"active.properties"));
        writePropertiesAtomically(new File(mod,"active.properties"),p);
        if(validInstalled(mod,now))writePropertiesAtomically(prev,now);
        return true;
    }

    /**
     * Remove optional installed content only. Native saves are in the
     * separate native-modules directory and must never be touched here.
     * Caller must ensure this game's session is stopped.
     */
    synchronized boolean removeContent(String id)throws IOException{
        if(namespace(id)==null)throw new IOException("Unknown module");
        File mod=new File(root,id);
        if(!mod.exists())return false;
        if(!mod.getCanonicalFile().getParentFile().equals(root.getCanonicalFile()))
            throw new IOException("Invalid content directory");
        deleteTree(mod);
        return true;
    }

    synchronized long installedBytes(String id){
        if(namespace(id)==null)return 0L;
        File mod=new File(root,id);
        return sizeOf(mod);
    }
    private static long sizeOf(File root){
        if(java.nio.file.Files.isSymbolicLink(root.toPath()))return 0L;
        if(root.isFile())return root.length();
        if(!root.isDirectory())return 0L;
        long total=0;
        File[] files=root.listFiles();
        if(files==null)return 0L;
        for(File f:files) {
            long size=sizeOf(f);
            if(size<0||total>Long.MAX_VALUE-size)return Long.MAX_VALUE;
            total+=size;
        }
        return total;
    }

    private static boolean validInstalled(File mod,Properties p){
        String id=p.getProperty("id"),rev=p.getProperty("revision"),
            sha=p.getProperty("sha256");
        if(!mod.getName().equals(id)||rev==null||
           !rev.matches("[1-9][0-9]{0,9}")||
           sha==null||!sha.matches("[a-f0-9]{64}"))return false;
        File dir=new File(new File(mod,"releases"),rev);
        File receipt=new File(dir,"receipt.properties");
        if(!receipt.isFile()||!new File(dir,"module.json").isFile())return false;
        try{
            Properties loaded=readProperties(receipt);
            return id.equals(loaded.getProperty("id")) &&
                rev.equals(loaded.getProperty("revision")) &&
                sha.equals(loaded.getProperty("sha256"));
        }catch(IOException e){return false;}
    }
    private static Properties readProperties(File file)throws IOException{
        Properties p=new Properties();
        try(FileInputStream in=new FileInputStream(file)){p.load(in);}
        return p;
    }
    private static void writeProperties(File f,Properties p)throws IOException{
        try(FileOutputStream out=new FileOutputStream(f)){
            p.store(out,"Verified game-content revision");out.getFD().sync();
        }
    }
    private static void writePropertiesAtomically(File dest,Properties p)
        throws IOException {
        File part=new File(dest.getParentFile(),dest.getName()+".partial");
        writeProperties(part,p);
        try {
            Files.move(part.toPath(),dest.toPath(),StandardCopyOption.ATOMIC_MOVE,
                       StandardCopyOption.REPLACE_EXISTING);
        }catch(AtomicMoveNotSupportedException e){
            part.delete();
            throw new IOException("Atomic activation unavailable",e);
        }
    }
    private static void deleteTree(File f)throws IOException{
        if(Files.isSymbolicLink(f.toPath()))throw new IOException("Symlink in content staging");
        if(f.isDirectory()){
            File[] children=f.listFiles();
            if(children==null)throw new IOException("Cannot inspect staging");
            for(File child:children)deleteTree(child);
        }
        if(!f.delete())throw new IOException("Cannot remove staged file");
    }
}
