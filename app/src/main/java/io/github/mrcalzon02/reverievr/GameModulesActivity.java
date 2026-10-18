package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/** Compact Stage A manager for independently downloaded game content. */
public final class GameModulesActivity extends Activity {
    private static final int PICK_RVPACK=7312;
    private GameModuleUpdateService network;
    private CompanionGameApkInstaller apkInstaller;
    private RvPackInstaller store;
    private final ExecutorService io=Executors.newSingleThreadExecutor();
    private GameModuleCatalog.Verified verified;
    private LinearLayout rows;
    private TextView status;

    @Override public void onCreate(Bundle saved){
        super.onCreate(saved);
        network=new GameModuleUpdateService(this);
        apkInstaller=new CompanionGameApkInstaller(this);
        store=new RvPackInstaller(getFilesDir());
        ScrollView scroll=new ScrollView(this);
        LinearLayout base=new LinearLayout(this);
        base.setPadding(16,16,16,16);
        base.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(base);
        status=new TextView(this);
        status.setText("Game content: local/offline. Built-in games remain playable.");
        base.addView(status);
        LinearLayout commands=new LinearLayout(this);
        commands.setOrientation(LinearLayout.HORIZONTAL);
        Button check=new Button(this);check.setText("Check releases");
        Button importFile=new Button(this);importFile.setText("Import .rvpack");
        commands.addView(check,new LinearLayout.LayoutParams(0,-2,1));
        commands.addView(importFile,new LinearLayout.LayoutParams(0,-2,1));
        base.addView(commands);
        rows=new LinearLayout(this);rows.setOrientation(LinearLayout.VERTICAL);
        base.addView(rows);
        check.setOnClickListener(v->checkRemote());
        importFile.setOnClickListener(v->chooseFile());
        setContentView(scroll);
        refreshRows();
    }

    @Override protected void onResume(){
        super.onResume();
        if(rows!=null)refreshRows();
    }
    private void refreshRows(){
        rows.removeAllViews();
        final boolean development=new ReveriePreferences(this)
            .getLoggingMode()==LoggingMode.DEVELOPMENT;
        for(GameModuleRegistry.Entry game:GameModuleRegistry.GAMES){
            final String id=game.id;
            File content=store.activeAssets(id);
            long contentRev=content==null?0L:parseRevision(content);
            long codeRev=CompanionGameRegistry.installedVersion(this,id);
            String details=contentRev==0?"Built-in":
                "Content r"+contentRev+" ("+
                (store.installedBytes(id)+1023L)/1024L+" KiB)";
            if(codeRev>0L)details+=" | Game APK r"+codeRev;
            GameModuleCatalog.Release contentUpdate=offer(id,"content",
                development);
            GameModuleCatalog.Release nativeUpdate=offer(id,"companion-apk",
                development);
            LinearLayout row=new LinearLayout(this);
            row.setOrientation(LinearLayout.HORIZONTAL);
            TextView title=new TextView(this);
            title.setText(game.displayName+"\n"+details);
            row.addView(title,new LinearLayout.LayoutParams(0,-2,1));
            Button action=new Button(this);
            action.setText("Manage");
            // A game with no installed content must still be able to check
            // its own signed release feed and install its first revision.
            action.setEnabled(true);
            action.setOnClickListener(v->showGameActions(id,contentRev,codeRev,
                contentUpdate,nativeUpdate));
            row.addView(action);
            rows.addView(row);
        }
    }
    private GameModuleCatalog.Release offer(String id,String kind,
                                              boolean development){
        if(verified==null)return null;
        GameModuleCatalog.Release release=verified.find(id,kind,
            development?"testing":"stable");
        if(release==null&&development)
            release=verified.find(id,kind,"stable");
        return GameModuleCatalog.compatibleWithThisDevice(release)?release:null;
    }
    private void showGameActions(String id,long contentRev,long codeRev,
                                 GameModuleCatalog.Release contentRelease,
                                 GameModuleCatalog.Release apkRelease){
        java.util.ArrayList<String> labels=new java.util.ArrayList<>();
        java.util.ArrayList<Runnable> actions=new java.util.ArrayList<>();
        labels.add("Check updates for this game");
        actions.add(()->checkRemoteFor(id));
        if(contentRelease!=null&&contentRelease.revision>contentRev){
            labels.add("Update content to r"+contentRelease.revision);
            actions.add(()->installRemote(contentRelease));
        }
        if(apkRelease!=null&&apkRelease.revision>codeRev){
            labels.add("Install game APK r"+apkRelease.revision);
            actions.add(()->apkInstaller.install(verified,apkRelease,
                (message,started)->{
                    status.setText(message);
                    if(started)refreshRows();
                }));
        }
        if(codeRev>0L){
            labels.add("Play installed game APK");
            actions.add(()->{
                if(!CompanionGameRegistry.launch(this,id))
                    status.setText("Companion launch rejected for "+id);
            });
        }
        if(contentRev>0L){
            labels.add("Rollback content");
            actions.add(()->{
                try{
                    status.setText(store.rollback(id)?"Rolled back "+id:
                        "No previous content revision");
                }catch(IOException error){
                    status.setText("Rollback failed: "+error.getMessage());
                }
                refreshRows();
            });
            labels.add("Remove downloaded content (keep saves)");
            actions.add(()->new AlertDialog.Builder(this)
                .setTitle("Remove "+id+" content?")
                .setMessage("Bundled game and player saves will remain.")
                .setNegativeButton("Cancel",null)
                .setPositiveButton("Remove",(dialog,which)->{
                    try{
                        status.setText(store.removeContent(id)?
                            "Removed downloaded content; saves preserved":
                            "No optional content installed");
                    }catch(IOException error){
                        status.setText("Content removal failed: "+
                            error.getMessage());
                    }
                    refreshRows();
                }).show());
        }
        if(labels.isEmpty())return;
        new AlertDialog.Builder(this).setTitle(id)
            .setItems(labels.toArray(new String[0]),
                (dialog,which)->actions.get(which).run())
            .setNegativeButton("Cancel",null)
            .show();
    }
    private static long parseRevision(File assets){
        try{return Long.parseLong(assets.getParentFile().getName());}
        catch(RuntimeException e){return 0L;}
    }
    private void checkRemoteFor(String gameId){
        status.setText("Checking "+gameId+" releases...");
        network.check((catalog,error)->{
            if(error!=null){
                status.setText(gameId+" check failed: "+error+
                    " (offline installation unchanged)");
                return;
            }
            verified=catalog;
            final boolean dev=new ReveriePreferences(this)
                .getLoggingMode()==LoggingMode.DEVELOPMENT;
            GameModuleCatalog.Release data=offer(gameId,"content",dev);
            GameModuleCatalog.Release code=offer(gameId,"companion-apk",dev);
            File local=store.activeAssets(gameId);
            long contentRev=local==null?0L:parseRevision(local);
            long codeRev=CompanionGameRegistry.installedVersion(this,gameId);
            String result=(data!=null&&data.revision>contentRev)
                ?"content r"+data.revision+" available"
                :"no new content release";
            if(code!=null&&code.revision>codeRev)
                result+=", game APK r"+code.revision+" available";
            else result+=", no new code release";
            status.setText(gameId+": "+result+
                " (authenticated catalog r"+catalog.catalogRevision+")");
            refreshRows();
        });
    }

    private void checkRemote(){
        status.setText("Checking signed game releases...");
        network.check((catalog,error)->{
            if(error!=null){status.setText("Release check: "+error);return;}
            verified=catalog;
            status.setText("Authenticated catalog r"+catalog.catalogRevision+
                " — "+catalog.releases.size()+" game releases.");
            refreshRows();
        });
    }
    private void installRemote(GameModuleCatalog.Release release){
        status.setText("Downloading "+release.id+" r"+release.revision+"...");
        network.install(release,(directory,error)->{
            if(isFinishing()||isDestroyed())return;
            status.setText(error==null?
                "Installed "+release.id+" r"+release.revision:
                "Install failed: "+error);
            refreshRows();
        },(read,total)->{
            if(isFinishing()||isDestroyed())return;
            long percentage=total>0?(read*100L)/total:0L;
            status.setText("Downloading "+release.id+" r"+release.revision+
                " — "+percentage+"% ("+(read+1023L)/1024L+" / "+
                (total+1023L)/1024L+" KiB)");
        });
    }
    private void chooseFile(){
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent,PICK_RVPACK);
    }
    @Override protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);
        if(request!=PICK_RVPACK||result!=RESULT_OK||data==null ||
           data.getData()==null)return;
        Uri uri=data.getData();
        status.setText("Validating local content pack...");
        io.execute(()->{
            File tmp;
            try {
                tmp=File.createTempFile("manual-game-content-", ".rvpack",getCacheDir());
            }catch(IOException creationError){
                runOnUiThread(()->status.setText(
                    "Cannot prepare imported content: "+creationError.getMessage()));
                return;
            }
            try{
                try(InputStream in=getContentResolver().openInputStream(uri);
                    FileOutputStream out=new FileOutputStream(tmp)){
                    if(in==null)throw new IOException("Selected file unavailable");
                    byte[] bytes=new byte[16384];long total=0;int n;
                    while((n=in.read(bytes))!=-1){
                        total+=n;if(total>RvPackInstaller.MAX_ARCHIVE)
                            throw new IOException("Selected pack too large");
                        out.write(bytes,0,n);
                    }
                    out.getFD().sync();
                }
                String id;long revision;
                try(ZipFile zip=new ZipFile(tmp)){
                    ZipEntry entry=zip.getEntry("module.json");
                    if(entry==null||entry.getSize()>16384)
                        throw new IOException("Missing/oversized module manifest");
                    byte[] bytes;
                    try(InputStream in=zip.getInputStream(entry)){
                        ByteArrayOutputStream out=new ByteArrayOutputStream();
                        byte[] buf=new byte[4096];int n;
                        while((n=in.read(buf))!=-1){
                            if(out.size()+n>16384)throw new IOException("Manifest too large");
                            out.write(buf,0,n);
                        }
                        bytes=out.toByteArray();
                    }
                    // UI import and installer share the SAME strict parser;
                    // never let JSONObject accept duplicate identity fields.
                    RvPackManifest metadata=RvPackManifest.parse(bytes);
                    id=metadata.id;
                    revision=metadata.revision;
                }
                final String selectedId=id;
                final long selectedRevision=revision;
                final String digest=RvPackInstaller.digest(tmp);
                runOnUiThread(()->new AlertDialog.Builder(this)
                    .setTitle("Import local game content?")
                    .setMessage(selectedId+" / revision "+selectedRevision+
                        "\nOnly data assets; no executable code is installed.")
                    .setNegativeButton("Cancel",(d,w)->tmp.delete())
                    .setPositiveButton("Import",(d,w)->installLocal(tmp,
                        selectedId,selectedRevision,digest))
                    .show());
            }catch(Exception error){
                tmp.delete();
                runOnUiThread(()->status.setText("Invalid pack: "+error.getMessage()));
            }
        });
    }
    private void installLocal(File tmp,String id,long revision,String hash){
        status.setText("Installing local "+id+"...");
        io.execute(()->{
            String result;
            try{
                store.install(tmp,id,revision,hash);
                result="Installed local "+id+" revision "+revision;
            }catch(Exception e){result="Local import failed: "+e.getMessage();}
            finally{tmp.delete();}
            final String message=result;
            runOnUiThread(()->{status.setText(message);refreshRows();});
        });
    }
    @Override protected void onDestroy(){
        network.close();
        apkInstaller.close();
        io.shutdownNow();
        super.onDestroy();
    }
}
