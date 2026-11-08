package io.github.mrcalzon02.reverievr;
import java.nio.file.*;
import javax.sound.sampled.*;
import java.io.*;
public class SynthExport {
 public static void main(String[] args) throws Exception {
  Path out=Paths.get(args[0]);Files.createDirectories(out);
  for(int cue=0;cue<NativeCombatWaveform.COUNT;cue++) {
   byte[] pcm=NativeCombatWaveform.generatePcm16(cue);
   AudioFormat format=new AudioFormat(22050,16,1,true,false);
   try(AudioInputStream stream=new AudioInputStream(new ByteArrayInputStream(pcm),format,pcm.length/2)) {
    AudioSystem.write(stream,AudioFileFormat.Type.WAVE,out.resolve("cue_"+cue+".wav").toFile());
   }
  }
 }
}
