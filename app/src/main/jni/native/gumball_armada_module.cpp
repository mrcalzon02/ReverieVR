#include "gumball_armada_simulation.h"
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
using reverie::gumball::Simulation;
using reverie::gumball::Phase;
using reverie::gumball::Upgrade;
using reverie::gumball::Enemy;
using reverie::gumball::EnemyKind;
using reverie::gumball::Snapshot;
using reverie::gumball::SnapshotV1;
using reverie::gumball::Weapon;
using reverie::gumball::Shipyard;
using reverie::gumball::ShipyardSave;
using reverie::gumball::Part;
const char kId[]="gumball-armada";
const char kName[]="Gumball Armada: Candy King Search [DEV]";
struct Module {
    const ReverieNativeHostV1* host=nullptr;
    Simulation sim;
    Shipyard yard;
    int selected=0; // routes, upgrades, weapons, shipyard
    bool inYard=false;
    int garageX=3,garageY=3;
    Part brush=Part::Frame;
    bool primary=false,saveError=false,upload=true;
    float navTime=0;
    GLuint program=0,vbo=0;GLint pos=-1,col=-1,mvp=-1;
    std::vector<float> mesh;
};
void Rect(Module* m,float x,float y,float w,float h,float z,float r,float g,float b){
    reverie::flatmesh::Rect(m->mesh,x,y,w,h,z,r,g,b);
}
void Disc(Module* m,float x,float y,float d,float z,float r,float g,float b){
    reverie::flatmesh::Disc(m->mesh,x,y,d,z,r,g,b);
}
void Tri(Module* m,float x,float y,float a,float b,float c,float d,float z,float r,float g,float bl){
    reverie::flatmesh::Triangle(m->mesh,x,y,a,b,c,d,z,r,g,bl);
}
const char* Glyph(char c){
    switch(c){
    case 'A':return "010101111101101";case 'B':return "110101110101110";
    case 'C':return "011100100100011";case 'D':return "110101101101110";
    case 'E':return "111100110100111";case 'F':return "111100110100100";
    case 'G':return "011100101101011";case 'H':return "101101111101111";
    case 'I':return "111010010010111";case 'J':return "001001001101010";
    case 'K':return "101101110101101";case 'L':return "100100100100111";
    case 'M':return "101111111101101";case 'N':return "101111111111101";
    case 'O':return "111101101101111";case 'P':return "110101110100100";
    case 'Q':return "111101101111011";case 'R':return "110101110101101";
    case 'S':return "011100010001110";case 'T':return "111010010010010";
    case 'U':return "101101101101111";case 'V':return "101101010101010";
    case 'W':return "101101111111101";case 'X':return "101101010101101";
    case 'Y':return "101101010010010";case 'Z':return "111001010010010";
    case '0':return "111101101101111";case '1':return "010110010010111";
    case '2':return "110001111100111";case '3':return "110001110001110";
    case '4':return "101101111001001";case '5':return "111100110001110";
    case '6':return "011100111101111";case '7':return "111001010010010";
    case '8':return "111101111101111";case '9':return "111101111001110";
    case '-':return "000000111000000";case '+':return "000010111010000";
    case ':':return "000010000010000";case '/':return "001001010100100";
    case '!':return "010010010000010";default:return "000000000000000";
    }
}
void Text(Module* m,const char* message,float x,float y,float sz,float r,float g,float b){
    if(!message)return;
    size_t n=std::strlen(message);float left=x-static_cast<float>(n)*sz*2.f;
    for(size_t k=0;k<n;++k){
        char c=message[k];if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');
        const char* bits=Glyph(c);
        for(int i=0;i<15;++i)if(bits[i]=='1')
            Rect(m,left+(k*4.f+i%3+.5f)*sz,y+(2.5f-i/3)*sz,
                 .9f*sz,.9f*sz,-2.86f,r,g,b);
    }
}
void Frame(Module* m,float x,float y,float w,float h,bool focus){
    Rect(m,x,y,w,h,-2.96f,focus?.35f:.13f,focus?.21f:.11f,focus?.48f:.18f);
    Rect(m,x,y,w-.035f,h-.035f,-2.94f,.052f,.060f,.115f);
}
void Background(Module* m){
    Rect(m,0,0,5.0f,3.48f,-3.10f,.018f,.021f,.055f);
    for(uint32_t i=0;i<110;++i){
        uint32_t t=reverie::gumball::Mix(i*19u+0x123BCu);
        float x=(t%1000u)*.00486f-2.43f,y=((t>>10)%1000u)*.0033f-1.65f;
        if(m->sim.phase()==Phase::Flight){
            const float depth=.08f+.025f*static_cast<float>((t>>20u)&3u);
            y=std::fmod(y+1.65f+m->sim.elapsed()*depth,3.30f)-1.65f;
            x-=m->sim.player_x()*(.015f+.01f*static_cast<float>((t>>24u)&3u));
        }
        const float size=(t&3u)==0u?.015f:.008f;
        Rect(m,x,y,size,size,-3.01f,.30f,.24f,.44f);
    }
}
void Chart(Module* m){
    char text[96];Background(m);
    Text(m,"GUMBALL ARMADA",0,1.38f,.079f,1.f,.81f,.94f);
    Text(m,"THE CANDY KING IS MISSING",0,1.07f,.044f,.89f,.89f,.98f);
    const char* generals[]={"SALLY SCRUNCH","CAPTAIN CRINGE","PETE PUCKER","SENATOR SAUER"};
    const unsigned sector=m->sim.node()/1024u;
    std::snprintf(text,sizeof(text),"SOUR GENERAL: %s",generals[sector<4u?sector:3u]);
    Text(m,text,0,.78f,.039f,1.f,.62f,.58f);
    for(int n=0;n<3;++n){
        float x=-1.54f+1.54f*n;
        Frame(m,x,.20f,1.43f,.70f,m->selected==n);
        auto lane=m->sim.lane(static_cast<unsigned>(n));
        std::snprintf(text,sizeof(text),"LANE %c %04u",'A'+n,lane.id);
        Text(m,text,x,.33f,.029f,.95f,.95f,1.f);
        if(lane.fortress)Text(m,"SOUR FORTRESS",x,.07f,.034f,1.f,.49f,.47f);
        else{
            std::snprintf(text,sizeof(text),"RISK %u LOOT %u",lane.hazard,lane.reward);
            Text(m,text,x,.12f,.024f,.88f,.83f,.63f);
        }
        const auto rules=reverie::gumball::MissionFor(static_cast<unsigned>(n),lane.hazard);
        Text(m,rules.kind==reverie::gumball::MissionKind::Sweep?"SWEEP":
            (rules.kind==reverie::gumball::MissionKind::Intercept?"INTERCEPT":"RECOVERY"),
            x,-.065f,.033f,.71f,.97f,.90f);
    }
    const char* upgrades[]={"BLASTER","HULL","THRUSTER","SHIELD"};
    for(int i=0;i<4;++i){
        float x=-1.82f+1.21f*i;
        Frame(m,x,-.53f,1.12f,.66f,m->selected==i+3);
        Text(m,upgrades[i],x,-.37f,.032f,.92f,.88f,1.f);
        std::snprintf(text,sizeof(text),"LV %d %dG",m->sim.level(static_cast<Upgrade>(i)),
                      m->sim.price(static_cast<Upgrade>(i)));
        Text(m,text,x,-.66f,.026f,1.f,.81f,.56f);
    }
    const char* weapons[]={"PEA REPEATER","ROCK CANNON","MINT SPREAD"};
    for(int i=0;i<3;++i){
        const float x=-1.54f+1.54f*i;
        const Weapon kind=static_cast<Weapon>(i);
        Frame(m,x,-1.16f,1.43f,.47f,m->selected==i+7);
        Text(m,weapons[i],x,-1.07f,.028f,.95f,.88f,.96f);
        if(m->sim.weapon()==kind)Text(m,"EQUIPPED",x,-1.25f,.030f,.45f,1.f,.65f);
        else if(m->sim.owns_weapon(kind))Text(m,"CLICK EQUIP",x,-1.25f,.030f,.80f,.89f,.95f);
        else{
            std::snprintf(text,sizeof(text),"BUY %dG",m->sim.weapon_price(kind));
            Text(m,text,x,-1.25f,.030f,1.f,.77f,.43f);
        }
    }
    std::snprintf(text,sizeof(text),"GUMBALLS %u  RANK %d  GENERALS %u/4",
                  m->sim.cash(),m->sim.command_rank(),
                  static_cast<unsigned>((m->sim.bosses()&1u)+((m->sim.bosses()>>1)&1u)
                    +((m->sim.bosses()>>2)&1u)+((m->sim.bosses()>>3)&1u)));
    Text(m,text,0,-1.51f,.035f,1.f,.86f,.60f);
    Text(m,"PAD MOVE  CLICK ACTIVATE",0,-1.66f,.030f,.72f,.87f,.93f);
}
void EnemyVisual(Module* m,const Enemy& e){
    const float x=e.x,y=e.y;
    switch(e.kind){
    case EnemyKind::Scout:
        Disc(m,x,y,.16f,-2.90f,.81f,.29f,.37f);
        Disc(m,x,y,.085f,-2.88f,1.f,.72f,.80f);
        Tri(m,x,y+.24f,x-.07f,y+.10f,x+.07f,y+.10f,-2.86f,.97f,.86f,.50f);
        break;
    case EnemyKind::Interceptor:
        Tri(m,x,y-.18f,x-.24f,y+.09f,x,y+.25f,-2.90f,.34f,.87f,.26f);
        Tri(m,x,y-.18f,x,y+.25f,x+.24f,y+.09f,-2.90f,.68f,.91f,.27f);
        Rect(m,x,y+.02f,.11f,.14f,-2.86f,.96f,.98f,.60f);
        break;
    case EnemyKind::Bomber:
        Rect(m,x,y,.25f,.29f,-2.90f,.72f,.64f,.18f);
        Disc(m,x-.18f,y-.04f,.105f,-2.87f,.96f,.80f,.23f);
        Disc(m,x+.18f,y-.04f,.105f,-2.87f,.96f,.80f,.23f);
        Rect(m,x,y+.12f,.12f,.09f,-2.86f,.96f,.94f,.59f);
        break;
    case EnemyKind::Ace:
        Tri(m,x,y+.24f,x-.29f,y-.19f,x+.29f,y-.19f,-2.91f,.83f,.26f,.69f);
        Tri(m,x,y+.20f,x-.16f,y-.12f,x+.16f,y-.12f,-2.88f,.96f,.74f,.24f);
        Disc(m,x,y,.078f,-2.86f,.99f,.90f,.63f);
        break;
    case EnemyKind::Courier:
        // Long, fast gold cargo pod: a visible priority target to intercept.
        Rect(m,x,y,.17f,.38f,-2.90f,.95f,.62f,.12f);
        Tri(m,x,y+.28f,x-.13f,y+.09f,x+.13f,y+.09f,
            -2.88f,1.f,.89f,.31f);
        Disc(m,x-.13f,y-.12f,.085f,-2.87f,.38f,.94f,.84f);
        Disc(m,x+.13f,y-.12f,.085f,-2.87f,.38f,.94f,.84f);
        break;
    case EnemyKind::Fortress:
        Rect(m,x,y,.52f,.34f,-2.92f,.56f,.15f,.19f);
        Disc(m,x-.28f,y-.10f,.13f,-2.89f,.81f,.18f,.23f);
        Disc(m,x+.28f,y-.10f,.13f,-2.89f,.81f,.18f,.23f);
        Rect(m,x,y+.12f,.27f,.18f,-2.87f,.92f,.58f,.27f);
        Disc(m,x,y+.10f,.09f,-2.85f,.98f,.92f,.55f);
        break;
    }
}
void Flight(Module* m){
    Background(m);
    const auto& s=m->sim;
    for(const auto& e:s.enemies())if(e.active)EnemyVisual(m,e);
    for(const auto& drop:s.salvage().items())if(drop.active){
        // Recovered gumdrop cash requires steering into the flying pickup.
        Disc(m,drop.x,drop.y,.092f,-2.81f,.92f,.81f,.21f);
        Rect(m,drop.x,drop.y,.063f,.063f,-2.79f,.98f,.97f,.66f);
    }
    for(const auto& shot:s.shots())if(shot.active){
        const float radius=shot.hostile?.039f:(shot.damage>=4?.085f:.037f);
        Rect(m,shot.x,shot.y,radius,shot.hostile?.12f:(shot.damage>=4?.24f:.18f),-2.82f,
             shot.hostile?1.f:(shot.damage>=4?1.f:.33f),
             shot.hostile?.26f:(shot.damage>=4?.68f:.92f),shot.hostile?.20f:1.f);
    }
    if(s.hurt_flash()>0.f){
        Rect(m,-2.25f,0,.13f,2.7f,-2.78f,1.f,.18f,.18f);
        Rect(m, 2.25f,0,.13f,2.7f,-2.78f,1.f,.18f,.18f);
    }
    float x=s.player_x(),y=s.player_y();
    Tri(m,x,y+.22f,x-.19f,y-.17f,x+.19f,y-.17f,-2.85f,.92f,.66f,.94f);
    Disc(m,x,y,.075f,-2.83f,.29f,.91f,.96f);
    float wing=.21f+.016f*s.level(Upgrade::Hull);
    Tri(m,x-.08f,y,x-wing,y-.22f,x-.10f,y-.12f,-2.84f,.21f,.81f,.71f);
    Tri(m,x+.08f,y,x+wing,y-.22f,x+.10f,y-.12f,-2.84f,.21f,.81f,.71f);
    for(int i=0;i<1+s.level(Upgrade::Blaster)/4;++i)
        Rect(m,x+(i-1)*.09f,y+.17f,.03f,.16f,-2.82f,1.f,.87f,.33f);
    char text[96];
    Rect(m,0,1.48f,5.f,.35f,-2.92f,.056f,.060f,.12f);
    std::snprintf(text,sizeof(text),"HULL %d  SHIELD %d  GUMBALLS %u",s.hp(),s.shield(),s.cash());
    Text(m,text,0,1.42f,.044f,.91f,.97f,.87f);
    const char* guns[]={"PEA REPEATER","ROCK CANNON","MINT SPREAD"};
    Text(m,guns[static_cast<unsigned>(s.weapon())],0,1.18f,.034f,.87f,.88f,.57f);
    if(s.combo()>=3){
        Text(m,s.combo()>=10?"SOUR ANNIHILATION!":(s.combo()>=6?"SPECTACULAR!":"SUGAR RUSH!"),
             0,.93f,.073f,1.f,.84f,.28f);
        std::snprintf(text,sizeof(text),"COMBO X%d",s.combo());
        Text(m,text,0,.69f,.042f,1.f,.93f,.55f);
    }
    char status[96];
    const auto rules=s.mission();
    if(rules.kind==reverie::gumball::MissionKind::Recovery)
        std::snprintf(status,sizeof(status),"SALVAGE RECOVERY %d/%d",
                      s.salvage_collected(),rules.required_salvage);
    else if(rules.kind==reverie::gumball::MissionKind::Intercept)
        std::snprintf(status,sizeof(status),"COURIER INTERCEPT  %d SEC",
                      static_cast<int>(s.remaining_time()));
    else std::snprintf(status,sizeof(status),"SOUR SWEEP");
    Text(m,status,0,-1.055f,.032f,.84f,.96f,.74f);
    std::snprintf(status,sizeof(status),"WAVE %d/3  CLEARED %d/%d  ESCAPED %d/3",
                  s.wave(),s.defeated(),s.target(),s.escaped());
    Text(m,status,0,-1.24f,.035f,.88f,.90f,.95f);
    std::snprintf(status,sizeof(status),"RUN CASH +%u  BEST COMBO X%d",
                  s.sortie_cash(),s.max_combo());
    Text(m,status,0,-1.43f,.031f,.86f,.80f,.58f);
}
void Result(Module* m){
    Background(m);auto phase=m->sim.phase();
    const char* headline="SHIP DISABLED";
    if(phase==Phase::ArmadaDefeated)headline="SOUR ARMADA DEFEATED";
    else if(phase==Phase::Won)headline="SWEET VICTORY!";
    else {
        switch(m->sim.failure()){
        case reverie::gumball::FailureReason::Deadline:headline="COURIER GOT AWAY";break;
        case reverie::gumball::FailureReason::SalvageLost:headline="SALVAGE LOST";break;
        case reverie::gumball::FailureReason::EliteEscaped:headline="SOUR ACE ESCAPED";break;
        case reverie::gumball::FailureReason::Escapes:headline="CONVOY OVERRUN";break;
        case reverie::gumball::FailureReason::HullDestroyed:headline="SHIP DISABLED";break;
        default:break;
        }
    }
    Text(m,headline,0,.81f,.061f,1.f,.78f,.35f);
    char text[80];
    std::snprintf(text,sizeof(text),"SORTIE CASH +%u  CLEAR BONUS +%u",
                  m->sim.sortie_cash(),m->sim.completion_bonus());
    Text(m,text,0,.33f,.041f,.98f,.91f,.62f);
    std::snprintf(text,sizeof(text),"DESTROYED %d/%d  ESCAPED %d  BEST X%d",
                  m->sim.defeated()-m->sim.escaped(),m->sim.target(),
                  m->sim.escaped(),m->sim.max_combo());
    Text(m,text,0,-.04f,.039f,.93f,.91f,.94f);
    std::snprintf(text,sizeof(text),"TOTAL %u G  RANK %d  TIME %dS",
                  m->sim.cash(),m->sim.command_rank(),static_cast<int>(m->sim.elapsed()));
    Text(m,text,0,-.43f,.041f,.93f,.73f,.92f);
    std::snprintf(text,sizeof(text),"SALVAGE %d  LOST %d",
                  m->sim.salvage().collected(),m->sim.salvage().expired());
    Text(m,text,0,-.70f,.035f,.76f,.93f,.77f);
    Text(m,phase==Phase::ArmadaDefeated?"THE KING IS STILL MISSING":
         "CLICK TO RETURN TO CHART",0,-1.f,.044f,.74f,.94f,.88f);
}
void Rebuild(Module* m){
    m->mesh.clear();
    if(m->sim.phase()==Phase::Chart)Chart(m);
    else if(m->sim.phase()==Phase::Flight)Flight(m);
    else Result(m);
    if(m->saveError)Text(m,"WARNING: PROGRESS SAVE FAILED",0,-1.62f,.031f,1.f,.35f,.28f);
    m->upload=true;
}
void Save(Module* m){
    if(!ReverieNativeHostSupportsSaveV1(m->host))return;
    const Snapshot s=m->sim.snapshot();
    const int code=m->host->write_save("campaign.v1",&s,static_cast<uint32_t>(sizeof(s)));
    if(code!=REVERIE_NATIVE_SAVE_OK){
        if(!m->saveError)ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,kId,"Campaign save failed");
        m->saveError=true;
    }else m->saveError=false;
}
void* Create(const ReverieNativeHostV1* host){
    if(!ReverieNativeHostSupportsLogV1(host))return nullptr;
    Module* m=new(std::nothrow)Module();if(!m)return nullptr;
    m->host=host;m->mesh.reserve(20000);
    if(ReverieNativeHostSupportsSaveV1(host)){
        Snapshot current{};uint32_t count=0;
        const int result=host->read_save("campaign.v1",&current,sizeof(current),&count);
        if(result==REVERIE_NATIVE_SAVE_OK){
            bool valid=false;
            if(count==sizeof(current))valid=m->sim.restore(current);
            else if(count==sizeof(SnapshotV1)){
                SnapshotV1 legacy{};
                std::memcpy(&legacy,&current,sizeof(legacy));
                valid=m->sim.restore(legacy);
                if(valid)Save(m); // one checked, atomically replaced save
            }
            if(!valid)ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_WARN,kId,
                                        "Invalid campaign save ignored");
        }else if(result!=REVERIE_NATIVE_SAVE_NOT_FOUND)
            ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_ERROR,kId,"Campaign save read failed");
    }
    Rebuild(m);return m;
}
void Destroy(void* p){delete static_cast<Module*>(p);}
int32_t GlStart(void* p){
    auto* m=static_cast<Module*>(p);if(!m)return 0;
    ReverieNativeGlStateV1 state{};ReverieNativeGlStateCaptureV1(&state);
    const char* vs="uniform mat4 u_Mvp;\nattribute vec3 a_Pos;\nattribute vec3 a_Color;\nvarying vec3 v_Color;\nvoid main(){v_Color=a_Color;gl_Position=u_Mvp*vec4(a_Pos,1.0);}\n";
    const char* fs="precision mediump float;\nvarying vec3 v_Color;\nvoid main(){gl_FragColor=vec4(v_Color,1.0);}\n";
    const ReverieNativeGlAttributeBindingV1 attrs[]={{0u,"a_Pos"},{1u,"a_Color"}};
    char diag[256]={};
    m->program=ReverieNativeBuildProgramDiagnosticV1(vs,fs,attrs,2,diag,sizeof(diag));
    if(!m->program){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,kId,diag);
        ReverieNativeGlStateRestoreV1(&state);return 0;
    }
    m->pos=glGetAttribLocation(m->program,"a_Pos");
    m->col=glGetAttribLocation(m->program,"a_Color");
    m->mvp=glGetUniformLocation(m->program,"u_Mvp");
    glGenBuffers(1,&m->vbo);
    bool good=m->vbo!=0&&m->pos>=0&&m->col>=0&&m->mvp>=0;
    if(!good){
        if(m->vbo)glDeleteBuffers(1,&m->vbo);
        glDeleteProgram(m->program);m->vbo=0;m->program=0;
    }
    ReverieNativeGlStateRestoreV1(&state);m->upload=true;
    return good?1:0;
}
void GlEnd(void* p){
    auto* m=static_cast<Module*>(p);if(!m)return;
    if(m->vbo)glDeleteBuffers(1,&m->vbo);
    if(m->program)glDeleteProgram(m->program);
    m->vbo=0;m->program=0;m->upload=true;
}
void Resume(void*){}void Pause(void*){}
void Update(void* p,const ReverieNativeInputV1* in){
    auto* m=static_cast<Module*>(p);
    if(!m||!ReverieNativeInputHasBaseV1(in))return;
    const float dt=std::max(0.f,std::min(.05f,std::isfinite(in->delta_seconds)?in->delta_seconds:0.f));
    const bool pressed=in->primary_down!=0;
    const Phase phase=m->sim.phase();
    if(phase==Phase::Flight){
        const uint32_t kills=m->sim.kills();const int hp=m->sim.hp();
        const int shieldBefore=m->sim.shield();
        const uint32_t fired=m->sim.shots_fired();
        m->sim.update(dt,in->move_x,in->move_y,pressed);
        if(kills!=m->sim.kills())ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        if(hp>m->sim.hp()||shieldBefore>m->sim.shield())ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
        if(m->sim.shots_fired()>fired)
            ReverieNativeRequestSoundV1(m->host,m->sim.weapon()==Weapon::RockCannon?
                REVERIE_NATIVE_SOUND_WEAPON_HEAVY:REVERIE_NATIVE_SOUND_WEAPON_MACHINE);
        if(m->sim.phase()!=Phase::Flight)Save(m);
        Rebuild(m);
    }else if(phase==Phase::Chart){
        m->navTime-=dt;
        const float x=in->move_x,y=in->move_y;
        if(m->navTime<=0.f&&(std::fabs(x)>.6f||std::fabs(y)>.6f)){
            if(std::fabs(y)>.6f){
                if(y>0&&m->selected<3)m->selected=3;
                else if(y>0&&m->selected<7)m->selected=7;
                else if(y<0&&m->selected>=7)m->selected=3;
                else if(y<0&&m->selected>=3)m->selected=0;
            }else if(m->selected<3)m->selected=(m->selected+(x>0?1:2))%3;
            else if(m->selected<7)m->selected=3+(m->selected-3+(x>0?1:3))%4;
            else m->selected=7+(m->selected-7+(x>0?1:2))%3;
            m->navTime=.20f;
            ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
            Rebuild(m);
        }
        if(pressed&&!m->primary){
            bool good=false;
            if(m->selected<3)good=m->sim.launch(static_cast<unsigned>(m->selected));
            else if(m->selected<7){
                good=m->sim.buy(static_cast<Upgrade>(m->selected-3));
                if(good)Save(m);
            }else{
                const Weapon requested=static_cast<Weapon>(m->selected-7);
                good=m->sim.owns_weapon(requested)?
                    m->sim.select_weapon(requested):m->sim.buy_weapon(requested);
                if(good)Save(m);
            }
            ReverieNativeRequestFeedbackV1(m->host,good?REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
            Rebuild(m);
        }
    }else if(pressed&&!m->primary&&m->sim.recover()){
        Save(m);Rebuild(m);
        ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
    }
    m->primary=pressed;
}
int32_t Render(void* p,const ReverieNativeEyeV1* eye){
    auto* m=static_cast<Module*>(p);
    if(!m||!ReverieNativeEyeRenderableV1(eye)||!m->program||!m->vbo)return 0;
    ReverieNativeGlStateV1 state{};
    ReverieNativeGlAttribStateV1 a{},b{};
    ReverieNativeGlStateCaptureV1(&state);
    ReverieNativeGlAttribCaptureV1(&a,static_cast<GLuint>(m->pos));
    ReverieNativeGlAttribCaptureV1(&b,static_cast<GLuint>(m->col));
    float mvp[16];ReverieNativeMat4Multiply(mvp,eye->projection,eye->view);
    glDisable(GL_CULL_FACE);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
    glUseProgram(m->program);glBindBuffer(GL_ARRAY_BUFFER,m->vbo);
    if(m->upload){
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(m->mesh.size()*sizeof(float)),
                     m->mesh.data(),GL_DYNAMIC_DRAW);
        m->upload=false;
    }
    glEnableVertexAttribArray(static_cast<GLuint>(m->pos));
    glVertexAttribPointer(static_cast<GLuint>(m->pos),3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(static_cast<GLuint>(m->col));
    glVertexAttribPointer(static_cast<GLuint>(m->col),3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<const void*>(3*sizeof(float)));
    glUniformMatrix4fv(m->mvp,1,GL_FALSE,mvp);
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(m->mesh.size()/6));
    const GLenum error=glGetError();
    ReverieNativeGlAttribRestoreV1(&b);
    ReverieNativeGlAttribRestoreV1(&a);
    ReverieNativeGlStateRestoreV1(&state);
    return error==GL_NO_ERROR?1:0;
}
const ReverieNativeModuleApiV1 api={
    sizeof(ReverieNativeModuleApiV1),REVERIE_NATIVE_MODULE_ABI_VERSION,
    {sizeof(ReverieNativeModuleDescriptorV1),REVERIE_NATIVE_MODULE_ABI_VERSION,kId,kName,2u,0u},
    Create,Destroy,GlStart,GlEnd,Resume,Pause,Update,Render,nullptr
};
}
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1* reverie_native_module_entry_v1(void){return &api;}
