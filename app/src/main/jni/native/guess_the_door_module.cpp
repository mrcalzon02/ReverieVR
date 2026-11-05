// Guess the Door: seated, deterministic native VR memory maze.
// Current milestone uses compact original vector doors. A reviewed Mechanist
// closed/open atlas remains a separate, explicitly unaccepted art-import gate.
#include "guess_the_door_game.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include "reverie_native_flat_mesh.h"
#include "reverie_native_lato_text.h"
#include <GLES2/gl2.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <new>
#include <vector>

namespace {
using reverie::doors::Game;
using reverie::doors::Phase;
constexpr char kId[]="guess-the-door";
constexpr char kTitle[]="Guess the Door [DEV]";
struct State {
    const ReverieNativeHostV1* host=nullptr;
    Game game;
    bool held=false,axisHeld=false,upload=true,hasPointer=false;
    int choice=0,doorGaze=-1,leverGaze=-1;
    float gazeSeconds=0.f;
    GLuint shader=0,vbo=0;
    GLint pos=-1,col=-1,mvp=-1;
    reverie::nativetext::LatoLayer font;
    std::vector<float> mesh;
};
void Tri(State* s,float ax,float ay,float az,float bx,float by,float bz,
         float cx,float cy,float cz,float r,float g,float b){
    auto& v=s->mesh;
    reverie::flatmesh::Vertex(v,ax,ay,az,r,g,b);
    reverie::flatmesh::Vertex(v,bx,by,bz,r,g,b);
    reverie::flatmesh::Vertex(v,cx,cy,cz,r,g,b);
}
void Quad(State* s,float ax,float ay,float az,float bx,float by,float bz,
          float cx,float cy,float cz,float dx,float dy,float dz,
          float r,float g,float b){
    Tri(s,ax,ay,az,bx,by,bz,cx,cy,cz,r,g,b);
    Tri(s,ax,ay,az,cx,cy,cz,dx,dy,dz,r,g,b);
}
void Rect(State* s,float x,float y,float w,float h,float z,
          float r,float g,float b){
    reverie::flatmesh::Rect(s->mesh,x,y,w,h,z,r,g,b);
}
void Line(State* s,float x,float y,float xx,float yy,float w,float z,
          float r,float g,float b){
    reverie::flatmesh::Segment(s->mesh,x,y,xx,yy,w,z,r,g,b);
}
void Text(State* s,const char* t,float x,float y,float w,float size,
          float r,float g,float b){
    s->font.Fit(t,x,y,w,size,r,g,b,-2.05f);
}
float Color(unsigned variant,unsigned offset){
    static const float k[]={.23f,.37f,.46f,.31f,.43f,.54f,.42f,.33f,
        .27f,.50f,.30f,.29f,.44f,.42f,.28f,.51f};
    return k[(variant+offset)%16u];
}
float Zoom(const Game& g){
    if(g.phase==Phase::Opening)return .25f*g.Progress();
    if(g.phase==Phase::Walking)return .25f+1.45f*g.Progress();
    if(g.phase==Phase::Falling)return .3f*g.Progress();
    return 0.f;
}
void Door(State* s,int side,float dx,float dz){
    const auto& game=s->game;
    const unsigned variant=side==0?game.current.leftDoor:game.current.rightDoor;
    const unsigned glyph=side==0?game.current.leftSymbol:game.current.rightSymbol;
    const float x=(side==0?-1.07f:1.07f)+dx,z=-3.43f+dz;
    const bool chosen=game.side==side&&game.phase!=Phase::Ready;
    const float opening=chosen&&(game.phase==Phase::Opening||
        game.phase==Phase::Walking||game.phase==Phase::Falling)?
        (game.phase==Phase::Opening?game.Progress():1.f):0.f;
    // Dark portal remains fixed behind the door slab. Wrong doors lead to a
    // visible black pit, with a reduced-motion fade rather than view rotation.
    Rect(s,x,0.f,1.25f,2.10f,z-.06f,.016f,.019f,.024f);
    Rect(s,x,1.05f,1.43f,.16f,z-.02f,.46f,.37f,.20f);
    Rect(s,x,-1.05f,1.43f,.13f,z-.02f,.31f,.32f,.35f);
    Rect(s,x-.69f,0.f,.13f,2.14f,z-.02f,.42f,.38f,.30f);
    Rect(s,x+.69f,0.f,.13f,2.14f,z-.02f,.42f,.38f,.30f);
    // Hinged leaf represented by lateral panel movement before art-sprite swap.
    const float width=1.18f*(1.f-opening*.96f);
    const float slide=opening*.54f*(side==0?-1.f:1.f);
    const float r=Color(variant,0u),g=Color(variant,5u),b=Color(variant,10u);
    Rect(s,x+slide,0.f,width,2.0f,z,r,g,b);
    if(width>.20f){
        for(int stripe=0;stripe<3;stripe++){
            const float h=.62f-.02f*stripe;
            Rect(s,x+slide, .60f-stripe*.60f,width*.86f,.055f,z+.008f,
                 r*.63f,g*.63f,b*.63f);
            if(variant%2u==0u)Line(s,x+slide-width*.32f,h,
                 x+slide+width*.32f,h,.025f,z+.011f,.60f,.53f,.29f);
        }
        Rect(s,x+slide+(side==0?.36f:-.36f)*(1.f-opening),
             -.10f,.10f,.10f,z+.015f,.84f,.76f,.40f);
    }
    // Symbols are stable visual information, not color-only correctness clues.
    Rect(s,x,.83f,.72f,.34f,z+.021f,.10f,.12f,.13f);
    const char* symbol=reverie::doors::kSymbolNames[glyph];
    Text(s,symbol,x-.27f,.78f,.56f,.027f,.97f,.87f,.61f);
    const float lx=x+(side==0?-.93f:.93f),ly=-.39f;
    reverie::flatmesh::Disc(s->mesh,lx,ly,.18f,z+.035f,.19f,.21f,.24f);
    const float lever=chosen&&game.phase!=Phase::Ready?
        (game.phase==Phase::Pulling?game.Progress():1.f):0.f;
    Line(s,lx,ly,lx+(side==0?-.12f:.12f)+lever*.18f,
         ly+.58f-lever*.55f,.083f,z+.055f,.83f,.70f,.39f);
    reverie::flatmesh::Disc(s->mesh,
        lx+(side==0?-.12f:.12f)+lever*.18f,
        ly+.58f-lever*.55f,.10f,z+.075f,.92f,.65f,.34f);
    if(game.phase==Phase::Ready &&
       (s->choice==side||s->leverGaze==side)){
        Line(s,lx-.23f,ly-.34f,lx+.23f,ly-.34f,.045f,z+.06f,
             .97f,.77f,.31f);
    }
}
void Scene(State* s) {
    s->mesh.clear();s->font.Clear();
    const Game& g=s->game;
    const float advance=Zoom(g);
    const float shift=(g.side<0?0.f:(g.side==0?1.f:-1.f))*
        (g.phase==Phase::Walking?g.Progress()*.86f:
         g.phase==Phase::Falling?g.Progress()*.16f:0.f);
    const float z=-5.5f+advance*.38f;
    Rect(s,0.f,.08f,10.0f,4.30f,-5.45f+advance*.38f,
         .22f,.24f,.25f);
    // Actual three-dimensional floor, ceiling, side panels and back wall;
    // restrained geometry, no scene-wide transforms or per-eye simulation.
    Quad(s,-3.5f,-1.1f,-2.2f,3.5f,-1.1f,-2.2f,
            5.f,-1.85f,z,-5.f,-1.85f,z,.13f,.14f,.16f);
    Quad(s,-3.5f,1.65f,-2.2f,-5.f,2.3f,z,
            5.f,2.3f,z,3.5f,1.65f,-2.2f,.17f,.18f,.20f);
    Quad(s,-3.5f,-1.1f,-2.2f,-5.f,-1.85f,z,
            -5.f,2.3f,z,-3.5f,1.65f,-2.2f,.15f,.19f,.20f);
    Quad(s,3.5f,-1.1f,-2.2f,3.5f,1.65f,-2.2f,
            5.f,2.3f,z,5.f,-1.85f,z,.13f,.18f,.19f);
    for(int stripe=0;stripe<5;stripe++){
        const float zz=-5.0f+stripe*.52f;
        Quad(s,-3.9f,-1.32f,zz,3.9f,-1.32f,zz,
               3.9f,-1.30f,zz+.025f,-3.9f,-1.30f,zz+.025f,
               .33f,.29f,.24f);
    }
    Door(s,0,shift,advance);Door(s,1,shift,advance);
    // Subdued fixed HUD, high-contrast lettering for stereo headset legibility.
    Rect(s,0.f,1.46f,5.35f,.52f,-2.12f,.02f,.035f,.052f);
    char status[120]={};
    std::snprintf(status,sizeof(status),"GUESS THE DOOR   ROOM %02u / %02u",
                  g.phase==Phase::Victory?reverie::doors::kRooms:g.room+1u,
                  reverie::doors::kRooms);
    Text(s,status,-2.51f,1.39f,5.05f,.039f,.92f,.89f,.70f);
    char seedLine[128]={};
    std::snprintf(seedLine,sizeof(seedLine),"SEED %016llX   FALLS %u",
        static_cast<unsigned long long>(g.seed),g.falls);
    Text(s,seedLine,-2.48f,-1.46f,5.0f,.034f,.81f,.85f,.92f);
    if(g.phase==Phase::Ready){
        if(g.ClueVisible()){
            Rect(s,0.f,.20f,2.42f,.53f,-2.12f,.05f,.12f,.13f);
            char hint[80]={};
            std::snprintf(hint,sizeof(hint),"REMEMBER: %s",
                          reverie::doors::kSymbolNames[g.current.clue]);
            Text(s,hint,-1.02f,.12f,2.05f,.043f,1.f,.88f,.49f);
        }else{
            Text(s,"CHOOSE THE SYMBOL YOU SAW",-1.62f,.12f,3.48f,
                 .041f,1.f,.82f,.51f);
        }
        if(s->doorGaze>=0&&s->gazeSeconds>.45f){
            Rect(s,0.f,-.99f,3.75f,.35f,-2.11f,.045f,.065f,.080f);
            Text(s,s->doorGaze==0?"LEFT DOOR. A LEVER WAITS BESIDE IT.":
                 "RIGHT DOOR. A LEVER WAITS BESIDE IT.",
                 -1.74f,-1.06f,3.46f,.029f,.89f,.91f,.83f);
        }
        Text(s,"AIM AT A LEVER AND SELECT; OR ROCKER LEFT / RIGHT",
             -2.27f,-1.74f,4.54f,.029f,.83f,.92f,.89f);
    }else if(g.easterActive){
        Rect(s,0.f,.12f,4.50f,.69f,-2.1f,.17f,.08f,.07f);
        Text(s,"PULL THE LEVER, CRUNK!",-1.95f,.02f,4.f,
             .056f,1.f,.88f,.58f);
    }else if(g.phase==Phase::Pulling){
        Text(s,"THE LEVER IS MOVING...",-1.55f,.10f,3.2f,
             .044f,.96f,.82f,.55f);
    }else if(g.phase==Phase::Opening){
        Text(s,"THE DOOR CREAKS OPEN...",-1.55f,.10f,3.2f,
             .044f,.92f,.83f,.58f);
    }else if(g.phase==Phase::Walking){
        Text(s,"WALKING THROUGH...",-1.55f,.10f,3.2f,
             .044f,.88f,.93f,.74f);
    }else if(g.phase==Phase::Falling){
        Rect(s,0.f,0.f,5.40f,2.55f,-2.14f,.009f,.012f,.014f);
        Text(s,"A PIT! BACK TO THE FIRST ROOM.",-2.27f,.03f,4.7f,
             .044f,1.f,.60f,.47f);
    }else if(g.phase==Phase::Victory){
        Text(s,"ESCAPED! SELECT FOR A NEW SEED.",-2.24f,.04f,4.6f,
             .052f,1.f,.88f,.53f);
    }
    s->upload=true;
}
void Sfx(State* s,unsigned events){
    if(!s->host)return;
    // Existing licensed owner-generated Red Ledger sounds: metal lever,
    // hinged door, short mechanical footfalls. No ripped movie audio.
    if(events&reverie::doors::LeverSound)
        ReverieNativeRequestSoundV1(s->host,REVERIE_NATIVE_SOUND_RED_LEDGER_METAL);
    if(events&reverie::doors::DoorSound)
        ReverieNativeRequestSoundV1(s->host,REVERIE_NATIVE_SOUND_RED_LEDGER_DOOR);
    if(events&reverie::doors::StepSound)
        ReverieNativeRequestSoundV1(s->host,REVERIE_NATIVE_SOUND_RED_LEDGER_CLICK);
    if(events&reverie::doors::PitSound)
        ReverieNativeRequestSoundV1(s->host,REVERIE_NATIVE_SOUND_RED_LEDGER_DRAWER);
    if(events&reverie::doors::VictorySound)
        ReverieNativeRequestSoundV1(s->host,REVERIE_NATIVE_SOUND_CAMPAIGN_VICTORY);
}
void* Create(const ReverieNativeHostV1* host){
    if(!ReverieNativeHostSupportsLogV1(host))return nullptr;
    State* s=new(std::nothrow) State();if(!s)return nullptr;
    s->host=host;
    const auto now=std::chrono::steady_clock::now().time_since_epoch().count();
    s->game.Reset(reverie::doors::Mix(static_cast<uint64_t>(now)));
    Scene(s);
    ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_INFO,kId,"Deterministic door maze started");
    return s;
}
void Destroy(void* v){delete static_cast<State*>(v);}
int32_t GlStart(void* v){
    State* s=static_cast<State*>(v);if(!s)return 0;
    constexpr char vs[]="uniform mat4 u_Mvp;attribute vec3 a_Pos;"
        "attribute vec3 a_Color;varying vec3 v_Color;"
        "void main(){v_Color=a_Color;gl_Position=u_Mvp*vec4(a_Pos,1.0);}";
    constexpr char fs[]="precision mediump float;varying vec3 v_Color;"
        "void main(){gl_FragColor=vec4(v_Color,1.0);}";
    const ReverieNativeGlAttributeBindingV1 binds[]={{0u,"a_Pos"},{1u,"a_Color"}};
    s->shader=ReverieNativeBuildProgram(vs,fs,binds,2u);
    if(!s->shader)return 0;
    s->pos=glGetAttribLocation(s->shader,"a_Pos");
    s->col=glGetAttribLocation(s->shader,"a_Color");
    s->mvp=glGetUniformLocation(s->shader,"u_Mvp");
    glGenBuffers(1,&s->vbo);
    if(!s->vbo||s->pos<0||s->col<0||s->mvp<0||!s->font.Create()){
        s->font.Release();
        if(s->vbo)glDeleteBuffers(1,&s->vbo);
        if(s->shader)glDeleteProgram(s->shader);
        s->vbo=s->shader=0;return 0;
    }
    s->upload=true;return 1;
}
void GlEnd(void* v){
    State* s=static_cast<State*>(v);if(!s)return;
    if(s->vbo)glDeleteBuffers(1,&s->vbo);
    if(s->shader)glDeleteProgram(s->shader);
    s->font.Release();s->vbo=s->shader=0;s->upload=true;
}
void Update(void* v,const ReverieNativeInputV1* in){
    State* s=static_cast<State*>(v);
    if(!s||!ReverieNativeInputHasBaseV1(in))return;
    const float dt=std::fmax(0.f,std::fmin(in->delta_seconds,.1f));
    const bool primary=in->primary_down!=0u;
    const bool axis=std::fabs(in->move_x)>.55f;
    if(s->game.phase==Phase::Victory&&primary&&!s->held){
        s->game.Reset(reverie::doors::Mix(s->game.seed^0x4e45574d415a45ULL));
        s->choice=0;ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
    }
    if(s->game.phase==Phase::Ready){
        s->game.AdvanceReady(dt);
        if(axis&&!s->axisHeld){
            s->choice=in->move_x>0.f?1:0;
            ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
        }
        s->hasPointer=false;s->leverGaze=s->doorGaze=-1;
        if(ReverieNativeInputHasPointerV1(in)&&
            in->pointer_kind!=REVERIE_NATIVE_POINTER_NONE&&
            in->pointer_direction[2]<-.02f){
            const float t=(-3.43f-in->pointer_origin[2])/in->pointer_direction[2];
            if(t>.05f&&t<30.f){
                const float x=in->pointer_origin[0]+in->pointer_direction[0]*t;
                const float y=in->pointer_origin[1]+in->pointer_direction[1]*t;
                s->hasPointer=true;
                if(y>-.95f&&y<1.16f&&std::fabs(x)<1.75f)
                    s->doorGaze=x<0.f?0:1;
                if(y>-.68f&&y<.46f){
                    if(std::fabs(x+2.f)<.35f)s->leverGaze=0;
                    if(std::fabs(x-2.f)<.35f)s->leverGaze=1;
                }
            }
        }
        const int gaze=s->doorGaze;
        if(gaze!=s->choice && s->leverGaze>=0)
            s->choice=s->leverGaze;
        if(gaze>=0)s->gazeSeconds+=dt;
        else s->gazeSeconds=0.f;
        if(primary&&!s->held){
            const int target=s->hasPointer?s->leverGaze:s->choice;
            if(s->game.Pull(target)){
                ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
            }else{
                ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
            }
        }
    }else{
        s->game.Update(dt);
    }
    s->held=primary;s->axisHeld=axis;
    Sfx(s,s->game.DrainSound());
    Scene(s); // One logical update; both eyes read immutable mesh and font.
}
int32_t Render(void* v,const ReverieNativeEyeV1* eye){
    State* s=static_cast<State*>(v);
    if(!s||!ReverieNativeEyeRenderableV1(eye)||!s->shader||!s->vbo)return 0;
    ReverieNativeGlStateV1 previous{};
    ReverieNativeGlAttribStateV1 a{},b{};
    ReverieNativeGlStateCaptureV1(&previous);
    ReverieNativeGlAttribCaptureV1(&a,static_cast<GLuint>(s->pos));
    ReverieNativeGlAttribCaptureV1(&b,static_cast<GLuint>(s->col));
    float vp[16];ReverieNativeMat4Multiply(vp,eye->projection,eye->view);
    glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
    glUseProgram(s->shader);glBindBuffer(GL_ARRAY_BUFFER,s->vbo);
    if(s->upload){
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(s->mesh.size()*sizeof(float)),
            s->mesh.data(),GL_DYNAMIC_DRAW);
        s->upload=false;
    }
    glEnableVertexAttribArray(static_cast<GLuint>(s->pos));
    glVertexAttribPointer(static_cast<GLuint>(s->pos),3,GL_FLOAT,GL_FALSE,
        6*sizeof(float),reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(static_cast<GLuint>(s->col));
    glVertexAttribPointer(static_cast<GLuint>(s->col),3,GL_FLOAT,GL_FALSE,
        6*sizeof(float),reinterpret_cast<const void*>(3*sizeof(float)));
    glUniformMatrix4fv(s->mvp,1,GL_FALSE,vp);
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(s->mesh.size()/6u));
    const bool meshOk=glGetError()==GL_NO_ERROR;
    const bool textOk=s->font.Draw(vp);
    ReverieNativeGlAttribRestoreV1(&b);
    ReverieNativeGlAttribRestoreV1(&a);
    ReverieNativeGlStateRestoreV1(&previous);
    return meshOk&&textOk?1:0;
}
const ReverieNativeModuleApiV1 kApi={
    sizeof(ReverieNativeModuleApiV1),REVERIE_NATIVE_MODULE_ABI_VERSION,
    {sizeof(ReverieNativeModuleDescriptorV1),REVERIE_NATIVE_MODULE_ABI_VERSION,
      kId,kTitle,2u,0u},
    Create,Destroy,GlStart,GlEnd,[](void*){},[](void*){},
    Update,Render,nullptr,nullptr
};
} // namespace
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1* reverie_native_module_entry_v1(void){
    return &kApi;
}
