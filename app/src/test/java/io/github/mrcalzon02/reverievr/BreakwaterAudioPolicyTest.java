package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public final class BreakwaterAudioPolicyTest {
    private BreakwaterAudioPolicy active() {
        BreakwaterAudioPolicy policy=new BreakwaterAudioPolicy();
        policy.setSessionActive(true); policy.setLifecycleActive(true);
        return policy;
    }

    @Test public void failedActionDoesNotPlaySuccessOrFocus() {
        assertEquals(BreakwaterAudioPolicy.FAILURE,BreakwaterAudioPolicy.controlForFeedback(7));
        assertEquals(BreakwaterAudioPolicy.ACTIVATE,BreakwaterAudioPolicy.controlForFeedback(3));
        assertEquals(BreakwaterAudioPolicy.FOCUS,BreakwaterAudioPolicy.controlForFeedback(1));
        assertEquals(-1,BreakwaterAudioPolicy.controlForFeedback(0));
    }
    @Test public void stalledProducerExpiresAllSevenLoops() {
        BreakwaterAudioPolicy policy=active();
        for(int id=41;id<=47;id++) {
            assertTrue(policy.refreshLoop(id,.25f,.75f,1000L));
            assertEquals(BreakwaterAudioPolicy.cue(id).gain*.25f,
                policy.loopGain(id,true,1899L),.00001f);
            assertEquals(0f,policy.loopGain(id,false,1900L),0f);
        }
    }
    @Test public void pauseAndExitDoNotResurrectPriorLeasesOrShots() {
        BreakwaterAudioPolicy policy=active();
        assertTrue(policy.refreshLoop(44,1f,1f,100L));
        assertTrue(policy.allowOneShot(1,100L));
        policy.setSuspended(true);
        assertFalse(policy.refreshLoop(44,1f,1f,101L));
        assertFalse(policy.allowOneShot(1,101L));
        policy.setSuspended(false);
        assertEquals(0f,policy.loopGain(44,true,102L),0f);
        assertTrue(policy.allowOneShot(1,102L));
        policy.refreshLoop(41,1f,1f,110L);
        policy.setLifecycleActive(false); policy.setLifecycleActive(true);
        assertEquals(0f,policy.loopGain(41,true,111L),0f);
        policy.refreshLoop(47,1f,1f,120L);
        policy.setSessionActive(false); policy.setSessionActive(true);
        assertEquals(0f,policy.loopGain(47,true,121L),0f);
    }
    @Test public void malformedGainsAndForeignCueIdsStayOutOfThisBank() {
        BreakwaterAudioPolicy policy=active();
        assertFalse(policy.refreshLoop(41,Float.NaN,1f,0L));
        assertFalse(policy.refreshLoop(41,1.01f,1f,0L));
        assertFalse(policy.refreshLoop(41,-.01f,1f,0L));
        assertFalse(policy.refreshLoop(40,1f,1f,0L));
        for(int id=22;id<=27;id++) assertNull(BreakwaterAudioPolicy.cue(id));
        assertNull(BreakwaterAudioPolicy.cue(0));
        assertNull(BreakwaterAudioPolicy.cue(255));
        assertFalse(BreakwaterAudioPolicy.isSpatialCue(28));
    }
    @Test public void heldFeedbackIsBoundedButRealGunCadenceStillPlays() {
        BreakwaterAudioPolicy policy=active();
        assertTrue(policy.allowOneShot(29,100L));
        assertFalse(policy.allowOneShot(29,110L));
        assertTrue(policy.allowOneShot(29,280L));
        assertTrue(policy.allowOneShot(1,1000L));
        assertTrue(policy.allowOneShot(1,1160L));
        assertTrue(policy.allowOneShot(2,1160L));
        assertFalse(policy.allowOneShot(1,1161L));
    }
    @Test public void explicitZeroStopsOneLayerWithoutMutingOtherLayers() {
        BreakwaterAudioPolicy policy=active();
        policy.refreshLoop(41,1f,1f,0L); policy.refreshLoop(46,1f,.1f,0L);
        policy.refreshLoop(46,0f,0f,100L);
        assertEquals(0f,policy.loopGain(46,true,100L),0f);
        assertTrue(policy.loopGain(41,true,100L)>0f);
    }
    @Test public void worstCaseBankAndMaximumMusicHavePeakHeadroom() {
        assertTrue(BreakwaterAudioPolicy.maximumEffectsPeak()+MusicVolumePolicy.gain(100,false)<1.0);
        assertTrue(BreakwaterAudioPolicy.maximumEffectsPeak()>.40);
        for(int id=0;id<BreakwaterAudioPolicy.CUE_COUNT;id++) {
            BreakwaterAudioPolicy.Cue cue=BreakwaterAudioPolicy.cue(id);
            if(cue!=null) {
                assertTrue(cue.gain>0f && cue.gain<=.22f);
                assertTrue(cue.file.matches("[a-z_]+\\.wav"));
            }
        }
    }
}
