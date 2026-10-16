#include "omega_front_campaign_save.h"
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
    game::Session session{414u};
    GLuint program=0,vbo=0;
    GLint pos=-1,color=-1,mvp=-1;
    bool upload=true,save_failed=false,load_failed=false;
    uint32_t saved_tick=0,saved_orders=0;
    bool saved_pause=false;
    game::Result saved_result=game::Result::Active;
    game::FrontMode saved_view=game::FrontMode::SectorMap;
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
void TextAt(State *s,const char *msg,float cx,float center_y,float px,float r,float g,float b){
    const size_t count=std::strlen(msg);
    const float start=cx-static_cast<float>(count)*px*2.f;
    for(size_t i=0;i<count;i++){
        char c=msg[i];if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');
        const char *glyph=Glyph(c);
        for(int p=0;p<15;p++)if(glyph[p]=='1')
            Rect(s,start+(i*4.f+static_cast<float>(p%3))*px,
                 center_y+(2.f-static_cast<float>(p/3))*px,
                 px*.92f,px*.92f,kNear,r,g,b);
    }
}
void DrawMap(State *s){
    const game::Session &session=s->session;
    const game::Frontier &f=session.frontier;
    s->mesh.clear();s->mesh.reserve(14000);
    Rect(s,0,0,2.15f,2.3f,kBoardZ-.09f,.018f,.036f,.049f);
    Text(s,"OMEGA FRONT  HEARTHFALL",1.070f,.014f,.92f,.94f,.81f);
    char line[110];
    std::snprintf(line,sizeof(line),"DAY %d  GUARDS %d  RATIONS %d  SALVAGE %d",f.day(),f.guards(),f.rations(),f.salvage());
    Text(s,line,.916f,.009f,.77f,.85f,.88f);
    constexpr float span=.395f;
    for(int i=0;i<game::kFrontierSectors;++i){
        const auto *sector=f.sector(i);
        if(!sector)continue;
        const float x=(i%4-1.5f)*span;
        const float y=kCenterY+(1.5f-i/4)*span;
        float r=.32f,g=.38f,b=.31f;
        if(sector->ecology==game::Ecology::Eden){r=.21f;g=.49f;b=.29f;}
        else if(sector->ecology==game::Ecology::Wasteland){r=.44f;g=.33f;b=.29f;}
        else {r=.44f;g=.43f;b=.33f;}
        if(sector->anomaly){r=.48f;g=.33f;b=.61f;}
        Rect(s,x,y,span*.96f,span*.96f,kBoardZ,r,g,b);
        if(i==f.location()){
            // White roadpost = real troop location, not selected hover.
            Rect(s,x,y+span*.12f,span*.66f,span*.07f,-1.86f,.96f,.96f,.89f);
        }
        if(i==session.focused){
            const float h=span*.475f;
            Line(s,x-h,y+h,x+h,y+h,.024f,-1.84f,1.f,.83f,.20f);
            Line(s,x+h,y-h,x+h,y+h,.024f,-1.84f,1.f,.83f,.20f);
            Line(s,x-h,y-h,x+h,y-h,.024f,-1.84f,1.f,.83f,.20f);
            Line(s,x-h,y-h,x-h,y+h,.024f,-1.84f,1.f,.83f,.20f);
        }
        char label[12];std::snprintf(label,sizeof(label),"S%03d",i+1);
        TextAt(s,label,x,y,.015f,.98f,.98f,.86f);
        const bool independent=sector->owner==game::kIndependentOwner;
        Rect(s,x+span*.31f,y-span*.31f,span*.14f,span*.14f,-1.85f,
             independent?.25f:sector->owner?.91f:.60f,
             independent?.90f:sector->owner?.32f:.64f,
             independent?.94f:sector->owner?.28f:.67f);
    }
    const auto *target=f.sector(session.focused);
    if(target){
        std::snprintf(line,sizeof(line),"S%03d %s",session.focused+1,target->name);
        Text(s,line,-.868f,.013f,.94f,.95f,.81f);
        std::snprintf(line,sizeof(line),"FOOD %u WATER %u WRECK %u  CLAIM %u",
            target->food,target->water,target->wreck_mass,target->claims);
        Text(s,line,-.982f,.010f,.77f,.86f,.86f);
    }
    const char *actions[]={"DEPLOY","FORAGE","RECRUIT","REPAIR","WAIT"};
    if(session.focused!=f.location()){
        Text(s,f.can_travel(session.focused)?"CLICK TO MARCH":"ROUTE BLOCKED",-1.098f,.013f,
             f.can_travel(session.focused)?.71f:.96f,
             f.can_travel(session.focused)?.98f:.43f,.49f);
    }else {
        std::snprintf(line,sizeof(line),"%s  CLICK EXECUTE  MENU CHANGE",
                      actions[static_cast<int>(session.action)]);
        Text(s,line,-1.098f,.010f,.93f,.92f,.60f);
    }
    if(f.operation()<3){
        std::snprintf(line,sizeof(line),"OP %d OF 3  TARGET S%03d  TRUST %d",
                      f.operation()+1,f.target_sector()+1,f.trust());
        Text(s,line,-1.194f,.009f,.78f,.90f,.93f);
    }else Text(s,"HEARTHWELL FIRST FRONT COMPLETE",-1.194f,.010f,.78f,.94f,.76f);
    if(f.next_morphology_day()>0){
        std::snprintf(line,sizeof(line),"EDEN FORECAST S%03d DAY %d",
                     f.forecast_sector()+1,f.next_morphology_day());
        Text(s,line,.82f,.010f,.92f,.77f,.48f);
    }
    if(session.rejected)Text(s,"ORDER DENIED",-.79f,.011f,1.f,.38f,.40f);
    if(s->save_failed||s->load_failed)
        Text(s,s->save_failed?"SAVE FAILED":"SAVE CORRUPT",-.73f,.011f,1.f,.38f,.40f);
    s->upload=true;
}
void UnitSprite(State *s,const game::Unit &u,float x,float y,game::Doctrine doctrine){
    const bool ally=u.side==game::Side::Hearthwell;
    float rr=ally?.29f:.83f, gg=ally?.80f:.31f, bb=ally?.56f:.29f;
    if(!ally){
        switch(doctrine){
        case game::Doctrine::Hearthward:rr=.65f;gg=.59f;bb=.34f;break;
        case game::Doctrine::Meridian:rr=.25f;gg=.48f;bb=.73f;break;
        case game::Doctrine::Scrapline:rr=.80f;gg=.47f;bb=.20f;break;
        case game::Doctrine::LostNinth:rr=.72f;gg=.26f;bb=.25f;break;
        default:break;
        }
    }
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
    }else if(u.role==game::Class::Salvager){
        // Orange pack / blue recovery tong distinguishes Scrapline crews.
        Rect(s,x-kCell*.29f,y+kCell*.02f,kCell*.20f,kCell*.41f,-1.847f,.92f,.56f,.19f);
        Line(s,x+kCell*.31f,y-kCell*.30f,x+kCell*.39f,y+kCell*.26f,
             kCell*.075f,-1.846f,.34f,.72f,.79f);
    }else if(u.role==game::Class::Sentinel){
        // Meridian ballistic helmet/shoulders and cyan visor.
        Rect(s,x,y+kCell*.37f,kCell*.62f,kCell*.17f,-1.846f,.23f,.27f,.36f);
        Rect(s,x,y+kCell*.24f,kCell*.41f,kCell*.085f,-1.845f,.31f,.86f,.96f);
        Rect(s,x,y-kCell*.02f,kCell*.74f,kCell*.13f,-1.846f,.27f,.37f,.53f);
    }else if(u.role==game::Class::Legionary){
        // Large metal shield and short bronze sword; no imaginary rifle.
        Rect(s,x-kCell*.28f,y-kCell*.09f,kCell*.31f,kCell*.49f,-1.847f,.70f,.22f,.20f);
        Rect(s,x-kCell*.28f,y-kCell*.09f,kCell*.13f,kCell*.25f,-1.846f,.95f,.75f,.34f);
        Line(s,x+kCell*.34f,y-kCell*.26f,x+kCell*.34f,y+kCell*.24f,
             kCell*.08f,-1.846f,.83f,.79f,.69f);
        Rect(s,x,y+kCell*.50f,kCell*.46f,kCell*.09f,-1.846f,.91f,.76f,.32f);
    }
    Rect(s,x,y-kCell*.43f,kCell*.68f,kCell*.065f,-1.844f,.14f,.16f,.19f);
    const float hp_ratio=static_cast<float>(u.hp)/static_cast<float>(game::Battle::MaxHp(u.role));
    Rect(s,x-kCell*.34f+hp_ratio*kCell*.34f,y-kCell*.43f,
         kCell*.68f*hp_ratio,kCell*.055f,-1.843f,.22f,.92f,.32f);
}
void Draw(State *s){
    if(s->session.view==game::FrontMode::SectorMap){DrawMap(s);return;}
    s->mesh.clear();s->mesh.reserve(75000);
    Rect(s,0,0,2.15f,2.3f,kBoardZ-.09f,.018f,.036f,.049f);
    Rect(s,0,kCenterY,2*kHalf+.035f,2*kHalf+.035f,kBoardZ-.04f,.21f,.28f,.20f);
    const game::Battle &battle=s->session.tactical.battle;
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
        if(x==s->session.tactical.cursor_x && y==s->session.tactical.cursor_y){
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
    // Mission marker is at the actual game-logic target; never an inert UI
    // ornament. Capture needs occupation, relay needs a living engineer and
    // two finite demolition charges from the team's field components.
    if(battle.mission()!=game::Mission::HoldPump){
        center(battle.objective_x(),battle.objective_y(),px,py);
        const bool orchard=battle.mission()==game::Mission::CaptureOrchard;
        Rect(s,px,py,kCell*.94f,kCell*.94f,-1.857f,
             orchard?.91f:.65f,orchard?.81f:.48f,orchard?.25f:.95f);
        Rect(s,px,py,kCell*.58f,kCell*.58f,-1.854f,
             orchard?.24f:.37f,orchard?.56f:.24f,orchard?.20f:.64f);
        for(int i=0;i<battle.objective_progress();++i){
            Rect(s,px-kCell*.27f+kCell*.16f*i,py-kCell*.27f,
                 kCell*.12f,kCell*.10f,-1.842f,.93f,.99f,.41f);
        }
    }
    for(int i=0;i<battle.unit_count();++i){
        const game::Unit *u=battle.unit(i);
        if(!u||!u->alive())continue;
        center(u->x,u->y,px,py);
        if(i==s->session.tactical.selected)
            Rect(s,px,py,kCell*.96f,kCell*.96f,-1.858f,.99f,.91f,.27f);
        UnitSprite(s,*u,px,py,battle.doctrine());
    }
    Rect(s,0,1.054f,2.05f,.30f,-1.92f,.07f,.13f,.19f);
    Text(s,"OMEGA FRONT  HEARTHWELL",1.076f,.013f,.91f,.93f,.78f);
    char status[96];
    if(battle.mission()==game::Mission::CaptureOrchard)
        std::snprintf(status,sizeof(status),"HOLD ORCHARD %d/3  NO ENEMY NEARBY  TICK %u",
                      battle.objective_progress(),battle.ticks());
    else if(battle.mission()==game::Mission::DisableRelay)
        std::snprintf(status,sizeof(status),"ENGINEER TO RELAY %d/2  PARTS %d  PUMP %d",
                      battle.objective_progress(),battle.components(),battle.pump_hp());
    else
        std::snprintf(status,sizeof(status),"DEFEND PUMP %d  AMMO %d  TICK %u",
                      battle.pump_hp(),battle.ammo_stock(),battle.ticks());
    Text(s,status,-.946f,.010f,.88f,.92f,.85f);
    // The military doctrine is real simulation state and is saved, not just
    // a label inferred from the operation title.
    const char *doctrine_name="UNALIGNED";
    switch(battle.doctrine()){
    case game::Doctrine::Hearthward:doctrine_name="HEARTHWARD WATCH";break;
    case game::Doctrine::Meridian:doctrine_name="MERIDIAN SECURITY";break;
    case game::Doctrine::Scrapline:doctrine_name="SCRAPLINE CREW";break;
    case game::Doctrine::LostNinth:doctrine_name="LOST NINTH COHORT";break;
    default:break;
    }
    if(battle.doctrine()==game::Doctrine::Scrapline){
        std::snprintf(status,sizeof(status),"%s  AMMO %d  STOLEN %d",
            doctrine_name,battle.ammo_stock(),battle.stolen_rounds());
    }else std::snprintf(status,sizeof(status),"%s",doctrine_name);
    Text(s,status,-.866f,.010f,.82f,.76f,.52f);
    const char *mode_names[]={"MOVE","ATTACK","REPAIR","RESUPPLY"};
    char action[90];
    if(battle.result()==game::Result::Defended)std::snprintf(action,sizeof(action),"DEFENDED  CLICK REPORT");
    else if(battle.result()==game::Result::Overrun)std::snprintf(action,sizeof(action),"OVERRUN  CLICK REPORT");
    else if(s->session.tactical.paused)std::snprintf(action,sizeof(action),"%s",
        s->session.tactical.selected<0?"PAUSED  MENU RESUME":"PAUSED  CLICK UNIT TO CLEAR");
    else if(s->session.tactical.selected<0)std::snprintf(action,sizeof(action),"PICK A GREEN UNIT  CLICK");
    else std::snprintf(action,sizeof(action),"UNIT %d  %s  CLICK ORDER",s->session.tactical.selected,
                       mode_names[static_cast<int>(s->session.tactical.mode)]);
    Text(s,action,-1.061f,.012f,s->session.tactical.rejected?1.f:.91f,
         s->session.tactical.rejected?.37f:.86f,.55f);
    if(s->save_failed || s->load_failed)
        Text(s,s->save_failed?"SAVE FAILED":"SAVE CORRUPT", -1.16f,.011f,1.f,.43f,.40f);
    s->upload=true;
}
constexpr char kSaveSlot[]="frontier.v1";
bool SaveState(State *s){
    if(!s || !ReverieNativeHostSupportsSaveV1(s->host))return false;
    game::CampaignSave::Bytes bytes{};
    if(!game::CampaignSave::Encode(s->session,&bytes))return false;
    const bool ok=s->host->write_save(kSaveSlot,bytes.data(),
        static_cast<uint32_t>(bytes.size()))==REVERIE_NATIVE_SAVE_OK;
    if(!ok && !s->save_failed)
        ReverieNativeLogV1(s->host,REVERIE_NATIVE_LOG_ERROR,kId,
                           "Hearthwell checkpoint write failed; playing in memory");
    s->save_failed=!ok;
    if(ok){
        s->load_failed=false;
        s->saved_tick=s->session.tactical.battle.ticks();
        s->saved_orders=s->session.orders+s->session.tactical.issued;
        s->saved_view=s->session.view;
        s->saved_pause=s->session.tactical.paused;
        s->saved_result=s->session.tactical.battle.result();
    }
    return ok;
}
void LoadState(State *s){
    if(!s || !ReverieNativeHostSupportsSaveV1(s->host))return;
    game::CampaignSave::Bytes bytes{};
    uint32_t size=0;
    const int32_t code=s->host->read_save(kSaveSlot,bytes.data(),
        static_cast<uint32_t>(bytes.size()),&size);
    if(code==REVERIE_NATIVE_SAVE_NOT_FOUND)return;
    if(code!=REVERIE_NATIVE_SAVE_OK || size!=bytes.size() ||
       !game::CampaignSave::Decode(&s->session,bytes.data(),size)){
        ReverieNativeLogV1(s->host,REVERIE_NATIVE_LOG_WARN,kId,
                           "Invalid or unreadable Hearthwell checkpoint; fresh skirmish kept");
        s->load_failed=true;
    }else{
        s->saved_tick=s->session.tactical.battle.ticks();
        s->saved_orders=s->session.orders+s->session.tactical.issued;
        s->saved_view=s->session.view;
        s->saved_pause=s->session.tactical.paused;
        s->saved_result=s->session.tactical.battle.result();
    }
}
void *Create(const ReverieNativeHostV1 *host){
    if(!ReverieNativeHostSupportsLogV1(host))return nullptr;
    State *s=new(std::nothrow) State();
    if(!s)return nullptr;
    s->host=host;LoadState(s);Draw(s);
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
void Resume(void *){}
void Pause(void *p){
    State *s=static_cast<State *>(p);
    if(s && ReverieNativeHostSupportsSaveV1(s->host)){
        const bool previous=s->save_failed;
        SaveState(s);
        if(previous!=s->save_failed)Draw(s);
    }
}
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
       std::fabs(in->pointer_direction[2])>.0001f){
        const float t=(kBoardZ-in->pointer_origin[2])/in->pointer_direction[2];
        if(t>0.f && t<100.f){
            const float x=in->pointer_origin[0]+t*in->pointer_direction[0];
            const float y=in->pointer_origin[1]+t*in->pointer_direction[1];
            if(s->session.view==game::FrontMode::SectorMap){
                constexpr float span=.395f;
                const int gx=static_cast<int>(std::floor((x+2.f*span)/span));
                const int gy=static_cast<int>(std::floor((kCenterY+2.f*span-y)/span));
                if(gx>=0 && gx<4 && gy>=0 && gy<4){
                    action.pointer_x=gx;action.pointer_y=gy;
                }
            }else{
                const int gx=static_cast<int>(std::floor((x+kHalf)/kCell));
                const int gy=static_cast<int>(std::floor((kCenterY+kHalf-y)/kCell));
                if(game::Battle::Inside(gx,gy)){
                    action.pointer_x=gx;action.pointer_y=gy;
                }
            }
        }
    }
    const uint32_t old_orders=s->session.orders+s->session.tactical.issued;
    const uint32_t old_tick=s->session.tactical.battle.ticks();
    const auto old_view=s->session.view;
    if(s->session.Input(action)){
        const uint32_t tick=s->session.tactical.battle.ticks();
        const uint32_t new_orders=s->session.orders+s->session.tactical.issued;
        // Strategy actions and tactical commands are saved immediately. The
        // eye passes never advance the world or write persistent storage.
        if(ReverieNativeHostSupportsSaveV1(s->host) &&
           (new_orders!=s->saved_orders || s->session.view!=s->saved_view ||
            s->session.tactical.paused!=s->saved_pause ||
            s->session.tactical.battle.result()!=s->saved_result ||
            (s->session.view==game::FrontMode::Tactical && tick>=s->saved_tick &&
             tick-s->saved_tick>=16u))) SaveState(s);
        Draw(s);
        if(new_orders!=old_orders || old_view!=s->session.view)
            ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        else if((s->session.view==game::FrontMode::SectorMap?s->session.rejected:
                 s->session.tactical.rejected) && action.primary)
            ReverieNativeRequestFeedbackV1(s->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
        else if(tick==old_tick)
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
