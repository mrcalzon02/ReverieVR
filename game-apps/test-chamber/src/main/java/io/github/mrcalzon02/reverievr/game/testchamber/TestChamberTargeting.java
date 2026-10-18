package io.github.mrcalzon02.reverievr.game.testchamber;

/**
 * Gaze targets are the actual imported triangle-mesh bounds, not guessed
 * points in an otherwise empty scene. Model transforms MUST match the
 * locations array in companion_showcase_renderer.cpp (TestShowcase::Draw).
 * Register meshes during Activity setup, before Cardboard rendering starts.
 */
final class TestChamberTargeting {
    static final int MODEL_COUNT=5;
    private static final float[][] TRANSFORMS={
        {-2.25f,-1.50f,-1.92f,1.00f}, // crate
        {-1.17f,-1.50f,-2.08f,1.00f}, // chair
        { 1.35f,-1.50f,-2.42f,1.00f}, // shelf
        { 0.00f,-1.50f,-2.05f,0.82f}, // table
        { 2.65f,-1.50f,-2.90f,0.86f}  // door
    };
    // xMin, yMin, zMin, xMax, yMax, zMax. Per-model, accumulating submeshes.
    private static final float[][] bounds=new float[MODEL_COUNT][6];
    private static final boolean[] registered=new boolean[MODEL_COUNT];
    private static final float AIM_PADDING=0.12f;

    static void includePrimitive(int id,float[] xyzuv){
        if(id<0||id>=MODEL_COUNT||xyzuv==null||xyzuv.length<15||
           xyzuv.length%15!=0)throw new IllegalArgumentException("Invalid mesh");
        float[] transform=TRANSFORMS[id];
        float[] next=registered[id]?bounds[id].clone():new float[]{
            Float.POSITIVE_INFINITY,Float.POSITIVE_INFINITY,Float.POSITIVE_INFINITY,
            Float.NEGATIVE_INFINITY,Float.NEGATIVE_INFINITY,Float.NEGATIVE_INFINITY};
        for(int i=0;i<xyzuv.length;i+=5){
            for(int axis=0;axis<3;axis++){
                float point=transform[axis]+transform[3]*xyzuv[i+axis];
                if(!Float.isFinite(point))
                    throw new IllegalArgumentException("Non-finite mesh vertex");
                next[axis]=Math.min(next[axis],point);
                next[axis+3]=Math.max(next[axis+3],point);
            }
        }
        System.arraycopy(next,0,bounds[id],0,6);
        registered[id]=true;
    }

    static int pick(float[] view){
        if(view==null||view.length<16)return -1;
        for(int i=0;i<16;i++)if(!Float.isFinite(view[i]))return -1;
        // Invert the rigid, column-major view transform: camera origin in
        // world space and the world-space direction of head-forward (-Z).
        final float ox=-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]);
        final float oy=-(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]);
        final float oz=-(view[8]*view[12]+view[9]*view[13]+view[10]*view[14]);
        final float dx=-view[2],dy=-view[6],dz=-view[10];
        float nearest=Float.POSITIVE_INFINITY;
        int selected=-1;
        for(int id=0;id<MODEL_COUNT;id++){
            if(!registered[id])continue;
            float tNear=0f,tFar=Float.POSITIVE_INFINITY;
            boolean misses=false;
            for(int axis=0;axis<3;axis++){
                float low=bounds[id][axis]-AIM_PADDING;
                float high=bounds[id][axis+3]+AIM_PADDING;
                float o=axis==0?ox:axis==1?oy:oz;
                float d=axis==0?dx:axis==1?dy:dz;
                if(Math.abs(d)<1.e-7f){
                    if(o<low||o>high){misses=true;break;}
                    continue;
                }
                float a=(low-o)/d,b=(high-o)/d;
                if(a>b){float swap=a;a=b;b=swap;}
                tNear=Math.max(tNear,a);
                tFar=Math.min(tFar,b);
                if(tFar<tNear){misses=true;break;}
            }
            if(!misses&&tFar>=0.1f&&tNear<nearest){
                nearest=tNear;
                selected=id;
            }
        }
        return selected;
    }

    static int cueFor(int model){
        switch(model){
            case 0:return 1; // crate -> wooden knock
            case 1:return 0; // chair -> selection chirp
            case 2:return 4; // shelf -> mechanism
            case 3:return 3; // table -> coin chime
            case 4:return 2; // door -> metallic clang
            default:return 5; // empty space -> beacon
        }
    }
    private TestChamberTargeting(){}
}
