package io.github.mrcalzon02.reverievr;

/**
 * Analytic two-bone IK and distance-driven foot-target gait for the local
 * stylized avatar. No claim of tracked feet on a 3DoF Daydream headset.
 *
 * Hip, knee and ankle are world-space mirror portrait coordinates, with a
 * stable forward/outward pole vector and fixed thigh/shin lengths. All
 * scratch arrays are owned by the rig and reused without frame allocations.
 */
final class HomeAvatarLegIk {
    static final float THIGH_METERS=.375f;
    static final float SHIN_METERS=.365f;
    static final int JOINT_FLOATS=18; // hip, knee, ankle x3 for two legs
    static final int MESH_FLOATS=108; // 4 limb ribbons + 2 foot ribbons
    private static final float FLOOR_Y=-.79f;
    private static final float HIP_Y=-.26f;
    private static final float SIDE_Z=.185f;
    private static final float STRIDE_METERS=.52f;
    private final float[] joints=new float[JOINT_FLOATS];
    private float previousX,previousZ;
    private float phase;
    private float motionBlend;
    private long lastFrameNanos;
    private boolean previousActive;
    private boolean initialized;

    HomeAvatarLegIk() { solvePose(); }

    void update(float worldX,float worldZ,long frameNanos,boolean enabled) {
        if(!Float.isFinite(worldX)||!Float.isFinite(worldZ)
            ||frameNanos<=0L) return;
        if(!initialized || lastFrameNanos==0L
            ||frameNanos<lastFrameNanos || enabled!=previousActive) {
            previousX=worldX;previousZ=worldZ;
            initialized=true;previousActive=enabled;
            lastFrameNanos=frameNanos;
            if(!enabled)motionBlend=0f;
            solvePose();
            return;
        }
        float dt=Math.min(.05f,Math.max(0f,
            (frameNanos-lastFrameNanos)/1_000_000_000f));
        lastFrameNanos=frameNanos;
        float displacement=enabled
            ?(float)Math.hypot(worldX-previousX,worldZ-previousZ):0f;
        previousX=worldX;previousZ=worldZ;
        // Teleports, recenter or scene reset must not kick legs into a spasm.
        if(displacement>.35f)displacement=0f;
        if(displacement>.00025f) {
            phase=(phase + displacement/STRIDE_METERS
                *(float)(Math.PI*2.0))%(float)(Math.PI*2.0);
        }
        float target=displacement>.00025f ? 1f:0f;
        motionBlend+= (target-motionBlend)*Math.min(1f,dt*12f);
        if(motionBlend<.0005f)motionBlend=0f;
        solvePose();
    }

    /** Caller-owned array: 2 legs * (hip/knee/ankle) * 3 floats. */
    void copyJoints(float[] out) {
        if(out==null||out.length<JOINT_FLOATS)
            throw new IllegalArgumentException("IK joint buffer too short");
        System.arraycopy(joints,0,out,0,JOINT_FLOATS);
    }

    /** A small dynamic 3D mesh: thigh, shin and shoe for each leg. */
    void writeMesh(float[] out) {
        if(out==null||out.length<MESH_FLOATS)
            throw new IllegalArgumentException("IK mesh buffer too short");
        int at=0;
        for(int i=0;i<2;i++) {
            int v=i*9;
            at=ribbon(out,at,v,v+3,.070f);
            at=ribbon(out,at,v+3,v+6,.055f);
            float x=joints[v+6]-.012f;
            float y=joints[v+7]+.012f;
            float z=joints[v+8];
            at=quad(out,at,
                x,y,z-.113f,x,y,z+.113f,
                x,y+.048f,z-.113f,x,y+.048f,z+.113f);
        }
        if(at!=MESH_FLOATS)
            throw new IllegalStateException("IK geometry length mismatch");
    }

    float motionBlend() { return motionBlend; }

    private void solvePose() {
        for(int leg=0;leg<2;leg++) {
            int i=leg*9;
            float side=leg==0?-1f:1f;
            float cycle=phase+(leg==0?0f:(float)Math.PI);
            float progress=(float)Math.sin(cycle);
            // Stance foot remains on floor; swing foot lifts into its arc.
            float ankleZ=side*SIDE_Z + motionBlend*.135f*(float)Math.cos(cycle);
            float ankleY=FLOOR_Y + motionBlend*.095f*Math.max(0f,progress);
            float hipY=HIP_Y + motionBlend*.020f*(float)Math.sin(phase*2f);
            float hipZ=side*SIDE_Z;
            float hx=HomeAvatarWardrobeGeometry.MIRROR_X-.020f;
            set(i,hx,hipY,hipZ);
            // Foot is slightly in front of knee, on the mirror side.
            float fx=HomeAvatarWardrobeGeometry.MIRROR_X-.032f;
            float dy=ankleY-hipY,dx=fx-hx,dz=ankleZ-hipZ;
            float d=(float)Math.sqrt(dx*dx+dy*dy+dz*dz);
            float max=THIGH_METERS+SHIN_METERS-.001f;
            float min=Math.abs(THIGH_METERS-SHIN_METERS)+.001f;
            float safe=Math.max(min,Math.min(max,d));
            float ux=dx/d,uy=dy/d,uz=dz/d;
            if(safe!=d) {
                fx=hx+ux*safe; ankleY=hipY+uy*safe;
                ankleZ=hipZ+uz*safe;
            }
            float a=(THIGH_METERS*THIGH_METERS
                -SHIN_METERS*SHIN_METERS+safe*safe)/(2f*safe);
            float h=(float)Math.sqrt(Math.max(0f,
                THIGH_METERS*THIGH_METERS-a*a));
            // Stable knee bend outward AND toward viewer in mirror preview.
            float px=-.84f,py=0f,pz=side*.54f;
            float dot=px*ux+py*uy+pz*uz;
            px-=dot*ux;py-=dot*uy;pz-=dot*uz;
            float poleLength=(float)Math.sqrt(px*px+py*py+pz*pz);
            if(poleLength<.00001f) {
                px=-1f;py=0f;pz=0f;poleLength=1f;
            }
            float scale=h/poleLength;
            set(i+3,hx+ux*a+px*scale,
                hipY+uy*a+py*scale,hipZ+uz*a+pz*scale);
            set(i+6,fx,ankleY,ankleZ);
        }
    }

    private void set(int i,float x,float y,float z) {
        joints[i]=x;joints[i+1]=y;joints[i+2]=z;
    }

    private int ribbon(float[] out,int at,int a,int b,float width) {
        // Billboard width in mirror-Z while retaining real 3D knee depth.
        float ax=joints[a],ay=joints[a+1],az=joints[a+2];
        float bx=joints[b],by=joints[b+1],bz=joints[b+2];
        return quad(out,at,
            ax,ay,az-width,ax,ay,az+width,
            bx,by,bz-width,bx,by,bz+width);
    }
    private static int quad(float[] out,int at,
        float ax,float ay,float az,float bx,float by,float bz,
        float cx,float cy,float cz,float dx,float dy,float dz) {
        at=put(out,at,ax,ay,az);at=put(out,at,bx,by,bz);
        at=put(out,at,cx,cy,cz);at=put(out,at,bx,by,bz);
        at=put(out,at,dx,dy,dz);return put(out,at,cx,cy,cz);
    }
    private static int put(float[] out,int i,float x,float y,float z) {
        out[i++]=x;out[i++]=y;out[i++]=z;
        return i;
    }
}
