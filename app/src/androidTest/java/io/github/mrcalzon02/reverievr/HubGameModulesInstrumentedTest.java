package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Test;
import org.junit.runner.RunWith;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import static org.junit.Assert.*;

/**
 * INTEGRATION test executed INSIDE the real ReverieVR hub APK on Android.
 * No mocked identity, fake release feed, bypassed signer or mock installer.
 * The public signed catalog is currently empty, so it proves real signed
 * discovery and fail-closed acquisition; the installed companion test proves
 * actual Android package admission. A published catalog entry is required
 * before an authenticated remote content/APK download can be asserted.
 */
@RunWith(AndroidJUnit4.class)
public final class HubGameModulesInstrumentedTest {
    private static final String CHAMBER="procedural-test-chamber";
    private static Context context(){
        return InstrumentationRegistry.getInstrumentation().getTargetContext();
    }

    private static File makePack(File root,GameModuleRegistry.Entry game,int revision)
        throws Exception {
        File zip=new File(root,game.id+"-"+revision+".rvpack");
        try(ZipOutputStream out=new ZipOutputStream(new FileOutputStream(zip))){
            out.putNextEntry(new ZipEntry("module.json"));
            String manifest="{\"schema\":1,\"kind\":\"content\",\"id\":\""+
                game.id+"\",\"version\":\"0.1."+revision+
                "\",\"revision\":"+revision+"}";
            out.write(manifest.getBytes(StandardCharsets.UTF_8));
            out.closeEntry();
            out.putNextEntry(new ZipEntry("assets/native/"+
                game.contentNamespace+"/acquisition-probe.json"));
            out.write(("{\"id\":\""+game.id+"\",\"revision\":"+revision+"}")
                .getBytes(StandardCharsets.UTF_8));
            out.closeEntry();
        }
        return zip;
    }

    @Test public void allRegisteredGamesAcceptIsolatedAcquisitionUpdateRollback()
        throws Exception{
        File root=new File(context().getCacheDir(),
            "game-integration-"+System.currentTimeMillis());
        assertTrue(root.mkdirs());
        try {
            RvPackInstaller store=new RvPackInstaller(root);
            assertTrue(GameModuleRegistry.GAMES.length>=15);
            for(GameModuleRegistry.Entry game:GameModuleRegistry.GAMES){
                File first=makePack(root,game,1);
                File v1=store.install(first,game.id,1,RvPackInstaller.digest(first));
                assertTrue(game.id,new File(v1,"native/"+game.contentNamespace+
                    "/acquisition-probe.json").isFile());
                File second=makePack(root,game,2);
                File v2=store.install(second,game.id,2,RvPackInstaller.digest(second));
                assertNotEquals("Revision did not change "+game.id,
                    v1.getCanonicalPath(),v2.getCanonicalPath());
                assertEquals("Update not active "+game.id,
                    v2.getCanonicalPath(),store.activeAssets(game.id).getCanonicalPath());
                assertTrue("Rollback rejected "+game.id,store.rollback(game.id));
                assertEquals("Rollback active pointer "+game.id,
                    v1.getCanonicalPath(),store.activeAssets(game.id).getCanonicalPath());
            }
            // Installing a different game cannot mutate an earlier game's
            // active pointer; removing one game must leave another untouched.
            GameModuleRegistry.Entry first=GameModuleRegistry.GAMES[0];
            GameModuleRegistry.Entry last=GameModuleRegistry.GAMES[
                GameModuleRegistry.GAMES.length-1];
            assertNotNull(store.activeAssets(first.id));
            assertTrue(store.removeContent(last.id));
            assertNull(store.activeAssets(last.id));
            assertNotNull(store.activeAssets(first.id));
            for(GameModuleRegistry.Entry e:GameModuleRegistry.GAMES)
                assertFalse("Cross-game assets allowed "+e.id,
                    RvPackInstaller.allowedEntry(e.id,
                        "assets/native/not-this-game/evil.json",false));
        } finally {
            delete(root);
        }
    }

    private static void delete(File file){
        if(file.isDirectory()){
            File[] children=file.listFiles();
            if(children!=null)for(File child:children)delete(child);
        }
        file.delete();
    }

    @Test public void allRegisteredNativeGameLibrariesStartUpdateAndStop()
        throws Exception{
        assertTrue("Native C++ host failed to load",NativeModuleRuntime.isAvailable());
        java.util.List<NativeModuleRuntime.Descriptor> discovered=
            NativeModuleRuntime.listBuiltIns(true);
        assertEquals("Native C++ catalog diverges from verified Java registry",
            GameModuleRegistry.GAMES.length,discovered.size());
        java.util.HashSet<String> found=new java.util.HashSet<>();
        for(NativeModuleRuntime.Descriptor module:discovered){
            assertNotNull("Unknown native game binary: "+module.id,
                GameModuleRegistry.find(module.id));
            assertTrue("Duplicate C++ game identity "+module.id,found.add(module.id));
        }
        // Preserve the strict /native-modules/<id> suffix while never
        // touching actual player saves. The app-private cache sandbox has
        // an independent module-packs directory alongside native-modules.
        File sandbox=new File(context().getCacheDir(),"all-games-native-tests");
        File home=new File(sandbox,"native-modules");
        assertTrue(home.isDirectory()||home.mkdirs());
        // Game state and extracted assets are isolated per ID. This probes
        // genuine C++ create/resume/update/pause/destroy callbacks rather
        // than simply checking whether ELF files exist in the APK.
        try(NativeModuleRuntime runtime=new NativeModuleRuntime(
            home,context().getAssets(),null,true,
            new ReveriePreferences(
                InstrumentationRegistry.getInstrumentation().getContext()))){
            for(GameModuleRegistry.Entry game:GameModuleRegistry.GAMES){
                boolean started=runtime.start(game.id);
                assertTrue("JNI/C++ game launch FAILED: "+game.id+
                    " cause="+runtime.getLastError(),started);
                assertEquals("Wrong active game after launch "+game.id,
                    game.id,runtime.getActiveModuleId());
                runtime.update(NativeModuleRuntime.POINTER_NONE,null,null);
                runtime.drainSoundCues();
                runtime.pauseForLifecycle();
                runtime.resumeForLifecycle();
                runtime.stop();
                assertFalse("Game remained active after stop: "+game.id,
                    runtime.isRunning());
            }
        } finally{
            delete(sandbox);
        }
    }

    @Test public void realBreakwaterBeckenburgContentLoadsThroughHub()
        throws Exception {
        File root=new File(context().getCacheDir(),
            "verified-breakwater-"+System.nanoTime());
        assertTrue(root.mkdirs());
        // Require every authentic model from the activated downloaded pack.
        final String[] names={
            "concrete_block_1.bwm","wooden_crate_1.bwm",
            "ammo_box_mp_1.bwm","owner_boat_rowboat.bwm",
            "owner_apc_armor.bwm"
        };
        try {
            File pack=new File(root,"breakwater-real-models.rvpack");
            try(ZipOutputStream out=new ZipOutputStream(new FileOutputStream(pack))){
                out.putNextEntry(new ZipEntry("module.json"));
                out.write(("{\"schema\":1,\"kind\":\"content\","+
                    "\"id\":\"breakwater-battery\",\"revision\":1,"+
                    "\"version\":\"0.1.0\"}").getBytes(StandardCharsets.UTF_8));
                out.closeEntry();
                for(String name:names){
                    String path="native/breakwater/beckenburg/"+name;
                    byte[] data;
                    try(java.io.InputStream stream=context().getAssets().open(path);
                        java.io.ByteArrayOutputStream bytes=
                            new java.io.ByteArrayOutputStream()){
                        byte[] buffer=new byte[8192];
                        int count;
                        while((count=stream.read(buffer))!=-1)
                            bytes.write(buffer,0,count);
                        data=bytes.toByteArray();
                    }
                    assertTrue("Game mesh absent from real hub "+path,data.length>=35);
                    out.putNextEntry(new ZipEntry("assets/"+path));
                    out.write(data);
                    out.closeEntry();
                }
            }
            RvPackInstaller installer=new RvPackInstaller(root);
            File assets=installer.install(pack,"breakwater-battery",1,
                RvPackInstaller.digest(pack));
            assertEquals(assets.getCanonicalPath(),
                installer.activeAssets("breakwater-battery").getCanonicalPath());
            for(String name:names)
                assertTrue("Actual imported model missing "+name,new File(
                    assets,"native/breakwater/beckenburg/"+name).isFile());
            File home=new File(root,"native-modules");
            assertTrue(home.isDirectory()||home.mkdirs());
            try(NativeModuleRuntime runtime=new NativeModuleRuntime(
                home,context().getAssets(),null,true,
            new ReveriePreferences(
                InstrumentationRegistry.getInstrumentation().getContext()))){
                boolean started=runtime.start("breakwater-battery");
                assertTrue("Verified model-backed Breakwater launch failed: "+
                    runtime.getLastError(),started);
                runtime.update(NativeModuleRuntime.POINTER_NONE,null,null);
                runtime.stop();
            }
        }finally{
            delete(root);
        }
    }

    @Test public void repeatedCompanionDownloadsUseUniquePrivateDestinations(){
        String first=CompanionGameApkInstaller.downloadFilename(
            "breakwater-battery",17);
        String retry=CompanionGameApkInstaller.downloadFilename(
            "breakwater-battery",17);
        assertNotEquals("Retry would collide with a previous Android download",
            first,retry);
        assertTrue(first.startsWith("reverie-game-breakwater-battery-r17-"));
        assertTrue(first.endsWith(".apk"));
        assertFalse(first.contains("/"));
        try{
            CompanionGameApkInstaller.downloadFilename("unknown-game",1);
            fail("Unregistered download game accepted");
        }catch(IllegalArgumentException expected){}
        try{
            CompanionGameApkInstaller.downloadFilename("breakwater-battery",0);
            fail("Invalid download version accepted");
        }catch(IllegalArgumentException expected){}
    }

    @Test public void actualInstalledCompanionMustMatchHubSignerAndIdentity(){
        // CI installs both DEBUG APKS signed by the SAME Android debug key.
        // A missing companion is a failure, not a skipped assertion.
        CompanionGameRegistry.Descriptor game=
            CompanionGameRegistry.installed(context(),CHAMBER);
        assertNotNull("Actual signed Test Chamber APK not admitted by hub",game);
        assertEquals("io.github.mrcalzon02.reverievr.game.testchamber",
            game.packageName);
        assertTrue("Installed Android version missing",
            CompanionGameRegistry.installedVersion(context(),CHAMBER)>0);
        CompanionGameRegistry.Descriptor breakwater=
            CompanionGameRegistry.installed(context(),"breakwater-battery");
        assertNotNull("Real standalone Breakwater APK not admitted by hub",breakwater);
        assertEquals("io.github.mrcalzon02.reverievr.game.breakwater",
            breakwater.packageName);
        assertTrue("Installed native Breakwater code revision missing",
            CompanionGameRegistry.installedVersion(context(),"breakwater-battery")>0);
        for(GameModuleRegistry.Entry item:GameModuleRegistry.GAMES){
            if(item.id.equals(CHAMBER) ||
               item.id.equals("breakwater-battery"))continue;
            assertNull("Unexpected independent companion installed: "+item.id,
                CompanionGameRegistry.installed(context(),item.id));
        }
    }

    @Test public void liveSignedCatalogCheckUsesHubNetworkAndRealTrustKey()
        throws Exception {
        try(GameModuleUpdateService update=new GameModuleUpdateService(context())){
            CountDownLatch done=new CountDownLatch(1);
            final GameModuleCatalog.Verified[] catalog=new GameModuleCatalog.Verified[1];
            final String[] failure=new String[1];
            update.check((v,error)->{
                catalog[0]=v;failure[0]=error;done.countDown();
            });
            assertTrue("Actual catalog fetch timed out",done.await(40,TimeUnit.SECONDS));
            assertNull("Authenticated hub catalog fetch FAILED: "+failure[0],failure[0]);
            assertNotNull("No verified production catalog",catalog[0]);
            assertTrue("Catalog rolled back",catalog[0].catalogRevision>=1);
            // The current public catalog intentionally declares zero updates.
            assertEquals("Unexpected public module release; inspect publication",
                0,catalog[0].releases.size());

            // A standalone Release object may look valid but may NOT be
            // downloaded without inclusion in the authenticated catalog.
            GameModuleCatalog.Release unverified=new GameModuleCatalog.Release(
                CHAMBER,"0.1.0","content",
                "https://github.com/mrcalzon02/ReverieVR/releases/download/module-test-chamber-v0.1.0/scene.rvpack",
                "0000000000000000000000000000000000000000000000000000000000000000",
                "reject untrusted descriptor","testing",2,100L,"0.1.0",26,1,1,2,
                new String[]{"arm64-v8a"});
            CountDownLatch rejected=new CountDownLatch(1);
            final String[] rejectedReason=new String[1];
            update.install(unverified,(where,error)->{
                assertNull("Unverified release installed",where);
                rejectedReason[0]=error;rejected.countDown();
            },null);
            assertTrue("Unverified acquisition did not respond",
                rejected.await(10,TimeUnit.SECONDS));
            assertTrue("Unverified release not rejected: "+rejectedReason[0],
                rejectedReason[0]!=null &&
                rejectedReason[0].contains("Unverified content release"));
        }
    }

    private static View findText(View view,String exact){
        if(view instanceof TextView && exact.equals(
            ((TextView)view).getText().toString()))return view;
        if(view instanceof ViewGroup){
            ViewGroup group=(ViewGroup)view;
            for(int i=0;i<group.getChildCount();i++){
                View found=findText(group.getChildAt(i),exact);
                if(found!=null)return found;
            }
        }
        return null;
    }

    @Test public void actualHubGameManagerShowsBundledLibraryWithoutIndependentDownloads()
        throws Exception {
        Instrumentation instrument=InstrumentationRegistry.getInstrumentation();
        Intent intent=new Intent(context(),MainActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        Activity hub=instrument.startActivitySync(intent);
        assertNotNull("Cannot launch real ReverieVR hub",hub);
        Instrumentation.ActivityMonitor monitor=instrument.addMonitor(
            GameModulesActivity.class.getName(),null,false);
        Activity manager=null;
        try {
            View manage=hub.findViewById(R.id.manage_game_modules_button);
            assertNotNull("Hub has no bundled game library",manage);
            instrument.runOnMainSync(manage::performClick);
            manager=instrument.waitForMonitorWithTimeout(monitor,15000);
            assertNotNull("Hub main-menu button did not open bundled library",manager);
            final Activity activity=manager;
            instrument.waitForIdleSync();
            final View root=activity.getWindow().getDecorView();
            final java.util.List<NativeModuleRuntime.Descriptor> games=
                NativeModuleRuntime.listBuiltIns(true);
            assertEquals("Missing compiled native game modules",
                GameModuleRegistry.GAMES.length,games.size());
            assertNotNull("Bundled inventory count absent",findText(root,
                games.size()+" native games included in this APK."));
            for(NativeModuleRuntime.Descriptor module:games){
                GameModuleRegistry.Entry info=GameModuleRegistry.find(module.id);
                assertNotNull("Unknown compiled native game "+module.id,info);
                boolean development=new ReveriePreferences(activity)
                    .getLoggingMode()==LoggingMode.DEVELOPMENT;
                String label=module.displayName+" — Included";
                if(info.developmentOnly&&!development)
                    label+=" (enable Development logging mode to launch)";
                assertNotNull("Bundled game missing from library: "+module.id,
                    findText(root,label));
            }
            assertNull("Retired remote catalog button still visible",
                findText(root,"Check releases"));
            assertNull("Retired content package importer still visible",
                findText(root,"Import .rvpack"));
            assertNull("Independent game APK action still visible",
                findText(root,"Install game APK"));
            View update=findText(root,"Check ReverieVR updates");
            assertNotNull("Bundled library cannot request whole-app update",update);
            assertTrue("Whole-app update control not usable",
                update instanceof Button && update.isEnabled());
            // Real UI click: return to Stage A and request the whole signed
            // hub updater, never the retired per-game catalog/installer.
            instrument.runOnMainSync(update::performClick);
            assertTrue("Bundled library did not return to Stage A",
                activity.isFinishing());
        } finally {
            instrument.removeMonitor(monitor);
            final Activity ownedManager=manager;
            instrument.runOnMainSync(()->{
                if(ownedManager!=null)ownedManager.finish();
                hub.finish();
            });
        }
    }

}
