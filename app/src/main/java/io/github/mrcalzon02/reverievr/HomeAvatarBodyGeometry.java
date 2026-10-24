package io.github.mrcalzon02.reverievr;

import java.util.ArrayList;
import java.util.List;

/**
 * Portrait geometry built only when body proportions/clothing change.
 * All meshes use the same 3D mirror plane as the existing headset mirror.
 * Individual body axes are independent of presentation and clothing.
 */
final class HomeAvatarBodyGeometry {
    private HomeAvatarBodyGeometry() {}
    static final float X=HomeAvatarWardrobeGeometry.MIRROR_X-.009f;
    static float shoulder(HomeAvatarProfile p) {return .235f+.051f*p.shoulders+.013f*p.muscle;}
    static float waist(HomeAvatarProfile p) {return .168f+.051f*p.waist;}
    static float hips(HomeAvatarProfile p) {return .196f+.051f*p.hips;}
    static float arm(HomeAvatarProfile p) {return .050f+.012f*p.muscle;}

    /** Interpolated upper-body taper: four independently shaped sections. */
    static float[] torso(HomeAvatarProfile p) {
        float s=shoulder(p),w=waist(p),h=hips(p);
        return join(band(X-.005f,.38f,s,.16f,s*.92f),
            band(X-.005f,.16f,s*.92f,-.19f,w),
            band(X-.005f,-.19f,w,-.39f,h));
    }
    static float[] arms(HomeAvatarProfile p) {
        float s=shoulder(p),w=arm(p);
        return join(bandSide(-1,s,w),bandSide(1,s,w));
    }
    private static float[] bandSide(int side,float shoulder,float size) {
        float upper=side*(shoulder+size*.62f);
        float lower=side*(shoulder+size*1.8f);
        return join(quadrilateral(X-.009f,upper-size,.29f,upper+size,.29f,
            lower+size,-.30f,lower-size,-.30f));
    }
    static float[] outfit(HomeAvatarProfile p) {
        float s=shoulder(p),w=waist(p),h=hips(p);
        List<float[]> out=new ArrayList<>();
        switch(p.outfit%8) {
            case 0: // cardigan seam
                out.add(rect(-.04f,.37f,.04f,-.35f)); break;
            case 1: // lapels
                out.add(tri(-s,.34f,-.06f,.34f,0f,-.14f));
                out.add(tri(s,.34f,.06f,.34f,0f,-.14f)); break;
            case 2: // robe and central piping
                out.add(rect(-.10f,.40f,.10f,-.62f));
                out.add(band(X-.023f,-.31f,h,-.69f,h+.10f)); break;
            case 3: // dress or ceremonial skirt
                out.add(band(X-.023f,-.27f,w,-.76f,h+.20f));break;
            case 4: // armored pauldrons
                out.add(rect(-s-.09f,.40f,-s+.10f,.19f));
                out.add(rect(s-.10f,.40f,s+.09f,.19f));
                out.add(rect(-w,-.13f,w,-.24f));break;
            case 5: // working belt
                out.add(rect(-w-.025f,-.09f,w+.025f,-.22f));
                out.add(rect(-.075f,.27f,.075f,-.34f)); break;
            case 6: // diagonal ceremonial sash
                out.add(tri(-s,.39f,-s+.12f,.39f,w,-.31f));
                out.add(tri(-s+.12f,.39f,w,-.31f,w-.12f,-.31f));break;
            case 7: // long coat tails
                out.add(rect(-s,.37f,-s+.105f,-.66f));
                out.add(rect(s-.105f,.37f,s,-.66f));break;
            default:break;
        }
        // Three individually visible embellishment families, yielding 24
        // recognizably different layered outfits without duplicate geometry.
        switch(p.outfit/8) {
            case 0:out.add(rect(-.058f,.07f,.058f,.13f));break;
            case 1:
                out.add(rect(-s*.8f,.28f,s*.8f,.31f));
                out.add(rect(-w,-.33f,w,-.29f));break;
            default:
                out.add(rect(-.03f,.35f,.03f,-.38f));
                out.add(rect(-w,-.30f,w,-.255f));
                out.add(rect(-s*.65f,.15f,s*.65f,.20f));break;
        }
        return join(out.toArray(new float[out.size()][]));
    }
    static float[] hair(HomeAvatarProfile p) {
        switch(p.hair) {
            case 0:return new float[0];
            case 1:return rect(-.24f,.80f,.24f,.88f);
            case 2:return rect(-.27f,.72f,.27f,.89f);
            case 3:return join(rect(-.31f,.77f,.31f,.87f),
                rect(-.31f,.77f,-.24f,.32f),rect(.24f,.77f,.31f,.32f));
            case 4:return join(rect(-.30f,.81f,.30f,.88f),
                rect(-.31f,.79f,-.23f,-.13f),rect(.23f,.79f,.31f,-.13f));
            case 5:return join(rect(-.33f,.80f,.33f,.86f),
                rect(-.36f,.72f,-.29f,.48f),rect(.29f,.72f,.36f,.48f));
            case 6:return join(rect(-.30f,.79f,.30f,.88f),
                rect(-.34f,.68f,-.25f,-.31f),rect(.25f,.68f,.34f,-.31f));
            case 7:return join(rect(-.30f,.80f,.30f,.88f),
                rect(.27f,.72f,.38f,-.40f));
            case 8:return join(rect(-.25f,.78f,.25f,.86f),
                rect(-.14f,.85f,.14f,1.12f));
            case 9:return join(rect(-.22f,.84f,.22f,1.16f),
                rect(-.33f,.74f,-.27f,.81f));
            case 10:return join(rect(-.29f,.84f,.29f,.90f),
                tri(-.30f,.89f,.24f,.89f,-.04f,1.02f));
            case 11:return join(tri(-.30f,.81f,-.14f,.81f,-.29f,1.09f),
                tri(-.12f,.82f,.12f,.82f,0f,1.15f),
                tri(.14f,.81f,.30f,.81f,.29f,1.09f));
            default:return new float[0];
        }
    }
    static float[] face(HomeAvatarProfile p) {
        // Jaw treatment changes independently of chosen body, hairstyle,
        // and body presentation does not constrain other selections.
        float w=p.presentation==1?.27f:p.presentation==0?.22f:.245f;
        return join(band(X-.015f,.81f,w,.55f,w*.87f),
            band(X-.015f,.55f,w*.87f,.42f,w*.67f));
    }
    static float[] band(float x,float yTop,float top,float yBottom,float bottom) {
        return quadrilateral(x,-top,yTop,top,yTop,bottom,yBottom,-bottom,yBottom);
    }
    private static float[] rect(float z0,float y0,float z1,float y1) {
        return HomeAvatarWardrobeGeometry.rectangle(X-.023f,z0,y1,z1,y0);
    }
    private static float[] tri(float az,float ay,float bz,float by,float cz,float cy) {
        return new float[]{X-.024f,ay,az,X-.024f,by,bz,X-.024f,cy,cz};
    }
    private static float[] quadrilateral(float x,float z0,float y0,float z1,float y1,
        float z2,float y2,float z3,float y3) {
        return new float[]{x,y0,z0,x,y1,z1,x,y3,z3,
            x,y1,z1,x,y2,z2,x,y3,z3};
    }
    private static float[] join(float[]... parts) {
        int count=0;for(float[] p:parts)count+=p.length;
        float[] result=new float[count];int at=0;
        for(float[] p:parts){System.arraycopy(p,0,result,at,p.length);at+=p.length;}
        return result;
    }
}
