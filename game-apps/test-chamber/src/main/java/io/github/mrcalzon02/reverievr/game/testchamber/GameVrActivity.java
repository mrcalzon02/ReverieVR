package io.github.mrcalzon02.reverievr.game.testchamber;

import android.app.Activity;
import android.content.Intent;
import android.opengl.GLES20;
import android.os.Bundle;
import android.view.View;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;
import com.google.cardboard.sdk.CardboardView;
import com.google.cardboard.sdk.HeadTransform;
import com.google.cardboard.sdk.Viewport;
import java.util.concurrent.atomic.AtomicBoolean;
import javax.microedition.khronos.egl.EGLConfig;

/**
 * Small independent APK exercising the actual shared native Test Chamber.
 * It uses Cardboard stereoscopic eye transforms; no desktop or VR shell is
 * necessary after this Android package has been installed. This does NOT
 * yet replace the production hub/controller/runtime integration.
 */
public final class GameVrActivity extends Activity
        implements CardboardView.Renderer {
    private static final String GAME_ID="procedural-test-chamber";
    private static final String HOST_PACKAGE="io.github.mrcalzon02.reverievr";
    private static final float NEAR=0.08f, FAR=120.f;
    private static final boolean AVAILABLE;
    static {
        boolean loaded;
        try{
            System.loadLibrary("reverie_companion_test_chamber");
            loaded=true;
        }catch(UnsatisfiedLinkError failure){
            loaded=false;
        }
        AVAILABLE=loaded;
    }
    private static native long nativeCreate();
    private static native boolean nativeAddModelMesh(long handle,int modelId,
        float[] vertices,int width,int height,int[] argb);
    private static native void nativeSetFocusedModel(long handle,int modelId);
    private static native boolean nativeSurfaceCreated(long handle);
    private static native void nativeSurfaceReleased(long handle);
    private static native void nativeResume(long handle);
    private static native void nativePause(long handle);
    private static native void nativeUpdate(long handle,float seconds,
                                            boolean primaryPressed);
    private static native boolean nativeRenderEye(long handle,int eyeIndex,
                                                   float[] view,float[] projection);
    private static native void nativeDestroy(long handle);

    private final Object nativeLock=new Object();
    private final AtomicBoolean pendingTrigger=new AtomicBoolean();
    private final float[] headView=new float[16];
    private volatile int focusedModel=-1;
    private CardboardView viewer;
    private TestChamberSounds audio;
    private long instance;
    private long previousFrameNanos;
    private volatile boolean surfaceReady;
    private final AtomicBoolean failureReported=new AtomicBoolean();

    @Override protected void onCreate(Bundle saved){
        super.onCreate(saved);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                             WindowManager.LayoutParams.FLAG_FULLSCREEN);
        immersive();
        final Intent requested=getIntent();
        if(requested!=null && requested.hasExtra("reverie_game_id") &&
           (!GAME_ID.equals(requested.getStringExtra("reverie_game_id")) ||
            requested.getIntExtra("reverie_protocol",-1)!=1)){
            showFailure("Companion launch protocol mismatch");
            return;
        }
        if(!AVAILABLE){
            showFailure("Native Test Chamber library unavailable");
            return;
        }
        synchronized(nativeLock){instance=nativeCreate();}
        if(instance==0L){
            showFailure("Native Test Chamber could not initialize");
            return;
        }
        // Fail visibly if the APK lacks any purchased game models/textures.
        // Do not disguise an incomplete build by displaying only cube props.
        try{
            java.util.List<TestChamberModels.Primitive> meshes=
                TestChamberModels.load(getAssets());
            for(TestChamberModels.Primitive primitive:meshes){
                if(!nativeAddModelMesh(instance,primitive.modelId,
                    primitive.vertices,primitive.width,primitive.height,
                    primitive.argb))
                    throw new java.io.IOException(
                        "JNI/GLES2 asset staging failed for model "+primitive.modelId);
            }
            Log.i("TestChamberAssets","Staged "+meshes.size()+
                " textured primitives from five approved Beckenburg GLB models");
            audio=new TestChamberSounds(this,getAssets());
        }catch(Exception error){
            Log.e("TestChamberAssets","Cannot activate full test scene",error);
            showFailure("Missing game models or sound assets: "+error.getMessage());
            return;
        }
        CardboardView.setUseCardboardGlSurfaceView(true);
        viewer=new CardboardView(this);
        viewer.setStereoRenderMode(true);
        viewer.setRenderer(this);
        viewer.setOnTriggerEvent(this::triggerTestEffect);
        setContentView(viewer);
    }

    private void triggerTestEffect(){
        // Each real model is a selectable audio diagnostic station.
        // Looking at empty space plays the sixth beacon cue.
        pendingTrigger.set(true);
        if(audio!=null)audio.playForFocus(focusedModel);
    }
    @Override public boolean onTouchEvent(MotionEvent event){
        if(event.getActionMasked()==MotionEvent.ACTION_DOWN){
            triggerTestEffect();
            return true;
        }
        return super.onTouchEvent(event);
    }

    private static boolean isActivationKey(int key){
        // Non-Daydream Bluetooth gamepads and phone/VR remotes commonly
        // surface face click as a standard Android KeyEvent, not as a
        // Cardboard magnet/trigger callback. Never intercept Back/Home.
        return key==KeyEvent.KEYCODE_DPAD_CENTER ||
            key==KeyEvent.KEYCODE_BUTTON_A ||
            key==KeyEvent.KEYCODE_BUTTON_1 ||
            key==KeyEvent.KEYCODE_ENTER ||
            key==KeyEvent.KEYCODE_SPACE;
    }

    @Override public boolean onKeyDown(int key,KeyEvent event){
        if(isActivationKey(key)){
            // Do not play a new effect on every key-repeat tick.
            if(event.getRepeatCount()==0)triggerTestEffect();
            return true;
        }
        return super.onKeyDown(key,event);
    }

    @Override public boolean onKeyUp(int key,KeyEvent event){
        if(isActivationKey(key))return true;
        return super.onKeyUp(key,event);
    }

    @Override public boolean onGenericMotionEvent(MotionEvent event){
        if(event.getActionMasked()==MotionEvent.ACTION_BUTTON_PRESS &&
           (event.getActionButton()&MotionEvent.BUTTON_PRIMARY)!=0){
            triggerTestEffect();
            return true;
        }
        return super.onGenericMotionEvent(event);
    }

    private void immersive(){
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|
            View.SYSTEM_UI_FLAG_FULLSCREEN|
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }
    private void showFailure(String explanation){
        if(!failureReported.compareAndSet(false,true))return;
        runOnUiThread(()->{
            Toast.makeText(this,explanation,Toast.LENGTH_LONG).show();
            finish();
        });
    }
    @Override protected void onResume(){
        super.onResume();
        immersive();
        synchronized(nativeLock){if(instance!=0L)nativeResume(instance);}
        if(audio!=null)audio.resume();
        if(viewer!=null)viewer.onResume();
    }
    @Override protected void onPause(){
        if(viewer!=null)viewer.onPause();
        if(audio!=null)audio.pause();
        synchronized(nativeLock){if(instance!=0L)nativePause(instance);}
        super.onPause();
    }
    @Override protected void onDestroy(){
        if(viewer!=null)viewer.onPause();
        synchronized(nativeLock){
            if(instance!=0L){
                nativeDestroy(instance);
                instance=0L;
            }
        }
        if(audio!=null){
            audio.close();
            audio=null;
        }
        super.onDestroy();
    }
    @Override public void onNewFrame(HeadTransform pose){
        long now=System.nanoTime();
        float seconds=previousFrameNanos==0L?0.f:
            Math.max(0.f,Math.min(.05f,(now-previousFrameNanos)*1.0e-9f));
        previousFrameNanos=now;
        pose.getHeadView(headView,0);
        int target=TestChamberTargeting.pick(headView);
        focusedModel=target;
        synchronized(nativeLock){
            if(instance!=0L){
                nativeSetFocusedModel(instance,target);
                nativeUpdate(instance,seconds,pendingTrigger.getAndSet(false));
            }
        }
    }
    @Override public void onDrawEye(CardboardView.Eye eye){
        GLES20.glClearColor(.045f,.085f,.14f,1.f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT|GLES20.GL_DEPTH_BUFFER_BIT);
        synchronized(nativeLock){
            if(instance==0L||!surfaceReady)return;
            int eyeIndex=eye.getEyeType()==CardboardView.Eye.LEFT?0:1;
            if(!nativeRenderEye(instance,eyeIndex,
                eye.getEyeView(),eye.getPerspective(NEAR,FAR)))
                showFailure("Native game stereo render failed");
        }
    }
    @Override public void onFinishFrame(Viewport vp){}
    @Override public void onSurfaceChanged(int width,int height){
        GLES20.glViewport(0,0,width,height);
    }
    @Override public void onSurfaceCreated(EGLConfig config){
        synchronized(nativeLock){
            surfaceReady=instance!=0L&&nativeSurfaceCreated(instance);
        }
        if(!surfaceReady)showFailure("Native game GL resources failed");
    }
    @Override public void onRendererShutdown(){
        synchronized(nativeLock){
            if(instance!=0L&&surfaceReady)nativeSurfaceReleased(instance);
            surfaceReady=false;
        }
    }
    @Override public void onBackPressed(){
        Intent hub=getPackageManager().getLaunchIntentForPackage(HOST_PACKAGE);
        if(hub!=null){
            try{startActivity(hub);}catch(RuntimeException ignored){}
        }
        finish();
    }
}
