package io.github.mrcalzon02.reverievr;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.Binder;
import android.os.ParcelFileDescriptor;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.List;

/**
 * Signature-permission protected read-only bridge for a one-time migration
 * from hub-owned native saves to a separately installed companion game APK.
 * The original player data is never moved or deleted by this provider.
 */
public final class GameSaveMigrationProvider extends ContentProvider {
    static final String AUTHORITY=
        "io.github.mrcalzon02.reverievr.game-saves";
    static final String[] COLUMNS={"slot","bytes","sha256"};
    private static final int MAX_SLOTS=128;

    @Override public boolean onCreate(){return true;}

    private String allowedGame(Uri uri){
        if(uri==null||getContext()==null)return null;
        List<String> parts=uri.getPathSegments();
        if(parts.size()<2||parts.size()>3||
           !"saves".equals(parts.get(0)))return null;
        String id=parts.get(1);
        CompanionGameRegistry.Descriptor expected=CompanionGameRegistry.find(id);
        if(expected==null)return null;
        String pkg=getCallingPackage();
        if(!expected.packageName.equals(pkg) ||
           CompanionGameRegistry.installed(getContext(),id)==null)return null;
        String[] uidPackages=getContext().getPackageManager().getPackagesForUid(
            Binder.getCallingUid());
        boolean uidMatches=false;
        if(uidPackages!=null)
            for(String name:uidPackages)
                if(expected.packageName.equals(name))uidMatches=true;
        return uidMatches?id:null;
    }

    private static boolean safeSlot(String slot){
        return slot!=null&&slot.length()>0&&slot.length()<=64&&
            !slot.equals(".")&&!slot.equals("..")&&
            slot.matches("[A-Za-z0-9_.-]+")&&!slot.contains("..");
    }
    private File fileFor(String id,String slot) throws IOException{
        if(!safeSlot(slot))throw new IOException("Invalid save slot");
        File dir=new File(new File(getContext().getFilesDir(),"native-modules"),id);
        File file=new File(dir,slot);
        if(!file.getCanonicalPath().startsWith(dir.getCanonicalPath()+
               File.separator) || java.nio.file.Files.isSymbolicLink(file.toPath()))
            throw new IOException("Invalid save path");
        return file;
    }

    @Override public Cursor query(Uri uri,String[] projection,
                                  String selection,String[] selectionArgs,
                                  String sortOrder){
        String id=allowedGame(uri);
        if(id==null||uri.getPathSegments().size()!=2)
            throw new SecurityException("Unknown or untrusted companion");
        MatrixCursor result=new MatrixCursor(COLUMNS);
        File dir=new File(new File(getContext().getFilesDir(),"native-modules"),id);
        File[] files=dir.listFiles();
        if(files==null)return result;
        int count=0;
        for(File file:files){
            if(++count>MAX_SLOTS)break;
            if(!file.isFile()||!safeSlot(file.getName())||
               file.length()<0||file.length()>65536)continue;
            try{
                File readable=fileFor(id,file.getName());
                result.addRow(new Object[]{
                    readable.getName(),readable.length(),
                    RvPackInstaller.digest(readable)
                });
            }catch(IOException ignored){}
        }
        return result;
    }

    @Override public ParcelFileDescriptor openFile(Uri uri,String mode)
        throws FileNotFoundException {
        String id=allowedGame(uri);
        if(id==null||!"r".equals(mode)||uri.getPathSegments().size()!=3)
            throw new FileNotFoundException("Save export not authorized");
        try{
            File file=fileFor(id,uri.getLastPathSegment());
            if(!file.isFile()||file.length()>65536)
                throw new FileNotFoundException("Save not found");
            return ParcelFileDescriptor.open(file,ParcelFileDescriptor.MODE_READ_ONLY);
        }catch(IOException e){
            throw new FileNotFoundException("Rejected save slot");
        }
    }

    @Override public String getType(Uri uri){
        return uri.getPathSegments().size()==2?
            "vnd.android.cursor.dir/vnd.reverievr.game-save":
            "application/octet-stream";
    }
    @Override public Uri insert(Uri uri,ContentValues values){
        throw new UnsupportedOperationException("Read-only save export");
    }
    @Override public int delete(Uri uri,String selection,String[] args){
        throw new UnsupportedOperationException("Read-only save export");
    }
    @Override public int update(Uri uri,ContentValues values,
                                String selection,String[] args){
        throw new UnsupportedOperationException("Read-only save export");
    }
}
