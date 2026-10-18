package io.github.mrcalzon02.reverievr.game.breakwater;

import android.app.Activity;
import android.content.Intent;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.opengl.GLES20;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;
import com.google.cardboard.sdk.CardboardView;
import com.google.cardboard.sdk.HeadTransform;
import com.google.cardboard.sdk.Viewport;
import java.util.concurrent.atomic.AtomicBoolean;
import javax.microedition.khronos.egl.EGLConfig;

/** Real, independently installed Breakwater Battery VR game.
 * Uses the same C++ gameplay rules, sound bank and game assets as the hub;
 * deliberately has separate private save slots and Android lifecycle.
 */
public final class GameVrActivity extends Activity
        implements CardboardView.Renderer {
    private static final String TAG="BreakwaterCompanion";
    private static final String GAME_ID="breakwater-battery";
    private static final String HUB="io.github.mrcalzon02.reverievr";
    private static final float NEAR=.08f,FAR=200.f;
    private static final boolean NATIVE_AVAILABLE;
    static {
        boolean ready;
        try{
            System.loadLibrary("reverie_companion_breakwater");
            ready=true;
        }catch(UnsatisfiedLinkError error){
            Log.e(TAG,"Native Breakwater game library missing",error);
            ready=false;
        }
        NATIVE_AVAILABLE=ready;
    }
    private static native long nativeCreate(android.content.res.AssetManager manager,
        String privateFiles);
    private static native boolean nativeSurfaceCreated(long handle);
    private static native void nativeSurfaceReleased(long handle);
    private static native void nativeResume(long handle);
    private static native void nativePause(long handle);
    private static native long nativeUpdate(long handle,float delta,
        boolean primary,boolean secondary,float moveX,float moveY,float[] head);
    private static native boolean nativeRenderEye(long handle,int eye,
        float[] view,float[] projection);
    private static native void nativeDestroy(long handle);

    private final Object gameLock=new Object();
    private final AtomicBoolean oneShot=new AtomicBoolean();
    private final AtomicBoolean failed=new AtomicBoolean();
    private final float[] headView=new float[16];
    private CardboardView viewer;
    private NativeCombatSound sound;
    private AudioManager audioManager;
    private AudioFocusRequest audioRequest;
    private volatile boolean focused;
    private volatile boolean triggerHeld,secondaryHeld;
    private volatile float moveX,moveY;
    private volatile boolean ready;
    private long handle;
    private long lastFrameNanos;
    private int eyesRendered;

    @Override protected void onCreate(Bundle saved){
        super.onCreate(saved);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN);
        immersive();
        Intent intent=getIntent();
        if(intent!=null&&intent.hasExtra("reverie_game_id")&&
           (!GAME_ID.equals(intent.getStringExtra("reverie_game_id"))||
            intent.getIntExtra("reverie_protocol",-1)!=1)){
            failure("Hub/game launch protocol mismatch");return;
        }
        if(!NATIVE_AVAILABLE){failure("Native Breakwater library missing");return;}
        synchronized(gameLock){
            handle=nativeCreate(getAssets(),getFilesDir().getAbsolutePath());
        }
        if(handle==0L){failure("Native Breakwater could not start");return;}
        // Reuse the original 26-cue static PCM game audio engine (no
        // synthesized-in-the-frame fallback or replacement menu click).
        sound=new NativeCombatSound();
        audioManager=(AudioManager)getSystemService(AUDIO_SERVICE);
        if(audioManager!=null){
            AudioAttributes attrs=new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build();
            audioRequest=new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
                .setAudioAttributes(attrs)
                .setOnAudioFocusChangeListener(change->{
                    focused=change==AudioManager.AUDIOFOCUS_GAIN;
                    if(!focused&&sound!=null)sound.stop();
                }).build();
        }
        CardboardView.setUseCardboardGlSurfaceView(true);
        viewer=new CardboardView(this);
        viewer.setStereoRenderMode(true);
        viewer.setRenderer(this);
        viewer.setOnTriggerEvent(()->oneShot.set(true));
        setContentView(viewer);
    }

    private void immersive(){
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|
            View.SYSTEM_UI_FLAG_FULLSCREEN|View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }
    private void failure(String error){
        if(!failed.compareAndSet(false,true))return;
        Log.e(TAG,error);
        runOnUiThread(()->{
            Toast.makeText(this,error,Toast.LENGTH_LONG).show();
            finish();
        });
    }
    private static boolean primaryKey(int code){
        return code==KeyEvent.KEYCODE_DPAD_CENTER||
            code==KeyEvent.KEYCODE_BUTTON_A||
            code==KeyEvent.KEYCODE_BUTTON_1||
            code==KeyEvent.KEYCODE_ENTER||
            code==KeyEvent.KEYCODE_SPACE;
    }
    private static boolean secondaryKey(int code){
        return code==KeyEvent.KEYCODE_BUTTON_B||
            code==KeyEvent.KEYCODE_BUTTON_2;
    }
    @Override public boolean onKeyDown(int key,KeyEvent event){
        if(primaryKey(key)){triggerHeld=true;return true;}
        if(secondaryKey(key)){secondaryHeld=true;return true;}
        return super.onKeyDown(key,event);
    }
    @Override public boolean onKeyUp(int key,KeyEvent event){
        if(primaryKey(key)){triggerHeld=false;return true;}
        if(secondaryKey(key)){secondaryHeld=false;return true;}
        return super.onKeyUp(key,event);
    }
    @Override public boolean onTouchEvent(MotionEvent event){
        switch(event.getActionMasked()){
            case MotionEvent.ACTION_DOWN:triggerHeld=true;return true;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:triggerHeld=false;return true;
            default:return super.onTouchEvent(event);
        }
    }
    @Override public boolean onGenericMotionEvent(MotionEvent event){
        if((event.getSource()&android.view.InputDevice.SOURCE_JOYSTICK)!=0&&
           event.getActionMasked()==MotionEvent.ACTION_MOVE){
            moveX=event.getAxisValue(MotionEvent.AXIS_X);
            moveY=event.getAxisValue(MotionEvent.AXIS_Y);
            return true;
        }
        if(event.getActionMasked()==MotionEvent.ACTION_BUTTON_PRESS&&
           (event.getActionButton()&MotionEvent.BUTTON_PRIMARY)!=0){
            oneShot.set(true);return true;
        }
        return super.onGenericMotionEvent(event);
    }
    private void requestGameAudio(){
        if(audioManager==null||audioRequest==null)return;
        focused=audioManager.requestAudioFocus(audioRequest)==
            AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
        if(!focused)Log.w(TAG,"Android audio focus unavailable");
    }
    @Override protected void onResume(){
        super.onResume();
        immersive();
        lastFrameNanos=0L;
        requestGameAudio();
        if(sound!=null)sound.resumeForLifecycle();
        synchronized(gameLock){if(handle!=0L)nativeResume(handle);}
        if(viewer!=null)viewer.onResume();
    }
    @Override protected void onPause(){
        if(viewer!=null)viewer.onPause();
        triggerHeld=false;secondaryHeld=false;moveX=moveY=0.f;
        if(sound!=null)sound.stop();
        if(audioManager!=null&&audioRequest!=null)
            audioManager.abandonAudioFocusRequest(audioRequest);
        focused=false;
        synchronized(gameLock){if(handle!=0L)nativePause(handle);}
        super.onPause();
    }
    @Override protected void onDestroy(){
        if(viewer!=null)viewer.onPause();
        synchronized(gameLock){
            if(handle!=0L){nativeDestroy(handle);handle=0L;}
        }
        if(sound!=null){sound.close();sound=null;}
        super.onDestroy();
    }
    @Override public void onNewFrame(HeadTransform pose){
        final long now=System.nanoTime();
        float dt=lastFrameNanos==0L?0.f:
            Math.min(.05f,Math.max(0.f,(now-lastFrameNanos)*1.0e-9f));
        lastFrameNanos=now;
        pose.getHeadView(headView,0);
        long cues;
        synchronized(gameLock){
            if(handle==0L)return;
            cues=nativeUpdate(handle,dt,triggerHeld||oneShot.getAndSet(false),
                secondaryHeld,moveX,moveY,headView);
        }
        if(cues!=0L){
            final long events=cues;
            runOnUiThread(()->{
                if(sound==null||!focused)return;
                for(int i=0;i<8;i++){
                    int cue=(int)((events>>(8*i))&255L);
                    if(cue==0)break;
                    sound.playNativeCue(cue);
                }
            });
        }
    }
    @Override public void onDrawEye(CardboardView.Eye eye){
        // Required by this repository's pinned Cardboard library. Without
        // applying headView the renderer receives an all-zero eye matrix.
        eye.applyHeadView(headView);
        int[] viewport=new int[4],prior=new int[4];
        GLES20.glGetIntegerv(GLES20.GL_VIEWPORT,viewport,0);
        final boolean scissorWasEnabled=GLES20.glIsEnabled(GLES20.GL_SCISSOR_TEST);
        if(scissorWasEnabled)GLES20.glGetIntegerv(GLES20.GL_SCISSOR_BOX,prior,0);
        GLES20.glEnable(GLES20.GL_SCISSOR_TEST);
        GLES20.glScissor(viewport[0],viewport[1],viewport[2],viewport[3]);
        GLES20.glClearColor(.06f,.12f,.20f,1.f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT|GLES20.GL_DEPTH_BUFFER_BIT);
        if(scissorWasEnabled)GLES20.glScissor(prior[0],prior[1],prior[2],prior[3]);
        else GLES20.glDisable(GLES20.GL_SCISSOR_TEST);
        synchronized(gameLock){
            if(handle==0L||!ready)return;
            int eyeIndex=eye.getEyeType()==CardboardView.Eye.LEFT?0:1;
            boolean rendered=nativeRenderEye(handle,eyeIndex,
                eye.getEyeView(),eye.getPerspective(NEAR,FAR));
            if(!rendered)failure("Breakwater GLES2 rendering failed");
        }
        ++eyesRendered;
        if(eyesRendered==2||eyesRendered%600==0)
            Log.i(TAG,"Native stereo eyes rendered "+eyesRendered);
    }
    @Override public void onSurfaceCreated(EGLConfig config){
        synchronized(gameLock){
            ready=handle!=0L&&nativeSurfaceCreated(handle);
        }
        if(!ready)failure("Breakwater GPU initialization failed");
    }
    @Override public void onSurfaceChanged(int width,int height){
        GLES20.glViewport(0,0,width,height);
    }
    @Override public void onFinishFrame(Viewport viewport){}
    @Override public void onRendererShutdown(){
        synchronized(gameLock){
            if(handle!=0L&&ready)nativeSurfaceReleased(handle);
            ready=false;
        }
    }
    @Override public void onBackPressed(){
        Intent hub=getPackageManager().getLaunchIntentForPackage(HUB);
        if(hub!=null){
            try{startActivity(hub);}catch(RuntimeException failed){
                Log.w(TAG,"ReverieVR hub could not resume",failed);
            }
        }
        finish();
    }
}
