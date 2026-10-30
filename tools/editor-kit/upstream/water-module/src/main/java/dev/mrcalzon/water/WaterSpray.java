package dev.mrcalzon.water;
import java.util.*;
import static dev.mrcalzon.water.WaterModule.*;

/** Bounded cosmetic ballistic droplets at real visible contact points.
 * Stable world-cell phases; light shaded and opaque/water depth tested.
 * They never create colliders, evidence or network authority. */
public final class WaterSpray {
 public record Emitter(double x,double y,double z,double outwardX,double outwardZ,double strength,long seed){}
 public static long cell(double x,double z){return ((long)(int)Math.floor(x/.4)<<32)^((int)Math.floor(z/.4)&0xffffffffL);}
 private static double fraction(double v){return v-Math.floor(v);}
 public static int draw(Camera c,int[] pixels,double[] depth,double seconds,Collection<Emitter> sources,Lighting lighting){
  double cy=Math.cos(c.yaw()),sy=Math.sin(c.yaw()),cp=Math.cos(c.pitch()),sp=Math.sin(c.pitch());int drawn=0,emitters=0;
  for(var e:sources){if(++emitters>256)break;
   double hash=fraction(Math.sin(e.seed*.000001+17.31)*43758.5453);
   for(int drop=0;drop<3;drop++){
    double age=fraction(seconds/1.9+hash+drop*.173)*1.9;if(age>.62)continue;
    double launch=.8+hash*.5,wx=e.x+e.outwardX*(.025+age*.28)+(drop-1)*.013,wy=e.y+.025+launch*age-1.4*age*age,wz=e.z+e.outwardZ*(.025+age*.28);
    double dx=wx-c.x(),dy=wy-c.y(),dz=wz-c.z(),forward=dx*sy+dz*cy,t=dy*sp+forward*cp;if(t<.065||t>70)continue;
    int x=(int)Math.round(c.width()*.5+(-dx*cy+dz*sy)*c.focal()/t),y=(int)Math.round(c.height()*.5-(dy*cp-forward*sp)*c.focal()/t),radius=Math.min(2,Math.max(0,(int)(.013*c.focal()/t)));
    int colour=lighting.shade(0xb7c7c4,wx,wy,wz);double alpha=Math.max(0,Math.min(.85,e.strength*(1-age/.72)));
    for(int oy=-radius;oy<=radius;oy++)for(int ox=-radius;ox<=radius;ox++){
     int px=x+ox,py=y+oy;if(px<0||px>=c.width()||py<0||py>=c.height()||ox*ox+oy*oy>radius*radius+1)continue;int i=py*c.width()+px;
     if(t>=depth[i]-.005)continue;int old=pixels[i],rgb=0;for(int shift:new int[]{16,8,0})rgb|=(int)Math.round(((old>>shift)&255)*(1-alpha)+((colour>>shift)&255)*alpha)<<shift;pixels[i]=rgb;drawn++;
    }
   }
  }return drawn;
 }
 private WaterSpray(){}
}
