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

import org.json.JSONObject;
import java.io.*;
import java.nio.charset.StandardCharsets;
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
        for(GameModuleRegistry.Entry game:GameModuleRegistry.GAMES){
            final String id=game.id;
            File installed=store.activeAssets(id);
            String local=installed==null?"Built-in / no content pack":
                "Content r"+installed.getParentFile().getName();
            final boolean development=new ReveriePreferences(this)
                .getLoggingMode()==LoggingMode.DEVELOPMENT;
            GameModuleCatalog.Release release=verified==null?null:
                verified.find(id,"content",development?"testing":"stable");
            if(release==null&&verified!=null && development)
                release=verified.find(id,"content","stable");
            LinearLayout row=new LinearLayout(this);
            row.setOrientation(LinearLayout.HORIZONTAL);
            TextView title=new TextView(this);
            title.setText(game.displayName+"\n"+local);
            row.addView(title,new LinearLayout.LayoutParams(0,-2,1));
            Button action=new Button(this);
            if(release!=null&& (installed==null ||
                release.revision>parseRevision(installed))){
                GameModuleCatalog.Release selected=release;
                action.setText("Update r"+release.revision);
                action.setOnClickListener(v->installRemote(selected));
            }else if(installed!=null){
                action.setText("Rollback");
                action.setOnClickListener(v->{
                    try{
                        status.setText(store.rollback(id)?"Rolled back "+id:
                            "No previous revision for "+id);
                    }catch(IOException e){status.setText("Rollback: "+e.getMessage());}
                    refreshRows();
                });
            }else{
                action.setText("No pack");
                action.setEnabled(false);
            }
            row.addView(action);
            GameModuleCatalog.Release apkRelease=verified==null?null:
                verified.find(id,"companion-apk",
                    development?"testing":"stable");
            if(apkRelease==null&&verified!=null && development)
                apkRelease=verified.find(id,"companion-apk","stable");
            if(apkRelease!=null){
                GameModuleCatalog.Release selectedApk=apkRelease;
                Button getApk=new Button(this);
                getApk.setText("Game APK");
                getApk.setOnClickListener(v->apkInstaller.install(
                    verified,selectedApk,(message,started)->{
                        status.setText(message);
                        if(started)refreshRows();
                    }));
                row.addView(getApk);
            }
            // Native code is launched ONLY from an independently installed,
            // Android-signed companion package with the expected identity.
            if(CompanionGameRegistry.installed(this,id)!=null){
                Button play=new Button(this);play.setText("Play APK");
                play.setOnClickListener(v->{
                    if(!CompanionGameRegistry.launch(this,id))
                        status.setText("Companion launch rejected for "+id);
                });
                row.addView(play);
            }
            rows.addView(row);
        }
    }
    private static long parseRevision(File assets){
        try{return Long.parseLong(assets.getParentFile().getName());}
        catch(RuntimeException e){return 0L;}
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
            status.setText(error==null?
                "Installed "+release.id+" r"+release.revision:
                "Install failed: "+error);
            refreshRows();
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
                    JSONObject metadata=new JSONObject(
                        new String(bytes,StandardCharsets.UTF_8));
                    id=metadata.getString("id");
                    revision=metadata.getLong("revision");
                    if(metadata.getInt("schema")!=1 ||
                       !"content".equals(metadata.getString("kind")) ||
                       RvPackInstaller.namespace(id)==null || revision<1)
                        throw new IOException("Unsupported game content manifest");
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
