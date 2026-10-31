package org.beckenburg.editor;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Set;
import javax.imageio.ImageIO;

/**
 * Non-destructive image/texture preparation for future scene billboards.
 * This is an authoring-only 2D conversion, NOT a live 3D rendering adapter.
 */
public final class EditorBillboardProfile {
    public static final int MAX_SIDE=4096;
    public static final long MAX_FILE_BYTES=16L*1024*1024;
    public enum Palette { ORIGINAL, RGB_555, INDEXED_332 }
    public record Source(String name,long fileBytes,BufferedImage image) {
        public int width(){return image.getWidth();}
        public int height(){return image.getHeight();}
        public long decodedRgbaBytes(){return (long)width()*height()*4;}
    }
    public record Output(BufferedImage image,int width,int height,long decodedRgbaBytes,
                         Palette palette,boolean dither) {}
    private EditorBillboardProfile(){}
    public static Source read(Path path)throws IOException{
        if(!Files.isRegularFile(path))throw new IOException("Billboard image missing");
        long bytes=Files.size(path);
        if(bytes<=0||bytes>MAX_FILE_BYTES)throw new IOException("Billboard image exceeds 16 MiB source limit");
        String name=path.getFileName().toString().toLowerCase(java.util.Locale.ROOT);
        if(!name.endsWith(".png")&&!name.endsWith(".jpg")&&!name.endsWith(".jpeg"))
            throw new IOException("Supported images: PNG and JPEG");
        BufferedImage picture=ImageIO.read(path.toFile());
        if(picture==null||picture.getWidth()<1||picture.getHeight()<1
            ||picture.getWidth()>MAX_SIDE||picture.getHeight()>MAX_SIDE)
            throw new IOException("Unreadable or oversized billboard dimensions");
        return new Source(path.getFileName().toString(),bytes,picture);
    }
    /** Aspect-preserving nearest-neighbor fit; pads letterbox/pillarbox to output canvas.
     * Palette quantization never modifies input pixels.
     */
    public static Output convert(Source source,int width,int height,Palette palette,boolean dither){
        if(source==null||palette==null||(width!=256&&width!=512)
            ||(height!=256&&height!=512))
            throw new IllegalArgumentException("Billboard preset must be 256/512-pixel canvas");
        BufferedImage target=new BufferedImage(width,height,BufferedImage.TYPE_INT_ARGB);
        Graphics2D g=target.createGraphics();
        g.setRenderingHint(RenderingHints.KEY_INTERPOLATION,RenderingHints.VALUE_INTERPOLATION_NEAREST_NEIGHBOR);
        double scale=Math.min((double)width/source.width(),(double)height/source.height());
        int w=Math.max(1,(int)Math.round(source.width()*scale));
        int h=Math.max(1,(int)Math.round(source.height()*scale));
        g.drawImage(source.image(),(width-w)/2,(height-h)/2,w,h,null);
        g.dispose();
        if(palette!=Palette.ORIGINAL){
            final int[] bayer={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
            int rb=palette==Palette.RGB_555?5:3,gb=palette==Palette.RGB_555?5:3,bb=palette==Palette.RGB_555?5:2;
            for(int y=0;y<height;y++)for(int x=0;x<width;x++){
                int argb=target.getRGB(x,y),a=(argb>>>24)&255;
                if(a==0)continue;
                int step=dither?(bayer[(y&3)*4+(x&3)]-8):0;
                int red=quant((argb>>>16)&255,rb,step);
                int green=quant((argb>>>8)&255,gb,step);
                int blue=quant(argb&255,bb,step);
                target.setRGB(x,y,(a<<24)|(red<<16)|(green<<8)|blue);
            }
        }
        return new Output(target,width,height,(long)width*height*4,palette,dither);
    }
    private static int quant(int value,int bits,int threshold){
        int max=(1<<bits)-1;
        int adjusted=Math.min(255,Math.max(0,value+threshold*2));
        int level=(int)Math.round(adjusted*(double)max/255);
        return (int)Math.round(level*255.0/max);
    }
    public static void save(Path destination,Output output)throws IOException{
        Path folder=destination.toAbsolutePath().getParent();
        Files.createDirectories(folder);
        Path temporary=Files.createTempFile(folder,"billboard-", ".pending");
        try{
            if(!ImageIO.write(output.image(),"png",temporary.toFile()))
                throw new IOException("PNG encoder unavailable");
            try{Files.move(temporary,destination,StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);}
            catch(java.nio.file.AtomicMoveNotSupportedException e){
                Files.move(temporary,destination,StandardCopyOption.REPLACE_EXISTING);
            }
        }finally{Files.deleteIfExists(temporary);}
    }
}
