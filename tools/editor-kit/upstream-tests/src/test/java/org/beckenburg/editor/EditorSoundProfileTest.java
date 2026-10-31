package org.beckenburg.editor;

import java.io.ByteArrayInputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.AudioFileFormat;

public final class EditorSoundProfileTest {
    private static void check(boolean value,String what){if(!value)throw new AssertionError(what);}
    public static void main(String[] args)throws Exception {
        Path root=Files.createTempDirectory("psx-audio-test-");
        try{
            int frames=4410;
            byte[] raw=new byte[frames*4];
            for(int i=0;i<frames;i++){
                int sample=(int)(12000*Math.sin(2*Math.PI*440*i/44100));
                for(int c=0;c<2;c++){
                    int at=(i*2+c)*2;
                    raw[at]=(byte)sample;raw[at+1]=(byte)(sample>>>8);
                }
            }
            Path original=root.resolve("source.wav");
            AudioFormat stereo=new AudioFormat(44100,16,2,true,false);
            try(AudioInputStream stream=new AudioInputStream(new ByteArrayInputStream(raw),stereo,frames)){
                AudioSystem.write(stream,AudioFileFormat.Type.WAVE,original.toFile());
            }
            byte[] pristine=Files.readAllBytes(original);
            var source=EditorSoundProfile.read(original);
            check(source.sampleRate()==44100&&source.channels()==2&&source.frames()==frames,
                "Read original PCM");
            check(source.decodedBytes()==raw.length,"Decoded-byte accounting");
            var output=EditorSoundProfile.export(source,11025,8);
            check(output.frames()==1103||output.frames()==1102,"Target length 11.025 kHz");
            Path converted=root.resolve("source-retro.wav");
            EditorSoundProfile.save(converted,output);
            check(Arrays.equals(pristine,Files.readAllBytes(original)),"Preserve original unmodified");
            var back=EditorSoundProfile.read(converted);
            check(back.channels()==1&&back.sampleRate()==11025,"Decode derivative as PCM mono");
            check(back.frames()==output.frames(),"Derivative frame count");
            check(output.decodedBytes()==output.frames(),"Eight-bit frame bytes");
            boolean invalid=false;
            try{EditorSoundProfile.export(source,96000,16);}
            catch(java.io.IOException expected){invalid=true;}
            check(invalid,"Unsupported export profile rejected");
            System.out.println("PASS WAV inspect / stereo to 11kHz mono / 8-bit / source preservation / limits");
        }finally{
            try(var walk=Files.walk(root)){
                walk.sorted(java.util.Comparator.reverseOrder()).forEach(p->{
                    try{Files.deleteIfExists(p);}catch(Exception ignored){}
                });
            }
        }
    }
}
