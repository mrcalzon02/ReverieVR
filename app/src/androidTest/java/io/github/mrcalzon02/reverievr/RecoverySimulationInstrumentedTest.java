package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.util.Log;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Test;
import org.junit.runner.RunWith;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import static org.junit.Assert.*;

/**
 * Android-emulator recovery lab: REAL production state policies, SYNTHETIC
 * controller packet timeline. No claim of physical Daydream BLE/IMU coverage.
 * The debug APK is upgraded in place under the same Android package and signer.
 */
@RunWith(AndroidJUnit4.class)
public final class RecoverySimulationInstrumentedTest {
    private static final String TAG="RecoverySimulation";
    private static final long SECOND=1_000_000_000L;
    private static final String PROBE="reverie-recovery-upgrade-sentinel-v1";
    private static Context app(){
        return InstrumentationRegistry.getInstrumentation().getTargetContext();
    }

    private static void row(StringBuilder trace,GazeFallbackPolicy policy,
                            String name,long now,boolean linked,
                            boolean poseValid,long poseAt,VrPointerMode mode,
                            boolean expectedGaze) {
        boolean live=ControllerPoseVisibility.live(linked,poseValid,poseAt,now);
        boolean held=ControllerPoseVisibility.holdModel(
            linked,poseValid,poseAt,now);
        boolean gaze=policy.shouldUseGaze(mode,live,linked,now);
        assertEquals("Unexpected headset gaze projection at "+name,
                     expectedGaze,gaze);
        if(!linked)assertFalse("Disconnected controller mesh held",held);
        if(held)assertFalse("Stale controller treated as clickable",live);
        String line=name+","+now+","+linked+","+live+","+held+","+gaze;
        trace.append(line).append('\n');
        Log.i(TAG,"TRACE "+line);
    }

    @Test public void controllerDropoutReconnectAndGazeCapture()
        throws Exception {
        final long t=10*SECOND;
        GazeFallbackPolicy policy=new GazeFallbackPolicy();
        StringBuilder timeline=new StringBuilder(
            "phase,elapsed_nanos,linked,live_pose,held_mesh,gaze_projected\n");
        row(timeline,policy,"headset_only",t,false,false,-1,
            VrPointerMode.AUTO,true);
        row(timeline,policy,"controller_ready",t+SECOND,true,true,
            t+SECOND,VrPointerMode.AUTO,false);
        row(timeline,policy,"ble_pose_gap_800ms",t+SECOND+800_000_000L,
            true,true,t+SECOND,VrPointerMode.AUTO,false);
        row(timeline,policy,"held_noninteractive_model",t+3*SECOND,
            true,true,t+SECOND,VrPointerMode.AUTO,false);
        row(timeline,policy,"connected_stall_under_30s",t+30*SECOND,
            true,true,t+SECOND,VrPointerMode.AUTO,false);
        row(timeline,policy,"connected_stall_30s",t+31*SECOND,
            true,true,t+SECOND,VrPointerMode.AUTO,true);
        row(timeline,policy,"tracking_resumes",t+32*SECOND,
            true,true,t+32*SECOND,VrPointerMode.AUTO,false);
        row(timeline,policy,"actual_disconnect",t+33*SECOND,
            false,false,-1,VrPointerMode.AUTO,false);
        row(timeline,policy,"disconnect_grace_4s",t+36*SECOND,
            false,false,-1,VrPointerMode.AUTO,true);
        row(timeline,policy,"reconnect_before_pose",t+36*SECOND+1,
            true,false,-1,VrPointerMode.AUTO,false);
        row(timeline,policy,"reconnect_still_acquiring",t+38*SECOND,
            true,false,-1,VrPointerMode.AUTO,false);
        row(timeline,policy,"reconnected_live_pose",t+39*SECOND,
            true,true,t+39*SECOND,VrPointerMode.AUTO,false);
        row(timeline,policy,"explicit_gaze_setting",t+40*SECOND,
            true,true,t+40*SECOND,VrPointerMode.GAZE,true);
        // Five thousand real policy invocations with intermittent 1.6s
        // notification gaps. The connected controller never causes a
        // headset-gaze flicker, even though the ray is not clickable stale.
        GazeFallbackPolicy stress=new GazeFallbackPolicy();
        long lastPose=t;
        int staleFrames=0;
        for(int frame=0;frame<5000;++frame) {
            long now=t+frame*20_000_000L;
            if(frame%500<5 || frame%500>=85)
                lastPose=now;
            boolean live=ControllerPoseVisibility.live(true,true,lastPose,now);
            if(!live)staleFrames++;
            assertFalse("Spurious gaze at frame "+frame,
                stress.shouldUseGaze(VrPointerMode.AUTO,live,true,now));
        }
        assertTrue("No stale pose frames exercised",staleFrames>0);
        timeline.append("jitter_5000_frames_stale_count,")
            .append(staleFrames).append("\n");
        byte[] bytes=timeline.toString().getBytes(StandardCharsets.UTF_8);
        try(FileOutputStream out=app().openFileOutput(
            "recovery-controller-trace.csv",Context.MODE_PRIVATE)) {
            out.write(bytes);
        }
        Log.i(TAG,"PASS controller gaze trace: 13 transitions, 5000 jitter frames, "+
            staleFrames+" stale non-clickable frames, 0 spurious gaze frames");
    }

    @Test public void writeUpgradeSentinel() throws Exception {
        assertTrue(app().getSharedPreferences(
            "recovery_emulator_probe",Context.MODE_PRIVATE).edit()
            .putString("sentinel",PROBE).commit());
        try(FileOutputStream out=app().openFileOutput(
                "recovery-upgrade-sentinel.txt",Context.MODE_PRIVATE)){
            out.write(PROBE.getBytes(StandardCharsets.UTF_8));
            out.getFD().sync();
        }
        assertEquals(PROBE,app().getSharedPreferences(
            "recovery_emulator_probe",Context.MODE_PRIVATE)
            .getString("sentinel",null));
        Log.i(TAG,"PASS wrote private SharedPreferences and file sentinel before APK update");
    }

    @Test public void verifyUpgradeSentinel() throws Exception {
        assertEquals("APK replacement erased SharedPreferences",PROBE,
            app().getSharedPreferences(
                "recovery_emulator_probe",Context.MODE_PRIVATE)
                .getString("sentinel",null));
        File file=new File(app().getFilesDir(),"recovery-upgrade-sentinel.txt");
        assertTrue("APK replacement deleted private file",file.isFile());
        assertEquals("APK replacement modified private file",PROBE,
            new String(Files.readAllBytes(file.toPath()),StandardCharsets.UTF_8));
        Log.i(TAG,"PASS private SharedPreferences and file preserved across APK upgrade");
    }
}
