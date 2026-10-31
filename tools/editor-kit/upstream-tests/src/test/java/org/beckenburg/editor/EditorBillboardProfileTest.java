package org.beckenburg.editor;

import java.awt.image.BufferedImage;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import javax.imageio.ImageIO;

public final class EditorBillboardProfileTest {
    private static void check(boolean success,String message){if(!success)throw new AssertionError(message);}
    public static void main(String[] args)throws Exception {
        Path root=Files.createTempDirectory("billboard-profile-test-");
        try{
            BufferedImage src=new BufferedImage(200,100,BufferedImage.TYPE_INT_ARGB);
            for(int y=0;y<100;y++)for(int x=0;x<200;x++)
                src.setRGB(x,y,0xff000000|((x%256)<<16)|((y%256)<<8)|(x+y)%256);
            Path origin=root.resolve("original.png");
            ImageIO.write(src,"png",origin.toFile());
            byte[] unmodified=Files.readAllBytes(origin);
            var source=EditorBillboardProfile.read(origin);
            check(source.decodedRgbaBytes()==200*100*4,"Decoded bytes estimated");
            var retro=EditorBillboardProfile.convert(source,512,256,
                EditorBillboardProfile.Palette.RGB_555,true);
            check(retro.image().getWidth()==512&&retro.image().getHeight()==256,
                "Requested aspect-preserving canvas");
            check(retro.decodedRgbaBytes()==512L*256*4,"Derivative byte estimate");
            Path baked=root.resolve("derived.png");
            EditorBillboardProfile.save(baked,retro);
            check(Arrays.equals(unmodified,Files.readAllBytes(origin)),"Original never modified");
            check(ImageIO.read(baked.toFile()).getWidth()==512,"Usable PNG derivative");
            var flat=EditorBillboardProfile.convert(source,256,256,
                EditorBillboardProfile.Palette.INDEXED_332,false);
            check((flat.image().getRGB(128,128)>>>24)!=0,"Ditherless quantization emits mapped source pixels");
            boolean refused=false;
            try{EditorBillboardProfile.convert(source,4096,4096,
                EditorBillboardProfile.Palette.ORIGINAL,false);}
            catch(IllegalArgumentException ok){refused=true;}
            check(refused,"Unsupported output canvas refused");
            System.out.println("PASS PNG import, aspect-preserving palette profile, source protection, budget and bake");
        }finally{
            try(var paths=Files.walk(root)){
                paths.sorted(java.util.Comparator.reverseOrder()).forEach(path->{
                    try{Files.deleteIfExists(path);}catch(Exception ignored){}
                });
            }
        }
    }
}
