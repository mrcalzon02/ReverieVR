#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include "reverie_native_flat_mesh.h"
#include "reverie_native_lato_text.h"
#include "recon_projects_save.h"
#if !defined(RECON_PROJECT_KIND) || (RECON_PROJECT_KIND != 0 && RECON_PROJECT_KIND != 1)
#error "Select a recon project"
#endif
#if RECON_PROJECT_KIND == 0
#include "lantern_desk_simulation.h"
namespace sim = reverie::lantern;
using SavedSession=reverie::recon::LanternState;
constexpr char kId[]="ministry-lantern-desk";
constexpr char kTitle[]="Ministry of Intelligence: Lantern Desk VR";
constexpr char kHeader[]="LANTERN DESK";
#else
#include "forward_detachment_simulation.h"
namespace sim = reverie::forward;
using SavedSession=reverie::recon::ForwardState;
constexpr char kId[]="iron-sight-forward-detachment";
constexpr char kTitle[]="Iron Sight: Forward Detachment VR";
constexpr char kHeader[]="FORWARD DETACHMENT";
#endif
#include <GLES2/gl2.h>
#include <algorithm>
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
    bool saveDirty=false,saveFailed=false,loadBlocked=false,inputReady=false;
    float retrySeconds=1.f;
#if RECON_PROJECT_KIND == 1
    bool confirmRefit=false;
#endif
    bool primary=false,secondary=false,axis=false,upload=true;
    GLuint shader=0,vbo=0;
    GLint pos=-1,col=-1,mvp=-1;
    std::vector<float> mesh;
    reverie::nativetext::LatoLayer font;
};
void Rect(State *s,float x,float y,float w,float h,float z,float r,float g,float b){
    reverie::flatmesh::Rect(s->mesh,x,y,w,h,z,r,g,b);
}
void Text(State *s,const char *str,float y,float size,
          float r,float g,float b){
    s->font.Fit(str,0.f,y,4.66f,size,r,g,b,-2.87f);
}
int Choices(const State *s){
#if RECON_PROJECT_KIND == 0
    if(s->simulation.phase()==sim::Phase::Briefing)return 3;
    if(s->simulation.phase()==sim::Phase::PhotoReady)return 4;
#else
    if(s->confirmRefit)return 2;
    if(s->simulation.phase()==sim::Phase::Briefing)return 3;
    if(s->simulation.phase()==sim::Phase::Contact)return 3;
#endif
    return 1;
}
void DrawBoard(State *s){
    s->mesh.clear();s->font.Clear();
    Rect(s,0,0,5.0f,3.45f,-3.03f,.028f,.050f,.076f);
    Rect(s,0,1.43f,4.75f,.44f,-2.99f,.13f,.21f,.22f);
    Text(s,kHeader,1.39f,.053f,.93f,.94f,.83f);
    Text(s,"ROCKER NEXT  SELECT ACTION  MENU EXIT",-1.54f,.027f,.65f,.83f,.78f);
    if(s->loadBlocked){
        Text(s,"CHECKPOINT UNAVAILABLE",.68f,.044f,1.f,.68f,.48f);
        Text(s,"YOUR SAVE HAS NOT BEEN CHANGED",.15f,.032f,.88f,.91f,.88f);
        Text(s,"SELECT TO RETRY",-.55f,.040f,1.f,.82f,.38f);
        s->upload=true;return;
    }
    if(s->saveFailed){
        Rect(s,0,1.85f,4.9f,.24f,-2.91f,.14f,.035f,.025f);
        Text(s,"SAVE FAILED - CHANGES RETAINED / RETRYING",1.85f,.026f,1.f,.48f,.30f);
    }
    char text[80]={};
#if RECON_PROJECT_KIND == 0
    switch(s->simulation.phase()){
    case sim::Phase::Briefing:{
        const char *labels[]={"RAIL APPROACH","ORCHARD","RIVER CROSSING"};
        Text(s,"CHOOSE RECON SECTOR",.92f,.039f,.75f,.89f,.95f);
        std::snprintf(text,sizeof(text),"TOTAL TRUST %+d / EFFECT %+d",
            s->simulation.credibility(),s->simulation.operational_effect());
        Text(s,text,.45f,.031f,.80f,.89f,.87f);
        Text(s,labels[s->choice%3],-.22f,.055f,1.f,.81f,.37f);
        Text(s,"SELECT TO DISPATCH",-.89f,.037f,.79f,.90f,.89f);
        break;
    }
    case sim::Phase::InFlight:
        Text(s,"RECON AIRCRAFT IN FLIGHT",.54f,.040f,.80f,.87f,.95f);
        std::snprintf(text,sizeof(text),"TOTAL TRUST %+d / EFFECT %+d",
            s->simulation.credibility(),s->simulation.operational_effect());
        Text(s,text,.04f,.031f,.80f,.89f,.87f);
        Text(s,"SELECT TO RECEIVE PHOTO",-.63f,.038f,1.f,.82f,.39f);
        break;
    case sim::Phase::PhotoReady:
    case sim::Phase::Filed:{
        if(s->simulation.phase()==sim::Phase::Filed){
            std::snprintf(text,sizeof(text),"TOTAL TRUST %+d / EFFECT %+d",
                s->simulation.credibility(),s->simulation.operational_effect());
            Text(s,text,.99f,.034f,.79f,.91f,.96f);
        }else Text(s,"PHOTOGRAPH - INTERPRET EVIDENCE",.99f,.029f,.79f,.91f,.96f);
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
    case sim::Phase::Briefing:{
        std::snprintf(text,sizeof(text),"VEHICLE %d / FUEL %d",
            s->simulation.vehicle_condition(),s->simulation.fuel_remaining());
        if(s->confirmRefit){
            Text(s,"CONFIRM BASE SERVICE",.87f,.043f,.77f,.91f,.96f);
            Text(s,"REPAIR TO 100 / REFUEL TO 20",.43f,.036f,.90f,.86f,.70f);
            Text(s,text,.02f,.033f,.80f,.90f,.86f);
            Text(s,s->choice==0?"CANCEL":"CONFIRM REFIT",-.57f,.050f,1.f,.82f,.36f);
        }else{
            Text(s,"CHOOSE ROUTE OR BASE SERVICE",.87f,.040f,.77f,.91f,.96f);
            Text(s,text,.43f,.034f,.80f,.90f,.86f);
            const char* labels[]={"MAIN ROAD","RIDGE BYPASS","BASE SERVICE"};
            Text(s,labels[s->choice%3],-.16f,.051f,1.f,.82f,.36f);
            if(s->choice==2){
                Text(s,"REPAIR / REFUEL - CONFIRM FIRST",-.61f,.032f,.90f,.85f,.72f);
                Text(s,"SELECT TO REVIEW",-1.02f,.035f,.79f,.92f,.87f);
            }else{
                const auto route=static_cast<sim::Route>(s->choice);
                std::snprintf(text,sizeof(text),"ROUND TRIP: %d FUEL",sim::Simulation::RoundTripFuel(route));
                Text(s,text,-.61f,.033f,.88f,.89f,.75f);
                Text(s,s->simulation.CanBeginPatrol(route)?"SELECT TO DEPLOY":
                    s->simulation.vehicle_condition()<=0?"VEHICLE DISABLED - USE BASE SERVICE":
                    "LOW FUEL - CHOOSE BASE SERVICE",-1.02f,.032f,.92f,.80f,.57f);
            }
        }
        break;
    }
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
        Text(s,"SELECT NEXT PATROL",-.94f,.037f,1.f,.85f,.36f);
        Text(s,"KEEPS CONDITION AND FUEL",-1.24f,.027f,.80f,.89f,.86f);
        break;
    }
#endif
    s->upload=true;
}
constexpr char kSaveSlot[]="session-v1.bin";
void RefreshEvidence(State* s){
#if RECON_PROJECT_KIND == 0
    const auto* photo=s->simulation.evidence_photo();
    s->hasPhoto=photo!=nullptr;s->photo=photo?*photo:sim::Photograph{};
    const auto* report=s->simulation.last_report();s->result=report?*report:sim::ReportResult{};
#else
    s->evidence=sim::ContactEvidence{};s->debrief=sim::Debrief{};
    s->simulation.InspectContact(&s->evidence);s->simulation.LastDebrief(&s->debrief);
#endif
}
SavedSession Snapshot(const State* s){
    SavedSession saved;saved.simulation=s->simulation;saved.choice=static_cast<uint8_t>(s->choice);
#if RECON_PROJECT_KIND == 1
    saved.confirm_refit=s->confirmRefit;
#endif
    return saved;
}
bool Save(State* s){
    if(!s||s->loadBlocked)return false;
    s->saveDirty=true;s->retrySeconds=1.f;
    reverie::recon::Checkpoint::Bytes bytes{};
    const bool ok=ReverieNativeHostSupportsSaveV1(s->host)
        &&reverie::recon::Checkpoint::Encode(Snapshot(s),&bytes)
        &&s->host->write_save(kSaveSlot,bytes.data(),static_cast<uint32_t>(bytes.size()))==REVERIE_NATIVE_SAVE_OK;
    if(!ok&&!s->saveFailed)ReverieNativeLogV1(s->host,REVERIE_NATIVE_LOG_WARN,kId,
        "Checkpoint failed; latest progress retained for retry");
    s->saveFailed=!ok;if(ok)s->saveDirty=false;
    return ok;
}
bool Load(State* s){
    reverie::recon::Checkpoint::Bytes bytes{};uint32_t count=0;
    const int status=ReverieNativeHostSupportsSaveV1(s->host)?
        s->host->read_save(kSaveSlot,bytes.data(),static_cast<uint32_t>(bytes.size()),&count):REVERIE_NATIVE_SAVE_ERROR;
    if(status==REVERIE_NATIVE_SAVE_NOT_FOUND&&!s->loadBlocked)return true;
    SavedSession next;
    if(status!=REVERIE_NATIVE_SAVE_OK||!reverie::recon::Checkpoint::Decode(&next,bytes.data(),count)){
        s->loadBlocked=true;
        ReverieNativeLogV1(s->host,REVERIE_NATIVE_LOG_WARN,kId,
            "Checkpoint unavailable or corrupt; original retained, Select retries");
        return false;
    }
    s->simulation=next.simulation;s->choice=next.choice;
#if RECON_PROJECT_KIND == 1
    s->confirmRefit=next.confirm_refit;
#endif
    s->loadBlocked=false;s->saveDirty=false;s->saveFailed=false;
    RefreshEvidence(s);return true;
}
bool Act(State *s){
    if(s->loadBlocked)return false;
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
        ok=s->simulation.NextSortie();break;
    }
#else
    switch(s->simulation.phase()){
    case sim::Phase::Briefing:
        if(s->confirmRefit){
            ok=s->choice==0||s->simulation.RefitAtBase();
            if(ok)s->confirmRefit=false;
        }else if(s->choice==2){s->confirmRefit=true;ok=true;}
        else ok=s->simulation.BeginPatrol(static_cast<sim::Route>(s->choice));
        break;
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
        ok=s->simulation.NextPatrol();break;
    }
#endif
    if(ok){s->choice=0;RefreshEvidence(s);Save(s);DrawBoard(s);}
    return ok;
}
void *Create(const ReverieNativeHostV1 *host){
    if(!ReverieNativeHostSupportsLogV1(host))return nullptr;
    State *s=new(std::nothrow) State();
    if(!s)return nullptr;
    s->host=host;Load(s);DrawBoard(s);
    ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_INFO,kId,"Native prototype started");
    return s;
}
void Destroy(void *p){
    auto* s=static_cast<State*>(p);if(!s)return;
    if(s->saveDirty&&!s->loadBlocked)Save(s);
    delete s;
}
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
    if(!s->font.Create()){
        ReverieNativeLogV1(s->host,REVERIE_NATIVE_LOG_ERROR,kId,
                           "Lato font GPU atlas initialization failed");
        glDeleteBuffers(1,&s->vbo);glDeleteProgram(s->shader);
        s->vbo=s->shader=0;return 0;
    }
    s->upload=true;
    return 1;
}
void GlEnd(void *p){
    State *s=static_cast<State *>(p);if(!s)return;
    if(s->vbo)glDeleteBuffers(1,&s->vbo);
    if(s->shader)glDeleteProgram(s->shader);
    s->font.Release();
    s->vbo=0;s->shader=0;s->upload=true;
}
void Resume(void *p){
    auto* s=static_cast<State*>(p);if(!s)return;
    if(s->loadBlocked)Load(s);else if(s->saveDirty)Save(s);
    s->inputReady=false;s->primary=s->secondary=s->axis=false;DrawBoard(s);
}
void Pause(void *p){
    auto* s=static_cast<State*>(p);if(!s)return;
    if(s->saveDirty&&!s->loadBlocked)Save(s);
    s->inputReady=false;s->primary=s->secondary=s->axis=false;DrawBoard(s);
}
int32_t PrepareExit(void *p){
    auto* s=static_cast<State*>(p);if(!s)return 1;
    if(s->loadBlocked)return 0;
    return !s->saveDirty||Save(s)?1:0;
}
void Update(void *p,const ReverieNativeInputV1 *in){
    State *s=static_cast<State *>(p);
    if(!s||!ReverieNativeInputHasBaseV1(in))return;
    bool first=in->primary_down!=0,second=in->secondary_down!=0;
    bool axis=std::fabs(in->move_x)>.65f||std::fabs(in->move_y)>.65f;
    if(!s->inputReady){
        if(!first&&!second&&!axis)s->inputReady=true;
        return; // Recreated/lifecycle-restored boards require released controls.
    }
    if(s->loadBlocked){
        if(first&&!s->primary){Load(s);DrawBoard(s);}
        s->primary=first;s->secondary=second;s->axis=axis;return;
    }
    if(s->saveDirty){
        const float dt=std::isfinite(in->delta_seconds)?std::max(0.f,std::min(.05f,in->delta_seconds)):0.f;
        s->retrySeconds-=dt;
        if(s->retrySeconds<=0.f){Save(s);DrawBoard(s);}
    }
    if((second&&!s->secondary)||(axis&&!s->axis)){
        s->choice=(s->choice+1)%Choices(s);
        Save(s);DrawBoard(s);
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
    const bool baseOk=glGetError()==GL_NO_ERROR;
    const bool fontOk=s->font.Draw(matrix);
    ReverieNativeGlAttribRestoreV1(&b);
    ReverieNativeGlAttribRestoreV1(&a);
    ReverieNativeGlStateRestoreV1(&old);
    return baseOk && fontOk ? 1:0;
}
const ReverieNativeModuleApiV1 api={
    sizeof(ReverieNativeModuleApiV1),REVERIE_NATIVE_MODULE_ABI_VERSION,
    {sizeof(ReverieNativeModuleDescriptorV1),REVERIE_NATIVE_MODULE_ABI_VERSION,
     kId,kTitle,2u,0u},
    Create,Destroy,GlStart,GlEnd,Resume,Pause,Update,Render,nullptr,nullptr,PrepareExit
};
}
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1 *reverie_native_module_entry_v1(void){
    return &api;
}
