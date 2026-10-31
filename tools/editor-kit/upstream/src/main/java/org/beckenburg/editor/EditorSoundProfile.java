package org.beckenburg.editor;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;

/** Non-destructive PCM-WAV authoring and size diagnostics. No game audio engine
 * or PlayStation VAG/XA encoder is installed or needed at runtime.
 */
public final class EditorSoundProfile {
    public static final long MAX_SOURCE_BYTES=32L*1024*1024;
    public static final int MAX_DECODED_BYTES=8_000_000;
    public record Source(String filename,int frames,int channels,int sampleRate,long sourceBytes,
                         int decodedBytes,short[] mono) {
        public Source {mono=mono.clone();}
        @Override public short[] mono(){return mono.clone();}
    }
    public record Output(byte[] wav,int sampleRate,int bitDepth,int frames,long decodedBytes) {
        public Output {wav=wav.clone();}
        @Override public byte[] wav(){return wav.clone();}
    }
    private EditorSoundProfile(){}
    public static Source read(Path file)throws IOException {
        if(!Files.isRegularFile(file))throw new IOException("Missing sound input");
        long bytes=Files.size(file);
        if(bytes<44||bytes>MAX_SOURCE_BYTES)throw new IOException("Source sound exceeds 32 MiB bound");
        try(AudioInputStream input=AudioSystem.getAudioInputStream(file.toFile())){
            AudioFormat source=input.getFormat();
            if(!source.getEncoding().equals(AudioFormat.Encoding.PCM_SIGNED)
                &&!source.getEncoding().equals(AudioFormat.Encoding.PCM_UNSIGNED))
                throw new IOException("PCM WAV required: compressed audio is not decoded by this authoring tool");
            int channels=source.getChannels(),sampleRate=Math.round(source.getSampleRate());
            if(channels<1||channels>2||sampleRate<8000||sampleRate>96000
                ||source.getSampleSizeInBits()!=8&&source.getSampleSizeInBits()!=16)
                throw new IOException("WAV requires 8/16-bit PCM, mono/stereo, 8–96 kHz");
            AudioFormat signed=new AudioFormat(AudioFormat.Encoding.PCM_SIGNED,
                source.getSampleRate(),16,channels,channels*2,source.getSampleRate(),false);
            try(AudioInputStream normalized=AudioSystem.getAudioInputStream(signed,input)){
                byte[] decoded=normalized.readNBytes(MAX_DECODED_BYTES+1);
                if(decoded.length>MAX_DECODED_BYTES)throw new IOException("Decoded sound exceeds 8 MiB authoring limit");
                if(decoded.length<channels*2||decoded.length%(channels*2)!=0)
                    throw new IOException("Incomplete PCM frames");
                ByteBuffer data=ByteBuffer.wrap(decoded).order(ByteOrder.LITTLE_ENDIAN);
                int frames=decoded.length/(channels*2);
                short[] mono=new short[frames];
                for(int i=0;i<frames;i++){
                    int sum=0;
                    for(int c=0;c<channels;c++)sum+=data.getShort();
                    mono[i]=(short)(sum/channels);
                }
                return new Source(file.getFileName().toString(),frames,channels,sampleRate,bytes,decoded.length,mono);
            }
        }catch(UnsupportedAudioFileException | IllegalArgumentException ex){
            throw new IOException("Unsupported WAV or PCM decoder: "+ex.getMessage(),ex);
        }
    }
    public static Output export(Source source,int targetRate,int bits)throws IOException {
        if(source==null||targetRate!=11025&&targetRate!=22050||bits!=8&&bits!=16)
            throw new IOException("Choose 11.025/22.05 kHz and 8/16-bit PCM");
        int frames=Math.max(1,(int)Math.round((double)source.frames()*targetRate/source.sampleRate()));
        if(frames>1_000_000)throw new IOException("Converted audio too long");
        byte[] pcm=new byte[frames*(bits/8)];
        ByteBuffer dest=ByteBuffer.wrap(pcm).order(ByteOrder.LITTLE_ENDIAN);
        short[] samples=source.mono();
        for(int i=0;i<frames;i++){
            double index=(double)i*source.sampleRate()/targetRate;
            int lo=Math.min(samples.length-1,(int)Math.floor(index));
            int hi=Math.min(samples.length-1,lo+1);
            double mix=index-lo;
            int value=(int)Math.round(samples[lo]*(1-mix)+samples[hi]*mix);
            value=Math.max(-32768,Math.min(32767,value));
            if(bits==16)dest.putShort((short)value);
            else dest.put((byte)Math.max(0,Math.min(255,128+Math.round(value/256f))));
        }
        AudioFormat format=new AudioFormat(bits==8?AudioFormat.Encoding.PCM_UNSIGNED:
            AudioFormat.Encoding.PCM_SIGNED,targetRate,bits,1,bits/8,targetRate,false);
        ByteArrayOutputStream bytes=new ByteArrayOutputStream();
        try(AudioInputStream stream=new AudioInputStream(new ByteArrayInputStream(pcm),format,frames)){
            if(AudioSystem.write(stream,javax.sound.sampled.AudioFileFormat.Type.WAVE,bytes)<1)
                throw new IOException("PCM-WAV encoder unsupported");
        }
        return new Output(bytes.toByteArray(),targetRate,bits,frames,pcm.length);
    }
    public static void save(Path output,Output encoded)throws IOException{
        Path folder=output.toAbsolutePath().getParent();
        Files.createDirectories(folder);
        Path staging=Files.createTempFile(folder,"retro-sfx-", ".pending");
        try{
            Files.write(staging,encoded.wav());
            try{Files.move(staging,output,java.nio.file.StandardCopyOption.ATOMIC_MOVE,
                java.nio.file.StandardCopyOption.REPLACE_EXISTING);}
            catch(java.nio.file.AtomicMoveNotSupportedException e){
                Files.move(staging,output,java.nio.file.StandardCopyOption.REPLACE_EXISTING);
            }
        }finally{Files.deleteIfExists(staging);}
    }
}
