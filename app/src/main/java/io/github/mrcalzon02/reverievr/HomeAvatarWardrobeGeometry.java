package io.github.mrcalzon02.reverievr;

import java.util.ArrayList;
import java.util.List;

/** Static triangular silhouette geometry alongside the existing mirror. */
final class HomeAvatarWardrobeGeometry {
    private HomeAvatarWardrobeGeometry() {}
    static final float MIRROR_X=2.12f;
    static final float Z_MIN=-2.47f,Z_MAX=-1.07f;
    static float[] rectangle(float x,float z0,float y0,float z1,float y1) {
        return new float[]{x,y0,z0,x,y0,z1,x,y1,z0,
            x,y0,z1,x,y1,z1,x,y1,z0};
    }
    static float[] wardrobeCase(){return rectangle(2.055f,Z_MIN,-1.29f,Z_MAX,1.33f);}
    static float[] wardrobeDoor(){return rectangle(2.016f,Z_MIN+.10f,-1.18f,Z_MAX-.10f,1.20f);}
    static float[] wardrobeTrim(){
        return join(rectangle(2.006f,Z_MIN,-1.30f,Z_MIN+.10f,1.34f),
            rectangle(2.006f,Z_MAX-.10f,-1.30f,Z_MAX,1.34f),
            rectangle(2.006f,Z_MIN,1.20f,Z_MAX,1.34f),
            rectangle(2.006f,Z_MIN,-1.30f,Z_MAX,-1.18f),
            rectangle(1.999f,-1.785f,-1.18f,-1.745f,1.20f));
    }
    static float[] wardrobeHandles(){return join(
        rectangle(1.991f,-1.88f,-.03f,-1.81f,.20f),
        rectangle(1.991f,-1.73f,-.03f,-1.66f,.20f));}
    /** Camera can click the front face; never activate from behind the cabinet. */
    static float hitDistance(float[] origin,float[] ray) {
        if(origin==null||ray==null||origin.length<3||ray.length<3
            ||!Float.isFinite(ray[0])||ray[0]<=.0001f)return -1f;
        float t=(1.975f-origin[0])/ray[0];
        if(!Float.isFinite(t)||t<=0f)return -1f;
        float y=origin[1]+t*ray[1],z=origin[2]+t*ray[2];
        return Float.isFinite(y)&&Float.isFinite(z)&&y>=-1.29f&&y<=1.33f
            && z>=Z_MIN&&z<=Z_MAX?t:-1f;
    }
    static float[] legs(){return join(
        rectangle(MIRROR_X,-.30f,-.78f,-.06f,-.28f),
        rectangle(MIRROR_X,.06f,-.78f,.30f,-.28f));}
    static float[] arms(){return join(
        rectangle(MIRROR_X,-.49f,-.33f,-.36f,.30f),
        rectangle(MIRROR_X,.36f,-.33f,.49f,.30f));}
    static float[] torso(){return rectangle(MIRROR_X,-.37f,-.35f,.37f,.40f);}
    static float[] head(){return rectangle(MIRROR_X-.006f,-.27f,.39f,.27f,.81f);}
    static float[] face(){return rectangle(MIRROR_X-.012f,-.19f,.52f,.19f,.70f);}
    static float[] eyes(int type){
        float height=type==1?.025f:type==2?.07f:.045f;
        return join(rectangle(MIRROR_X-.018f,-.15f,.60f,-.055f,.60f+height),
            rectangle(MIRROR_X-.018f,.055f,.60f,.15f,.60f+height));
    }
    static float[] headwear(int type) {
        switch(type) {
            case 1:return join(rectangle(MIRROR_X-.019f,-.35f,.78f,.35f,.88f),
                rectangle(MIRROR_X-.020f,-.44f,.75f,.44f,.79f));
            case 2:return join(rectangle(MIRROR_X-.019f,-.35f,.78f,.35f,.85f),
                triangle(-.29f,.85f,.29f,.85f,0f,1.20f));
            case 3:return join(rectangle(MIRROR_X-.019f,-.22f,.81f,-.16f,1.17f),
                rectangle(MIRROR_X-.019f,.16f,.81f,.22f,1.17f));
            case 4:return join(triangle(-.30f,.77f,-.10f,.77f,-.24f,1.12f),
                triangle(.10f,.77f,.30f,.77f,.24f,1.12f));
            case 5:return join(rectangle(MIRROR_X-.019f,-.27f,.79f,.27f,1.15f),
                rectangle(MIRROR_X-.020f,-.40f,.75f,.40f,.81f));
            case 6:return join(rectangle(MIRROR_X-.019f,-.30f,.80f,.30f,.90f),
                triangle(-.30f,.88f,-.10f,.88f,-.22f,1.12f),
                triangle(-.07f,.88f,.07f,.88f,0f,1.16f),
                triangle(.10f,.88f,.30f,.88f,.22f,1.12f));
            case 7:return rectangle(MIRROR_X-.019f,-.28f,1.01f,.28f,1.07f);
            case 8:return join(rectangle(MIRROR_X-.019f,-.31f,.79f,.31f,.84f),
                triangle(-.32f,.83f,-.25f,.83f,-.29f,.98f),
                triangle(.25f,.83f,.32f,.83f,.29f,.98f));
            case 9:return join(rectangle(MIRROR_X-.019f,-.29f,.75f,.29f,1.07f),
                rectangle(MIRROR_X-.021f,-.34f,.55f,-.25f,.89f),
                rectangle(MIRROR_X-.021f,.25f,.55f,.34f,.89f));
            case 10:return join(rectangle(MIRROR_X-.019f,-.39f,.76f,.39f,.83f),
                triangle(-.35f,.83f,-.01f,.83f,-.25f,1.08f),
                triangle(.01f,.83f,.35f,.83f,.25f,1.08f),
                triangle(-.11f,.81f,.11f,.81f,0f,1.11f));
            case 11:return join(rectangle(MIRROR_X-.019f,-.30f,.78f,.30f,.94f),
                rectangle(MIRROR_X-.020f,-.43f,.75f,.18f,.80f));
            case 12:return join(rectangle(MIRROR_X-.019f,-.32f,.73f,-.23f,.37f),
                rectangle(MIRROR_X-.019f,.23f,.73f,.32f,.37f),
                rectangle(MIRROR_X-.019f,-.32f,.77f,.32f,.93f));
            case 13:return join(rectangle(MIRROR_X-.020f,-.31f,.78f,.31f,.84f),
                rectangle(MIRROR_X-.021f,-.07f,.82f,.07f,.96f));
            case 14:return rectangle(MIRROR_X-.023f,-.31f,.59f,.31f,.71f);
            case 15:return join(rectangle(MIRROR_X-.019f,-.31f,.78f,.31f,.84f),
                triangle(-.31f,.83f,-.19f,.83f,-.25f,.99f),
                triangle(-.06f,.83f,.06f,.83f,0f,1.02f),
                triangle(.19f,.83f,.31f,.83f,.25f,.99f));
            default:return new float[0];
        }
    }
    static float[] accessory(int type) {
        switch(type) {
            case 1:return join(rectangle(MIRROR_X+.01f,-.58f,-.79f,-.40f,.35f),
                rectangle(MIRROR_X+.01f,.40f,-.79f,.58f,.35f));
            case 2:return join(triangle(-.34f,.35f,-.20f,.35f,.36f,-.28f),
                triangle(-.34f,.35f,.36f,-.28f,.22f,-.28f));
            case 3:return rectangle(MIRROR_X-.018f,-.10f,.07f,.10f,.23f);
            case 4:return join(triangle(-.42f,.25f,-.42f,-.37f,-.83f,.03f),
                triangle(.42f,.25f,.42f,-.37f,.83f,.03f));
            case 5:return rectangle(MIRROR_X-.018f,-.37f,-.25f,.37f,-.14f);
            case 6:return join(triangle(-.16f,.06f,.16f,.06f,0f,.30f),
                triangle(-.16f,.24f,.16f,.24f,0f,-.03f));
            case 7:return join(triangle(-.04f,.34f,-.04f,.17f,-.27f,.27f),
                triangle(.04f,.34f,.04f,.17f,.27f,.27f));
            case 8:return join(rectangle(MIRROR_X-.017f,-.53f,.17f,-.31f,.41f),
                rectangle(MIRROR_X-.017f,.31f,.17f,.53f,.41f));
            case 9:return join(rectangle(MIRROR_X-.017f,.33f,-.31f,.52f,-.68f),
                triangle(-.35f,.34f,-.24f,.34f,.47f,-.42f));
            case 10:return join(rectangle(MIRROR_X-.018f,-.34f,.29f,.34f,.37f),
                rectangle(MIRROR_X-.019f,.30f,.29f,.40f,-.44f));
            case 11:return join(rectangle(MIRROR_X-.024f,-.34f,.55f,-.02f,.69f),
                rectangle(MIRROR_X-.024f,.02f,.55f,.34f,.69f));
            case 12:return join(rectangle(MIRROR_X-.018f,-.62f,-.33f,-.49f,.29f),
                rectangle(MIRROR_X-.018f,.49f,-.33f,.62f,.29f),
                rectangle(MIRROR_X-.019f,-.68f,-.40f,-.46f,-.30f),
                rectangle(MIRROR_X-.019f,.46f,-.40f,.68f,-.30f));
            case 13:return join(triangle(-.42f,.33f,-.73f,-.23f,-.54f,.38f),
                triangle(.42f,.33f,.73f,-.23f,.54f,.38f));
            case 14:return join(rectangle(MIRROR_X-.019f,-.04f,.22f,.04f,-.06f),
                rectangle(MIRROR_X-.020f,-.16f,-.04f,.16f,-.19f));
            case 15:return join(rectangle(MIRROR_X+.012f,-.42f,.27f,.42f,-.51f),
                rectangle(MIRROR_X-.018f,-.35f,.34f,-.28f,-.32f),
                rectangle(MIRROR_X-.018f,.28f,.34f,.35f,-.32f));
            default:return new float[0];
        }
    }
    private static float[] triangle(float z0,float y0,float z1,float y1,float z2,float y2) {
        return new float[]{MIRROR_X-.019f,y0,z0,MIRROR_X-.019f,y1,z1,MIRROR_X-.019f,y2,z2};
    }
    private static float[] join(float[]...parts) {
        int total=0;for(float[] p:parts)total+=p.length;
        float[] out=new float[total];int offset=0;
        for(float[] p:parts){System.arraycopy(p,0,out,offset,p.length);offset+=p.length;}
        return out;
    }
}
