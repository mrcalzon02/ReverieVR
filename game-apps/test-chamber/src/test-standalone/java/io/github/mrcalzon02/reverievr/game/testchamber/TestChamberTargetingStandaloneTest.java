package io.github.mrcalzon02.reverievr.game.testchamber;

/** Run with javac/java on a developer machine; no Android SDK required. */
public final class TestChamberTargetingStandaloneTest {
    private static void require(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    private static float[] lookAt(float[] target){
        float length=(float)Math.sqrt(target[0]*target[0]+target[1]*target[1]+
            target[2]*target[2]);
        float fx=target[0]/length,fy=target[1]/length,fz=target[2]/length;
        float rx=-fz,ry=0f,rz=fx;
        length=(float)Math.sqrt(rx*rx+rz*rz);
        rx/=length;rz/=length;
        float ux=ry*fz-rz*fy;
        float uy=rz*fx-rx*fz;
        float uz=rx*fy-ry*fx;
        return new float[]{rx,ux,-fx,0,ry,uy,-fy,0,rz,uz,-fz,0,0,0,0,1};
    }
    public static void main(String[] args){
        require(TestChamberTargeting.MODEL_COUNT==5,"Unexpected model count");
        for(int model=0;model<5;model++){
            require(TestChamberTargeting.pick(
                lookAt(TestChamberTargeting.CENTERS[model]))==model,
                "Target not selectable: "+model);
        }
        require(TestChamberTargeting.pick(lookAt(new float[]{0f,2f,-1f}))==-1,
            "Empty sky must not select a model");
        require(TestChamberTargeting.pick(lookAt(new float[]{0f,0f,1f}))==-1,
            "Backward gaze must not select front-row objects");
        require(TestChamberTargeting.pick(null)==-1,"Null pose not rejected");
        require(TestChamberTargeting.pick(new float[4])==-1,"Short pose not rejected");
        float[] corrupt=lookAt(TestChamberTargeting.CENTERS[0]);
        corrupt[0]=Float.NaN;
        require(TestChamberTargeting.pick(corrupt)!=0,"NaN pose selected invalid model");
        boolean[] cues=new boolean[6];
        for(int model=-1;model<5;model++){
            int id=TestChamberTargeting.cueFor(model);
            require(id>=0&&id<6&&!cues[id],"Duplicate or missing sound mapping");
            cues[id]=true;
        }
        for(boolean known:cues)require(known,"Unmapped packaged sound effect");
        System.out.println("PASS: all 5 gaze stations, no-target cases, malformed poses, and 6 distinct audio cues");
    }
}
