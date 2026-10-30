package dev.mrcalzon.water;
import java.io.*;
import java.util.*;
import javax.imageio.ImageIO;
import static dev.mrcalzon.water.WaterModule.*;

/** Built-in original ripple sheets plus a bounded optional PNG importer. */
public final class WaterTextures {
 private static final Map<String,Texture> BUILTINS=make();
 private static Map<String,Texture> make(){int[] a=new int[4096],b=new int[4096];for(int z=0;z<64;z++)for(int x=0;x<64;x++){double u=x/64.,v=z/64.,warp=.19*Math.sin(u*Math.PI*2)+.13*Math.sin((u+v)*Math.PI*2),noise=(Math.sin(x*127.1+z*311.7)*43758.5453)%1;int av=(int)(128+16*Math.sin(Math.PI*2*(v*5+warp))+7*noise),bv=(int)(128+14*Math.sin(Math.PI*2*(u*3+v*2+.21*Math.sin(v*Math.PI*4)))-6*noise);a[z*64+x]=av*0x010101;b[z*64+x]=bv*0x010101;}return Map.of("water://ripple-a",new Texture(64,64,a),"water://ripple-b",new Texture(64,64,b));}
 public static Set<String> builtins(){return BUILTINS.keySet();}
 public static Texture builtin(String uri){var t=BUILTINS.get(uri);if(t==null)throw new IllegalArgumentException("Unknown water texture "+uri);return t;}
 public static Texture png(InputStream input)throws IOException{
  if(input==null)throw new IOException("Missing water texture");byte[] bytes;try(input){bytes=input.readNBytes(16_000_001);}if(bytes.length>16_000_000)throw new IOException("Water PNG exceeds16MB");if(bytes.length<8||!Arrays.equals(Arrays.copyOf(bytes,8),new byte[]{(byte)137,80,78,71,13,10,26,10}))throw new IOException("Water sheet must be a PNG");
  try(var stream=ImageIO.createImageInputStream(new ByteArrayInputStream(bytes))){var readers=ImageIO.getImageReaders(stream);if(!readers.hasNext())throw new IOException("Unsupported water image");var reader=readers.next();try{reader.setInput(stream);int w=reader.getWidth(0),h=reader.getHeight(0);if(w<2||h<2||w>2048||h>2048)throw new IOException("Water texture dimensions must be2..2048");var image=reader.read(0);return new Texture(w,h,image.getRGB(0,0,w,h,null,0,w));}finally{reader.dispose();}}
 }
 private WaterTextures(){}
}
