package io.github.mrcalzon02.reverievr.game.testchamber;

/** Android-free coverage for imported triangle bounds, aim and audio. */
public final class TestChamberTargetingStandaloneTest {
    private static void require(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    private static float[] lookAt(float x,float y,float z){
        float length=(float)Math.sqrt(x*x+y*y+z*z);
        float fx=x/length,fy=y/length,fz=z/length;
        float rx=-fz,rz=fx;
        length=(float)Math.sqrt(rx*rx+rz*rz);
        rx/=length;rz/=length;
        float ux=-rz*fy,uy=rz*fx-rx*fz,uz=rx*fy;
        return new float[]{rx,ux,-fx,0,0,uy,-fy,0,rz,uz,-fz,0,0,0,0,1};
    }
    private static float[] pointMesh(float x,float y,float z){
        return new float[]{
            x-.15f,y-.15f,z,0,0,
            x+.15f,y-.15f,z,1,0,
            x,y+.15f,z,0,1};
    }
    public static void main(String[] args){
        require(TestChamberTargeting.MODEL_COUNT==5,"Unexpected model count");
        require(TestChamberTargeting.pick(lookAt(0,-1.5f,-2))==-1,
            "Unregistered models must not receive phantom gaze hits");
        // The mesh is intentionally OFF-CENTER in each station's local
        // coordinates. A guessed target point would miss these objects.
        float[][] places={
            {-2.25f,-1.50f,-1.92f,1f},
            {-1.17f,-1.50f,-2.08f,1f},
            {1.35f,-1.50f,-2.42f,1f},
            {0,-1.50f,-2.05f,.82f},
            {2.65f,-1.50f,-2.90f,.86f}
        };
        for(int model=0;model<5;model++){
            TestChamberTargeting.includePrimitive(model,pointMesh(.24f,.22f,0));
            float[] p=places[model];
            require(TestChamberTargeting.pick(lookAt(p[0]+p[3]*.24f,
                p[1]+p[3]*.22f,p[2]))==model,
                "Actual textured geometry not selectable: "+model);
        }
        // Submesh union: second triangle moves the bounds, not replaces it.
        TestChamberTargeting.includePrimitive(0,pointMesh(.50f,.20f,-.15f));
        float[] p=places[0];
        require(TestChamberTargeting.pick(lookAt(p[0]+.5f,p[1]+.2f,p[2]-.15f))==0,
            "Second GLB primitive was not incorporated");
        require(TestChamberTargeting.pick(lookAt(0,2,-1))==-1,
            "Empty sky must not select an object");
        require(TestChamberTargeting.pick(lookAt(0,0,1))==-1,
            "Backward gaze must not select front objects");
        require(TestChamberTargeting.pick(null)==-1,"Null view accepted");
        require(TestChamberTargeting.pick(new float[4])==-1,"Short view accepted");
        float[] bad=lookAt(-2.25f,-1.5f,-2);
        bad[0]=Float.NaN;
        require(TestChamberTargeting.pick(bad)==-1,"Corrupt view selected model");
        boolean rejected=false;
        try{TestChamberTargeting.includePrimitive(0,pointMesh(Float.NaN,0,0));}
        catch(IllegalArgumentException expected){rejected=true;}
        require(rejected,"Malformed GLB submesh not rejected");
        boolean[] cues=new boolean[6];
        for(int model=-1;model<5;model++){
            int cue=TestChamberTargeting.cueFor(model);
            require(cue>=0&&cue<6&&!cues[cue],"Duplicate or missing sound mapping");
            cues[cue]=true;
        }
        for(boolean mapped:cues)require(mapped,"Unmapped WAV");
        System.out.println("PASS: imported mesh bounds, five selectable models, submesh union, invalid inputs and six WAV cues");
    }
}
