package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Color;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.zip.CRC32;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.junit.Test;
import org.junit.runner.RunWith;
import static org.junit.Assert.*;

/** Exercises REAL verified game content in the actual ReverieVR JNI host.
 * One short-lived Android OpenGL ES 2 pbuffer is created in each test.
 * Unlike archive-only tests, this executes the native GPU upload path.
 * All test saves and content are isolated from ordinary player storage.
 */
@RunWith(AndroidJUnit4.class)
public final class DownloadedNativeGameAssetsInstrumentedTest {
    private static final String TAG="GameContentIntegration";

    private static class Pbuffer implements AutoCloseable {
        final EGLDisplay display;
        final EGLContext context;
        final EGLSurface surface;
        Pbuffer(){
            display=EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
            assertNotEquals(EGL14.EGL_NO_DISPLAY,display);
            int[] version=new int[2];
            assertTrue("Cannot start GLES2 display",
                EGL14.eglInitialize(display,version,0,version,1));
            int[] attrs={
                EGL14.EGL_RENDERABLE_TYPE,EGL14.EGL_OPENGL_ES2_BIT,
                EGL14.EGL_SURFACE_TYPE,EGL14.EGL_PBUFFER_BIT,
                EGL14.EGL_RED_SIZE,8,EGL14.EGL_GREEN_SIZE,8,
                EGL14.EGL_BLUE_SIZE,8,EGL14.EGL_DEPTH_SIZE,16,
                EGL14.EGL_NONE
            };
            EGLConfig[] configs=new EGLConfig[1];
            int[] count=new int[1];
            assertTrue("No GLES2 config",EGL14.eglChooseConfig(
                display,attrs,0,configs,0,1,count,0));
            assertTrue("Empty GLES2 config list",count[0]>0);
            context=EGL14.eglCreateContext(display,configs[0],
                EGL14.EGL_NO_CONTEXT,new int[]{EGL14.EGL_CONTEXT_CLIENT_VERSION,2,
                    EGL14.EGL_NONE},0);
            assertNotEquals("Cannot create GLES2 context",
                EGL14.EGL_NO_CONTEXT,context);
            surface=EGL14.eglCreatePbufferSurface(display,configs[0],
                new int[]{EGL14.EGL_WIDTH,512,EGL14.EGL_HEIGHT,512,
                    EGL14.EGL_NONE},0);
            assertNotEquals("Cannot create GLES2 pbuffer",
                EGL14.EGL_NO_SURFACE,surface);
            assertTrue("Cannot make GLES2 test surface current",
                EGL14.eglMakeCurrent(display,surface,surface,context));
            GLES20.glViewport(0,0,512,512);
        }
        @Override public void close(){
            EGL14.eglMakeCurrent(display,EGL14.EGL_NO_SURFACE,
                EGL14.EGL_NO_SURFACE,EGL14.EGL_NO_CONTEXT);
            EGL14.eglDestroySurface(display,surface);
            EGL14.eglDestroyContext(display,context);
            EGL14.eglTerminate(display);
        }
    }

    private static void remove(File file)throws IOException {
        if(file.isDirectory()){
            File[] children=file.listFiles();
            if(children==null)throw new IOException("Cannot read test sandbox");
            for(File child:children)remove(child);
        }
        if(file.exists()&&!file.delete())
            throw new IOException("Cannot remove game test data");
    }

    private static byte[] sourceAsset(AssetManager assets,String name)
        throws IOException {
        try(InputStream in=assets.open(name);
            ByteArrayOutputStream buffer=new ByteArrayOutputStream()){
            byte[] data=new byte[16384];
            for(int n;(n=in.read(data))!=-1;)buffer.write(data,0,n);
            return buffer.toByteArray();
        }
    }

    private static void packageOne(ZipOutputStream output,AssetManager assets,
                                   String path)throws IOException {
        byte[] data=sourceAsset(assets,path);
        assertTrue("Empty game asset "+path,data.length>0);
        output.putNextEntry(new ZipEntry("assets/"+path));
        output.write(data);
        output.closeEntry();
    }

    private static int packageFolder(ZipOutputStream output,AssetManager assets,
                                     String folder)throws IOException {
        int files=0;
        String[] names=assets.list(folder);
        if(names==null)throw new IOException("Missing assets directory "+folder);
        for(String name:names){
            if(!name.matches("[A-Za-z0-9_.-]+") ||
               !(name.endsWith(".png")||name.endsWith(".json")))
                throw new IOException("Invalid fixture path "+name);
            packageOne(output,assets,folder+"/"+name);
            files++;
        }
        return files;
    }

    private static int makeRealPack(File pack,AssetManager assets,String id)
        throws IOException {
        int count=0;
        try(ZipOutputStream output=new ZipOutputStream(new FileOutputStream(pack))){
            output.putNextEntry(new ZipEntry("module.json"));
            output.write(("{\"schema\":1,\"kind\":\"content\",\"id\":\""+id+
                "\",\"revision\":1,\"version\":\"0.1.0\"}")
                .getBytes(StandardCharsets.UTF_8));
            output.closeEntry();
            if("between-deliveries-red-ledger".equals(id)){
                count+=packageFolder(output,assets,"native/red-ledger/icons");
                assertEquals("Real Red Ledger icon inventory",24,count);
            }else if("omega-front".equals(id)){
                count+=packageFolder(output,assets,"native/omega-front/terrain");
                count+=packageFolder(output,assets,"native/omega-front/shared");
                // The original Omega private concrete and steel copies were
                // retired. Include only the two common materials it consumes.
                packageOne(output,assets,
                    "native/shared/v1/materials/coastal/quay_concrete.png");
                packageOne(output,assets,
                    "native/shared/v1/materials/coastal/oxidized_steel.png");
                count+=2;
                count+=packageFolder(output,assets,"native/omega-front/units/kenney");
                count+=packageFolder(output,assets,"native/omega-front/rpg_cc0");
                assertEquals("Real Omega Front CC0 source + sprite inventory",141,count);
            }else throw new IOException("Unknown integrated game test");
        }
        return count;
    }

    private static void runInstalledAssets(String id,int expected){
        Context app=InstrumentationRegistry.getInstrumentation().getTargetContext();
        File base=new File(app.getCacheDir(),"real-native-art-"+id+"-"+System.nanoTime());
        assertTrue(base.mkdirs());
        try{
            File pack=new File(base,"assets.rvpack");
            int files=makeRealPack(pack,app.getAssets(),id);
            assertEquals(expected,files);
            RvPackInstaller installer=new RvPackInstaller(base);
            File staged=installer.install(pack,id,1,RvPackInstaller.digest(pack));
            assertEquals("Content activation pointer missing "+id,
                staged.getCanonicalPath(),
                installer.activeAssets(id).getCanonicalPath());
            File home=new File(base,"native-modules");
            assertTrue(home.mkdirs());
            try(Pbuffer egl=new Pbuffer();
                NativeModuleRuntime runtime=new NativeModuleRuntime(
                    home,app.getAssets(),null,true,
                new ReveriePreferences(
                InstrumentationRegistry.getInstrumentation().getContext()))){
                boolean started=runtime.start(id);
                assertTrue("Native game failed to start "+id+": "+
                    runtime.getLastError(),started);
                try {
                    assertTrue("Native GLES2 upload failed for "+id+": "+
                        runtime.getLastError(),runtime.onSurfaceCreated());
                    try{
                        runtime.update(NativeModuleRuntime.POINTER_NONE,null,null);
                        float[] view=new float[16],projection=new float[16];
                        Matrix.setIdentityM(view,0);
                        Matrix.perspectiveM(projection,0,80.f,1.f,0.08f,150.f);
                        for(int eye=0;eye<2;eye++)
                            assertTrue("Native game rendered invalid eye "+id+
                                ":"+eye+" "+runtime.getLastError(),
                                runtime.renderEye(eye,view,projection));
                        GLES20.glFinish();
                        assertEquals("Game GLES2 error "+id,
                            GLES20.GL_NO_ERROR,GLES20.glGetError());
                        Log.i(TAG,"PASS real downloaded "+id+" assets="+files+
                            " nativeStart=true stereoGles2=true");
                    }finally{
                        runtime.releaseSurface();
                    }
                }finally{runtime.stop();}
            }
        }catch(Exception e){
            throw new AssertionError("Real downloaded game-content failure: "+id,e);
        }finally{
            try{remove(base);}catch(IOException failure){
                throw new AssertionError("Cannot clean game-content test sandbox",failure);
            }
        }
    }

    // These methods operate the exact Java -> JNI -> C++ native game path
    // used by Daydream. The emulator substitutes a scripted controller and
    // GLES2 pbuffer for a headset; it does not fake tactical game state.
    private static long omegaFrame(NativeModuleRuntime runtime,File evidence)
        throws IOException {
        return omegaFrame(runtime,evidence,0.f);
    }

    // Optional true head-rotation diagnostic. All ordinary gameplay captures
    // retain the original zero-pitch camera, viewport, stereo and pixel gates.
    private static long omegaFrame(NativeModuleRuntime runtime,File evidence,
                                   float cameraPitchDegrees) throws IOException {
        float[] view=new float[16],projection=new float[16];
        Matrix.setIdentityM(view,0);
        if(cameraPitchDegrees!=0.f)
            Matrix.rotateM(view,0,cameraPitchDegrees,1.f,0.f,0.f);
        Matrix.perspectiveM(projection,0,80.f,1.f,0.08f,150.f);
        GLES20.glClearColor(0,0,0,1);
        // Each headset eye has its own cleared depth attachment. Rendering
        // both eyes into this one test pbuffer without clearing between them
        // lets the second eye's sky cover the first eye's geometry while
        // the first eye's depth rejects the second eye's identical 3D meshes.
        // Clear each simulated eye, then inspect the completed final eye.
        for(int eye=0;eye<2;eye++) {
            GLES20.glDepthMask(true);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT|GLES20.GL_DEPTH_BUFFER_BIT);
            assertTrue("Omega Front native eye rendering failed: "+
                runtime.getLastError(),runtime.renderEye(eye,view,projection));
        }
        GLES20.glFinish();
        ByteBuffer pixels=ByteBuffer.allocateDirect(512*512*4);
        GLES20.glReadPixels(0,0,512,512,GLES20.GL_RGBA,
            GLES20.GL_UNSIGNED_BYTE,pixels);
        assertEquals("Omega Front GPU readback error",
            GLES20.GL_NO_ERROR,GLES20.glGetError());
        pixels.rewind();
        CRC32 crc=new CRC32();
        byte[] piece=new byte[8192];
        while(pixels.hasRemaining()){
            int count=Math.min(pixels.remaining(),piece.length);
            pixels.get(piece,0,count);crc.update(piece,0,count);
        }
        if(evidence!=null){
            pixels.rewind();
            int[] argb=new int[512*512];
            for(int y=0;y<512;y++)for(int x=0;x<512;x++){
                int red=pixels.get()&255,green=pixels.get()&255;
                int blue=pixels.get()&255,alpha=pixels.get()&255;
                argb[(511-y)*512+x]=(alpha<<24)|(red<<16)|(green<<8)|blue;
            }
            Bitmap image=Bitmap.createBitmap(argb,512,512,Bitmap.Config.ARGB_8888);
            try(FileOutputStream out=new FileOutputStream(evidence)){
                assertTrue("Could not encode Omega Front framebuffer",
                    image.compress(Bitmap.CompressFormat.PNG,100,out));
            }
            image.recycle();
        }
        return crc.getValue();
    }

    private static long omegaTap(NativeModuleRuntime runtime,
                                 VirtualInputBus controls,float x,float y){
        float[] aim={x,y,0.f},forward={0.f,0.f,-1.f};
        VirtualOutput fire=VirtualOutput.joystickButton(0);
        controls.applyDigital(fire,true);
        runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,aim,forward);
        long sound=runtime.drainSoundCues();
        controls.applyDigital(fire,false);
        runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,aim,forward);
        return sound;
    }

    @Test public void omegaFrontReallyTravelsDeploysOrdersAndRestoresInGles()
        throws Exception {
        Context app=InstrumentationRegistry.getInstrumentation().getTargetContext();
        assertFalse("Omega Front hidden behind Development mode",
            GameModuleRegistry.find("omega-front").developmentOnly);
        boolean listed=false;
        for(NativeModuleRuntime.Descriptor d:NativeModuleRuntime.listBuiltIns(false))
            if(d.id.equals("omega-front"))listed=true;
        assertTrue("Omega Front absent from normal playable native games",listed);
        File base=new File(app.getCacheDir(),"omega-front-ci-"+System.nanoTime());
        assertTrue(base.mkdirs());
        File home=new File(base,"native-modules");
        assertTrue(home.mkdirs());
        File saved=new File(home,"omega-front/frontier.v1");
        File screenshot=new File(app.getFilesDir(),"omega-front-ci-tactical.png");
        File restoredScreenshot=new File(app.getFilesDir(),"omega-front-ci-restored.png");
        VirtualInputBus bus=new VirtualInputBus();
        try(Pbuffer egl=new Pbuffer();
            NativeModuleRuntime runtime=new NativeModuleRuntime(
                home,app.getAssets(),bus,false,
                new ReveriePreferences(
                InstrumentationRegistry.getInstrumentation().getContext()))){
            assertTrue("Omega Front normal native start failed: "+
                runtime.getLastError(),runtime.start("omega-front"));
            assertTrue("Omega Front GLES2 initialization failed",
                runtime.onSurfaceCreated());
            long title=omegaFrame(runtime,null);
            // Real initial screen is now a title, then campaign dossier.
            omegaTap(runtime,bus,0.f,.42f); // CAMPAIGN
            long dossier=omegaFrame(runtime,null);
            assertNotEquals("Campaign menu did not open",title,dossier);
            omegaTap(runtime,bus,0.f,.42f); // BEGIN CAMPAIGN
            long sector=omegaFrame(runtime,null);
            assertNotEquals("Begin campaign did not enter strategy view",dossier,sector);
            assertTrue("New campaign did not write its initial checkpoint",saved.isFile());
            byte[] initialCheckpoint=Files.readAllBytes(saved.toPath());
            // A new campaign starts on the continent overview. Opening the
            // Hearthfall region is navigation only: it must not silently
            // march to S011 or deploy a battle on the same controller click.
            omegaTap(runtime,bus,0.f,0.f);
            long region=omegaFrame(runtime,null);
            assertNotEquals("Continent selection did not open the sector map",
                sector,region);
            // Sector 11 (x=2,y=2) is the first real deployment mission.
            omegaTap(runtime,bus,.1975f,-.1775f);
            long traveled=omegaFrame(runtime,null);
            assertNotEquals("Strategic travel did not update real 2D pixels",
                region,traveled);
            long deploySound=omegaTap(runtime,bus,.1975f,-.1775f);
            long tactical=omegaFrame(runtime,null);
            assertNotEquals("Deploy did not enter visible tactical battle",
                traveled,tactical);
            assertEquals("Real native battle-start cue not emitted",14L,
                deploySound&255L);
            // Aim at Hearthwell's starting soldier, then issue an actual
            // movement order to cell (6,11) via the same controller button.
            omegaTap(runtime,bus,-.6205f,.0565f);
            omegaTap(runtime,bus,-.4015f,.0565f);
            for(int pulse=0;pulse<4;pulse++){
                Thread.sleep(115);
                runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
                    new float[]{-.4015f,.0565f,0.f},
                    new float[]{0.f,0.f,-1.f});
            }
            long moved=omegaFrame(runtime,screenshot);
            assertNotEquals("Tactical input/step produced no visible change",
                tactical,moved);
            runtime.pauseForLifecycle();
            assertTrue("Lifecycle pause did not suspend native input",runtime.isPaused());
            assertTrue("No actual native campaign checkpoint",saved.isFile());
            assertEquals("Native campaign checkpoint size",2048L,saved.length());
            byte[] savedCheckpoint=Files.readAllBytes(saved.toPath());
            assertFalse("Tactical movement/order never reached the actual save",
                Arrays.equals(initialCheckpoint,savedCheckpoint));
            Files.write(new File(app.getFilesDir(),"omega-front-ci-checkpoint-before.bin").toPath(),
                savedCheckpoint);
            runtime.releaseSurface();
            runtime.stop();
            assertTrue("Cannot relaunch saved native Omega Front",
                runtime.start("omega-front"));
            assertTrue("Cannot restore Omega Front GLES2 after relaunch",
                runtime.onSurfaceCreated());
            // A stopped module does not end the Activity lifecycle pause.
            // The Android Activity resumes explicitly before accepting input;
            // without that transition both CONTINUE taps are ignored and the
            // captured frame is still the title, not the restored battle.
            assertTrue("Relaunch unexpectedly cleared Activity pause",runtime.isPaused());
            runtime.resumeForLifecycle();
            assertFalse("Reopened Omega Front still rejects input",runtime.isPaused());
            long reloadTitle=omegaFrame(runtime,null);
            assertNotEquals("Relaunch should show title, not dispatch old battle input",
                moved,reloadTitle);
            omegaTap(runtime,bus,0.f,.42f); // CAMPAIGN
            omegaTap(runtime,bus,0.f,.42f); // CONTINUE CAMPAIGN
            byte[] resumedCheckpoint=Files.readAllBytes(saved.toPath());
            Files.write(new File(app.getFilesDir(),"omega-front-ci-checkpoint-after.bin").toPath(),
                resumedCheckpoint);
            assertArrayEquals("Continuing campaign unexpectedly rewrote the checkpoint",
                savedCheckpoint,resumedCheckpoint);
            long restored=omegaFrame(runtime,restoredScreenshot);
            Log.i(TAG,"Omega checkpoint comparison movedCRC="+moved+
                " restoredCRC="+restored+" saveBytes="+savedCheckpoint.length);
            assertEquals("Continuing native checkpoint changed saved battle view",
                moved,restored);
            runtime.releaseSurface();
            runtime.stop();
            Log.i("OmegaFrontPlayability",
                "PASS omega-front emulator nativeLaunch=true publicGames=true "+
                "travel=true deploy=true controllerOrder=true combatStep=true "+
                "stereoGles2=true saveResume=true screenshot="+
                screenshot.getAbsolutePath()+" initialCRC="+sector+
                " travelCRC="+traveled+" battleCRC="+tactical+
                " movedCRC="+moved+" restoredCRC="+restored);
        }finally{remove(base);}
    }


    /**
     * Preserve real installed-GLES2 initial/menu frames for EVERY registered
     * native game, including built-in classic board/arcade games. No generated
     * thumbnails, no synthetic scene and no temporary 'proof' splash art.
     * DOS titles are a DIFFERENT runtime and need their own DOS capture gate.
     */
    /**
     * Genuine installed native C++ GLES2 test of the Gumball route map:
     * pointer focus does not launch, first click arms, premature repeat
     * cannot fly, locked route is visibly distinct, third edge click launches.
     * No fake screenshot, no synthetic direct Simulation::launch shortcut.
     */
    @Test public void gumballRouteMapFocusArmLockThenFlyInGles()
        throws Exception {
        Context app=InstrumentationRegistry.getInstrumentation().getTargetContext();
        File base=new File(app.getCacheDir(),"gumball-route-ci-"+System.nanoTime());
        assertTrue("No isolated Gumball route sandbox",base.mkdirs());
        File home=new File(base,"native-modules");
        assertTrue("No isolated Gumball save home",home.mkdirs());
        final float[] target={reverieRouteX(),.27f,0.f};
        final float[] direction={0.f,0.f,-1.f};
        File saved=new File(home,"gumball-armada/campaign.v1");
        File focused=new File(app.getFilesDir(),"gumball-route-focused.png");
        File armed=new File(app.getFilesDir(),"gumball-route-armed.png");
        File locked=new File(app.getFilesDir(),"gumball-route-locked.png");
        File flight=new File(app.getFilesDir(),"gumball-route-flight.png");
        VirtualInputBus input=new VirtualInputBus();
        VirtualOutput fire=VirtualOutput.joystickButton(0);
        try(Pbuffer egl=new Pbuffer();
            NativeModuleRuntime runtime=new NativeModuleRuntime(
                home,app.getAssets(),input,true,
                new ReveriePreferences(
                    InstrumentationRegistry.getInstrumentation().getContext()))){
            assertTrue("Gumball native route chart failed to start: "+
                runtime.getLastError(),runtime.start("gumball-armada"));
            assertTrue("Gumball real GLES2 context failed: "+
                runtime.getLastError(),runtime.onSurfaceCreated());
            runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
                target,direction); // controller ray hover selects lane C
            long focusedFrame=omegaFrame(runtime,focused);
            assertFalse("Simply hovering launched a sortie",saved.exists());
            input.applyDigital(fire,true);
            runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
                target,direction); // first edge: arm selected lane
            input.applyDigital(fire,false);
            runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
                target,direction);
            input.applyDigital(fire,true);
            runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
                target,direction); // too-quick second edge MUST NOT launch
            input.applyDigital(fire,false);
            runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
                target,direction);
            assertFalse("Quick-repeat launched before route lock",saved.exists());
            long armedFrame=omegaFrame(runtime,armed);
            assertNotEquals("Gumball map did not visibly highlight armed route",
                            focusedFrame,armedFrame);
            for(int tick=0;tick<9;++tick){
                Thread.sleep(60L);
                runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
                    target,direction);
            }
            assertFalse("Route timer auto-launched without separate click",
                        saved.exists());
            long lockedFrame=omegaFrame(runtime,locked);
            assertNotEquals("Route LOCKED state did not change real map pixels",
                            armedFrame,lockedFrame);
            input.applyDigital(fire,true);
            runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
                target,direction); // deliberately confirm same lane
            input.applyDigital(fire,false);
            runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
                target,direction);
            long flightFrame=omegaFrame(runtime,flight);
            assertNotEquals("Confirmed lane did not render actual flight",
                            lockedFrame,flightFrame);
            assertTrue("Confirmed lane did not write actual campaign save",
                       saved.isFile()&&saved.length()>0L);
            Log.i(TAG,"PASS real Gumball map hover, armed highlight, "
                +"350ms lock, explicit second-click flight and save");
            runtime.releaseSurface();
            runtime.stop();
        }finally{remove(base);}
    }

    private static float reverieRouteX(){
        // Same x as kChartRouteX[2]; no hidden Android-only hitbox expansion.
        return 1.55f;
    }

    @Test public void captureHomeGalleryHoverInfoFromActualAtlas() throws Exception {
        Context app=InstrumentationRegistry.getInstrumentation().getTargetContext();
        Bitmap atlas;
        try(InputStream in=app.getAssets().open("native/gallery/menu-preview-atlas.jpg")){
            atlas=BitmapFactory.decodeStream(in);
        }
        assertNotNull("Bundled game screenshot atlas absent",atlas);
        assertEquals("Unexpected gallery width",672,atlas.getWidth());
        assertEquals("Unexpected gallery height",920,atlas.getHeight());
        java.util.List<NativeModuleRuntime.Descriptor> games=
            new java.util.ArrayList<>();
        for(GameModuleRegistry.Entry game:GameModuleRegistry.GAMES)
            games.add(new NativeModuleRuntime.Descriptor(game.id,game.displayName));
        games.add(new NativeModuleRuntime.Descriptor(
            NativeGamesGallery.SITKA_PROJECT_ID,"Sitka"));
        java.util.Collections.sort(games,(a,b)->Integer.compare(
            NativeGamesGallery.order(a.id),NativeGamesGallery.order(b.id)));
        assertEquals("Home gallery must include all native games plus Sitka",
            GameModuleRegistry.GAMES.length+1,games.size());
        final String[] names={
            "gallery-hover-native.png","gallery-hover-classic.png",
            "gallery-hover-sitka.png","gallery-sitka-project.png"
        };
        final int[] pages={0,1,1,1};
        final int[] slots={0,1,0,-1};
        for(int i=0;i<names.length;i++){
            Bitmap screen=Bitmap.createBitmap(1024,768,Bitmap.Config.ARGB_8888);
            Canvas canvas=new Canvas(screen);
            Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
            canvas.drawColor(Color.rgb(9,18,31));
            NativeGamesGallery.drawInHomeRoom(
                canvas,paint,games,pages[i],slots[i],1.0f,"",atlas);
            if(i==3) {
                NativeGamesGallery.drawSitkaProjectDetails(canvas,paint,true);
                assertNotEquals("Sitka inspector missing",
                    Color.rgb(9,18,31),screen.getPixel(330,250));
            } else {
                int[] popup=NativeGamesGallery.hoverPopupRect(true,slots[i]);
                assertNotNull(popup);
                assertNotEquals("Hover pane missing",
                    Color.rgb(9,18,31),
                    screen.getPixel(popup[0]+5,popup[1]+7));
            }
            File evidence=new File(app.getFilesDir(),names[i]);
            try(FileOutputStream out=new FileOutputStream(evidence)){
                assertTrue("Could not save hover screenshot",
                    screen.compress(Bitmap.CompressFormat.PNG,100,out));
                out.getFD().sync();
            }
            assertTrue(evidence.isFile()&&evidence.length()>5000);
            Log.i(TAG,"PASS real Home gallery hover screenshot "+names[i]+
                " bytes="+evidence.length());
            screen.recycle();
        }
        atlas.recycle();
    }


    // This test drives the actual installed JNI Breakwater game and reads
    // GLES2 pixels. A purchased gun must be selectable through its physical
    // left-wall rack, not merely appear in a shop label or a mesh unit test.
    // Sound-backed Breakwater confirmations intentionally suppress the legacy
    // FEEDBACK_ACTIVATION bit. Check the actual pressed-frame JNI sound instead
    // of treating a successfully played control sound as an input failure.
    // Cue IDs below follow the ReverieNativeSoundCueV1 native ABI.
    private static boolean breakwaterHasCue(long packed,int expected) {
        for(int i=0;i<8;i++)
            if(((packed >>> (8*i)) & 255L)==expected)return true;
        return false;
    }

    private static int breakwaterPress(NativeModuleRuntime game,
                                       VirtualInputBus controls,
                                       float[] origin,float[] direction,
                                       int expectedSoundCue) {
        VirtualOutput primary=VirtualOutput.joystickButton(0);
        game.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
            origin,direction); // hover and re-arm after any focus change
        game.drainSoundCues(); // stale audio cannot prove a new action
        controls.applyDigital(primary,true);
        int feedback=game.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
            origin,direction);
        long sounds=game.drainSoundCues();
        controls.applyDigital(primary,false);
        game.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
            origin,direction);
        game.drainSoundCues();
        if(expectedSoundCue!=0) {
            assertEquals("Breakwater rejected action with cue "+expectedSoundCue,
                0,feedback & NativeModuleRuntime.FEEDBACK_FAILURE);
            assertTrue("Breakwater action sound "+expectedSoundCue+
                " absent from pressed frame "+Long.toHexString(sounds),
                breakwaterHasCue(sounds,expectedSoundCue));
            // A success bit is returned only after matching the native cue.
            return feedback | NativeModuleRuntime.FEEDBACK_ACTIVATION;
        }
        return feedback;
    }

    private static int breakwaterMenuPress(NativeModuleRuntime game,
                                           VirtualInputBus controls,float x,float y,
                                           int expectedSoundCue) {
        return breakwaterPress(game,controls,new float[]{x,y,0.f},
            new float[]{0.f,0.f,-1.f},expectedSoundCue);
    }

    private static int breakwaterSaveWord(byte[] save,int byteOffset){
        assertEquals("Real Breakwater checkpoint must be 88 bytes",88,save.length);
        return (save[byteOffset]&255) |
            ((save[byteOffset+1]&255)<<8) |
            ((save[byteOffset+2]&255)<<16) |
            ((save[byteOffset+3]&255)<<24);
    }

    private static int breakwaterGunPixels(File a,File b) {
        Bitmap first=BitmapFactory.decodeFile(a.getAbsolutePath());
        Bitmap second=BitmapFactory.decodeFile(b.getAbsolutePath());
        assertNotNull("Missing first actual Breakwater GLES2 frame",first);
        assertNotNull("Missing second actual Breakwater GLES2 frame",second);
        try {
            assertEquals(512,first.getWidth());
            assertEquals(512,second.getWidth());
            int changed=0;
            // The forward-mounted artillery occupies this lower-center view.
            // Exclude the sea/horizon so a moving enemy cannot masquerade as
            // evidence that changing the actual 3D gun altered pixels.
            for(int y=280;y<512;y++)for(int x=135;x<377;x++) {
                int c=first.getPixel(x,y),d=second.getPixel(x,y);
                int delta=Math.abs(Color.red(c)-Color.red(d))+
                    Math.abs(Color.green(c)-Color.green(d))+
                    Math.abs(Color.blue(c)-Color.blue(d));
                if(delta>48)changed++;
            }
            return changed;
        }finally{first.recycle();second.recycle();}
    }

    private static void verifyBreakwaterPurchasedRack(
        NativeModuleRuntime game,VirtualInputBus controls,File home,Context app)
        throws Exception {
        File save=new File(home,"breakwater-battery/breakwater-run-1-v2.bin");
        final int activated=NativeModuleRuntime.FEEDBACK_ACTIVATION;
        assertTrue("Breakwater did not select real slot 1",
            (breakwaterMenuPress(game,controls,-1.79f,.20f,0)&activated)!=0);
        assertTrue("Breakwater did not start Cadet shop",
            (breakwaterMenuPress(game,controls,-2.10f,.28f,37)&activated)!=0);
        assertTrue("Breakwater did not open armory",
            (breakwaterMenuPress(game,controls,-.90f,-.36f,35)&activated)!=0);
        assertTrue("Real Heavy Gun purchase was rejected",
            (breakwaterMenuPress(game,controls,-.70f,.34f,16)&activated)!=0);
        assertTrue("Breakwater purchase was not durably saved",save.isFile());
        byte[] purchased=Files.readAllBytes(save.toPath());
        assertEquals("Saved purchase must use checked v3 format",3,
            purchased[4]&255);
        assertEquals("Purchase must retain starter and newly owned Heavy Gun",3,
            breakwaterSaveWord(purchased,64));
        assertEquals("Purchased Heavy Gun must be selected on disk",1,
            breakwaterSaveWord(purchased,68));
        assertEquals("Heavy Gun must actually spend 900 of 1300 credits",400,
            breakwaterSaveWord(purchased,32));

        game.releaseSurface();
        game.stop();
        assertTrue("Cannot reopen purchased Breakwater game",game.start("breakwater-battery"));
        assertTrue("Cannot restore purchased Breakwater GLES2",game.onSurfaceCreated());
        assertTrue("Reopened Breakwater did not load saved slot",
            (breakwaterMenuPress(game,controls,-1.79f,.20f,0)&activated)!=0);
        assertArrayEquals("Reopen modified real shop checkpoint unexpectedly",
            purchased,Files.readAllBytes(save.toPath()));
        // The saved shop's Heavy Gun becomes the active mounted mesh on the
        // first combat frame. In-wave rack switches remain transient by
        // design; after a crash the last acknowledged shop is restored.
        assertTrue("Restored Heavy Gun could not enter its actual wave",
            (breakwaterMenuPress(game,controls,.90f,-.36f,14)&activated)!=0);
        File heavyRestored=new File(app.getFilesDir(),
            "breakwater-rack-heavy-restored.png");
        long restoredCrc=omegaFrame(game,heavyRestored);
        assertTrue("Starter Machine Gun physical rack click rejected",
            (breakwaterPress(game,controls,new float[]{0.f,0.f,0.f},
                new float[]{-1.85f,.55f,.48f},28)&activated)!=0);
        File machine=new File(app.getFilesDir(),"breakwater-rack-machine.png");
        long machineCrc=omegaFrame(game,machine);
        // Same selected gameplay gun, actual tilted-head GLES2 eye frames:
        // these expose any gun mesh hidden under the neutral view's sight.
        omegaFrame(game,new File(app.getFilesDir(),
            "breakwater-rack-machine-pitch20.png"),20.f);
        omegaFrame(game,new File(app.getFilesDir(),
            "breakwater-rack-machine-pitch35.png"),35.f);
        assertTrue("Purchased Heavy Gun physical rack click rejected",
            (breakwaterPress(game,controls,new float[]{0.f,0.f,0.f},
                new float[]{-1.85f,.20f,.48f},28)&activated)!=0);
        File heavySelected=new File(app.getFilesDir(),
            "breakwater-rack-heavy-selected.png");
        long selectedCrc=omegaFrame(game,heavySelected);
        omegaFrame(game,new File(app.getFilesDir(),
            "breakwater-rack-heavy-pitch20.png"),20.f);
        omegaFrame(game,new File(app.getFilesDir(),
            "breakwater-rack-heavy-pitch35.png"),35.f);
        int restoredDifference=breakwaterGunPixels(heavyRestored,machine);
        int rackDifference=breakwaterGunPixels(machine,heavySelected);
        assertTrue("Restore changed only UI, not actual mounted weapon pixels: "+
            restoredDifference,restoredDifference>=48);
        assertTrue("Physical rack changed only text, not actual 3D weapon pixels: "+
            rackDifference,rackDifference>=48);
        assertNotEquals("Rack switch did not change actual GLES2 framebuffer",
            machineCrc,selectedCrc);
        assertArrayEquals("Combat rack press must not replace durable shop checkpoint",
            purchased,Files.readAllBytes(save.toPath()));
        Log.i(TAG,"PASS Breakwater purchased Heavy rack GLES2 "+
            "purchase=true modelPixels="+rackDifference+
            " restoredPixels="+restoredDifference+
            " restoredCRC="+restoredCrc+" machineCRC="+machineCrc+
            " selectedCRC="+selectedCrc+" saveBytes="+purchased.length);
    }

    @Test public void captureAllNativeMainMenus() throws Exception {
        Context app=InstrumentationRegistry.getInstrumentation().getTargetContext();
        File root=new File(app.getCacheDir(),"native-menu-ci-"+System.nanoTime());
        assertTrue("Could not allocate game menu capture sandbox",root.mkdirs());
        File home=new File(root,"native-modules");
        assertTrue("Could not allocate isolated native game saves",home.mkdirs());
        java.util.List<NativeModuleRuntime.Descriptor> built=
            NativeModuleRuntime.listBuiltIns(true);
        StringBuilder rows=new StringBuilder("module_id,status,eye_frame_crc32,png_bytes\n");
        StringBuilder failures=new StringBuilder();
        int saved=0;
        try(Pbuffer egl=new Pbuffer()){
            for(GameModuleRegistry.Entry game:GameModuleRegistry.GAMES){
                boolean present=false;
                for(NativeModuleRuntime.Descriptor item:built)
                    if(game.id.equals(item.id))present=true;
                if(!present){
                    rows.append(game.id).append(",NOT_REGISTERED,0,0\n");
                    failures.append(game.id).append(": missing native library entry; ");
                    continue;
                }
                File evidence=new File(app.getFilesDir(),
                    "native-menu-"+game.id+".png");
                VirtualInputBus galleryBus=("omega-front".equals(game.id) ||
                    "breakwater-battery".equals(game.id))?
                    new VirtualInputBus():null;
                try(NativeModuleRuntime runtime=new NativeModuleRuntime(
                    home,app.getAssets(),galleryBus,true,
                    new ReveriePreferences(
                        InstrumentationRegistry.getInstrumentation().getContext()))){
                    assertTrue("Native menu launch failed "+game.id+": "+
                        runtime.getLastError(),runtime.start(game.id));
                    try{
                        assertTrue("GLES2 context failed "+game.id+": "+
                            runtime.getLastError(),runtime.onSurfaceCreated());
                        runtime.update(NativeModuleRuntime.POINTER_NONE,null,null);
                        long crc=omegaFrame(runtime,evidence);
                        assertTrue("Empty real frame "+game.id,
                            evidence.isFile() && evidence.length()>300L);
                        if("breakwater-battery".equals(game.id))
                            verifyBreakwaterPurchasedRack(runtime,galleryBus,home,app);
                        rows.append(game.id).append(",CAPTURED,")
                            .append(crc).append(",").append(evidence.length())
                            .append("\n");
                        saved++;
                        Log.i(TAG,"PASS REAL native menu capture id="+game.id+
                            " crc32="+crc+" bytes="+evidence.length());
                        if("omega-front".equals(game.id)){
                            // Visual-only diagnostic: real gallery navigation,
                            // read_asset + GLES2 texture upload + pixel captures.
                            // Failures must not block playable game releases.
                            try{
                                omegaTap(runtime,galleryBus,0.f,.195f);
                                File first=new File(app.getFilesDir(),
                                    "omega-front-gallery-f06.png");
                                long f06=omegaFrame(runtime,first);
                                omegaTap(runtime,galleryBus,-.54f,.56f);
                                File last=new File(app.getFilesDir(),
                                    "omega-front-gallery-f10.png");
                                long f10=omegaFrame(runtime,last);
                                Log.i(TAG,"DIAGNOSTIC Omega Front gallery "+
                                    "title="+crc+" f06="+f06+" f10="+f10+
                                    " firstBytes="+first.length()+
                                    " lastBytes="+last.length());
                                if(f06==crc || f10==f06)
                                    Log.w(TAG,"DIAGNOSTIC Omega Front gallery "+
                                        "portrait frames did not change");
                            }catch(Exception | AssertionError failure){
                                Log.w(TAG,"DIAGNOSTIC Omega Front gallery "+
                                    "capture unavailable (nonblocking)",failure);
                            }
                        }
                    }finally{
                        runtime.releaseSurface();
                        runtime.stop();
                    }
                }catch(Exception | AssertionError failure){
                    rows.append(game.id).append(",FAILED,0,0\n");
                    failures.append(game.id).append(": ")
                        .append(failure.toString().replace(',', ' '))
                        .append("; ");
                    Log.e(TAG,"FAIL native menu capture id="+game.id,failure);
                }
            }
        }finally{
            try(FileOutputStream csv=app.openFileOutput(
                    "native-menu-capture.csv",Context.MODE_PRIVATE)){
                csv.write(rows.toString().getBytes(StandardCharsets.UTF_8));
                csv.getFD().sync();
            }
            remove(root);
        }
        assertEquals("A native game menu was not captured: "+failures,
            GameModuleRegistry.GAMES.length,saved);
        Log.i(TAG,"PASS all "+saved+" registered native game initial menus "
            +"captured from installed Android GLES2 implementation");
    }



    // Script the very same controller ray and primary input as the headset.
    // Reject an interaction which does not reach its real native target.
    private static int redLedgerSelect(NativeModuleRuntime runtime,
                                      VirtualInputBus controls,float x,float y){
        float[] origin={x,y,0.f},direction={0.f,0.f,-1.f};
        VirtualOutput select=VirtualOutput.joystickButton(0);
        runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
            origin,direction);
        controls.applyDigital(select,true);
        int feedback=runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
            origin,direction);
        controls.applyDigital(select,false);
        runtime.update(NativeModuleRuntime.POINTER_VIRTUAL_CONTROLLER,
            origin,direction);
        return feedback;
    }

    // Count differences in the actual foreground inspection region, not
    // differences in HUD text that could make a broken model appear to pass.
    private static int redLedgerPreviewPixelChanges(Bitmap a,Bitmap b){
        int changed=0;
        for(int y=310;y<440;++y)for(int x=330;x<465;++x){
            int lhs=a.getPixel(x,y),rhs=b.getPixel(x,y);
            int delta=Math.abs(Color.red(lhs)-Color.red(rhs))
                +Math.abs(Color.green(lhs)-Color.green(rhs))
                +Math.abs(Color.blue(lhs)-Color.blue(rhs));
            if(delta>=60)changed++;
        }
        return changed;
    }

    // This is the real saved/game-controlled SKU: 505Snacks.fbx and its
    // 1024px owner texture, not a generated proof image or thumbnail.
    private static void verifyRedLedger505VisibleOnAndroidGles()
        throws Exception {
        Context app=InstrumentationRegistry.getInstrumentation().getTargetContext();
        File root=new File(app.getCacheDir(),
            "red-ledger-505-ci-"+System.nanoTime());
        assertTrue("Cannot allocate isolated Red Ledger test storage",root.mkdirs());
        File home=new File(root,"native-modules");
        assertTrue("Cannot allocate Red Ledger game save home",home.mkdirs());
        File selected=new File(app.getFilesDir(),"red-ledger-505-selected.png");
        File alternate=new File(app.getFilesDir(),"red-ledger-505-other.png");
        File reselected=new File(app.getFilesDir(),"red-ledger-505-reselected.png");
        File restored=new File(app.getFilesDir(),"red-ledger-505-restored.png");
        File saved=new File(home,"between-deliveries-red-ledger/state-v4.bin");
        try(Pbuffer egl=new Pbuffer()){
            VirtualInputBus controls=new VirtualInputBus();
            try(NativeModuleRuntime game=new NativeModuleRuntime(
                home,app.getAssets(),controls,true,
                new ReveriePreferences(
                    InstrumentationRegistry.getInstrumentation().getContext()))){
                assertTrue("Red Ledger native launch failed: "+game.getLastError(),
                    game.start("between-deliveries-red-ledger"));
                assertTrue("Red Ledger GLES2 initialization failed: "+game.getLastError(),
                    game.onSurfaceCreated());
                try{
                // Door to stockroom: physical AABB (2.53,-.20,-2.93).
                assertTrue("Could not enter genuine stockroom",
                    (redLedgerSelect(game,controls,2.53f,-.20f)
                     &NativeModuleRuntime.FEEDBACK_ACTIVATION)!=0);
                // Card 22 (zero-based): supplier 505, snack row, offer 23.
                assertTrue("505 snack card did not select offer 23",
                    (redLedgerSelect(game,controls,1.23f,-.86f)
                     &NativeModuleRuntime.FEEDBACK_ACTIVATION)!=0);
                long selectedCrc=omegaFrame(game,selected);
                // Card 21: neighboring supplier 404 snack, offer 22.
                assertTrue("Neighbor snack card was not selectable",
                    (redLedgerSelect(game,controls,.41f,-.86f)
                     &NativeModuleRuntime.FEEDBACK_ACTIVATION)!=0);
                long otherCrc=omegaFrame(game,alternate);
                assertNotEquals("Changing SKU did not change real GLES2 frame",
                    selectedCrc,otherCrc);
                Bitmap first=BitmapFactory.decodeFile(selected.getAbsolutePath());
                Bitmap second=BitmapFactory.decodeFile(alternate.getAbsolutePath());
                assertNotNull("No selected-product framebuffer PNG",first);
                assertNotNull("No alternate-product framebuffer PNG",second);
                try{
                    // The podium projects to the lower-right of the 512px
                    // camera. Require material change THERE, not only HUD text.
                    int changed=redLedgerPreviewPixelChanges(first,second);
                    Log.i(TAG,"DIAGNOSTIC native 505 pixels="+changed
                        +" selectedCRC="+selectedCrc+" otherCRC="+otherCrc);
                    assertTrue("Selected original product did not alter podium "
                        +"GLES2 pixels (changed="+changed+")",changed>=32);
                }finally{first.recycle();second.recycle();}
                assertTrue("505 snack could not be reselected",
                    (redLedgerSelect(game,controls,1.23f,-.86f)
                     &NativeModuleRuntime.FEEDBACK_ACTIVATION)!=0);
                long again=omegaFrame(game,reselected);
                assertNotEquals("Reselection did not redraw original SKU",
                    otherCrc,again);
                Bitmap otherFrame=BitmapFactory.decodeFile(alternate.getAbsolutePath());
                Bitmap againFrame=BitmapFactory.decodeFile(reselected.getAbsolutePath());
                assertNotNull("Missing alternate product frame",otherFrame);
                assertNotNull("Missing reselected product frame",againFrame);
                try{
                    assertTrue("Reselection changed text but not real podium geometry",
                        redLedgerPreviewPixelChanges(otherFrame,againFrame)>=32);
                }finally{otherFrame.recycle();againFrame.recycle();}
                assertTrue("No durable native stock selection",saved.isFile()
                    &&saved.length()>100L);
                Log.i(TAG,"PASS Red Ledger original 505 selected/reselected "
                    +"native GLES2 realFrames=true saveBytes="+saved.length()
                    +" selectedCRC="+selectedCrc+" otherCRC="+otherCrc
                    +" reselectedCRC="+again);
                }finally{
                    game.releaseSurface();
                    game.stop();
                }
            }
            // A distinct JNI instance must restore the selected offer;
            // clicking the same card then fails rather than changing state.
            VirtualInputBus resumedControls=new VirtualInputBus();
            try(NativeModuleRuntime resumed=new NativeModuleRuntime(
                home,app.getAssets(),resumedControls,true,
                new ReveriePreferences(
                    InstrumentationRegistry.getInstrumentation().getContext()))){
                assertTrue("Saved native Red Ledger launch failed: "
                    +resumed.getLastError(),
                    resumed.start("between-deliveries-red-ledger"));
                assertTrue("Saved Red Ledger GLES2 context failed: "
                    +resumed.getLastError(),resumed.onSurfaceCreated());
                try{
                long restoredCrc=omegaFrame(resumed,restored);
                int same=redLedgerSelect(resumed,resumedControls,1.23f,-.86f);
                assertTrue("505 selection did not survive native save/reopen",
                    (same&NativeModuleRuntime.FEEDBACK_FAILURE)!=0
                    &&(same&NativeModuleRuntime.FEEDBACK_ACTIVATION)==0);
                assertTrue("Saved selected SKU screenshot missing",
                    restored.isFile()&&restored.length()>300L);
                Log.i(TAG,"PASS Red Ledger 505 save/reopen real Android GLES2 "
                    +"restoredCRC="+restoredCrc+" bytes="+restored.length());
                }finally{
                    resumed.releaseSurface();
                    resumed.stop();
                }
            }
        }finally{remove(root);}
    }


    // Actual installed JNI/GLES2 acceptance for the newly imported sixth supplier.
    // 606 must render differently from the previously validated 505 model on
    // the inspection podium, support a real stock purchase, and survive a
    // distinct native-module process opening the same persisted game save.
    private static void verifyRedLedger606VisibleOnAndroidGles()
        throws Exception {
        Context app=InstrumentationRegistry.getInstrumentation().getTargetContext();
        File root=new File(app.getCacheDir(),
            "red-ledger-606-ci-"+System.nanoTime());
        assertTrue("Cannot allocate isolated 606 test storage",root.mkdirs());
        File home=new File(root,"native-modules");
        assertTrue("Cannot allocate 606 game save home",home.mkdirs());
        File comparison=new File(app.getFilesDir(),"red-ledger-606-compare-505.png");
        File selected=new File(app.getFilesDir(),"red-ledger-606-selected.png");
        File stocked=new File(app.getFilesDir(),"red-ledger-606-stocked.png");
        File restored=new File(app.getFilesDir(),"red-ledger-606-restored.png");
        File saved=new File(home,"between-deliveries-red-ledger/state-v4.bin");
        try(Pbuffer egl=new Pbuffer()){
            VirtualInputBus controls=new VirtualInputBus();
            try(NativeModuleRuntime game=new NativeModuleRuntime(
                home,app.getAssets(),controls,true,
                new ReveriePreferences(
                    InstrumentationRegistry.getInstrumentation().getContext()))){
                assertTrue("606 game launch failed: "+game.getLastError(),
                    game.start("between-deliveries-red-ledger"));
                assertTrue("606 GLES2 context failed: "+game.getLastError(),
                    game.onSurfaceCreated());
                try{
                    assertTrue("606 test could not enter stockroom",
                        (redLedgerSelect(game,controls,2.53f,-.20f)
                         &NativeModuleRuntime.FEEDBACK_ACTIVATION)!=0);
                    // Sixth supplier snack card: slot 23, offer 24.
                    // Neighbor 505 snack card: slot 22, offer 23.
                    assertTrue("Could not select authentic 505 comparison SKU",
                        (redLedgerSelect(game,controls,1.23f,-.86f)
                         &NativeModuleRuntime.FEEDBACK_ACTIVATION)!=0);
                    long referenceCrc=omegaFrame(game,comparison);
                    assertTrue("606 snack card did not activate offer 24",
                        (redLedgerSelect(game,controls,2.05f,-.86f)
                         &NativeModuleRuntime.FEEDBACK_ACTIVATION)!=0);
                    long selectedCrc=omegaFrame(game,selected);
                    assertNotEquals("606 versus 505 did not alter GLES2 output",
                        referenceCrc,selectedCrc);
                    Bitmap before=BitmapFactory.decodeFile(comparison.getAbsolutePath());
                    Bitmap after=BitmapFactory.decodeFile(selected.getAbsolutePath());
                    assertNotNull("Missing actual 505 comparison PNG",before);
                    assertNotNull("Missing actual 606 selected PNG",after);
                    try{
                        int changed=redLedgerPreviewPixelChanges(before,after);
                        Log.i(TAG,"DIAGNOSTIC Red Ledger 606-vs-505 podiumPixels="
                            +changed+" sku23CRC="+referenceCrc
                            +" sku24CRC="+selectedCrc);
                        assertTrue("606 differs only in text, not visible stock "
                            +"podium model/texture (pixels="+changed+")",
                            changed>=32);
                    }finally{before.recycle();after.recycle();}
                    // The original game begins with enough cash for the 700c
                    // sixth-supplier snack lot. Use the real stock-buy collider.
                    assertTrue("606 wholesale lot was not actually purchased",
                        (redLedgerSelect(game,controls,.40f,-.48f)
                         &NativeModuleRuntime.FEEDBACK_ACTIVATION)!=0);
                    long stockedCrc=omegaFrame(game,stocked);
                    assertTrue("606 purchase did not produce a durable save",
                        saved.isFile()&&saved.length()>100L);
                    Log.i(TAG,"DIAGNOSTIC Red Ledger 606 stock purchase "
                        +"sku24CRC="+stockedCrc+" saveBytes="+saved.length());
                }finally{
                    game.releaseSurface();
                    game.stop();
                }
            }
            VirtualInputBus resumedControls=new VirtualInputBus();
            try(NativeModuleRuntime resumed=new NativeModuleRuntime(
                home,app.getAssets(),resumedControls,true,
                new ReveriePreferences(
                    InstrumentationRegistry.getInstrumentation().getContext()))){
                assertTrue("606 save/reopen native launch failed: "
                    +resumed.getLastError(),
                    resumed.start("between-deliveries-red-ledger"));
                assertTrue("606 save/reopen GLES2 context failed: "
                    +resumed.getLastError(),resumed.onSurfaceCreated());
                try{
                    long restoredCrc=omegaFrame(resumed,restored);
                    int same=redLedgerSelect(resumed,resumedControls,2.05f,-.86f);
                    assertTrue("Saved SKU24 selection was not restored",
                        (same&NativeModuleRuntime.FEEDBACK_FAILURE)!=0
                        &&(same&NativeModuleRuntime.FEEDBACK_ACTIVATION)==0);
                    assertTrue("606 reopened real GLES2 image was not captured",
                        restored.isFile()&&restored.length()>300L);
                    Log.i(TAG,"PASS Red Ledger original 606 selected/purchased/"
                        +"save-reopen native GLES2 realFrames=true "
                        +"saveBytes="+saved.length()
                        +" restoredCRC="+restoredCrc);
                }finally{
                    resumed.releaseSurface();
                    resumed.stop();
                }
            }
        }finally{remove(root);}
    }

    @Test public void redLedgerReal24DownloadedIconsRenderThroughNativeGles()
        throws Exception {
        runInstalledAssets("between-deliveries-red-ledger",24);
        verifyRedLedger505VisibleOnAndroidGles();
        verifyRedLedger606VisibleOnAndroidGles();
    }
    @Test public void omegaFrontDownloadedTexturesRenderThroughNativeGles(){
        // The 141-file fixture selects exactly two shared concrete/steel
        // textures in place of deleted Omega-private copies.
        runInstalledAssets("omega-front",141);
    }
}
