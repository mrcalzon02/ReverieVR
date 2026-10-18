package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Looper;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URI;
import java.net.URL;
import java.security.KeyFactory;
import java.security.PublicKey;
import java.security.spec.X509EncodedKeySpec;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Game-specific signed catalog and content update client. This intentionally
 * does not use UpdateInstaller or request installation of native executable
 * files. Companion APK updates require a separate Android PackageInstaller.
 */
final class GameModuleUpdateService implements AutoCloseable {
    interface Callback<T> {void complete(T result,String error);}
    interface Progress {void onBytes(long read,long total);}
    private final Context context;
    private final Handler ui=new Handler(Looper.getMainLooper());
    private final ExecutorService executor=Executors.newSingleThreadExecutor();
    private final RvPackInstaller installer;
    private final SharedPreferences revisions;
    private final PublicKey trustedKey;
    private volatile GameModuleCatalog.Verified verified;

    GameModuleUpdateService(Context context) {
        this.context=context.getApplicationContext();
        this.installer=new RvPackInstaller(this.context.getFilesDir());
        this.revisions=this.context.getSharedPreferences(
            "game-module-revisions",Context.MODE_PRIVATE);
        PublicKey key=null;
        try(InputStream in=this.context.getAssets().open(
            "native/modules/catalog-key.der")){
            byte[] der=bounded(in,2048);
            key=KeyFactory.getInstance("EC").generatePublic(
                new X509EncodedKeySpec(der));
        }catch(Exception e){
            // No trusted key means no online game release may be installed.
            ReverieLog.incident("GAME_MODULES",
                "Publisher catalog key not configured: "+e.getClass().getSimpleName());
        }
        trustedKey=key;
    }

    void check(Callback<GameModuleCatalog.Verified> callback) {
        executor.execute(()->{
            GameModuleCatalog.Verified value=null;String error=null;
            try{
                if(trustedKey==null)throw new IOException("Publisher key not configured");
                byte[] json=fetchCatalog("catalog-v1.json",256*1024);
                byte[] sig=fetchCatalog("catalog-v1.sig",256);
                long minimum=revisions.getLong("catalog_revision",0L);
                value=GameModuleCatalog.verify(json,sig,trustedKey,minimum);
                if(!revisions.edit().putLong("catalog_revision",
                    value.catalogRevision).commit())
                    throw new IOException("Could not persist trusted revision");
                verified=value;
            }catch(Exception e){error=e.getMessage();}
            final GameModuleCatalog.Verified answer=value;
            final String message=error;
            ui.post(()->callback.complete(answer,message));
        });
    }

    void install(GameModuleCatalog.Release release,Callback<File> callback,
                 Progress progress){
        executor.execute(()->{
            File installed=null;String error=null;
            try {
                if(release==null||!"content".equals(release.kind) ||
                   verified==null || !verified.releases.contains(release))
                    throw new IOException("Unverified content release");
                if(!GameModuleCatalog.compatibleWithThisDevice(release))
                    throw new IOException("Content requires a different hub/device/ABI");
                if(!GameModuleCatalog.approvedReleaseUrl(release.url,"content"))
                    throw new IOException("Untrusted game release URL");
                File downloads=new File(context.getFilesDir(),"game-module-downloads");
                if(!downloads.isDirectory()&&!downloads.mkdirs())
                    throw new IOException("Cannot create download directory");
                File partial=new File(downloads,
                    release.id+"-"+release.revision+".partial");
                try {
                    download(release,partial,progress);
                    installed=installer.install(partial,release.id,
                        release.revision,release.sha256);
                }finally{if(partial.exists())partial.delete();}
            }catch(Exception e){error=e.getMessage();}
            final File location=installed;
            final String message=error;
            ui.post(()->callback.complete(location,message));
        });
    }

    File activeAssets(String moduleId){return installer.activeAssets(moduleId);}
    boolean rollback(String moduleId)throws IOException{return installer.rollback(moduleId);}

    private static byte[] fetchCatalog(String name,int max)throws IOException{
        if(!name.matches("catalog-v1\\.(json|sig)"))
            throw new IOException("Unapproved metadata path");
        HttpURLConnection c=(HttpURLConnection)new URL(
            GameModuleCatalog.APPROVED_CATALOG_BASE+name).openConnection();
        c.setConnectTimeout(10000);c.setReadTimeout(12000);
        c.setRequestProperty("User-Agent","ReverieVR-GameModules/1");
        try{
            if(c.getResponseCode()!=200)throw new IOException("Catalog HTTP "+
                c.getResponseCode());
            try(InputStream input=c.getInputStream()){return bounded(input,max);}
        }finally{c.disconnect();}
    }
    private static byte[] bounded(InputStream in,int max)throws IOException{
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        byte[] buf=new byte[4096];int total=0,n;
        while((n=in.read(buf))!=-1){
            total+=n;if(total>max)throw new IOException("Metadata exceeds limit");
            out.write(buf,0,n);
        }
        return out.toByteArray();
    }

    private void download(GameModuleCatalog.Release release,File partial,
                          Progress progress)throws IOException {
        URL next=new URL(release.url);
        HttpURLConnection c=null;
        for(int hop=0;hop<5;++hop){
            String host=next.getHost();
            if(!"https".equals(next.getProtocol()) ||
               (!"github.com".equals(host) &&
                !"release-assets.githubusercontent.com".equals(host) &&
                !"objects.githubusercontent.com".equals(host)))
                throw new IOException("Unexpected download host");
            c=(HttpURLConnection)next.openConnection();
            c.setInstanceFollowRedirects(false);
            c.setConnectTimeout(10000);c.setReadTimeout(20000);
            c.setRequestProperty("User-Agent","ReverieVR-GameModules/1");
            int status=c.getResponseCode();
            if(status==301||status==302||status==303||status==307||status==308){
                String location=c.getHeaderField("Location");
                if(location==null)throw new IOException("Missing download redirect");
                try{next=next.toURI().resolve(location).toURL();}
                catch(Exception e){throw new IOException("Invalid release redirect",e);}
                c.disconnect();c=null;continue;
            }
            if(status!=200)throw new IOException("Download HTTP "+status);
            try(InputStream in=c.getInputStream();
                FileOutputStream out=new FileOutputStream(partial)){
                byte[] buf=new byte[16384];long total=0;int n;
                long lastBucket=-1;
                while((n=in.read(buf))!=-1){
                    total+=n;
                    if(total>release.byteCount ||
                       total>RvPackInstaller.MAX_ARCHIVE)
                        throw new IOException("Release exceeds declared size");
                    out.write(buf,0,n);
                    // One UI callback per 5% progress interval, never one
                    // runnable for every socket read or on the GL thread.
                    long bucket=(total*20L)/release.byteCount;
                    if(progress!=null&&bucket>lastBucket){
                        lastBucket=bucket;
                        final long completed=total;
                        ui.post(()->progress.onBytes(completed,release.byteCount));
                    }
                }
                out.getFD().sync();
                if(total!=release.byteCount)
                    throw new IOException("Incomplete game release");
            }finally{c.disconnect();}
            if(!RvPackInstaller.digest(partial).equals(release.sha256))
                throw new IOException("Downloaded game checksum mismatch");
            return;
        }
        throw new IOException("Excessive release redirects");
    }

    @Override public void close(){executor.shutdownNow();}
}
