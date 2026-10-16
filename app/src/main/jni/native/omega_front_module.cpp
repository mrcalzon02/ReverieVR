#include "omega_front_controls.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include "reverie_native_flat_mesh.h"
#include <GLES2/gl2.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

namespace {
namespace game = reverie::omega;
constexpr char kId[] = "omega-front";
constexpr float kBoardZ = -1.9f;
constexpr float kCell = .073f;
constexpr float kHalf = game::kWidth*kCell*.5f;
constexpr float kCenterY = .02f;
constexpr float kNear = -1.86f;
struct State {
    const ReverieNativeHostV1 *host=nullptr;
    game::Controller control{104u};
    GLuint program=0,vbo=0;
    GLint pos=-1,color=-1,mvp=-1;
    bool upload=true;
    std::vector<float> mesh;
};
void Rect(State *s,float x,float y,float w,float h,float z,float r,float g,float b){
    reverie::flatmesh::Rect(s->mesh,x,y,w,h,z,r,g,b);
}
void Line(State *s,float x,float y,float xx,float yy,float w,float z,float r,float g,float b){
    reverie::flatmesh::Segment(s->mesh,x,y,xx,yy,w,z,r,g,b);
}
// Compact original chip font used for the game-private command/status display.
const char *Glyph(char c) {
    switch(c){
    case 'A':return "010101111101101";case 'B':return "110101110101110";
    case 'C':return "011100100100011";case 'D':return "110101101101110";
    case 'E':return "111100110100111";case 'F':return "111100110100100";
    case 'G':return "011100111101011";case 'H':return "101101111101101";
    case 'I':return "111010010010111";case 'J':return "001001001101010";
    case 'K':return "101101110101101";case 'L':return "100100100100111";
    case 'M':return "101111111101101";case 'N':return "101111111111101";
    case 'O':return "111101101101111";case 'P':return "110101110100100";
    case 'Q':return "111101101111011";case 'R':return "110101110101101";
    case 'S':return "011100010001110";case 'T':return "111010010010010";
    case 'U':return "101101101101111";case 'V':return "101101101010010";
    case 'W':return "101101111111101";case 'X':return "101101010101101";
    case 'Y':return "101101010010010";case 'Z':return "111001010100111";
    case '0':return "111101101101111";case '1':return "010110010010111";
    case '2':return "110001111100111";case '3':return "110001110001110";
    case '4':return "101101111001001";case '5':return "111100110001110";
    case '6':return "011100111101111";case '7':return "111001010010010";
    case '8':return "111101111101111";case '9':return "111101111001110";
    case '-':return "000000111000000";case ':':return "000010000010000";
    case '/':return "001001010100100";default:return "000000000000000";
    }
}
void Text(State *s,const char *msg,float center_y,float px,float r,float g,float b){
    const size_t count=std::strlen(msg);
    const float start=-static_cast<float>(count)*px*2.f;
    for(size_t i=0;i<count;i++){
        char c=msg[i];if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');
        const char *glyph=Glyph(c);
        for(int p=0;p<15;p++)if(glyph[p]=='1') {
            Rect(s,start+(i*4.f+static_cast<float>(p%3))*px,
                 center_y+(2.f-static_cast<float>(p/3))*px,
                 px*.92f,px*.92f,kNear,r,g,b);
        }
    }
}
void UnitSprite(State *s,const game::Unit &u,float x,float y){
    const bool ally=u.side==game::Side::Hearthwell;
    const float rr=ally?.29f:.83f, gg=ally?.80f:.31f, bb=ally?.56f:.29f;
    Rect(s,x,y-kCell*.12f,kCell*.51f,kCell*.37f,-1.853f,rr*.70f,gg*.74f,bb*.78f);
    Rect(s,x,y+kCell*.18f,kCell*.29f,kCell*.30f,-1.850f,.95f,.77f,.59f);
    Rect(s,x,y+kCell*.35f,kCell*.47f,kCell*.13f,-1.848f,rr,gg,bb);
    Line(s,x+kCell*.18f,y-kCell*.1f,x+kCell*.43f,y-kCell*.1f,
         kCell*.075f,-1.848f,.91f,.90f,.70f);
    if(u.role==game::Class::Heavy){
        Rect(s,x-kCell*.30f,y-kCell*.1f,kCell*.17f,kCell*.39f,-1.847f,.60f,.61f,.66f);
        Rect(s,x+kCell*.30f,y-kCell*.1f,kCell*.17f,kCell*.39f,-1.847f,.60f,.61f,.66f);
        Rect(s,x,y+kCell*.43f,kCell*.57f,kCell*.09f,-1.847f,.75f,.76f,.77f);
    }else if(u.role==game::Class::Engineer){
        Rect(s,x-kCell*.33f,y-kCell*.08f,kCell*.15f,kCell*.26f,-1.847f,.98f,.76f,.17f);
        Line(s,x+kCell*.31f,y-kCell*.21f,x+kCell*.39f,y+kCell*.22f,
             kCell*.09f,-1.846f,.98f,.76f,.21f);
    }
    Rect(s,x,y-kCell*.43f,kCell*.68f,kCell*.065f,-1.844f,.14f,.16f,.19f);
    const float hp_ratio=static_cast<float>(u.hp)/static_cast<float>(u.role==game::Class::Heavy?13:9);
    Rect(s,x-kCell*.34f+hp_ratio*kCell*.34f,y-kCell*.43f,
         kCell*.68f*hp_ratio,kCell*.055f,-1.843f,.22f,.92f,.32f);
}
void Draw(State *s){
    s->mesh.clear();s->mesh.reserve(75000);
    Rect(s,0,0,2.15f,2.3f,kBoardZ-.09f,.018f,.036f,.049f);
    Rect(s,0,kCenterY,2*kHalf+.035f,2*kHalf+.035f,kBoardZ-.04f,.21f,.28f,.20f);
    const game::Battle &battle=s->control.battle;
    for(int y=0;y<game::kWidth;++y)for(int x=0;x<game::kWidth;++x){
        float wx=(x+.5f)*kCell-kHalf;
        float wy=kCenterY+kHalf-(y+.5f)*kCell;
        float r=.23f,g=.35f,b=.24f;
        const game::Ground ground=battle.ground(x,y);
        if(ground==game::Ground::Road){r=.39f;g=.36f;b=.30f;}
        else if(ground==game::Ground::Rubble){r=.48f;g=.43f;b=.40f;}
        else if(ground==game::Ground::Wall){r=.13f;g=.14f;b=.18f;}
        else if(ground==game::Ground::DeepWater){r=.10f;g=.33f;b=.53f;}
        if((x+y)%2){r*=.92f;g*=.92f;b*=.92f;}
        Rect(s,wx,wy,kCell*.965f,kCell*.965f,kBoardZ,r,g,b);
        if(ground==game::Ground::Rubble) {
            Line(s,wx-kCell*.20f,wy-kCell*.13f,wx+kCell*.15f,wy+kCell*.13f,
                 kCell*.15f,kBoardZ+.008f,.64f,.55f,.47f);
        }
        if(ground==game::Ground::Wall)
            Rect(s,wx,wy,kCell*.77f,kCell*.74f,kBoardZ+.015f,.36f,.39f,.39f);
        if(x==s->control.cursor_x && y==s->control.cursor_y){
            Line(s,wx-kCell*.43f,wy+kCell*.42f,wx+kCell*.43f,wy+kCell*.42f,
                 kCell*.070f,-1.84f,1.f,.90f,.38f);
            Line(s,wx+kCell*.43f,wy-kCell*.42f,wx+kCell*.43f,wy+kCell*.42f,
                 kCell*.070f,-1.84f,1.f,.90f,.38f);
            Line(s,wx-kCell*.43f,wy-kCell*.42f,wx+kCell*.43f,wy-kCell*.42f,
                 kCell*.070f,-1.84f,1.f,.90f,.38f);
            Line(s,wx-kCell*.43f,wy-kCell*.42f,wx-kCell*.43f,wy+kCell*.42f,
                 kCell*.070f,-1.84f,1.f,.90f,.38f);
        }
    }
    // Pump and ammunition depot are actual simulation objectives.
    auto center=[](int x,int y,float &wx,float &wy){
        wx=(x+.5f)*kCell-kHalf;wy=kCenterY+kHalf-(y+.5f)*kCell;
    };
    float px=0,py=0;center(2,12,px,py);
    Rect(s,px,py,kCell*.76f,kCell*.74f,-1.85f,.22f,.74f,.87f);
    Rect(s,px,py+kCell*.18f,kCell*.36f,kCell*.15f,-1.84f,.75f,.90f,.95f);
    center(5,12,px,py);
    Rect(s,px,py,kCell*.78f,kCell*.72f,-1.85f,.65f,.44f,.15f);
    Line(s,px-kCell*.25f,py,px+kCell*.25f,py,kCell*.09f,-1.84f,.95f,.77f,.34f);
    for(int i=0;i<battle.unit_count();++i){
        const game::Unit *u=battle.unit(i);
        if(!u||!u->alive())continue;
        center(u->x,u->y,px,py);
        if(i==s->control.selected)
            Rect(s,px,py,kCell*.96f,kCell*.96f,-1.858f,.99f,.91f,.27f);
        UnitSprite(s,*u,px,py);
    }
    Rect(s,0,1.054f,2.05f,.30f,-1.92f,.07f,.13f,.19f);
    Text(s,"OMEGA FRONT  HEARTHWELL",1.076f,.013f,.91f,.93f,.78f);
    char status[96];
    std::snprintf(status,sizeof(status),"PUMP %d  PARTS %d  AMMO %d  TICK %u",
                  battle.pump_hp(),battle.components(),battle.ammo_stock(),battle.ticks());
    Text(s,status,-.946f,.010f,.88f,.92f,.85f);
    const char *mode_names[]={"MOVE","ATTACK","REPAIR","RESUPPLY"};
    char action[90];
    if(battle.result()==game::Result::Defended)std::snprintf(action,sizeof(action),"DEFENDED  MENU RESET");
    else if(battle.result()==game::Result::Overrun)std::snprintf(action,sizeof(action),"OVERRUN  MENU RESET");
    else if(s->control.paused)std::snprintf(action,sizeof(action),"PAUSED  MENU RESUME");
    else if(s->control.selected<0)std::snprintf(action,sizeof(action),"PICK A GREEN UNIT  CLICK");
    else std::snprintf(action,sizeof(action),"UNIT %d  %s  CLICK ORDER",s->control.selected,
                       mode_names[static_cast<int>(s->control.mode)]);
    Text(s,action,-1.061f,.012f,s->control.rejected?1.f:.91f,
         s->control.rejected?.37f:.86f,.55f);
    s->upload=true;
}
void *Create(const ReverieNativeHostV1 *host){
    if(!ReverieNativeHostSupportsLogV1(host))return nullptr;
    State *s=new(std::nothrow) State();
    if(!s)return nullptr;
    s->host=host;Draw(s);
    ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_INFO,kId,"Hearthwell tactical development mission");
    return s;
}
void Destroy(void *p){delete static_cast<State *>(p);}
int32_t GlStart(void *p){
    State *s=static_cast<State *>(p);if(!s)return 0;
    const char *vs="uniform mat4 u_Mvp;\nattribute vec3 a_Pos;\n"
        "attribute vec3 a_Color;\nvarying vec3 v_Color;\n"
        "void main(){v_Color=a_Color;gl_Position=u_Mvp*vec4(a_Pos,1.0);}\n";
    const char *fs="precision mediump float;\nvarying vec3 v_Color;\n"
        "void main(){gl_FragColor=vec4(v_Color,1.0);}\n";
    const ReverieNativeGlAttributeBindingV1 bindings[]={{0u,"a_Pos"},{1u,"a_Color"}};
    s->program=ReverieNativeBuildProgram(vs,fs,bindings,2);
    if(!s->program)return 0;
    s->pos=glGetAttribLocation(s->program,"a_Pos");
    s->color=glGetAttribLocation(s->program,"a_Color");
    s->mvp=glGetUniformLocation(s->program,"u_Mvp");
    glGenBuffers(1,&s->vbo);
    if(!s->vbo||s->pos<0||s->color<0||s->mvp<0){
        if(s->vbo)glDeleteBuffers(1,&s->vbo);
        glDeleteProgram(s->program);
        s->program=s->vbo=0;return 0;
    }
    s->upload=true;return 1;
}
void GlEnd(void *p){
    State *s=static_cast<State *>(p);if(!s)return;
    if(s->vbo)glDeleteBuffers(1,&s->vbo);
    if(s->program)glDeleteProgram(s->program);
    s->vbo=0;s->program=0;s->upload=true;
}
void Resume(void *){} void Pause(void *){}
// UI and 250-ms simulation advance only here, never once per rendered eye.
void Update(void *p,const ReverieNativeInputV1 *in){
    State *s=static_cast<State *>(p);
    if(!s||!ReverieNativeInputHasBaseV1(in))return;
    game::ControlsInput action{};
    action.dt=in->delta_seconds;
    action.x=in->move_x;action.y=in->move_y;
    action.primary=in->primary_down!=0;action.secondary=in->secondary_down!=0;
    if(ReverieNativeInputHasPointerV1(in) &&
        in->pointer_kind!=REVERIE_NATIVE_POINTER_NONE &&
        std::fabs(in->pointer_direction[2])>.0001f) {
        const float t=(kBoardZ-in->pointer_origin[2])/in->pointer_direction[2];
        if(t>0.f && t<100.f){
            const float x=in->pointer_origin[0]+t*in->pointer_direction[0];
            const float y=in->pointer_origin[1]+t*in->pointer_direction[1];
            const int grid_x=static_cast<int>(std::floor((x+kHalf)/kCell));
            const int grid_y=static_cast<int>(std::floor((kCenterY+kHalf-y)/kCell));
            if(game::Battle::Inside(grid_x,grid_y)){
                action.pointer_x=grid_x;action.pointer_y=grid_y;
            }
        }
    }
    const uint32_t old_issued=s->control.issued;
    const uint32_t old_tick=s->control.battle.ticks();
    if(s->control.Input(action)){
        Draw(s);
        if(s->control.issued!=old_issued)
            ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        else if(s->control.rejected && action.primary)
            ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
        else if(s->control.battle.ticks()==old_tick)
            ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
    }
}
int32_t Render(void *p,const ReverieNativeEyeV1 *eye){
    State *s=static_cast<State *>(p);
    if(!s||!ReverieNativeEyeRenderableV1(eye)||!s->program||!s->vbo)return 0;
    ReverieNativeGlStateV1 old={};
    ReverieNativeGlAttribStateV1 a={},b={};
    ReverieNativeGlStateCaptureV1(&old);
    ReverieNativeGlAttribCaptureV1(&a,static_cast<GLuint>(s->pos));
    ReverieNativeGlAttribCaptureV1(&b,static_cast<GLuint>(s->color));
    float matrix[16];ReverieNativeMat4Multiply(matrix,eye->projection,eye->view);
    glDisable(GL_CULL_FACE);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
    glUseProgram(s->program);glBindBuffer(GL_ARRAY_BUFFER,s->vbo);
    if(s->upload){
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(s->mesh.size()*sizeof(float)),
                     s->mesh.data(),GL_DYNAMIC_DRAW);
        s->upload=false;
    }
    glEnableVertexAttribArray(static_cast<GLuint>(s->pos));
    glVertexAttribPointer(static_cast<GLuint>(s->pos),3,GL_FLOAT,GL_FALSE,
                          6*sizeof(float),reinterpret_cast<const void *>(0));
    glEnableVertexAttribArray(static_cast<GLuint>(s->color));
    glVertexAttribPointer(static_cast<GLuint>(s->color),3,GL_FLOAT,GL_FALSE,
                          6*sizeof(float),reinterpret_cast<const void *>(3*sizeof(float)));
    glUniformMatrix4fv(s->mvp,1,GL_FALSE,matrix);
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(s->mesh.size()/6));
    GLenum error=glGetError();
    ReverieNativeGlAttribRestoreV1(&b);
    ReverieNativeGlAttribRestoreV1(&a);
    ReverieNativeGlStateRestoreV1(&old);
    return error==GL_NO_ERROR?1:0;
}
const ReverieNativeModuleApiV1 api={
    sizeof(ReverieNativeModuleApiV1),REVERIE_NATIVE_MODULE_ABI_VERSION,
    {sizeof(ReverieNativeModuleDescriptorV1),REVERIE_NATIVE_MODULE_ABI_VERSION,
     kId,"Omega Front: Hearthwell Defense [DEV]",2u,0u},
    Create,Destroy,GlStart,GlEnd,Resume,Pause,Update,Render,nullptr
};
}
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1 *reverie_native_module_entry_v1(void){return &api;}
