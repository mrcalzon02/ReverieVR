package dev.mrcalzon.water;
import static dev.mrcalzon.water.WaterModule.*;

/** Visible solid/water contacts reconstructed from the real opaque depth buffer.
 * Distances are world metres, so foam width is independent of camera distance.
 * This is a rendering contact query, not an offscreen gameplay collider bake. */
public final class WaterContact {
 public record Contact(double x,double y,double z,double distance,double outwardX,double outwardZ){public double foam(){return Math.max(0,1-distance/.28);}}
 private final Camera c;private final double[] depth;private final float[] normalCache;private final double cy,sy,cp,sp;
 public WaterContact(Camera c,double[] opaqueDepth){if(opaqueDepth.length!=c.width()*c.height())throw new IllegalArgumentException("Contact depth");this.c=c;depth=opaqueDepth;normalCache=new float[opaqueDepth.length];cy=Math.cos(c.yaw());sy=Math.sin(c.yaw());cp=Math.cos(c.pitch());sp=Math.sin(c.pitch());}
 private record Position(double x,double y,double z){}
 private Position point(int x,int y){if(x<0||x>=c.width()||y<0||y>=c.height())return null;double t=depth[y*c.width()+x];if(!Double.isFinite(t)||t<.065||t>180)return null;double sx=(x+.5-c.width()*.5)/c.focal(),sv=(c.height()*.5-y-.5)/c.focal(),f=cp-sv*sp;return new Position(c.x()+(-sx*cy+f*sy)*t,c.y()+(sv*cp+sp)*t,c.z()+(sx*sy+f*cy)*t);}
 private static double distance(Position a,Position b){return Math.sqrt(Math.pow(a.x-b.x,2)+Math.pow(a.y-b.y,2)+Math.pow(a.z-b.z,2));}
 private double upNormal(int x,int y,double wx,double wy,double wz){
  int i=y*c.width()+x;if(normalCache[i]>0)return normalCache[i]-1;var p=new Position(wx,wy,wz);Position a=null,b=null;
  for(int d:new int[]{1,-1}){var q=point(x+d,y);if(q!=null&&distance(p,q)<.45){a=q;break;}}
  for(int d:new int[]{1,-1}){var q=point(x,y+d);if(q!=null&&distance(p,q)<.45){b=q;break;}}
  double n=0;if(a!=null&&b!=null){double ax=a.x-wx,ay=a.y-wy,az=a.z-wz,bx=b.x-wx,by=b.y-wy,bz=b.z-wz,nx=ay*bz-az*by,ny=az*bx-ax*bz,nz=ax*by-ay*bx,length=Math.sqrt(nx*nx+ny*ny+nz*nz);if(length>1e-10)n=Math.abs(ny)/length;}
  normalCache[i]=(float)(n+1);return n;
 }
 public Contact near(int px,int py,double wx,double wy,double wz,double forwardDepth,double slopeX,double slopeZ){
  int radius=Math.max(1,Math.min(16,(int)Math.ceil(.28*c.focal()/forwardDepth)));double best=.28,bx=0,by=0,bz=0;
  for(int step:new int[]{0,1,Math.max(1,radius/2),radius})for(int iz=-1;iz<=1;iz++)for(int ix=-1;ix<=1;ix++){
   if(step==0&&(ix!=0||iz!=0)||step!=0&&ix==0&&iz==0)continue;
   int x=px+ix*step,y=py+iz*step;if(x<0||x>=c.width()||y<0||y>=c.height())continue;
   double t=depth[y*c.width()+x];if(!Double.isFinite(t)||t<.065||t>180)continue;
   double sx=(x+.5-c.width()*.5)/c.focal(),sv=(c.height()*.5-y-.5)/c.focal(),f=cp-sv*sp;
   double ox=c.x()+(-sx*cy+f*sy)*t,oy=c.y()+(sv*cp+sp)*t,oz=c.z()+(sx*sy+f*cy)*t;
   double dx=wx-ox,dz=wz-oz,localY=wy-slopeX*dx-slopeZ*dz,vertical=localY-oy;
   // A floating object's underside above the current wave is not a wet edge.
   if(vertical<-.015||vertical>.28||Math.abs(dx)>.28||Math.abs(dz)>.28)continue;
   // Shallow horizontal bed is not an object-impact edge. Shore-run-up foam
   // has its own wave train; solid-contact foam follows steep faces or a true
   // waterline crossing within20mm of an upward-facing surface.
   if(vertical>.02&&upNormal(x,y,ox,oy,oz)>.85)continue;
   double distance=Math.sqrt(dx*dx+dz*dz+vertical*vertical);
   if(distance<best){best=distance;bx=ox;by=oy;bz=oz;}
  }
  if(best>=.28)return null;double dx=wx-bx,dz=wz-bz,length=Math.hypot(dx,dz);
  return new Contact(bx,by,bz,best,length<1e-6?0:dx/length,length<1e-6?0:dz/length);
 }
}
