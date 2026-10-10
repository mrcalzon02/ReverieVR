#include "breakwater_campaign.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"

#include <GLES2/gl2.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

namespace {
using namespace reverie::breakwater;
constexpr float kShopZ = -3.0f;
struct State {
    const ReverieNativeHostV1 *host = nullptr;
    Campaign campaign{Difficulty::Regular, 1049u};
    GLuint program = 0, vbo = 0;
    GLint matrix = -1, position = -1, color = -1;
    bool held = false, secondary_held = false, scene_dirty = true, gpu_dirty = true;
    float flash = 0.0f, elapsed = 0.0f;
    std::vector<float> mesh;
};
void Feedback(State *s, uint32_t code) {
    ReverieNativeRequestFeedbackV1(s->host, code);
}
void Vertex(State *s, float x,float y,float z, float r,float g,float b) {
    auto &v=s->mesh;
    v.push_back(x);v.push_back(y);v.push_back(z);
    v.push_back(r);v.push_back(g);v.push_back(b);
}
void Triangle(State *s, float x0,float y0,float z0,
              float x1,float y1,float z1,float x2,float y2,float z2,
              float r,float g,float b) {
    Vertex(s,x0,y0,z0,r,g,b);Vertex(s,x1,y1,z1,r,g,b);Vertex(s,x2,y2,z2,r,g,b);
}
void Panel(State *s,float x,float y,float z,float w,float h,
           float r,float g,float b) {
    const float a=x-w*0.5f, c=x+w*0.5f, d=y-h*0.5f, e=y+h*0.5f;
    Triangle(s,a,d,z,c,d,z,c,e,z,r,g,b);
    Triangle(s,a,d,z,c,e,z,a,e,z,r,g,b);
}
void Box(State *s,float x,float y,float z,float w,float h,float d,
         float r,float g,float b) {
    const float a=x-w*.5f,c=x+w*.5f,l=y-h*.5f,t=y+h*.5f;
    const float n=z-d*.5f,f=z+d*.5f;
    Panel(s,x,y,n,w,h,r,g,b);
    Panel(s,x,y,f,w,h,r*.78f,g*.78f,b*.78f);
    Triangle(s,a,t,n,c,t,n,c,t,f,r*1.12f,g*1.12f,b*1.12f);
    Triangle(s,a,t,n,c,t,f,a,t,f,r*1.12f,g*1.12f,b*1.12f);
    Triangle(s,a,l,n,a,l,f,a,t,f,r*.62f,g*.62f,b*.62f);
    Triangle(s,a,l,n,a,t,f,a,t,n,r*.62f,g*.62f,b*.62f);
    Triangle(s,c,l,n,c,t,f,c,l,f,r*.70f,g*.70f,b*.70f);
    Triangle(s,c,l,n,c,t,n,c,t,f,r*.70f,g*.70f,b*.70f);
}
const char *Glyph(char c) {
    // Five rows of three pixels, a deliberately tiny generated DOS-era font.
    switch(c) {
        case 'A':return "010101111101101"; case 'B':return "110101110101110";
        case 'C':return "011100100100011"; case 'D':return "110101101101110";
        case 'E':return "111100110100111"; case 'F':return "111100110100100";
        case 'G':return "011100101101011"; case 'H':return "101101111101101";
        case 'I':return "111010010010111"; case 'J':return "001001001101010";
        case 'K':return "101101110101101"; case 'L':return "100100100100111";
        case 'M':return "101111111101101"; case 'N':return "101111111111101";
        case 'O':return "111101101101111"; case 'P':return "110101110100100";
        case 'Q':return "111101101111011"; case 'R':return "110101110101101";
        case 'S':return "011100010001110"; case 'T':return "111010010010010";
        case 'U':return "101101101101111"; case 'V':return "101101101101010";
        case 'W':return "101101111111101"; case 'X':return "101101010101101";
        case 'Y':return "101101010010010"; case 'Z':return "111001010100111";
        case '0':return "111101101101111"; case '1':return "010110010010111";
        case '2':return "110001111100111"; case '3':return "110001110001110";
        case '4':return "101101111001001"; case '5':return "111100110001110";
        case '6':return "011100111101111"; case '7':return "111001010010010";
        case '8':return "111101111101111"; case '9':return "111101111001110";
        case '-':return "000000111000000"; case '/':return "001001010100100";
        case ':':return "000010000010000"; default:return "000000000000000";
    }
}
void Text(State *s,const char *text,float x,float y,float z,float scale,
          float r,float g,float b) {
    if (!text) return;
    const size_t len=std::strlen(text);
    float px=x-static_cast<float>(len)*scale*2.0f;
    for(size_t i=0;i<len;i++) {
        const char ascii=text[i];
        const char upper=(ascii>='a' && ascii<='z') ?
            static_cast<char>(ascii-'a'+'A') : ascii;
        const char *pixels=Glyph(upper);
        for(int bit=0;bit<15;bit++) {
            if(pixels[bit]!='1')continue;
            const int cx=bit%3, cy=bit/3;
            Panel(s,px+(cx+.5f)*scale,y+(2.5f-cy)*scale,z,
                  scale*.93f,scale*.93f,r,g,b);
        }
        px+=4*scale;
    }
}
float AimAtPlane(const ReverieNativeInputV1 &input,float z,float *x,float *y) {
    if(input.pointer_kind==REVERIE_NATIVE_POINTER_NONE ||
       std::abs(input.pointer_direction[2])<.00001f)return -1.0f;
    const float distance=(z-input.pointer_origin[2])/input.pointer_direction[2];
    if(distance<=0 || distance>40.0f)return -1.0f;
    *x=input.pointer_origin[0]+distance*input.pointer_direction[0];
    *y=input.pointer_origin[1]+distance*input.pointer_direction[1];
    return distance;
}
int ShopTarget(const ReverieNativeInputV1 &input) {
    float x=0,y=0;
    if(AimAtPlane(input,kShopZ,&x,&y)<0)return -1;
    for(int n=0;n<8;n++) {
        const int col=n%4,row=n/4;
        if(std::abs(x-(-2.1f+col*1.4f))<.62f &&
           std::abs(y-(.05f-row*.65f))<.27f)return n;
    }
    if(std::abs(x)<.88f && y<-.82f && y>-1.40f)return 8;
    return -1;
}
bool AimAtEnemy(const ReverieNativeInputV1 &input,const Enemy &enemy,
                float *distance) {
    if(!enemy.active)return false;
    const float cx=enemy.x,cy=enemy.type==UnitType::Aircraft?1.6f:-.63f,cz=enemy.z;
    const float radius=enemy.type==UnitType::Rowboat?.62f:
         enemy.type==UnitType::LandingCraft?.82f:.78f;
    const float ox=input.pointer_origin[0]-cx;
    const float oy=input.pointer_origin[1]-cy;
    const float oz=input.pointer_origin[2]-cz;
    const float projection=-(ox*input.pointer_direction[0]+
        oy*input.pointer_direction[1]+oz*input.pointer_direction[2]);
    if(projection<=0)return false;
    const float d2=ox*ox+oy*oy+oz*oz-projection*projection;
    if(d2>radius*radius)return false;
    *distance=projection;
    return true;
}
int CombatTarget(const ReverieNativeInputV1 &input,const Campaign &game) {
    if(input.pointer_kind==REVERIE_NATIVE_POINTER_NONE)return -1;
    float best=1e9f;int index=-1;
    for(uint8_t i=0;i<game.enemies().size();i++) {
        float depth=0;
        if(AimAtEnemy(input,game.enemies()[i],&depth) && depth<best){
            best=depth; index=i;
        }
    }
    return index;
}
constexpr char kSaveSlot[]="breakwater-v1.bin";
bool SaveState(State *s) {
    if(!ReverieNativeHostSupportsSaveV1(s->host))return false;
    uint8_t bytes[Campaign::kSerializedSize]={};
    if(!s->campaign.SerializeShop(bytes,sizeof(bytes)))return false;
    return s->host->write_save(kSaveSlot,bytes,
        static_cast<uint32_t>(sizeof(bytes)))==REVERIE_NATIVE_SAVE_OK;
}
void LoadState(State *s) {
    if(!ReverieNativeHostSupportsSaveV1(s->host))return;
    uint8_t bytes[Campaign::kSerializedSize]={};
    uint32_t size=0;
    const int32_t status=s->host->read_save(kSaveSlot,bytes,
        static_cast<uint32_t>(sizeof(bytes)),&size);
    if(status==REVERIE_NATIVE_SAVE_NOT_FOUND)return;
    if(status!=REVERIE_NATIVE_SAVE_OK || size!=sizeof(bytes) ||
       !s->campaign.DeserializeShop(bytes,size)) {
        ReverieNativeLogV1(s->host,REVERIE_NATIVE_LOG_WARN,"Breakwater",
            "Save rejected; keeping fresh campaign instead of resetting partially.");
    }
}
void Interact(State *s,const ReverieNativeInputV1 &input,bool edge) {
    Campaign &game=s->campaign;
    if(game.phase()==Phase::Shop) {
        if(!edge)return;
        const int target=ShopTarget(input);
        bool done=false;
        if(target==8)done=game.BeginWave();
        else if(target>=0 && target<8){
            done=game.Purchase(static_cast<Upgrade>(target));
            if(done)SaveState(s);
        }
        Feedback(s,done?REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
    } else if(game.phase()==Phase::Combat) {
        float x=0,y=0;
        if(edge && AimAtPlane(input,-1.8f,&x,&y)>0 && y>.65f && y<1.25f){
            if(x<-1.05f && x>-3.4f){
                const bool ok=game.CallArtillery();
                Feedback(s,ok?REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
                return;
            }
            if(x>1.05f && x<3.4f){
                const bool ok=game.CallAirstrike();
                Feedback(s,ok?REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
                return;
            }
        }
        const int target=CombatTarget(input,game);
        bool shot=false;
        if(target>=0)shot=game.FireAt(static_cast<uint8_t>(target));
        else if(game.infantry()>0 &&
                AimAtPlane(input,-2.2f,&x,&y)>0 &&
                std::abs(x)<1.6f && y<.0f && y>-1.3f)
            shot=game.FireAtInfantry();
        if(shot){s->flash=.09f;Feedback(s,REVERIE_NATIVE_FEEDBACK_ACTIVATION);}
        else if(edge)Feedback(s,REVERIE_NATIVE_FEEDBACK_FAILURE);
    } else if(edge && (game.phase()==Phase::Defeat || game.phase()==Phase::Victory)) {
        game=Campaign(Difficulty::Regular,1049u);
        game.Start();
        SaveState(s);
        Feedback(s,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
    }
}
void BuildScene(State *s) {
    auto &game=s->campaign;
    s->mesh.clear();
    const bool night=game.period()==Period::Night;
    // Generated sky bands and large sparse sea strips, no image assets.
    for(int row=0;row<10;row++) {
        const float bottom=-.2f+row*1.7f;
        Panel(s,0,bottom+.86f,-26.5f,42.0f,1.73f,
              night?.025f+.008f*row:.13f+.035f*row,
              night?.045f+.010f*row:.24f+.035f*row,
              night?.08f+.012f*row:.36f+.03f*row);
    }
    for(int z=0;z<24;z++) {
        const float back=-2.0f-z*.92f;
        const float shimmer=.02f*std::sin(s->elapsed*1.6f+z*.7f);
        Triangle(s,-20,-1.10f,back,20,-1.10f,back,
                 20,-1.10f,back-.92f,
                 night?.035f:.08f,night?.13f:.31f,night?.23f:.43f+shimmer);
        Triangle(s,-20,-1.10f,back,20,-1.10f,back-.92f,
                 -20,-1.10f,back-.92f,
                 night?.04f:.09f,night?.15f:.36f,night?.27f:.49f+shimmer);
    }
    // Shore parapet, mounted barrel and enemy silhouettes.
    Box(s,0,-1.24f,-.95f,7.8f,.55f,.7f,.37f,.38f,.36f);
    Box(s,0,-.83f,-1.2f,.65f,.38f,1.9f,.27f,.29f,.31f);
    if(game.stats().turret)
        Box(s,2.15f,-.76f,-1.40f,.42f,.32f,1.14f,.36f,.44f,.39f);
    for(const Enemy &e:game.enemies())if(e.active) {
        const bool aircraft=e.type==UnitType::Aircraft;
        const float y=aircraft?1.6f:-.68f;
        const float width=e.type==UnitType::Rowboat?.7f:
            e.type==UnitType::LandingCraft?1.1f:1.25f;
        Box(s,e.x,y,e.z,width,.29f,1.0f,
            aircraft?.65f:.40f,aircraft?.57f:.36f,aircraft?.47f:.26f);
        if(aircraft)Box(s,e.x,y+.04f,e.z,.25f,.10f,2.1f,.44f,.46f,.43f);
        else Box(s,e.x,y+.20f,e.z,.31f,.17f,.48f,.25f,.25f,.24f);
    }
    for(int n=0;n<game.infantry();n++) {
        const float x=(n%7-3)*.39f;
        Box(s,x,-.75f,-2.25f,.14f,.48f,.18f,.51f,.45f,.35f);
    }
    if(game.phase()==Phase::Shop) {
        Panel(s,0,.4f,-3.06f,6.35f,3.75f,.08f,.12f,.16f);
        Text(s,"BREAKWATER",0,1.67f,-3.0f,.073f,.92f,.86f,.58f);
        Text(s,Campaign::Locations()[game.location()].name,0,1.28f,-3.0f,
             .055f,.69f,.78f,.76f);
        Text(s,night?"NIGHT":"DAY",-1.55f,.88f,-3.0f,.065f,.93f,.81f,.62f);
        char digits[32];std::snprintf(digits,sizeof(digits),"D%u W%u/%u",
               static_cast<unsigned>(game.day()),
               static_cast<unsigned>(game.wave_index()),
               static_cast<unsigned>(game.waves_this_period()));
        Text(s,digits,1.05f,.88f,-3.0f,.053f,.90f,.89f,.82f);
        const char *labels[]={"FIRE","POWER","CLIP","ANTI","ARTY","AIR","TURRET","REPAIR"};
        for(int n=0;n<8;n++) {
            const float x=-2.1f+(n%4)*1.4f,y=.05f-(n/4)*.65f;
            const int price=game.price(static_cast<Upgrade>(n));
            Panel(s,x,y,-3.0f,1.23f,.49f,price<0?.20f:.21f,
                  price<0?.21f:.32f,price<0?.23f:.26f);
            Text(s,labels[n],x,y+.10f,-2.98f,.051f,.94f,.92f,.72f);
            char amount[20];
            if(price<0)std::snprintf(amount,sizeof(amount),"MAX");
            else std::snprintf(amount,sizeof(amount),"%d",price);
            Text(s,amount,x,y-.10f,-2.98f,.053f,.89f,.89f,.84f);
        }
        Panel(s,0,-1.11f,-3.0f,1.72f,.48f,.27f,.45f,.33f);
        Text(s,"START",0,-1.10f,-2.97f,.080f,1,.97f,.78f);
        char credits[32];std::snprintf(credits,sizeof(credits),"CREDITS %d",game.credits());
        Text(s,credits,0,-1.7f,-2.96f,.062f,.90f,.84f,.62f);
    } else if(game.phase()==Phase::Combat) {
        char digits[32];std::snprintf(digits,sizeof(digits),"HULL %d",game.integrity());
        Text(s,digits,0,1.20f,-2.0f,.075f,.92f,.92f,.74f);
        char ammo[32];
        std::snprintf(ammo,sizeof(ammo),"CLIP %u",
            static_cast<unsigned>(game.magazine_left()));
        Text(s,ammo,0,.93f,-1.79f,.055f,.88f,.9f,.68f);
        Text(s,"ARTY",-2.07f,.97f,-1.79f,.078f,.89f,.81f,.42f);
        Text(s,"AIR",2.10f,.97f,-1.79f,.078f,.89f,.81f,.42f);
        if(s->flash>0)Panel(s,0,-.65f,-1.85f,.28f,.20f,1,.82f,.25f);
    } else {
        Panel(s,0,0,-3.0f,4.2f,1.6f,.07f,.10f,.14f);
        Text(s,game.phase()==Phase::Victory?"VICTORY":"BATTERY LOST",
             0,.3f,-2.95f,.09f,.9f,.81f,.50f);
        Text(s,"FIRE TO RESTART",0,-.35f,-2.95f,.06f,.87f,.82f,.69f);
    }
}
void DestroyGl(State *s) {
    if(s->vbo)glDeleteBuffers(1,&s->vbo);
    if(s->program)glDeleteProgram(s->program);
    s->vbo=0;s->program=0;
    s->scene_dirty=true;s->gpu_dirty=true;
}
void *Create(const ReverieNativeHostV1 *host) {
    if(!ReverieNativeHostSupportsLogV1(host))return nullptr;
    State *s=new(std::nothrow) State();
    if(!s)return nullptr;
    s->host=host;
    s->mesh.reserve(81920);
    s->campaign.Start();
    LoadState(s);
    return s;
}
void Destroy(void *instance) { delete static_cast<State *>(instance); }
int32_t GlCreate(void *instance) {
    State *s=static_cast<State *>(instance);
    if(!s)return 0;
    ReverieNativeGlStateV1 previous={};
    ReverieNativeGlStateCaptureV1(&previous);
    s->vbo=0;s->program=0;
    const char *vs="uniform mat4 u_Mvp; attribute vec3 a_Pos; attribute vec3 a_Col;"
                   "varying vec3 v_Col;void main(){gl_Position=u_Mvp*vec4(a_Pos,1.);"
                   "v_Col=a_Col;}";
    const char *fs="precision mediump float;varying vec3 v_Col;"
                   "void main(){gl_FragColor=vec4(v_Col,1.);}";
    const ReverieNativeGlAttributeBindingV1 bindings[]={{0u,"a_Pos"},{1u,"a_Col"}};
    s->program=ReverieNativeBuildProgram(vs,fs,bindings,2);
    if(s->program) {
        s->position=glGetAttribLocation(s->program,"a_Pos");
        s->color=glGetAttribLocation(s->program,"a_Col");
        s->matrix=glGetUniformLocation(s->program,"u_Mvp");
        glGenBuffers(1,&s->vbo);
    }
    const bool okay=s->program && s->vbo && s->position>=0 &&
        s->color>=0 && s->matrix>=0;
    if(!okay)DestroyGl(s);
    s->scene_dirty=true;s->gpu_dirty=true;
    ReverieNativeGlStateRestoreV1(&previous);
    return okay?1:0;
}
void GlRelease(void *instance) {
    State *s=static_cast<State *>(instance);if(s)DestroyGl(s);
}
void Resume(void *) {}
void Pause(void *) {}
void Update(void *instance,const ReverieNativeInputV1 *input) {
    State *s=static_cast<State *>(instance);
    if(!s || !ReverieNativeInputHasPointerV1(input))return;
    ReverieNativeInputV1 copy=*input;
    ReverieNativeSanitizeBaseInputV1(&copy);
    ReverieNativeSanitizePointerV1(&copy);
    const bool down=copy.primary_down!=0;
    const bool secondary=copy.secondary_down!=0;
    s->elapsed+=copy.delta_seconds;
    s->flash=std::max(0.0f,s->flash-copy.delta_seconds);
    if(s->campaign.phase()==Phase::Combat) {
        s->campaign.Tick(copy.delta_seconds);
        if(s->campaign.phase()==Phase::Shop)SaveState(s);
    }
    if(down)Interact(s,copy,!s->held);
    if(secondary&&!s->secondary_held && s->campaign.phase()==Phase::Shop)
        s->campaign.BeginWave();
    s->held=down;s->secondary_held=secondary;
    s->scene_dirty=true;
}
int32_t Draw(void *instance,const ReverieNativeEyeV1 *eye) {
    State *s=static_cast<State *>(instance);
    if(!s || !ReverieNativeEyeRenderableV1(eye) || !s->program || !s->vbo)return 0;
    if(s->scene_dirty) {
        BuildScene(s);
        s->scene_dirty=false;s->gpu_dirty=true;
    }
    ReverieNativeGlStateV1 prior={};
    ReverieNativeGlAttribStateV1 pa={},ca={};
    ReverieNativeGlStateCaptureV1(&prior);
    ReverieNativeGlAttribCaptureV1(&pa,static_cast<GLuint>(s->position));
    ReverieNativeGlAttribCaptureV1(&ca,static_cast<GLuint>(s->color));
    glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
    glUseProgram(s->program);
    glBindBuffer(GL_ARRAY_BUFFER,s->vbo);
    if(s->gpu_dirty) {
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(s->mesh.size()*sizeof(float)),
                     s->mesh.data(),GL_DYNAMIC_DRAW);
        s->gpu_dirty=false;
    }
    glEnableVertexAttribArray(static_cast<GLuint>(s->position));
    glEnableVertexAttribArray(static_cast<GLuint>(s->color));
    glVertexAttribPointer(static_cast<GLuint>(s->position),3,GL_FLOAT,GL_FALSE,
                          6*sizeof(float),reinterpret_cast<void *>(0));
    glVertexAttribPointer(static_cast<GLuint>(s->color),3,GL_FLOAT,GL_FALSE,
                          6*sizeof(float),reinterpret_cast<void *>(3*sizeof(float)));
    float vp[16];
    ReverieNativeMat4Multiply(vp,eye->projection,eye->view);
    glUniformMatrix4fv(s->matrix,1,GL_FALSE,vp);
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(s->mesh.size()/6));
    ReverieNativeGlAttribRestoreV1(&ca);
    ReverieNativeGlAttribRestoreV1(&pa);
    ReverieNativeGlStateRestoreV1(&prior);
    return glGetError()==GL_NO_ERROR?1:0;
}
const ReverieNativeModuleApiV1 kApi={
    sizeof(ReverieNativeModuleApiV1),REVERIE_NATIVE_MODULE_ABI_VERSION,
    {sizeof(ReverieNativeModuleDescriptorV1),
     REVERIE_NATIVE_MODULE_ABI_VERSION,
     "breakwater-battery","Breakwater Battery VR [DEV]",2u,0u},
    Create,Destroy,GlCreate,GlRelease,Resume,Pause,Update,Draw,nullptr
};
} // namespace
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1 *reverie_native_module_entry_v1(void) {
    return &kApi;
}
