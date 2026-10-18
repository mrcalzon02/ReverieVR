package io.github.mrcalzon02.reverievr.game.testchamber;

/**
 * Android-free gaze targeting for five physical GLB test stations.
 * Uses the same column-major world-to-head view convention as GLES2.
 * The cue table intentionally covers every station plus empty space.
 */
final class TestChamberTargeting {
    static final int MODEL_COUNT=5;
    static final float[][] CENTERS={
        {-2.25f,-1.15f,-1.92f}, // crate
        {-1.17f,-1.02f,-2.08f}, // chair
        { 1.35f,-0.85f,-2.42f}, // shelf
        { 0.00f,-1.05f,-2.05f}, // table
        { 2.20f,-0.45f,-2.90f}  // door
    };
    static int pick(float[] view){
        if(view==null||view.length<16)return -1;
        int selected=-1;
        float nearest=0.23f*0.23f;
        for(int i=0;i<MODEL_COUNT;i++){
            final float[] p=CENTERS[i];
            final float x=view[0]*p[0]+view[4]*p[1]+view[8]*p[2]+view[12];
            final float y=view[1]*p[0]+view[5]*p[1]+view[9]*p[2]+view[13];
            final float z=view[2]*p[0]+view[6]*p[1]+view[10]*p[2]+view[14];
            if(!Float.isFinite(x)||!Float.isFinite(y)||!Float.isFinite(z)||
               z>=-0.1f)continue;
            final float squared=(x*x+y*y)/(z*z);
            if(squared<nearest){nearest=squared;selected=i;}
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
            default:return 5; // nothing focused -> beacon
        }
    }
    private TestChamberTargeting(){}
}
