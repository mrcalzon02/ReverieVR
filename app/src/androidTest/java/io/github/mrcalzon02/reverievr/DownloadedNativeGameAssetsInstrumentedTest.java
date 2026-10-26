package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
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

    private static int packageFolder(ZipOutputStream output,AssetManager assets,
                                     String folder)throws IOException {
        int files=0;
        String[] names=assets.list(folder);
        if(names==null)throw new IOException("Missing assets directory "+folder);
        for(String name:names){
            if(!name.matches("[A-Za-z0-9_.-]+") ||
               !(name.endsWith(".png")||name.endsWith(".json")))
                throw new IOException("Invalid fixture path "+name);
            String path=folder+"/"+name;
            byte[] data=sourceAsset(assets,path);
            assertTrue("Empty game asset "+path,data.length>0);
            output.putNextEntry(new ZipEntry("assets/"+path));
            output.write(data);
            output.closeEntry();
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
        float[] view=new float[16],projection=new float[16];
        Matrix.setIdentityM(view,0);
        Matrix.perspectiveM(projection,0,80.f,1.f,0.08f,150.f);
        GLES20.glClearColor(0,0,0,1);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT|GLES20.GL_DEPTH_BUFFER_BIT);
        for(int eye=0;eye<2;eye++)
            assertTrue("Omega Front native eye rendering failed: "+
                runtime.getLastError(),runtime.renderEye(eye,view,projection));
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
        File screenshot=new File(app.getFilesDir(),"omega-front-ci-tactical.png");
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
            long sector=omegaFrame(runtime,null);
            // Sector 11 (x=2,y=2) is the first real deployment mission.
            long travelSound=omegaTap(runtime,bus,.1975f,-.1775f);
            long traveled=omegaFrame(runtime,null);
            assertNotEquals("Strategic travel did not update real 2D pixels",
                sector,traveled);
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
            File saved=new File(home,"omega-front/frontier.v1");
            assertTrue("No actual native campaign checkpoint",saved.isFile());
            assertEquals("Native campaign checkpoint size",2048L,saved.length());
            runtime.releaseSurface();
            runtime.stop();
            assertTrue("Cannot relaunch saved native Omega Front",
                runtime.start("omega-front"));
            assertTrue("Cannot restore Omega Front GLES2 after relaunch",
                runtime.onSurfaceCreated());
            long restored=omegaFrame(runtime,null);
            assertEquals("Native reload changed the saved 2D battle view",
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
                try(NativeModuleRuntime runtime=new NativeModuleRuntime(
                    home,app.getAssets(),null,true,
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
                        rows.append(game.id).append(",CAPTURED,")
                            .append(crc).append(",").append(evidence.length())
                            .append("\n");
                        saved++;
                        Log.i(TAG,"PASS REAL native menu capture id="+game.id+
                            " crc32="+crc+" bytes="+evidence.length());
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


    @Test public void redLedgerReal24DownloadedIconsRenderThroughNativeGles(){
        runInstalledAssets("between-deliveries-red-ledger",24);
    }
    @Test public void omegaFrontDownloadedTexturesRenderThroughNativeGles(){
        // 76 original files plus 64 genuinely CC0 16px art files and
        // one licensing manifest, all loaded through the native asset scope.
        runInstalledAssets("omega-front",141);
    }
}
