package dev.mrcalzon.weather;

import java.awt.*;

/** Layered low-resolution skybox: day/night, clouds blocking sunlight,
 * storm shadows and gentle NON-STROBING directional lightning illumination.
 * Produces sky pixels to put behind the world's geometry (not an image asset).
 */
public final class WeatherSky {
    private WeatherSky(){}
    private static double clamp(double v,double a,double b){return Math.max(a,Math.min(b,v));}
    public record State(double timeOfDay,double daylight,double sunBrightness,
                        double cloudCover,double skyBrightness,double flash){}
    public static State state(double timeOfDay,WeatherSystem.State w){
        if(!Double.isFinite(timeOfDay)||timeOfDay<0||timeOfDay>=1||w==null)
            throw new IllegalArgumentException("Invalid sky clock");
        double solar=Math.sin(2*Math.PI*(timeOfDay-.25));
        double daylight=clamp((solar+.13)/.45,0,1);
        double cover=clamp(w.rain()*.66+w.snow()*.35+w.fog()*.27+
            w.dust()*.56+w.ash()*.58+(w.type()==WeatherType.OVERCAST?.60:0),0,1);
        double sun=daylight*(1-cover*.92);
        double ambient=.07+daylight*.85*(1-cover*.70);
        return new State(timeOfDay,daylight,sun,cover,ambient,w.flash());
    }
    /** Legacy entry point for callers without a vertical camera angle. */
    public static void paint(Graphics2D target,int width,int height,State sky,
                             long seed,double windHeading,double elapsedSeconds,
                             double viewerHeading,double lightningHeading){
        paint(target,width,height,sky,seed,windHeading,elapsedSeconds,
            viewerHeading,lightningHeading,0,height*.765);
    }
    /** Sky directions and elevation live in world space; camera turns reveal a
     * different portion of that same sky instead of dragging clouds with the HUD. */
    public static void paint(Graphics2D target,int width,int height,State sky,
                             long seed,double windHeading,double elapsedSeconds,
                             double viewerHeading,double lightningHeading,
                             double viewerPitch,double focal){
        if(target==null||sky==null||width<16||height<16||width>8192||height>8192
                ||!Double.isFinite(viewerHeading)||!Double.isFinite(viewerPitch)
                ||!Double.isFinite(focal)||focal<=0)
            throw new IllegalArgumentException("Invalid sky viewport");
        Graphics2D g=(Graphics2D)target.create();
        try{
            int horizon=(int)Math.round(height*.56+Math.tan(viewerPitch)*focal);
            double d=sky.daylight(),cloud=sky.cloudCover();
            for(int row=0;row<height;row+=2){
                double pos=clamp(row/(double)Math.max(1,horizon),0,1);
                int r=(int)clamp((11+7*pos+d*(42+33*pos))*(1-cloud*.55),0,255);
                int gr=(int)clamp((17+9*pos+d*(70+35*pos))*(1-cloud*.53),0,255);
                int b=(int)clamp((36+14*pos+d*(111+11*pos))*(1-cloud*.48),0,255);
                g.setColor(new Color(r,gr,b));g.fillRect(0,row,width,2);
            }
            int bands=(int)(4+cloud*28);
            for(int i=0;i<bands;i++){
                long h=mix(seed+i*0x9e3779b97f4a7c15L);
                double direction=((h&0xffff)/65536.0-.5)*2*Math.PI;
                // Wind moves cloud positions in world azimuth, not screen pixels.
                direction+=elapsedSeconds*(.00022+.00006*(i%4))*Math.cos(windHeading);
                double relative=Math.atan2(Math.sin(direction-viewerHeading),
                    Math.cos(direction-viewerHeading));
                if(Math.abs(relative)>1.45)continue;
                int x=(int)Math.round(width*.5-Math.tan(relative)*focal);
                double elevation=.12+((h>>>16)&0xffff)/65536.0*.80;
                int y=(int)Math.round(horizon-Math.tan(elevation)*focal);
                int w=(int)Math.round((21+((h>>>31)&63)+cloud*65)*focal/153.0);
                int hh=(int)Math.round((5+((h>>>41)&11)+cloud*10)*focal/153.0);
                if(x+w<0||x-w>width||y+hh<0||y-hh>height)continue;
                int shade=(int)(28+sky.daylight()*70*(1-cloud*.85));
                g.setColor(new Color(shade,shade+2,shade+6,(int)(35+cloud*140)));
                g.fillPolygon(new int[]{x,x+w/4,x+w*3/4,x+w,x+w*3/4,x+w/5},
                    new int[]{y+hh/2,y,y-2,y+hh/2,y+hh,y+hh},6);
            }
            if(sky.sunBrightness()>.17){
                double solarHeading=2*Math.PI*(sky.timeOfDay()-.25);
                double relative=Math.atan2(Math.sin(solarHeading-viewerHeading),
                    Math.cos(solarHeading-viewerHeading));
                if(Math.abs(relative)<1.45){
                    int x=(int)Math.round(width*.5-Math.tan(relative)*focal);
                    int y=(int)Math.round(horizon-Math.tan(.08+sky.daylight()*.60)*focal);
                    int glow=(int)(185+sky.sunBrightness()*50);
                    g.setColor(new Color(glow,(int)(glow*.85),(int)(glow*.66),
                        (int)(sky.sunBrightness()*180)));
                    g.fillOval(x-7,y-7,14,14);
                }
            }
            if(sky.flash()>0){
                double off=Math.cos(lightningHeading-viewerHeading);
                int alpha=(int)clamp(sky.flash()*(off*.4+.6)*255,0,85);
                g.setColor(new Color(148,167,190,alpha));
                g.fillRect(0,0,width,Math.max(0,Math.min(height,horizon)));
            }
        }finally{g.dispose();}
    }
    private static long mix(long x){
        x=(x^(x>>>30))*0xbf58476d1ce4e5b9L;
        x=(x^(x>>>27))*0x94d049bb133111ebL;
        return x^(x>>>31);
    }
}
