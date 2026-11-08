package io.github.mrcalzon02.reverievr;
import android.os.Handler; import android.media.ToneGenerator;
public final class UiFeedbackHarness {
 static int checks;
 static void check(boolean value,String message){checks++;if(!value)throw new AssertionError(message);}
 static void drain(){Handler.drain();}
 static void start(UiFeedback ui){ui.setNativeModuleSession("breakwater-battery");ui.setAudioFocusAllowed(true);ui.setNativeAudioSuspended(false);drain();}
 public static void main(String[] args){
  UiFeedback ui=new UiFeedback(new android.content.Context()); start(ui);
  BreakwaterAudio bank=BreakwaterAudio.instance;
  check(bank.active&&bank.lifecycle&&!bank.suspended,"lazy bank begins active with fresh pointer");
  ui.nativeSoundCue(22);drain();check(RedLedgerSound.played.isEmpty(),"Breakwater cannot leak another game bank");
  ui.nativeSoundCue(1); ui.setNativeAudioSuspended(true); drain();
  check(bank.cues.isEmpty()&&NativeCombatSound.played.isEmpty(),"Quick Menu invalidates already posted native shot");
  ui.nativeSoundCue(1);ui.nativeFeedback(8);drain();
  check(bank.cues.isEmpty()&&NativeCombatSound.played.isEmpty(),"pointerless native events never fall back");
  int before=ToneGenerator.tones.size();int shellBefore=bank.feedback.size();ui.activation();drain();
  check(ToneGenerator.tones.size()==before&&bank.feedback.size()==shellBefore+1,"Quick Menu activation stays in normalized control bank");
  ui.setNativeAudioSuspended(false);drain();ui.nativeSoundCue(2);ui.setNativeModuleSession(null);drain();
  check(bank.cues.isEmpty()&&!bank.active,"exit invalidates queued game cue and ends bank");
  start(ui);ui.nativeSoundCue(3);ui.pauseForLifecycle();ui.resumeForLifecycle();drain();
  check(bank.cues.isEmpty()&&bank.suspended,"background/resume discards old shot and awaits fresh pointer");
  ui.nativeSoundCue(3);drain();check(bank.cues.isEmpty(),"resume alone does not resume game audio");
  ui.setNativeAudioSuspended(false);drain();ui.nativeSoundCue(3);drain();
  check(bank.cues.size()==1&&bank.cues.get(0)==3,"fresh pointer permits current cue");
  int stopped=bank.sessionStops; ui.setNativeModuleSession(null);ui.setNativeModuleSession("breakwater-battery");ui.setAudioFocusAllowed(true);ui.setNativeAudioSuspended(false);drain();
  check(bank.sessionStops>stopped,"collapsed same-module restart resets prior bank session");
  int nativeCount=bank.cues.size();ui.nativeSpatialSoundCue(41,Float.NaN,1f);ui.nativeSpatialSoundCue(28,1f,1f);ui.nativeSpatialSoundCue(41,.5f,.4f);drain();
  check(bank.cues.size()==nativeCount+1&&bank.cues.get(nativeCount)==41,"only valid spatial IDs and finite gains reach bank");
  ui.nativeSoundCue(1);ui.setAudioFocusAllowed(false);drain();
  check(bank.cues.size()==nativeCount+1&&bank.suspended,"audio focus loss invalidates queued events");
  before=ToneGenerator.tones.size();ui.activation();drain();check(ToneGenerator.tones.size()==before,"focus loss does not leak shell fallback tones");
  ui.setAudioFocusAllowed(true);drain();BreakwaterAudio.ready=false;
  before=ToneGenerator.tones.size();ui.nativeFeedback(1|2|4);drain();
  check(ToneGenerator.tones.size()==before&&NativeCombatSound.played.isEmpty(),"unready Breakwater feedback stays quiet without legacy fallback");
  ui.activation();ui.focus();ui.failure();ui.springRecenter();drain();
  check(ToneGenerator.tones.size()==before&&android.media.AudioManager.plays==0,"unready Breakwater shell controls also never use legacy tones/effects");
  ui.setNativeModuleSession("omega-front");ui.setNativeAudioSuspended(false);drain();ui.nativeSoundCue(21);ui.nativeSoundCue(22);ui.nativeSoundCue(48);drain();
  check(NativeCombatSound.played.size()==1&&NativeCombatSound.played.get(0)==21,"other modules retain their legacy cue bank");
  check(RedLedgerSound.played.size()==1&&RedLedgerSound.played.get(0)==22,"Red Ledger retains its own cue namespace");
  ui.nativeSoundCue(1);ui.close();drain();check(NativeCombatSound.played.size()==1&&bank.closed,"close cannot replay queued sounds");
  System.out.println("UiFeedback lifecycle/routing harness: "+checks+" checks passed");
 }
}
