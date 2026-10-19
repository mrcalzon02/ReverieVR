package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.app.DownloadManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;
import java.io.*;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Explicit Android-user-approved installation of signed companion game APKs.
 * Never treats a finished download as an installed app. Native code is loaded
 * only by Android as part of a separately installed, signed package.
 */
final class CompanionGameApkInstaller implements AutoCloseable {
    interface Callback {void result(String message,boolean installerLaunched);}
    private final Activity activity;
    private final DownloadManager downloads;
    private final ExecutorService io=Executors.newSingleThreadExecutor();
    private GameModuleCatalog.Release active;
    private Callback callback;
    private long pendingId=-1;
    private boolean registered;

    CompanionGameApkInstaller(Activity activity){
        this.activity=activity;
        this.downloads=(DownloadManager)activity.getSystemService(
            Context.DOWNLOAD_SERVICE);
        IntentFilter filter=new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE);
        if(Build.VERSION.SDK_INT>=33)
            activity.registerReceiver(receiver,filter,Context.RECEIVER_NOT_EXPORTED);
        else activity.registerReceiver(receiver,filter);
        registered=true;
    }

    // Every attempt gets its own DownloadManager destination. Android will
    // not replace an existing destination on retry; fixed filenames made
    // failed/abandoned updates permanently un-downloadable.
    static String downloadFilename(String id,long revision){
        if(GameModuleRegistry.find(id)==null||revision<1)
            throw new IllegalArgumentException("Invalid game download identity");
        return "reverie-game-"+id+"-r"+revision+"-"+
            UUID.randomUUID().toString()+".apk";
    }

    synchronized void install(GameModuleCatalog.Verified catalog,
                              GameModuleCatalog.Release release,Callback result){
        if(pendingId>=0){result.result("A game APK is already downloading",false);return;}
        if(catalog==null||release==null||!catalog.releases.contains(release)||
           !"companion-apk".equals(release.kind) ||
           CompanionGameRegistry.find(release.id)==null ||
           !GameModuleCatalog.approvedReleaseUrl(release.url,"companion-apk")){
            result.result("Native game release has not been authenticated",false);
            return;
        }
        if(!GameModuleCatalog.compatibleWithThisDevice(release)){
            result.result("Companion APK incompatible with this hub/device",false);
            return;
        }
        if(downloads==null){
            result.result("Android Download Manager unavailable",false);return;
        }
        if(Build.VERSION.SDK_INT>=26 &&
           !activity.getPackageManager().canRequestPackageInstalls()){
            Intent permit=new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                Uri.parse("package:"+activity.getPackageName()));
            try{
                activity.startActivity(permit);
                result.result("Allow game package installs and retry",false);
            }catch(RuntimeException e){
                result.result("Android denied installer permissions",false);
            }
            return;
        }
        String filename=downloadFilename(release.id,release.revision);
        DownloadManager.Request request=new DownloadManager.Request(
            Uri.parse(release.url));
        request.setTitle("ReverieVR: "+release.id+" r"+release.revision);
        request.setDescription("Independent signed native game update");
        request.setNotificationVisibility(
            DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
        request.setDestinationInExternalFilesDir(activity,
            Environment.DIRECTORY_DOWNLOADS,filename);
        try{
            pendingId=downloads.enqueue(request);
            active=release;callback=result;
            result.result("Downloading verified game APK...",false);
        }catch(RuntimeException e){
            pendingId=-1;active=null;callback=null;
            result.result("Android rejected game download",false);
        }
    }

    private final BroadcastReceiver receiver=new BroadcastReceiver(){
        @Override public void onReceive(Context context,Intent intent){
            if(!DownloadManager.ACTION_DOWNLOAD_COMPLETE.equals(intent.getAction()))
                return;
            long id=intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID,-1);
            final GameModuleCatalog.Release candidate;
            final Callback cb;
            synchronized(CompanionGameApkInstaller.this){
                if(id<0||id!=pendingId)return;
                candidate=active;cb=callback;
                pendingId=-1;active=null;callback=null;
            }
            Uri uri=downloads.getUriForDownloadedFile(id);
            if(uri==null){
                downloads.remove(id);
                if(cb!=null)cb.result("Game APK download failed",false);
                return;
            }
            try{
                io.execute(()->verifyAndOffer(candidate,cb,uri,id));
            }catch(java.util.concurrent.RejectedExecutionException cancelled){
                downloads.remove(id);
                if(cb!=null)cb.result("Game APK verification interrupted",false);
            }
        }
    };

    private void verifyAndOffer(GameModuleCatalog.Release release,
                                Callback cb,Uri uri,long downloadId){
        File temp=null;
        boolean handedToInstaller=false;
        try{
            temp=VerifiedCompanionApkProvider.create(activity);
            try(InputStream in=activity.getContentResolver().openInputStream(uri);
                FileOutputStream out=new FileOutputStream(temp)){
                if(in==null)throw new IOException("Downloaded APK cannot be opened");
                byte[] bytes=new byte[32768];long count=0;int n;
                while((n=in.read(bytes))!=-1){
                    count+=n;
                    if(count>release.byteCount||count>512L*1024L*1024L)
                        throw new IOException("Companion APK exceeds declared size");
                    out.write(bytes,0,n);
                }
                out.getFD().sync();
            }
            CompanionApkVerifier.Result verified=
                CompanionApkVerifier.verify(activity,temp,release);
            if(!verified.valid)throw new IOException(verified.reason);
            // A verified private APK, never the original download URI,
            // is the data Android Package Installer will actually read.
            final File trustedFile=temp;
            final Uri trustedUri=VerifiedCompanionApkProvider.uriFor(
                activity,trustedFile);
            activity.runOnUiThread(()->{
                Intent install=new Intent(Intent.ACTION_VIEW);
                install.setDataAndType(trustedUri,
                    "application/vnd.android.package-archive");
                install.setClipData(android.content.ClipData.newRawUri(
                    "Verified ReverieVR companion APK",trustedUri));
                install.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                try{
                    activity.startActivity(install);
                    if(cb!=null)cb.result(
                        "Verified APK passed to Android; approval required",true);
                }catch(RuntimeException e){
                    trustedFile.delete();
                    if(cb!=null)cb.result("Android package installer unavailable",false);
                }
            });
            handedToInstaller=true;
        }catch(Exception e){
            String problem=e.getMessage();
            activity.runOnUiThread(()->{
                if(cb!=null)cb.result("APK rejected: "+problem,false);
            });
        }finally{
            // Keep the exact verified inode available until Android has
            // consumed its URI grant; discard expired files after 48 hours.
            if(temp!=null&&!handedToInstaller)temp.delete();
            // Android reads a separate private, verified copy through the
            // controlled provider, so the untrusted DownloadManager file
            // must not be retained after validation or failure.
            downloads.remove(downloadId);
        }
    }

    @Override public void close(){
        io.shutdownNow();
        if(registered){
            try{activity.unregisterReceiver(receiver);}
            catch(IllegalArgumentException ignored){}
            registered=false;
        }
    }
}
