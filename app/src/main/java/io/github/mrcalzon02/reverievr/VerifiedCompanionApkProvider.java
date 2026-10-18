package io.github.mrcalzon02.reverievr;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * Grants Android Package Installer a read-only handle to EXACTLY the private
 * file whose bytes and Android signing certificate were verified. Avoids
 * handing off a different mutable DownloadManager URI after verification.
 */
public final class VerifiedCompanionApkProvider extends ContentProvider {
    static final String AUTHORITY=
        "io.github.mrcalzon02.reverievr.verified-game-apk";
    private static final long MAX_APK=512L*1024L*1024L;
    private static final long RETENTION_MS=48L*60L*60L*1000L;

    private static File folder(Context context)throws IOException{
        File dir=new File(context.getCacheDir(),"verified-companion-apks");
        if(!dir.isDirectory()&&!dir.mkdirs())
            throw new IOException("Cannot stage verified game installer data");
        return dir.getCanonicalFile();
    }
    static File create(Context context)throws IOException{
        File dir=folder(context);
        return File.createTempFile("verified-companion-", ".apk",dir);
    }
    static Uri uriFor(Context context,File file)throws IOException{
        if(context==null||file==null)
            throw new IOException("Missing verified game APK");
        File dir=folder(context),trusted=file.getCanonicalFile();
        if(!dir.equals(trusted.getParentFile()) ||
           !trusted.getName().matches("verified-companion-[A-Za-z0-9-]{6,80}\\.apk") ||
           !trusted.isFile()||trusted.length()<22||trusted.length()>MAX_APK)
            throw new IOException("APK is outside verified installation staging");
        return new Uri.Builder().scheme("content").authority(AUTHORITY)
            .appendPath(trusted.getName()).build();
    }
    static void discardExpired(Context context){
        if(context==null)return;
        try{
            File dir=folder(context);
            File[] items=dir.listFiles();
            if(items==null)return;
            long now=System.currentTimeMillis();
            for(File f:items){
                if(f.isFile()&&!java.nio.file.Files.isSymbolicLink(f.toPath()) &&
                   f.getName().matches("verified-companion-[A-Za-z0-9-]{6,80}\\.apk") &&
                   now-f.lastModified()>RETENTION_MS)f.delete();
            }
        }catch(IOException ignored){
            // Cache cleaning is not allowed to prevent recovery/launch.
        }
    }
    @Override public boolean onCreate(){
        discardExpired(getContext());
        return true;
    }
    @Override public ParcelFileDescriptor openFile(Uri uri,String mode)
        throws FileNotFoundException{
        if(uri==null||!"r".equals(mode)||
           !AUTHORITY.equals(uri.getAuthority())||
           uri.getPathSegments().size()!=1)
            throw new FileNotFoundException("Invalid game APK request");
        try{
            File dir=folder(getContext());
            File f=new File(dir,uri.getLastPathSegment()).getCanonicalFile();
            if(!f.getParentFile().equals(dir) ||
               !f.getName().matches("verified-companion-[A-Za-z0-9-]{6,80}\\.apk") ||
               java.nio.file.Files.isSymbolicLink(f.toPath()) ||
               !f.isFile()||f.length()<22||f.length()>MAX_APK)
                throw new FileNotFoundException("Unverified APK request");
            return ParcelFileDescriptor.open(f,ParcelFileDescriptor.MODE_READ_ONLY);
        }catch(IOException e){throw new FileNotFoundException("APK staging unavailable");}
    }
    @Override public String getType(Uri uri){
        return "application/vnd.android.package-archive";
    }
    @Override public Cursor query(Uri uri,String[] projection,String selection,
                                  String[] args,String sortOrder){return null;}
    @Override public Uri insert(Uri uri,ContentValues values){
        throw new UnsupportedOperationException("Read-only verified game packages");
    }
    @Override public int delete(Uri uri,String selection,String[] args){
        throw new UnsupportedOperationException("Read-only verified game packages");
    }
    @Override public int update(Uri uri,ContentValues values,String selection,
                                String[] args){
        throw new UnsupportedOperationException("Read-only verified game packages");
    }
}
