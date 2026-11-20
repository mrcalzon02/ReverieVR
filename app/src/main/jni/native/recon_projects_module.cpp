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
    sim::Photograph photo{},prior{};
    sim::ReportResult result{};
    bool hasPhoto=false,hasPrior=false,panChord=false;
    int cropX=4,cropY=4,lastAimX=-1,lastAimY=-1;
    std::array<uint16_t,16> photoPins{};
#else
    sim::ContactEvidence evidence{};
    sim::Debrief debrief{};
#endif
    int choice=0;
    bool saveDirty=false,saveFailed=false,loadBlocked=false,inputReady=false;
    bool viewDirty=false; // Inspection moves can wait until the next checkpoint or exit.
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
#if RECON_PROJECT_KIND == 0
constexpr float kPhotoPixel=.083f,kPhotoY=.16f,kPhotoZ=-2.92f,kLoupeX=1.59f;
constexpr int kFilmSize=16,kWindowSize=8;
// Distinct physical-Lato rows; source/loupe labels were previously
// crowded by the new pin counter. Keep the report and shell bar separated.
constexpr float kPhotoLabelRow=-.64f;
constexpr float kPhotoStatusRow=-.85f;
constexpr float kPhotoControlsRow=-1.05f;
constexpr float kPhotoReportRow=-1.30f;
static_assert(kPhotoLabelRow-kPhotoStatusRow>=.19f&&
              kPhotoStatusRow-kPhotoControlsRow>=.19f&&
              kPhotoControlsRow-kPhotoReportRow>=.24f,
              "Lantern photograph instructions require distinct rows");
void DrawFilm(State *s,const sim::Photograph &film,float cx,int startX,int startY,int size){
    const float step=kPhotoPixel*kFilmSize/size;
    Rect(s,cx,kPhotoY,1.46f,1.46f,-2.96f,.14f,.17f,.14f);
    for(int y=0;y<size;++y)for(int x=0;x<size;++x){
        if(film.pixels[startY+y] & (1u<<(startX+x)))
            Rect(s,cx+(x-(size-1)*.5f)*step,kPhotoY+((size-1)*.5f-y)*step,
                 step*.96f,step*.96f,kPhotoZ,.84f,.85f,.63f);
    }
}
// A descriptive comparison aid, NOT a diagnosis or an assessment hint.
 // Only source pixels within the actual synchronized 8x8 loupe window count.
 // No GPU readback, allocation, saved flag or per-eye scan is required.
int VisiblePhotoChangeCount(const State* s){
    if(!s||!s->hasPrior||!s->hasPhoto||
       s->simulation.phase()!=sim::Phase::PhotoReady||
       s->cropX<0||s->cropX>kFilmSize-kWindowSize||
       s->cropY<0||s->cropY>kFilmSize-kWindowSize)return -1;
    int changed=0;
    for(int row=0;row<kWindowSize;++row){
        const uint16_t diff=static_cast<uint16_t>(
            (s->photo.pixels[s->cropY+row]^s->prior.pixels[s->cropY+row])
            >>s->cropX);
        for(int col=0;col<kWindowSize;++col)
            changed+=(diff>>col)&1u;
    }
    return changed;
}
constexpr unsigned kMaxPhotoPins=12u;
unsigned PhotoPinCount(const State* s){
    unsigned total=0;
    for(uint16_t row:s->photoPins)
        for(;row;row=static_cast<uint16_t>(row&(row-1u)))++total;
    return total;
}
bool TogglePhotoPin(State* s){
    if(!s||!s->hasPhoto||s->simulation.phase()!=sim::Phase::PhotoReady)
        return false;
    // An 8x8 lens has no unique center. Pin the SE center cell at +4,+4.
    const int x=std::clamp(s->cropX+4,0,kFilmSize-1);
    const int y=std::clamp(s->cropY+4,0,kFilmSize-1);
    const uint16_t bit=static_cast<uint16_t>(1u<<x);
    if(s->photoPins[y]&bit)
        s->photoPins[y]=static_cast<uint16_t>(s->photoPins[y]&~bit);
    else{
        if(PhotoPinCount(s)>=kMaxPhotoPins)return false;
        s->photoPins[y]=static_cast<uint16_t>(s->photoPins[y]|bit);
    }
    return true;
}
void DrawPhotoPins(State* s,float cx,int firstX,int firstY,int size){
    const float step=kPhotoPixel*kFilmSize/size,half=step*.38f;
    const float stroke=std::max(.005f,step*.065f);
    for(int y=0;y<size;++y)for(int x=0;x<size;++x){
        if(!(s->photoPins[firstY+y]&(1u<<(firstX+x))))continue;
        const float px=cx+(x-(size-1)*.5f)*step;
        const float py=kPhotoY+((size-1)*.5f-y)*step;
        // Hollow outline: the original photographic pixel remains legible.
        Rect(s,px,py+half,2.f*half,stroke,-2.89f,.99f,.67f,.20f);
        Rect(s,px,py-half,2.f*half,stroke,-2.89f,.99f,.67f,.20f);
        Rect(s,px-half,py,stroke,2.f*half,-2.89f,.99f,.67f,.20f);
        Rect(s,px+half,py,stroke,2.f*half,-2.89f,.99f,.67f,.20f);
    }
}
void DrawLoupe(State *s){
    DrawFilm(s,s->photo,kLoupeX,s->cropX,s->cropY,kWindowSize);
    DrawPhotoPins(s,kLoupeX,s->cropX,s->cropY,kWindowSize);
    if(s->hasPrior){
        DrawFilm(s,s->prior,-kLoupeX,s->cropX,s->cropY,kWindowSize);
        s->font.Fit("2X PREVIOUS",-kLoupeX,kPhotoLabelRow,1.42f,.025f,.76f,.89f,.98f,-2.87f);
    }
    const float width=kWindowSize*kPhotoPixel;
    const float x=(s->cropX-4)*kPhotoPixel,y=kPhotoY-(s->cropY-4)*kPhotoPixel;
    for(int side:{-1,1}){
        Rect(s,x+side*width*.5f,y,.014f,width,-2.90f,.96f,.70f,.27f);
        Rect(s,x,y+side*width*.5f,width,.014f,-2.90f,.96f,.70f,.27f);
    }
    s->font.Fit("SOURCE FRAME",0.f,kPhotoLabelRow,1.42f,.025f,.77f,.89f,.84f,-2.87f);
    s->font.Fit(s->hasPrior?"2X CURRENT":"2X LOUPE",kLoupeX,kPhotoLabelRow,1.42f,.025f,.96f,.82f,.45f,-2.87f);
}
// Aim updates only when its source pixel changes. A stationary head ray
// cannot return the loupe to center immediately after a pad pan.
bool AimLoupe(State *s,const ReverieNativeInputV1 *in,bool padOwns){
    int hitX=-1,hitY=-1;
    if(ReverieNativeInputHasPointerV1(in) && in->pointer_kind!=REVERIE_NATIVE_POINTER_NONE &&
       in->pointer_direction[2]<-.0001f){
        const float t=(kPhotoZ-in->pointer_origin[2])/in->pointer_direction[2];
        const float x=in->pointer_origin[0]+t*in->pointer_direction[0];
        const float y=in->pointer_origin[1]+t*in->pointer_direction[1]-kPhotoY;
        const float half=kFilmSize*kPhotoPixel*.5f;
        if(t>0 && x>=-half && x<half && y<=half && y>-half){
            hitX=std::clamp(static_cast<int>((half+x)/kPhotoPixel),0,kFilmSize-1);
            hitY=std::clamp(static_cast<int>((half-y)/kPhotoPixel),0,kFilmSize-1);
        }
    }
    const bool moved=hitX!=s->lastAimX || hitY!=s->lastAimY;
    s->lastAimX=hitX;s->lastAimY=hitY;
    if(!moved || hitX<0 || padOwns)return false;
    const int x=std::clamp(hitX-kWindowSize/2,0,kFilmSize-kWindowSize);
    const int y=std::clamp(hitY-kWindowSize/2,0,kFilmSize-kWindowSize);
    if(x==s->cropX && y==s->cropY)return false;
    s->cropX=x;s->cropY=y;
    return true;
}
#endif
#if RECON_PROJECT_KIND == 1
float TrailX(int marker){return .48f*std::sin(.74f*marker);}
float TrailZ(int marker,int position){return -3.4f-2.f*(marker-position);}
void GroundBand(State *s,float nearX,float farX,float nearZ,float farZ,
                float halfWidth,float height,float r,float g,float b){
    auto v=[&](float x,float z){reverie::flatmesh::Vertex(s->mesh,x,height,z,r,g,b);};
    v(nearX-halfWidth,nearZ);v(nearX+halfWidth,nearZ);v(farX+halfWidth,farZ);
    v(nearX-halfWidth,nearZ);v(farX+halfWidth,farZ);v(farX-halfWidth,farZ);
}
void DrawRidgeTrail(State *s){
    const auto &patrol=s->simulation;
    const int pos=patrol.trail_position();
    const float origin=TrailX(pos);
    const auto tri=[&](float ax,float ay,float bx,float by,float cx,float cy,
                       float z,float r,float g,float b){
        reverie::flatmesh::Triangle(s->mesh,ax,ay,bx,by,cx,cy,z,r,g,b);
    };
    Rect(s,0,0,96,70,-28,.48f,.68f,.73f);
    for(int i=-3;i<5;++i){
        const float x=i*6.5f-origin;
        tri(x-7,-1.1f,x,2.1f+(i%3)*.47f,x+6,-1.1f,-20,.23f,.38f,.37f);
        tri(x,2.1f+(i%3)*.47f,x+6,-1.1f,x+1,-1.1f,-20,.31f,.45f,.42f);
    }
    GroundBand(s,0,0,-.3f,-27,40,-1.05f,.29f,.40f,.27f);
    for(int m=sim::Simulation::kRidgeExit+4;m>=pos-1;--m){
        const float z=TrailZ(m,pos),x=TrailX(m)-origin;
        const float closerZ=TrailZ(m-1,pos),closerX=TrailX(m-1)-origin;
        GroundBand(s,closerX,x,closerZ,z,.80f,-1.04f,.62f,.53f,.36f);
        if(m<0 || z>-.65f)continue;
        const float tree=x+(m%2?-1.f:1.f)*(2.9f+.12f*(m%3));
        Rect(s,tree,-.52f,.13f,1.f,z,.23f,.19f,.13f);
        tri(tree-.56f,-.56f,tree,1.06f,tree+.56f,-.56f,z,.14f,.29f,.22f);
        if(m==2){
            // A measuring staff sits in the stream. Its waterline and bands
            // stay in world coordinates as the observer approaches.
            GroundBand(s,x-2,x-2,z+1.1f,z-1.2f,.72f,-1.02f,.29f,.49f,.53f);
            Rect(s,x-1.25f,-.30f,.19f,1.50f,z,.89f,.82f,.62f);
            for(int mark=0;mark<5;++mark)
                Rect(s,x-1.28f,.22f-.14f*mark,.11f,.045f,z,.23f,.24f,.20f);
            Rect(s,x-1.25f,-.31f,.19f,.10f,z,.23f,.44f,.52f);
        }else if(m==5){
            // Posts, triangular roof and a bench make the shelter visible
            // before it becomes possible to record its condition.
            Rect(s,x+1.15f,-.37f,.09f,1.34f,z,.37f,.29f,.21f);
            Rect(s,x+2.43f,-.37f,.09f,1.34f,z,.37f,.29f,.21f);
            Rect(s,x+1.79f,-.61f,1.15f,.10f,z,.43f,.33f,.22f);
            tri(x+.93f,.25f,x+1.79f,.87f,x+2.65f,.25f,z,.55f,.30f,.20f);
        }else if(m==sim::Simulation::kRidgeExit){
            Rect(s,x-.99f,-.35f,.11f,1.4f,z,.34f,.28f,.21f);
            Rect(s,x-1.f,.14f,.84f,.33f,z,.80f,.71f,.46f);
        }
    }
    Rect(s,0,1.39f,4.62f,.52f,-2.99f,.09f,.16f,.16f);
    Text(s,"RIDGE BYPASS  /  FOOT SURVEY",1.39f,.044f,.91f,.96f,.85f);
    Rect(s,0,-1.21f,4.79f,.84f,-2.99f,.08f,.13f,.14f);
    char label[80];
    const int notes=(patrol.trail_notes()&1)+((patrol.trail_notes()>>1)&1);
    std::snprintf(label,sizeof(label),"TRAIL %d / %d      NOTES %d / 2",
                  pos,sim::Simulation::kRidgeExit,notes);
    Text(s,label,-.91f,.031f,.77f,.92f,.88f);
    const auto here=patrol.ObservationHere();
    const char *action="SELECT TO WALK ON";
    if(pos==sim::Simulation::kRidgeExit)action="RIDGE TURNOUT  /  SELECT TO INSPECT CONTACT";
    else if(here && !(patrol.trail_notes()&here))action=here==sim::Simulation::kGaugeNote?
        "SELECT TO RECORD THE HIGH WATER MARK":"SELECT TO RECORD THE INTACT SHELTER";
    else if(here)action="OBSERVATION LOGGED  /  SELECT TO WALK ON";
    Text(s,action,-1.18f,.033f,.99f,.85f,.52f);
    Text(s,"PAD RIGHT TO WALK   /   LEFT TO RETURN",-1.47f,.028f,.81f,.90f,.86f);
    if(s->saveFailed)Text(s,"SAVE FAILED - RETRYING",1.73f,.029f,1.f,.60f,.41f);
}
#endif
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
#if RECON_PROJECT_KIND == 1
    if(!s->loadBlocked && s->simulation.OnRidgeTrail()){
        DrawRidgeTrail(s);s->upload=true;return;
    }
#endif
    Rect(s,0,0,5.0f,3.45f,-3.03f,.028f,.050f,.076f);
    Rect(s,0,1.43f,4.75f,.44f,-2.99f,.13f,.21f,.22f);
    Text(s,kHeader,1.39f,.053f,.93f,.94f,.83f);
    Text(s,Choices(s)>1 && !s->loadBlocked ? "PAD PREV / NEXT   SELECT ACTION   MENU EXIT" :
         "SELECT ACTION   MENU EXIT",-1.54f,.027f,.65f,.83f,.78f);
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
        }else{
            const char *sectors[]={"RAIL APPROACH","ORCHARD","RIVER CROSSING"};
            std::snprintf(text,sizeof(text),"%s / PHOTO CLARITY %u%%",
                sectors[static_cast<unsigned>(s->photo.sector)],static_cast<unsigned>(s->photo.clarity));
            Text(s,text,.99f,.029f,.79f,.91f,.96f);
            if(!s->hasPrior){
                // The field reference is identical on all assignments. It describes
                // image features without identifying the current photograph.
                s->font.Fit("FIELD GUIDE",-1.57f,.65f,1.28f,.030f,.79f,.91f,.96f,-2.87f);
                s->font.Fit("COLUMN",-1.57f,.36f,1.28f,.028f,.92f,.82f,.59f,-2.87f);
                s->font.Fit("PAIRED PROFILES",-1.57f,.16f,1.35f,.025f,.83f,.90f,.87f,-2.87f);
                s->font.Fit("BRIDGE DAMAGE",-1.57f,-.12f,1.35f,.028f,.92f,.82f,.59f,-2.87f);
                s->font.Fit("GAP IN DECK",-1.57f,-.32f,1.28f,.025f,.83f,.90f,.87f,-2.87f);
            }
        }
        if(s->hasPhoto){
            DrawFilm(s,s->photo,0.f,0,0,kFilmSize);
            DrawPhotoPins(s,0.f,0,0,kFilmSize);
            if(s->simulation.phase()==sim::Phase::PhotoReady)DrawLoupe(s);
        }
        if(s->simulation.phase()==sim::Phase::PhotoReady){
            const char *labels[]={"NO CONTACT","POSSIBLE CONTACT",
                                  "VEHICLE COLUMN","BRIDGE DAMAGE"};
            const int changed=VisiblePhotoChangeCount(s);
            if(changed>=0)
                std::snprintf(text,sizeof(text),"WINDOW CHANGED %d / 64   PINS %u / %u",
                    changed,PhotoPinCount(s),kMaxPhotoPins);
            else
                std::snprintf(text,sizeof(text),"PHOTO PINS %u / %u",
                    PhotoPinCount(s),kMaxPhotoPins);
            Text(s,text,kPhotoStatusRow,.029f,.99f,.77f,.45f);
            Text(s,"MINUS+PAD INSPECT     MINUS+SELECT PIN",kPhotoControlsRow,.028f,.83f,.91f,.90f);
            std::snprintf(text,sizeof(text),"FILE: %s",labels[s->choice%4]);
            Text(s,text,kPhotoReportRow,.040f,1.f,.81f,.36f);
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
                Text(s,text,-.51f,.033f,.88f,.89f,.75f);
                const auto intel=s->simulation.LastRouteIntel(route);
                if(intel)std::snprintf(text,sizeof(text),"LAST FILED INTEL %+d",intel);
                else std::snprintf(text,sizeof(text),"NO FILED REPORT");
                Text(s,text,-.78f,.030f,.78f,.87f,.90f);
                if(route==sim::Route::RidgeBypass && s->simulation.filed_ridge_notes()){
                    const uint8_t filed=s->simulation.filed_ridge_notes();
                    Text(s,filed==3?"FILED: GAUGE AND SHELTER":filed==1?"FILED: WATER MARK":"FILED: SHELTER INTACT",
                         -1.24f,.025f,.76f,.87f,.86f);
                }
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
        if(s->simulation.trail_notes()){
            const auto notes=s->simulation.trail_notes();
            Text(s,notes==3?"FILED: HIGH WATER MARK / SHELTER INTACT":
                 notes==1?"FILED: HIGH WATER MARK":"FILED: SHELTER INTACT",
                 -.18f,.032f,.77f,.89f,.88f);
        }
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
    s->hasPrior=s->simulation.phase()==sim::Phase::PhotoReady && s->simulation.previous_exposure(&s->prior);
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
#else
    if(s->hasPrior){saved.crop_x=static_cast<uint8_t>(s->cropX);saved.crop_y=static_cast<uint8_t>(s->cropY);}
    saved.pins=s->photoPins;
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
    s->saveFailed=!ok;if(ok){s->saveDirty=false;s->viewDirty=false;}
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
#else
    s->cropX=next.crop_x;s->cropY=next.crop_y;s->lastAimX=s->lastAimY=-1;
    s->photoPins=next.pins;
#endif
    s->loadBlocked=false;s->saveDirty=false;s->saveFailed=false;s->viewDirty=false;
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
            s->cropX=s->cropY=4;s->lastAimX=s->lastAimY=-1;s->panChord=false;
            s->photoPins.fill(0);
        }
        break;
    case sim::Phase::PhotoReady:
        ok=s->simulation.SubmitReport(static_cast<sim::Assessment>(s->choice),
                                      &s->result);break;
    case sim::Phase::Filed:
        ok=s->simulation.NextSortie();
        if(ok)s->photoPins.fill(0); // Pins are local to the prior exposure.
        break;
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
        if(s->simulation.OnRidgeTrail() && s->simulation.trail_position()<sim::Simulation::kRidgeExit){
            ok=s->simulation.RecordObservation() || s->simulation.WalkRidge(1);
            break;
        }
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
    if((s->saveDirty || s->viewDirty)&&!s->loadBlocked)Save(s);
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
    if((s->saveDirty || s->viewDirty)&&!s->loadBlocked)Save(s);
    s->inputReady=false;s->primary=s->secondary=s->axis=false;DrawBoard(s);
}
int32_t PrepareExit(void *p){
    auto* s=static_cast<State*>(p);if(!s)return 1;
    if(s->loadBlocked)return 0;
    return (!s->saveDirty && !s->viewDirty)||Save(s)?1:0;
}
void Update(void *p,const ReverieNativeInputV1 *in){
    State *s=static_cast<State *>(p);
    if(!s||!ReverieNativeInputHasBaseV1(in))return;
    // POINTER_NONE on a full native prefix means the shell or Quick Menu
    // owns input. A shorter base-only legacy host is intentionally different:
    // its buttons and pad still work after a neutral rearm.
    if(ReverieNativeInputHasPointerV1(in)){
        ReverieNativeInputV1 ray{};
        std::memcpy(&ray,in,std::min<size_t>(in->struct_size,sizeof(ray)));
        if(!ReverieNativeSanitizePointerV1(&ray)){
            s->inputReady=false;s->primary=s->secondary=s->axis=false;
#if RECON_PROJECT_KIND == 0
            s->panChord=false;s->lastAimX=s->lastAimY=-1;
#endif
            return;
        }
    }
    // Invalid axes cannot stand in for a released trackpad or advance a
    // saved mission. Even a partially held pad must release before reentry.
    if(!std::isfinite(in->move_x)||!std::isfinite(in->move_y)){
        s->inputReady=false;s->primary=s->secondary=s->axis=false;
#if RECON_PROJECT_KIND == 0
        s->panChord=false;s->lastAimX=s->lastAimY=-1;
#endif
        return;
    }
    const bool first=in->primary_down!=0,second=in->secondary_down!=0;
    const bool axis=std::fabs(in->move_x)>.65f||std::fabs(in->move_y)>.65f;
    if(!s->inputReady){
        if(!first&&!second&&std::fabs(in->move_x)<=.20f&&
           std::fabs(in->move_y)<=.20f){
            s->inputReady=true;
#if RECON_PROJECT_KIND == 0
            s->panChord=false;
#endif
        }
        return; // One complete neutral release frame is never a game tick.
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
    const int choices=Choices(s);
    bool nextButton=second&&!s->secondary;
    bool stepAxis=axis&&!s->axis;
    bool activate=first&&!s->primary;
#if RECON_PROJECT_KIND == 0
    if(s->simulation.phase()==sim::Phase::PhotoReady){
        bool redraw=AimLoupe(s,in,second || s->panChord);
        bool pinAction=false;
        // A short Minus tap still advances the report. Holding it with a
        // rocker direction exclusively moves the window by two film pixels.
        if(second && axis){
            if(!s->panChord || !s->axis){
                const int dx=in->move_x>.65f?2:in->move_x<-.65f?-2:0;
                const int dy=in->move_y>.65f?2:in->move_y<-.65f?-2:0;
                const int x=std::clamp(s->cropX+dx,0,kFilmSize-kWindowSize);
                const int y=std::clamp(s->cropY+dy,0,kFilmSize-kWindowSize);
                redraw|=x!=s->cropX || y!=s->cropY;
                s->cropX=x;s->cropY=y;
            }
            s->panChord=true;
        }
        // Hold Minus first, then Select. A pin chord cannot submit a report
        // or cycle assessment when Minus is released.
        if(second && s->secondary && activate && !axis && !s->panChord){
            pinAction=true;
            const bool toggled=TogglePhotoPin(s);
            if(toggled) {
                s->viewDirty=true;
                const bool saved=Save(s);
                ReverieNativeRequestFeedbackV1(s->host,saved?
                    REVERIE_NATIVE_FEEDBACK_ACTIVATION:
                    REVERIE_NATIVE_FEEDBACK_FAILURE);
                redraw=true;
            }else ReverieNativeRequestFeedbackV1(s->host,
                           REVERIE_NATIVE_FEEDBACK_FAILURE);
            s->panChord=true;
        }
        nextButton=!second && s->secondary && !s->panChord;
        stepAxis=stepAxis && !second && !s->panChord;
        activate=activate && !second && !s->panChord;
        if(!second)s->panChord=false;
        if(redraw){
            if(s->hasPrior && !pinAction)s->viewDirty=true;
            DrawBoard(s);
        }
    }
#endif
#if RECON_PROJECT_KIND == 1
    if(s->simulation.OnRidgeTrail() && stepAxis && !first && std::fabs(in->move_x)>.65f){
        if(s->simulation.WalkRidge(in->move_x>0?1:-1)){
            Save(s);DrawBoard(s);
            ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
        }
    }
#endif
    if(choices>1 && (nextButton || stepAxis)){
        const bool horizontal=std::fabs(in->move_x)>=std::fabs(in->move_y);
        const bool previous=horizontal ? in->move_x<0.f : in->move_y<0.f;
        const int step=nextButton ? 1 : previous ? -1 : 1;
        s->choice=(s->choice+choices+step)%choices;
        Save(s);DrawBoard(s);
        ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
    }
    if(activate){
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
