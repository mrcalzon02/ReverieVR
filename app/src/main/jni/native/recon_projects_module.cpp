#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include "reverie_native_flat_mesh.h"
#if !defined(RECON_PROJECT_KIND) || (RECON_PROJECT_KIND != 0 && RECON_PROJECT_KIND != 1)
#error "Select a recon project"
#endif
#if RECON_PROJECT_KIND == 0
#include "lantern_desk_simulation.h"
namespace sim = reverie::lantern;
constexpr char kId[]="ministry-lantern-desk";
constexpr char kTitle[]="Ministry of Intelligence: Lantern Desk VR";
constexpr char kHeader[]="LANTERN DESK";
#else
#include "forward_detachment_simulation.h"
namespace sim = reverie::forward;
constexpr char kId[]="iron-sight-forward-detachment";
constexpr char kTitle[]="Iron Sight: Forward Detachment VR";
constexpr char kHeader[]="FORWARD DETACHMENT";
#endif
#include <GLES2/gl2.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>
namespace {
struct State {
    const ReverieNativeHostV1 *host=nullptr;
    sim::Simulation simulation;
#if RECON_PROJECT_KIND == 0
    sim::Photograph photo{};
    sim::ReportResult result{};
    bool hasPhoto=false;
#else
    sim::ContactEvidence evidence{};
    sim::Debrief debrief{};
#endif
    int choice=0;
    bool primary=false,secondary=false,axis=false,upload=true;
    GLuint shader=0,vbo=0;
    GLint pos=-1,col=-1,mvp=-1;
    std::vector<float> mesh;
};
void Rect(State *s,float x,float y,float w,float h,float z,float r,float g,float b){
    reverie::flatmesh::Rect(s->mesh,x,y,w,h,z,r,g,b);
}
const char *G(char c){
    switch(c){
    case 'A':return "010101111101101";case 'B':return "110101110101110";
    case 'C':return "011100100100011";case 'D':return "110101101101110";
    case 'E':return "111100110100111";case 'F':return "111100110100100";
    case 'G':return "011100101101011";case 'H':return "101101111101101";
    case 'I':return "111010010010111";case 'J':return "001001001101010";
    case 'K':return "101101110101101";case 'L':return "100100100100111";
    case 'M':return "101111111101101";case 'N':return "101111111111101";
    case 'O':return "111101101101111";case 'P':return "110101110100100";
    case 'Q':return "111101101111011";case 'R':return "110101110101101";
    case 'S':return "011100010001110";case 'T':return "111010010010111";
    case 'U':return "101101101101111";case 'V':return "101101010101010";
    case 'W':return "101101111111101";case 'X':return "101101010101101";
    case 'Y':return "101101010010010";case 'Z':return "111001010100111";
    case '0':return "111101101101111";case '1':return "010110010010111";
    case '2':return "110001111100111";case '3':return "110001110001110";
    case '4':return "101101111001001";case '5':return "111100110001110";
    case '6':return "011100111101111";case '7':return "111001010010010";
    case '8':return "111101111101111";case '9':return "111101111001110";
    case '-':return "000000111000000";case '+':return "000010111010000";
    case ':':return "000010000010000";default:return "000000000000000";
    }
}
void Text(State *s,const char *str,float y,float size,float r,float g,float b){
    size_t n=std::strlen(str);
    float start=-2.f*static_cast<float>(n)*size;
    for(size_t k=0;k<n;k++){
        char ch=str[k];if(ch>='a'&&ch<='z')ch=static_cast<char>(ch-'a'+'A');
        const char *bits=G(ch);
        for(int i=0;i<15;i++)if(bits[i]=='1')
            Rect(s,start+(k*4.f+i%3+.5f)*size,
                 y+(2-i/3)*size,size*.9f,size*.9f,-2.87f,r,g,b);
    }
}
int Choices(const State *s){
#if RECON_PROJECT_KIND == 0
    if(s->simulation.phase()==sim::Phase::Briefing)return 3;
    if(s->simulation.phase()==sim::Phase::PhotoReady)return 4;
#else
    if(s->simulation.phase()==sim::Phase::Briefing)return 2;
    if(s->simulation.phase()==sim::Phase::Contact)return 3;
#endif
    return 1;
}
void DrawBoard(State *s){
    s->mesh.clear();
    Rect(s,0,0,5.0f,3.45f,-3.03f,.028f,.050f,.076f);
    Rect(s,0,1.43f,4.75f,.44f,-2.99f,.13f,.21f,.22f);
    Text(s,kHeader,1.39f,.053f,.93f,.94f,.83f);
    Text(s,"ROCKER NEXT  SELECT ACTION  MENU EXIT",-1.54f,.027f,.65f,.83f,.78f);
    char text[64]={};
#if RECON_PROJECT_KIND == 0
    switch(s->simulation.phase()){
    case sim::Phase::Briefing:{
        const char *labels[]={"RAIL APPROACH","ORCHARD","RIVER CROSSING"};
        Text(s,"CHOOSE RECON SECTOR",.92f,.039f,.75f,.89f,.95f);
        Text(s,labels[s->choice%3],-.22f,.055f,1.f,.81f,.37f);
        Text(s,"SELECT TO DISPATCH",-.89f,.037f,.79f,.90f,.89f);
        break;
    }
    case sim::Phase::InFlight:
        Text(s,"RECON AIRCRAFT IN FLIGHT",.54f,.040f,.80f,.87f,.95f);
        Text(s,"SELECT TO RECEIVE PHOTO",-.63f,.038f,1.f,.82f,.39f);
        break;
    case sim::Phase::PhotoReady:
    case sim::Phase::Filed:{
        Text(s,"PHOTOGRAPH - INTERPRET EVIDENCE",.99f,.029f,.79f,.91f,.96f);
        if(s->hasPhoto){
            Rect(s,0,.16f,1.5f,1.5f,-2.96f,.14f,.17f,.14f);
            for(int y=0;y<16;y++)for(int x=0;x<16;x++)
                if((s->photo.pixels[y] & (1u<<x))!=0)
                    Rect(s,(x-7.5f)*.083f,(7.5f-y)*.083f+.16f,
                         .080f,.080f,-2.92f,.84f,.85f,.63f);
        }
        if(s->simulation.phase()==sim::Phase::PhotoReady){
            const char *labels[]={"NO CONTACT","POSSIBLE CONTACT",
                                  "VEHICLE COLUMN","BRIDGE DAMAGE"};
            Text(s,"FILE ASSESSMENT",-.88f,.033f,.83f,.91f,.90f);
            Text(s,labels[s->choice%4],-1.18f,.040f,1.f,.81f,.36f);
        }else{
            std::snprintf(text,sizeof(text),"TRUST %+d  EFFECT %+d",
                     s->result.credibility_delta,s->result.operational_effect);
            Text(s,text,-.84f,.040f,.96f,.82f,.38f);
            Text(s,"SELECT NEW SORTIE",-1.17f,.033f,.84f,.90f,.86f);
        }
        break;
    }
    }
#else
    switch(s->simulation.phase()){
    case sim::Phase::Briefing:
        Text(s,"CHOOSE PATROL ROUTE",.87f,.045f,.77f,.91f,.96f);
        Text(s,s->choice%2==0?"MAIN ROAD":"RIDGE BYPASS",
             -.17f,.055f,1.f,.82f,.36f);
        Text(s,"SELECT TO DEPLOY",-.84f,.040f,.79f,.92f,.87f);
        break;
    case sim::Phase::Travelling:
        Text(s,"PATROL TRAVELLING",.54f,.047f,.79f,.88f,.93f);
        Text(s,"SELECT TO INSPECT CONTACT",-.54f,.037f,1.f,.83f,.35f);
        break;
    case sim::Phase::Contact:{
        Text(s,"CONTACT OBSERVATIONS",1.02f,.041f,.83f,.90f,.92f);
        Text(s,s->evidence.disturbed_ground?"GROUND DISTURBED":"GROUND CLEAR",.61f,.033f,.85f,.85f,.72f);
        Text(s,s->evidence.vehicle_tracks?"TRACKS PRESENT":"NO VEHICLE TRACKS",.34f,.033f,.85f,.85f,.72f);
        Text(s,s->evidence.civilian_silhouettes?"CIVILIANS VISIBLE":"NO CIVILIANS",.07f,.033f,.85f,.85f,.72f);
        Text(s,s->evidence.weapon_identified?"WEAPON SEEN":"NO WEAPON SEEN",-.20f,.033f,.85f,.85f,.72f);
        const char *labels[]={"HALT AND REPORT","AVOID","ENGAGE"};
        Text(s,labels[s->choice%3],-.91f,.051f,1.f,.80f,.36f);
        break;
    }
    case sim::Phase::Resolved:
        Text(s,"CONTACT RESOLVED",.55f,.044f,.79f,.89f,.92f);
        Text(s,"SELECT RETURN TO BASE",-.52f,.041f,1.f,.83f,.35f);
        break;
    case sim::Phase::Returned:
        Text(s,"PATROL RETURNED",.55f,.048f,.81f,.91f,.91f);
        Text(s,"SELECT FILE DEBRIEF",-.54f,.040f,1.f,.83f,.35f);
        break;
    case sim::Phase::Debriefed:
        std::snprintf(text,sizeof(text),"INTEL %+d  FUEL %d",
                 s->debrief.information_value,s->debrief.fuel_remaining);
        Text(s,text,.56f,.046f,.94f,.82f,.42f);
        std::snprintf(text,sizeof(text),"VEHICLE %d",s->debrief.vehicle_condition);
        Text(s,text,.05f,.045f,.80f,.89f,.87f);
        if(s->debrief.unnecessary_force)
            Text(s,"UNNECESSARY FORCE",- .37f,.039f,1.f,.52f,.43f);
        Text(s,"SELECT NEW PATROL",-.97f,.040f,1.f,.85f,.36f);
        break;
    }
#endif
    s->upload=true;
}
bool Act(State *s){
    bool ok=false;
#if RECON_PROJECT_KIND == 0
    switch(s->simulation.phase()){
    case sim::Phase::Briefing:
        ok=s->simulation.AssignRecon(static_cast<sim::Sector>(s->choice));break;
    case sim::Phase::InFlight:
        ok=s->simulation.CompleteRecon();
        if(ok&&s->simulation.photo()){
            s->photo=*s->simulation.photo();s->hasPhoto=true;
        }
        break;
    case sim::Phase::PhotoReady:
        ok=s->simulation.SubmitReport(static_cast<sim::Assessment>(s->choice),
                                      &s->result);break;
    case sim::Phase::Filed:
        s->simulation=sim::Simulation();s->hasPhoto=false;ok=true;break;
    }
#else
    switch(s->simulation.phase()){
    case sim::Phase::Briefing:
        ok=s->simulation.BeginPatrol(static_cast<sim::Route>(s->choice));break;
    case sim::Phase::Travelling:
        ok=s->simulation.ReachContact();
        if(ok)ok=s->simulation.InspectContact(&s->evidence);
        break;
    case sim::Phase::Contact:
        ok=s->simulation.ResolveContact(static_cast<sim::Decision>(s->choice));break;
    case sim::Phase::Resolved:
        ok=s->simulation.ReturnToBase();break;
    case sim::Phase::Returned:
        ok=s->simulation.FileDebrief(&s->debrief);break;
    case sim::Phase::Debriefed:
        s->simulation=sim::Simulation();ok=true;break;
    }
#endif
    if(ok){s->choice=0;DrawBoard(s);}
    return ok;
}
void *Create(const ReverieNativeHostV1 *host){
    if(!ReverieNativeHostSupportsLogV1(host))return nullptr;
    State *s=new(std::nothrow) State();
    if(!s)return nullptr;
    s->host=host;DrawBoard(s);
    ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_INFO,kId,"Native prototype started");
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
    const ReverieNativeGlAttributeBindingV1 attrs[]={{0u,"a_Pos"},{1u,"a_Color"}};
    s->shader=ReverieNativeBuildProgram(vs,fs,attrs,2);
    if(!s->shader)return 0;
    s->pos=glGetAttribLocation(s->shader,"a_Pos");
    s->col=glGetAttribLocation(s->shader,"a_Color");
    s->mvp=glGetUniformLocation(s->shader,"u_Mvp");
    glGenBuffers(1,&s->vbo);
    if(!s->vbo||s->pos<0||s->col<0||s->mvp<0){
        if(s->vbo)glDeleteBuffers(1,&s->vbo);
        glDeleteProgram(s->shader);s->shader=0;s->vbo=0;return 0;
    }
    s->upload=true;
    return 1;
}
void GlEnd(void *p){
    State *s=static_cast<State *>(p);if(!s)return;
    if(s->vbo)glDeleteBuffers(1,&s->vbo);
    if(s->shader)glDeleteProgram(s->shader);
    s->vbo=0;s->shader=0;s->upload=true;
}
void Resume(void *){} void Pause(void *){}
void Update(void *p,const ReverieNativeInputV1 *in){
    State *s=static_cast<State *>(p);
    if(!s||!ReverieNativeInputHasBaseV1(in))return;
    bool first=in->primary_down!=0,second=in->secondary_down!=0;
    bool axis=std::fabs(in->move_x)>.65f||std::fabs(in->move_y)>.65f;
    if((second&&!s->secondary)||(axis&&!s->axis)){
        s->choice=(s->choice+1)%Choices(s);
        DrawBoard(s);
        ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
    }
    if(first&&!s->primary){
        bool ok=Act(s);
        ReverieNativeRequestFeedbackV1(s->host,ok?REVERIE_NATIVE_FEEDBACK_ACTIVATION:
                                      REVERIE_NATIVE_FEEDBACK_FAILURE);
    }
    s->primary=first;s->secondary=second;s->axis=axis;
}
int32_t Render(void *p,const ReverieNativeEyeV1 *eye){
    State *s=static_cast<State *>(p);
    if(!s||!ReverieNativeEyeRenderableV1(eye)||!s->shader||!s->vbo)return 0;
    ReverieNativeGlStateV1 old={};
    ReverieNativeGlAttribStateV1 a={},b={};
    ReverieNativeGlStateCaptureV1(&old);
    ReverieNativeGlAttribCaptureV1(&a,static_cast<GLuint>(s->pos));
    ReverieNativeGlAttribCaptureV1(&b,static_cast<GLuint>(s->col));
    float matrix[16];ReverieNativeMat4Multiply(matrix,eye->projection,eye->view);
    glDisable(GL_CULL_FACE);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
    glUseProgram(s->shader);glBindBuffer(GL_ARRAY_BUFFER,s->vbo);
    if(s->upload){
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(s->mesh.size()*sizeof(float)),
                     s->mesh.data(),GL_DYNAMIC_DRAW);
        s->upload=false;
    }
    glEnableVertexAttribArray(static_cast<GLuint>(s->pos));
    glVertexAttribPointer(static_cast<GLuint>(s->pos),3,GL_FLOAT,GL_FALSE,
                          6*sizeof(float),reinterpret_cast<const void *>(0));
    glEnableVertexAttribArray(static_cast<GLuint>(s->col));
    glVertexAttribPointer(static_cast<GLuint>(s->col),3,GL_FLOAT,GL_FALSE,
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
     kId,kTitle,2u,0u},
    Create,Destroy,GlStart,GlEnd,Resume,Pause,Update,Render,nullptr
};
}
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1 *reverie_native_module_entry_v1(void){
    return &api;
}
