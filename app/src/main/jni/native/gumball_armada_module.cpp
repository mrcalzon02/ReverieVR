#include "gumball_armada_simulation.h"
#include "gumball_shipyard_menu.h"
#include "gumball_ship_3d.h"
#include "gumball_shipyard_readiness.h"
#include "gumball_menu_hit.h"
#include "reverie_native_vector_text.h"
#include "gumball_menu_layout.h"
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
#include <array>
#include <vector>
namespace {
using reverie::gumball::Simulation;
using reverie::gumball::YardCursor;
using reverie::gumball::MenuHit;
using reverie::gumball::ChartNavigate;
using reverie::gumball::LoadoutNavigate;
using reverie::gumball::Phase;
using reverie::gumball::Upgrade;
using reverie::gumball::Enemy;
using reverie::gumball::EnemyKind;
using reverie::gumball::CastGuest;
using reverie::gumball::Snapshot;
using reverie::gumball::SnapshotV1;
using reverie::gumball::SnapshotV2;
using reverie::gumball::SupportKind;
using reverie::gumball::SupportSkill;
using reverie::gumball::SupportKit;
using reverie::gumball::Weapon;
using reverie::gumball::Shipyard;
using reverie::gumball::ShipyardSave;
using reverie::gumball::ShipAssembly;
using reverie::gumball::Ship3DSave;
using reverie::gumball::Part;
const char kId[]="gumball-armada";
const char kName[]="Gumball Armada: Candy King Search [DEV]";
constexpr unsigned kRouteArtworkCount=8u;
constexpr unsigned kArtworkCount=kRouteArtworkCount+2u;
constexpr uint32_t kRouteArtworkBytes=8u+96u*86u*2u;
const char* const kRouteArtworkNames[kArtworkCount]={
    "gummy_world","chocolate_world","ice_candy_world","citrus_world",
    "gas_giant","ring_world","outpost_station","crystal_field",
    "pip_parfait","lolly_wren"
};
struct Module {
    const ReverieNativeHostV1* host=nullptr;
    Simulation sim;
    ShipAssembly ship;
    int shipLayer=2;
    bool previewShip=false;
    float previewYaw=.5f,previewZoom=.20f;
    int selected=0; // routes, upgrades, weapons, shipyard
    bool inYard=false;
    bool inSupport=false;
    bool inLoadout=false;
    int supportSelected=0,loadoutSelected=0;
    YardCursor yardCursor{};
    bool secondary=false;
    bool flightPointer=false;
    float flightTargetX=0.f,flightTargetY=0.f;
    reverie::gumball::MenuPointerFocus pointerFocus{};
    char yardFeedback[84]="SELECT PART ON RIGHT / CLICK SHIP TO EDIT";
    Part brush=Part::Frame;
    bool primary=false,campaignSaveError=false,shipSaveError=false,shipLoadWarning=false,upload=true;
    float navTime=0;
    GLuint program=0,vbo=0;GLint pos=-1,col=-1,mvp=-1;
    GLuint routeProgram=0,routeVbo=0,routeTextures[kArtworkCount]{};
    GLint routeCorner=-1,routeMvp=-1,routeRect=-1,routeSampler=-1;
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
// Reuse the platform native vector font, not unreadable 3x5 pixel squares.
void Text(Module* m,const char* message,float x,float y,float sz,
          float r,float g,float b){
    if(message&&sz>0.f)
        reverie::nativeart::StrokeText(m->mesh,message,x,y,sz,r,g,b,-2.85f);
}
// Width remains four stroke-font units per character; prevent label overflow.
void TextFit(Module* m,const char* message,float x,float y,float maxWidth,
             float size,float r,float g,float b){
    if(!message||maxWidth<=0.f)return;
    const size_t count=std::strlen(message);
    if(!count)return;
    Text(m,message,x,y,
         std::min(size,maxWidth/(4.f*static_cast<float>(count))),r,g,b);
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
    Background(m);
    char text[112];
    Text(m,"GUMBALL ARMADA",0,1.42f,.066f,1.f,.82f,.93f);
    Text(m,"THE SEARCH FOR THE CANDY KING",0,1.15f,.032f,.84f,.89f,1.f);
    const char* generals[]={"SALLY SCRUNCH","CAPTAIN CRINGE","PETE PUCKER","SENATOR SAUER"};
    const unsigned sector=m->sim.node()/1024u;
    std::snprintf(text,sizeof(text),"SECTOR %u  /  %s",sector+1u,
                  generals[sector<4u?sector:3u]);
    TextFit(m,text,0,.94f,3.70f,.034f,1.f,.63f,.55f);

    const char* names[]={"SOUR SWEEP","INTERCEPT","RECOVERY"};
    for(int n=0;n<3;++n){
        const float x=reverie::gumball::kChartRouteX[n];
        Frame(m,x,.22f,1.44f,1.09f,m->selected==n);
        const auto lane=m->sim.lane(static_cast<unsigned>(n));
        std::snprintf(text,sizeof(text),"LANE %c  %04u",'A'+n,lane.id);
        TextFit(m,text,x-.17f,.57f,.94f,.034f,1.f,.88f,.95f);
        Text(m,names[n],x,.35f,.031f,.69f,1.f,.84f);
        const CastGuest rival=m->sim.route_guest(static_cast<unsigned>(n));
        TextFit(m,rival==CastGuest::None?"CLEAR OF KNOWN RIVALS":
            reverie::gumball::CastGuestName(rival),
            x,.13f,1.31f,.029f,1.f,.70f,.51f);
        if(lane.fortress)Text(m,"FORTRESS ROUTE",x,-.08f,.027f,1.f,.45f,.47f);
        else{
            std::snprintf(text,sizeof(text),"RISK %u   REWARD %uG",
                          lane.hazard,lane.reward);
            TextFit(m,text,x,-.08f,1.30f,.027f,.90f,.88f,.72f);
        }
        Text(m,m->selected==n?"CLICK TO LAUNCH":"SELECT LANE",
             x,-.23f,.026f,.61f,.94f,1.f);
    }

    const int selection[]={12,10,11};
    const char* labels[]={"LOADOUT","SHIPYARD","ALLIES"};
    for(int i=0;i<3;++i){
        const float x=reverie::gumball::kChartActionX[i];
        Frame(m,x,-.94f,1.43f,.55f,m->selected==selection[i]);
        Text(m,labels[i],x,-.94f,.043f,.92f,.96f,.82f);
    }
    std::snprintf(text,sizeof(text),"G %u  /  RANK %d  /  GENERALS %u OF 4",
                  m->sim.cash(),m->sim.command_rank(),
                  static_cast<unsigned>((m->sim.bosses()&1u)
                    +((m->sim.bosses()>>1u)&1u)
                    +((m->sim.bosses()>>2u)&1u)
                    +((m->sim.bosses()>>3u)&1u)));
    TextFit(m,text,0,-1.39f,4.2f,.034f,1.f,.88f,.64f);
    Text(m,m->ship.ready()?"PAD SELECT   CLICK TO OPEN / FLY":
         "SHIP NOT READY  /  OPEN SHIPYARD",
         0,-1.64f,.027f,m->ship.ready()?.73f:1.f,
         m->ship.ready()?.86f:.43f,.89f);
}
void LoadoutView(Module* m){
    Background(m);
    char text[96];
    Text(m,"SHIP LOADOUT",0,1.43f,.067f,1.f,.83f,.94f);
    std::snprintf(text,sizeof(text),"BALANCE %uG  /  MINUS: BACK",m->sim.cash());
    TextFit(m,text,0,1.12f,4.20f,.038f,.80f,.98f,.89f);
    Text(m,"SHIP SYSTEMS",0,.93f,.033f,1.f,.76f,.56f);
    const char* upgrades[]={"BLASTER","HULL","THRUSTER","SHIELD"};
    for(int i=0;i<4;++i){
        const float x=reverie::gumball::kLoadoutUpgradeX[i];
        Frame(m,x,.53f,1.12f,.65f,m->loadoutSelected==i);
        TextFit(m,upgrades[i],x,.64f,1.02f,.030f,.94f,.88f,1.f);
        std::snprintf(text,sizeof(text),"LV %d / %dG",
                      m->sim.level(static_cast<Upgrade>(i)),
                      m->sim.price(static_cast<Upgrade>(i)));
        TextFit(m,text,x,.39f,1.02f,.030f,1.f,.80f,.57f);
    }
    Text(m,"WEAPONS",0,.07f,.033f,1.f,.76f,.56f);
    const char* weapons[]={"PEA REPEATER","ROCK CANNON","MINT SPREAD"};
    for(int i=0;i<3;++i){
        const float x=reverie::gumball::kLoadoutWeaponX[i];
        const Weapon kind=static_cast<Weapon>(i);
        Frame(m,x,-.38f,1.43f,.67f,m->loadoutSelected==4+i);
        TextFit(m,weapons[i],x,-.24f,1.31f,.034f,.96f,.90f,1.f);
        if(m->sim.weapon()==kind)
            Text(m,"EQUIPPED",x,-.51f,.030f,.53f,1.f,.68f);
        else if(m->sim.owns_weapon(kind))
            Text(m,"CLICK TO EQUIP",x,-.51f,.029f,.75f,.95f,1.f);
        else{
            std::snprintf(text,sizeof(text),"BUY %dG",m->sim.weapon_price(kind));
            Text(m,text,x,-.51f,.029f,1.f,.77f,.48f);
        }
    }
    Frame(m,0,-1.10f,2.30f,.44f,m->loadoutSelected==7);
    Text(m,"RETURN TO ROUTES",0,-1.10f,.034f,.83f,.98f,.90f);
    char hint[124]="PAD OR POINT TO SELECT  /  CLICK TO ACT";
    if(m->loadoutSelected<4){
        const Upgrade u=static_cast<Upgrade>(m->loadoutSelected);
        if(m->sim.level(u)>=12)std::snprintf(hint,sizeof(hint),"UPGRADE AT MAXIMUM LEVEL");
        else if(m->sim.cash()<static_cast<uint32_t>(m->sim.price(u)))
            std::snprintf(hint,sizeof(hint),"NOT ENOUGH GUMBALLS: NEED %dG",
                          m->sim.price(u));
        else std::snprintf(hint,sizeof(hint),u==Upgrade::Thruster?
            "CLICK TO BOOST CONTROLLER-STEER SHIP SPEED":
            "CLICK TO BUY / PERMANENT SHIP UPGRADE");
    }else if(m->loadoutSelected<7){
        const Weapon w=static_cast<Weapon>(m->loadoutSelected-4);
        if(m->sim.weapon()==w)std::snprintf(hint,sizeof(hint),"ALREADY EQUIPPED");
        else if(m->sim.owns_weapon(w))
            std::snprintf(hint,sizeof(hint),"CLICK TO EQUIP OWNED WEAPON");
        else if(m->sim.cash()<static_cast<uint32_t>(m->sim.weapon_price(w)))
            std::snprintf(hint,sizeof(hint),"NOT ENOUGH GUMBALLS: NEED %dG",
                          m->sim.weapon_price(w));
        else std::snprintf(hint,sizeof(hint),"CLICK TO BUY AND EQUIP WEAPON");
    }else std::snprintf(hint,sizeof(hint),"RETURN TO MISSION ROUTES");
    TextFit(m,hint,0,-1.60f,4.70f,.032f,.76f,.94f,.94f);
}
void SupportView(Module* m){
    Background(m);
    TextFit(m,"ALLIED SUPPORT CONTRACTS",0,1.44f,4.60f,.055f,.94f,.91f,.74f);
    TextFit(m,"ONE ESCORT FOR NEXT SORTIE  /  MINUS: BACK",
            0,1.16f,4.55f,.032f,.83f,.97f,.86f);
    char text[116];
    const auto& contracts=m->sim.support();
    const char* items[]={"HIRE CAPITAINE ECLAIR","HIRE MLLE MACARON",
        "ECLAIR VOLLEY","ECLAIR DURATION","MACARON RELIEF","MACARON TRACTOR",
        "HULL PATCH","SHIELD TONIC","RETURN"};
    for(int i=0;i<9;++i){
        const float y=.88f-.28f*static_cast<float>(i);
        Frame(m,0,y,4.35f,.26f,i==m->supportSelected);
        if(i<2){
            const SupportKind kind=i==0?SupportKind::CapitaineEclair:
                                          SupportKind::MademoiselleMacaron;
            std::snprintf(text,sizeof(text),"%s  %uG",items[i],contracts.hire_price(kind));
        }else if(i<6){
            const SupportSkill kind=static_cast<SupportSkill>(i-2);
            const uint32_t cost=contracts.upgrade_cost(kind);
            if(cost>0)std::snprintf(text,sizeof(text),"%s  LV %u/3  %uG",
                                    items[i],contracts.level(kind),cost);
            else std::snprintf(text,sizeof(text),"%s  LV 3/3  MAX",items[i]);
        }else if(i<8){
            const SupportKit kind=static_cast<SupportKit>(i-6);
            if(contracts.has_kit(kind))
                std::snprintf(text,sizeof(text),"%s  READY",items[i]);
            else std::snprintf(text,sizeof(text),"%s  %uG",
                               items[i],contracts.kit_price(kind));
        }else std::snprintf(text,sizeof(text),"RETURN TO CHART");
        Text(m,text,0,y,.028f,.95f,.94f,.86f);
    }
    std::snprintf(text,sizeof(text),"BOOKED: %s    CASH %uG",
                  SupportName(contracts.booked()),m->sim.cash());
    TextFit(m,text,0,-1.52f,4.60f,.031f,.86f,1.f,.79f);
    char hint[140]="";
    if(m->supportSelected<2){
        const SupportKind kind=m->supportSelected==0?
            SupportKind::CapitaineEclair:SupportKind::MademoiselleMacaron;
        if(contracts.booked()!=SupportKind::None)
            std::snprintf(hint,sizeof(hint),"AN ESCORT IS ALREADY BOOKED");
        else if(m->sim.cash()<contracts.hire_price(kind))
            std::snprintf(hint,sizeof(hint),"NEED %uG TO HIRE",
                          contracts.hire_price(kind));
        else std::snprintf(hint,sizeof(hint),m->supportSelected==0?
            "TEMPORARY FIGHTER: REAL SUPPORT FIRE":
            "TEMPORARY RESCUE: HULL / SHIELD / SALVAGE");
    }else if(m->supportSelected<6){
        const SupportSkill skill=static_cast<SupportSkill>(m->supportSelected-2);
        const uint32_t cost=contracts.upgrade_cost(skill);
        if(cost==0u)std::snprintf(hint,sizeof(hint),"ABILITY AT MAX LEVEL");
        else if(m->sim.cash()<cost)
            std::snprintf(hint,sizeof(hint),"NEED %uG FOR UPGRADE",cost);
        else{
            const char* abilities[]={"INCREASE ESCORT SHOT DAMAGE",
                "INCREASE ESCORT SHOT COUNT","INCREASE HULL / SHIELD REPAIR",
                "INCREASE SALVAGE TRACTOR REACH"};
            std::snprintf(hint,sizeof(hint),"%s",abilities[m->supportSelected-2]);
        }
    }else if(m->supportSelected<8){
        const SupportKit kit=static_cast<SupportKit>(m->supportSelected-6);
        if(contracts.has_kit(kit))
            std::snprintf(hint,sizeof(hint),"SUPPLY ALREADY HELD");
        else if(m->sim.cash()<contracts.kit_price(kit))
            std::snprintf(hint,sizeof(hint),"NEED %uG FOR SUPPLY",
                          contracts.kit_price(kit));
        else std::snprintf(hint,sizeof(hint),m->supportSelected==6?
            "AUTO PATCH HULL AT HALF HEALTH":
            "AUTO RESTORE SHIELDS AT HALF CAPACITY");
    }else std::snprintf(hint,sizeof(hint),"RETURN TO MISSION ROUTES");
    TextFit(m,hint,0,-1.67f,4.75f,.029f,.82f,.94f,.92f);
}
const char* YardPartName(Part part){
    const char* names[]={"EMPTY","CORE","FRAME","ARMOR","WING","THRUSTER",
        "REACTOR","BATTERY","SHIELD","GUN MOUNT","CARGO"};
    const unsigned i=static_cast<unsigned>(part);
    return i<11u?names[i]:"UNKNOWN";
}
void ShipyardView(Module* m){
    Background(m);
    char line[116];
    Text(m,"CANDY SHIPYARD",0,1.48f,.061f,.97f,.83f,.96f);
    TextFit(m,m->previewShip?
        "3D VIEW: PAD LEFT/RIGHT ROTATES / UP/DOWN ZOOMS / CLICK TO EXIT":
        "PAD MOVE / CLICK TO EDIT / MINUS SWITCHES GRID AND PARTS",
            0,1.20f,4.64f,.028f,.76f,.94f,.93f);
    Text(m,"YOUR SHIP", -1.47f,1.00f,.032f,.83f,.96f,1.f);
    Text(m,"PARTS",.89f,1.00f,.032f,.83f,.96f,1.f);
    // The full 7x7 board now fits inside the 5-unit canvas. Previous x=-2.59
    // clipped the entire leftmost editing column outside the view.
    Rect(m,-1.47f,.075f,2.06f,2.02f,-2.96f,.045f,.065f,.105f);
    const char* marks[]={" ","C","F","A","W","T","R","B","S","G","H"};
    if(!m->previewShip)
    for(int y=0;y<7;++y)for(int x=0;x<7;++x){
        const float gx=-1.47f+(x-3)*.28f;
        const float gy=.075f+(3-y)*.28f;
        const bool focused=!m->yardCursor.palette()
            &&m->yardCursor.x==x&&m->yardCursor.y==y;
        Frame(m,gx,gy,.270f,.270f,focused);
        const Part part=m->ship.at(x,y,m->shipLayer);
        if(part!=Part::Empty){
            const unsigned id=static_cast<unsigned>(part);
            static const float colors[11][3]={
                {0,0,0},{.96f,.72f,.94f},{.40f,.24f,.26f},
                {.75f,.55f,.24f},{.70f,.87f,.48f},{.35f,.95f,.87f},
                {.96f,.66f,.25f},{.98f,.92f,.47f},{.27f,.77f,1.f},
                {1.f,.32f,.38f},{.58f,.79f,.98f}};
            Rect(m,gx,gy,.195f,.195f,-2.88f,
                 colors[id][0],colors[id][1],colors[id][2]);
            Text(m,marks[id],gx,gy,.038f,.08f,.08f,.13f);
        }
    }
    if(m->previewShip){
        reverie::gumball::AppendAssemblyMesh(
            m->mesh,m->ship,-1.47f,.075f,-2.34f,
            m->previewZoom,m->previewYaw);
    }
    for(int i=0;i<14;++i){
        const int row=i/2,col=i%2;
        const float x=.23f+col*1.34f;
        const float y=.79f-row*.270f;
        const Part p=i<9?static_cast<Part>(i+2):Part::Empty;
        const bool isActive=i<9&&m->brush==p;
        const bool focused=m->yardCursor.palette()
            &&m->yardCursor.x==7+col&&m->yardCursor.y==row;
        Frame(m,x,y,1.20f,.255f,focused);
        if(isActive){
            Rect(m,x-.55f,y,.045f,.20f,-2.85f,.31f,1.f,.63f);
        }
        if(i<9)
            std::snprintf(line,sizeof(line),"%s  %u",YardPartName(p),
                          m->ship.stock(p));
        else {
            const char* commands[]={"RETURN","SLICE -","SLICE +",
                                     "3D PREVIEW","ROTATE 45"};
            std::snprintf(line,sizeof(line),"%s",commands[i-9]);
        }
        TextFit(m,line,x+.020f,y,1.12f,.031f,
                .96f,
                isActive?1.f:.91f,isActive?.70f:.85f);
    }
    const auto profile=m->ship.profile();
    std::snprintf(line,sizeof(line),"SLICE %d/5  /  %s  /  STOCK %u",
                  m->shipLayer+1,YardPartName(m->brush),m->ship.stock(m->brush));
    TextFit(m,line,0,-1.10f,4.65f,.036f,1.f,.92f,.72f);
    TextFit(m,m->yardFeedback,0,-1.32f,4.65f,.032f,.84f,1.f,.87f);
    std::snprintf(line,sizeof(line),"MASS %d  /  POWER %d:%d  /  THRUSTERS %d",
                  profile.mass,profile.demand,profile.power,
                  m->ship.count(Part::Thruster));
    TextFit(m,line,0,-1.51f,4.72f,.029f,.77f,.92f,.99f);
    const bool flightReady=m->ship.ready();
    TextFit(m,reverie::gumball::FlightReadiness(m->ship),
            0,-1.68f,4.75f,.027f,
            flightReady?.65f:1.f,flightReady?1.f:.56f,flightReady?.77f:.48f);
}
void Flight(Module* m){
    Background(m);
    const auto& s=m->sim;
    // Live Sour vessels are assembled from exactly the player's voxel parts.
    // Simulation continues governing enemy tactics and bullet collisions.
    for(const auto& e:s.enemies())if(e.active){
        const unsigned kind=static_cast<unsigned>(e.kind);
        const auto enemy=ShipAssembly::enemy(kind,
            s.node()+static_cast<unsigned>(e.formation_slot));
        const float depth=-3.45f-std::max(0.f,e.y+1.3f)*.92f;
        reverie::gumball::AppendAssemblyMesh(
            m->mesh,enemy,e.x,e.y*.75f+.18f,depth,
            kind>=4u?.13f:.10f,3.14159265f,.92f);
    }
    for(const auto& drop:s.salvage().items())if(drop.active){
        // Recovered gumdrop cash requires steering into the flying pickup.
        Disc(m,drop.x,drop.y,.092f,-2.81f,.92f,.81f,.21f);
        Rect(m,drop.x,drop.y,.063f,.063f,-2.79f,.98f,.97f,.66f);
    }
    for(const auto& shot:s.shots())if(shot.active){
        if(shot.zone&&shot.warning>0.f){
            // Three fixed horizontal warning cells, safe until countdown ends.
            Rect(m,shot.x,shot.y,.44f,.09f,-2.81f,.99f,.34f,.20f);
            Rect(m,shot.x,shot.y,.07f,.26f,-2.79f,1.f,.93f,.46f);
        }else if(shot.zone){
            Rect(m,shot.x,shot.y,.26f,.24f,-2.80f,1.f,.57f,.25f);
        }else if(shot.mine){
            // Distinct fixed-travel warning symbol, no relying on green hue.
            Disc(m,shot.x,shot.y,.13f,-2.82f,.24f,.89f,.48f);
            Rect(m,shot.x,shot.y,.16f,.038f,-2.79f,.98f,.98f,.75f);
            Rect(m,shot.x,shot.y,.038f,.16f,-2.79f,.98f,.98f,.75f);
        }else if(shot.bomb){
            // Larger, visibly distinct slow hazards, shootable in real combat.
            Disc(m,shot.x,shot.y,.115f,-2.82f,.88f,.22f,.12f);
            Disc(m,shot.x,shot.y,.054f,-2.80f,1.f,.91f,.23f);
        }else if(shot.ally){
            Rect(m,shot.x,shot.y,.048f,.18f,-2.81f,.45f,1.f,.56f);
        }else{
            const float radius=shot.hostile?.039f:(shot.damage>=4?.085f:.037f);
            Rect(m,shot.x,shot.y,radius,shot.hostile?.12f:(shot.damage>=4?.24f:.18f),-2.82f,
                 shot.hostile?1.f:(shot.damage>=4?1.f:.33f),
                 shot.hostile?.26f:(shot.damage>=4?.68f:.92f),shot.hostile?.20f:1.f);
        }
    }
    if(s.guest_active()){
        const float ax=s.ally_x(),ay=-.79f;
        if(s.guest()==CastGuest::PipParfait){
            Tri(m,ax,ay+.145f,ax-.12f,ay-.12f,ax+.12f,ay-.12f,
                -2.81f,.98f,.41f,.52f);
            Disc(m,ax,ay,.056f,-2.79f,1.f,.94f,.76f);
            Text(m,"PIP: COVER FIRE",0,-.75f,.035f,.75f,1.f,.78f);
        }else if(s.guest()==CastGuest::LollyWren){
            Rect(m,ax,ay,.24f,.18f,-2.81f,.89f,.68f,.43f);
            Rect(m,ax,ay+.07f,.15f,.07f,-2.78f,1.f,.95f,.68f);
            Text(m,"LOLLY: RESCUE TUG",0,-.75f,.035f,.77f,1.f,.81f);
        }
    }else{
        bool blort=false,nib=false,twins=false;
        for(const auto& e:s.enemies())if(e.active){
            if(e.kind==EnemyKind::Blort)blort=true;
            if(e.kind==EnemyKind::Nib)nib=true;
            if(e.kind==EnemyKind::Biff||e.kind==EnemyKind::Baff)twins=true;
        }
        if(blort)Text(m,"BLORT: INCOMING BOMBS",0,-.75f,.035f,1.f,.55f,.38f);
        else if(nib)Text(m,"NIB: MINEFIELD",0,-.75f,.035f,1.f,.79f,.47f);
        else if(twins)Text(m,"BIFF + BAFF: DOUBLE BARRAGE",0,-.75f,.030f,1.f,.79f,.47f);
        else if(s.guest()==CastGuest::CinnamonCharles)
            Text(m,"CHARLES: ZONED BULLET RAIN",0,-.75f,.030f,1.f,.65f,.45f);
        else if(s.guest()==CastGuest::SydneySlipstream)
            Text(m,"SYDNEY: SHIFTING CURRENTS",0,-.75f,.030f,.55f,.95f,1.f);
        else if(s.guest()==CastGuest::JawbreakerJules)
            Text(m,"JULES: HEAVY JAWBREAKER",0,-.75f,.030f,1.f,.65f,.85f);
    }
    if(s.slipstream_active()){
        const bool left=s.left_fast();
        Rect(m,-1.11f,-.09f,1.75f,.032f,-2.93f,
             left?.24f:.94f,left?.94f:.45f,.61f);
        Rect(m,1.11f,-.09f,1.75f,.032f,-2.93f,
             left?.94f:.24f,left?.45f:.94f,.61f);
        Text(m,left?"LEFT FAST / RIGHT SLOW":"LEFT SLOW / RIGHT FAST",
             0,-.30f,.030f,1.f,.97f,.69f);
    }
    if(s.deployed_support()!=SupportKind::None){
        char hired[90];
        std::snprintf(hired,sizeof(hired),"%s  AID %d",
                      SupportName(s.deployed_support()),s.support_pulses());
        Text(m,hired,0,.95f,.027f,.66f,1.f,.83f);
        if(s.support_pulses()>0){
            if(s.deployed_support()==SupportKind::CapitaineEclair){
                Tri(m,s.support_x(),-.73f,s.support_x()-.11f,-.88f,
                    s.support_x()+.11f,-.88f,-2.79f,.62f,.81f,1.f);
            }else{
                Disc(m,s.support_x(),-.79f,.105f,-2.79f,1.f,.83f,.93f);
            }
        }
    }
    if(s.hurt_flash()>0.f){
        Rect(m,-2.25f,0,.13f,2.7f,-2.78f,1.f,.18f,.18f);
        Rect(m, 2.25f,0,.13f,2.7f,-2.78f,1.f,.18f,.18f);
    }
    // Visible motion-controller destination: the hull catches up at its
    // true thruster/shipyard speed; it never teleports to this marker.
    if(m->flightPointer){
        Rect(m,m->flightTargetX,m->flightTargetY,.12f,.022f,
             -2.76f,.38f,1.f,.84f);
        Rect(m,m->flightTargetX,m->flightTargetY,.022f,.12f,
             -2.76f,.38f,1.f,.84f);
    }
    // The live player model is the actual saved 3D X/Y/Z assembly.
    reverie::gumball::AppendAssemblyMesh(
        m->mesh,m->ship,s.player_x(),s.player_y(),
        -2.55f,.10f,0.f);
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
    std::snprintf(status,sizeof(status),"ASSAULT %d/%d  %s  DOWN %d/%d  ESC %d",
                  s.assault(),s.assault_count(),
                  reverie::gumball::FormationName(s.current_formation()),
                  s.defeated(),s.target(),s.escaped());
    TextFit(m,status,0,-1.24f,4.70f,.035f,.88f,.90f,.95f);
    std::snprintf(status,sizeof(status),"RUN CASH +%u  BEST COMBO X%d",
                  s.sortie_cash(),s.max_combo());
    Text(m,status,0,-1.43f,.031f,.86f,.80f,.58f);
    if(s.support().kit_mask()!=0u){
        std::snprintf(status,sizeof(status),"EMERGENCY: %s%s%s",
                      s.support().has_kit(SupportKit::HullPatch)?"HULL PATCH":"",
                      s.support().kit_mask()==3u?" + ":"",
                      s.support().has_kit(SupportKit::ShieldTonic)?"SHIELD TONIC":"");
        Text(m,status,0,-1.62f,.029f,.91f,1.f,.77f);
    }
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
    if(phase==Phase::ArmadaDefeated)
        Text(m,"THE KING IS STILL MISSING",0,-.95f,.044f,.91f,.80f,.48f);
    Text(m,"CLICK TO RETURN TO CHART",0,-1.17f,.038f,.74f,.94f,.88f);
}
void Rebuild(Module* m){
    m->mesh.clear();
    if(m->inYard)ShipyardView(m);
    else if(m->inSupport)SupportView(m);
    else if(m->inLoadout)LoadoutView(m);
    else if(m->sim.phase()==Phase::Chart)Chart(m);
    else if(m->sim.phase()==Phase::Flight)Flight(m);
    else Result(m);
    if(m->campaignSaveError)Text(m,"WARNING: CAMPAIGN SAVE FAILED",0,-1.68f,.029f,1.f,.35f,.28f);
    else if(m->shipSaveError)Text(m,"WARNING: SHIPYARD SAVE FAILED",0,-1.68f,.029f,1.f,.35f,.28f);
    else if(m->shipLoadWarning)Text(m,"SHIPYARD RESTORED TO DEFAULT",0,-1.68f,.029f,1.f,.57f,.28f);
    m->upload=true;
}
void Save(Module* m){
    if(!ReverieNativeHostSupportsSaveV1(m->host)){
        m->campaignSaveError=true;return;
    }
    const Snapshot s=m->sim.snapshot();
    const int code=m->host->write_save("campaign.v1",&s,static_cast<uint32_t>(sizeof(s)));
    if(code!=REVERIE_NATIVE_SAVE_OK){
        if(!m->campaignSaveError)ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,kId,"Campaign save failed");
        m->campaignSaveError=true;
    }else m->campaignSaveError=false;
}
void SaveShipyard(Module* m){
    if(!ReverieNativeHostSupportsSaveV1(m->host)){
        m->shipSaveError=true;return;
    }
    const ShipyardSave state=m->yard.snapshot();
    const int result=m->host->write_save("shipyard.v1",&state,
                                       static_cast<uint32_t>(sizeof(state)));
    if(result!=REVERIE_NATIVE_SAVE_OK){
        m->shipSaveError=true;
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,kId,"Shipyard save failed");
    }else {m->shipSaveError=false;m->shipLoadWarning=false;}
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
            else if(count==sizeof(SnapshotV2)){
                SnapshotV2 legacy{};
                std::memcpy(&legacy,&current,sizeof(legacy));
                valid=m->sim.restore(legacy);
                if(valid)Save(m);
            }else if(count==sizeof(SnapshotV1)){
                SnapshotV1 legacy{};
                std::memcpy(&legacy,&current,sizeof(legacy));
                valid=m->sim.restore(legacy);
                if(valid)Save(m); // one checked, atomically replaced save
            }
            if(!valid)ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_WARN,kId,
                                        "Invalid campaign save ignored");
        }else if(result!=REVERIE_NATIVE_SAVE_NOT_FOUND)
            ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_ERROR,kId,"Campaign save read failed");
        ShipyardSave ship{};
        uint32_t shipBytes=0;
        const int shipCode=host->read_save("shipyard.v1",&ship,sizeof(ship),&shipBytes);
        if(shipCode==REVERIE_NATIVE_SAVE_OK){
            if(shipBytes!=sizeof(ship)||!m->yard.restore(ship)){
                m->shipLoadWarning=true;
                ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_WARN,kId,
                                   "Invalid shipyard save; restored default ship");
            }
        }else if(shipCode!=REVERIE_NATIVE_SAVE_NOT_FOUND)
            ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_ERROR,kId,
                               "Shipyard save read failed");
    }
    m->sim.configure_ship(m->yard);
    Rebuild(m);return m;
}
void Destroy(void* p){delete static_cast<Module*>(p);}
// Runtime integration uses the host's game-scoped APK asset reader. The build
// task already converts approved atlas cells to RGB565; native code never
// decodes the full source PNG or accepts another module's asset path.
bool LoadRouteArtwork(Module* m,unsigned index){
    if(index>=kArtworkCount||!m->host
       ||m->host->struct_size<REVERIE_NATIVE_HOST_V1_ASSET_MIN_SIZE
       ||!m->host->read_asset)return false;
    char path[112];
    std::snprintf(path,sizeof(path),"native/gumball/%s/%s.rgb565",
                  index<kRouteArtworkCount?"nodes":"allies",
                  kRouteArtworkNames[index]);
    uint32_t required=0;
    const int info=m->host->read_asset(path,nullptr,0,&required);
    if(info!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL
       ||required!=kRouteArtworkBytes){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_WARN,kId,
                           "Gumball chart artwork missing or unexpected size");
        return false;
    }
    std::array<uint8_t,kRouteArtworkBytes> pixels{};
    uint32_t actual=0;
    if(m->host->read_asset(path,pixels.data(),required,&actual)
       !=REVERIE_NATIVE_SAVE_OK||actual!=required
       ||std::memcmp(pixels.data(),"GAI1",4)!=0
       ||pixels[4]!=96||pixels[5]!=0||pixels[6]!=86||pixels[7]!=0){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,kId,
                           "Gumball chart artwork failed header/read validation");
        return false;
    }
    GLuint texture=0;
    glGenTextures(1,&texture);
    if(!texture)return false;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    GLint unpack=4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpack);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,96,86,0,GL_RGB,
                 GL_UNSIGNED_SHORT_5_6_5,pixels.data()+8);
    glPixelStorei(GL_UNPACK_ALIGNMENT,unpack);
    if(glGetError()!=GL_NO_ERROR){
        glDeleteTextures(1,&texture);
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,kId,
                           "Gumball chart texture upload failed");
        return false;
    }
    m->routeTextures[index]=texture;
    return true;
}
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
    if(good){
        // Separate, persistent textured-quad renderer; it does not rebuild
        // the flat HUD mesh or allocate textures during stereo eye rendering.
        const char* imageVs=
            "uniform mat4 u_Mvp; uniform vec4 u_Rect; attribute vec2 a_Corner;"
            " varying vec2 v_Uv; void main(){"
            " vec2 p=u_Rect.xy+a_Corner*u_Rect.zw;"
            " v_Uv=vec2(a_Corner.x+0.5,0.5-a_Corner.y);"
            " gl_Position=u_Mvp*vec4(p,-2.80,1.0);}";
        const char* imageFs=
            "precision mediump float; uniform sampler2D u_Tex;"
            " varying vec2 v_Uv; void main(){"
            " gl_FragColor=texture2D(u_Tex,v_Uv);}";
        const ReverieNativeGlAttributeBindingV1 imageAttrs[]={{2u,"a_Corner"}};
        m->routeProgram=ReverieNativeBuildProgramDiagnosticV1(
            imageVs,imageFs,imageAttrs,1u,diag,sizeof(diag));
        if(m->routeProgram){
            m->routeCorner=glGetAttribLocation(m->routeProgram,"a_Corner");
            m->routeMvp=glGetUniformLocation(m->routeProgram,"u_Mvp");
            m->routeRect=glGetUniformLocation(m->routeProgram,"u_Rect");
            m->routeSampler=glGetUniformLocation(m->routeProgram,"u_Tex");
            if(m->routeCorner!=2||m->routeMvp<0||m->routeRect<0
               ||m->routeSampler<0){
                glDeleteProgram(m->routeProgram);m->routeProgram=0;
            }
        }
        if(m->routeProgram){
            constexpr GLfloat corners[]={
                -.5f,-.5f, .5f,-.5f, -.5f,.5f,
                -.5f,.5f, .5f,-.5f, .5f,.5f};
            glGenBuffers(1,&m->routeVbo);
            if(m->routeVbo){
                glBindBuffer(GL_ARRAY_BUFFER,m->routeVbo);
                glBufferData(GL_ARRAY_BUFFER,sizeof(corners),corners,GL_STATIC_DRAW);
                unsigned loaded=0;
                for(unsigned i=0;i<kArtworkCount;++i)
                    if(LoadRouteArtwork(m,i))++loaded;
                if(loaded!=kArtworkCount)
                    ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_WARN,kId,
                                       "Gumball route-card artwork partially unavailable");
            }
        }else{
            ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,kId,diag);
        }
    }
    ReverieNativeGlStateRestoreV1(&state);m->upload=true;
    return good?1:0;
}
void GlEnd(void* p){
    auto* m=static_cast<Module*>(p);if(!m)return;
    glDeleteTextures(kArtworkCount,m->routeTextures);
    for(GLuint& texture:m->routeTextures)texture=0;
    if(m->routeVbo)glDeleteBuffers(1,&m->routeVbo);
    if(m->routeProgram)glDeleteProgram(m->routeProgram);
    if(m->vbo)glDeleteBuffers(1,&m->vbo);
    if(m->program)glDeleteProgram(m->program);
    m->routeVbo=0;m->routeProgram=0;
    m->vbo=0;m->program=0;m->upload=true;
}
void Resume(void*){}void Pause(void*){}
void Update(void* p,const ReverieNativeInputV1* in){
    auto* m=static_cast<Module*>(p);
    if(!m||!ReverieNativeInputHasBaseV1(in))return;
    const float dt=std::max(0.f,std::min(.05f,std::isfinite(in->delta_seconds)?in->delta_seconds:0.f));
    const bool pressed=in->primary_down!=0;
    const Phase phase=m->sim.phase();
    // Aiming at a drawn control must focus that exact control, and primary
    // click acts on it. Pad navigation is preferred while the pad is moving.
    const bool padMoving=std::fabs(in->move_x)>.55f
        ||std::fabs(in->move_y)>.55f;
    if(phase==Phase::Chart&&padMoving)m->pointerFocus.padFocus=true;
    const bool pointerRay=phase==Phase::Chart
        &&ReverieNativeInputHasPointerV1(in)
        &&in->pointer_kind!=REVERIE_NATIVE_POINTER_NONE;
    MenuHit pointed{};
    if(pointerRay
       &&std::isfinite(in->pointer_origin[2])
       &&std::isfinite(in->pointer_direction[2])
       &&std::fabs(in->pointer_direction[2])>.0001f){
        const float travel=(-2.94f-in->pointer_origin[2])
                          /in->pointer_direction[2];
        if(travel>0.f&&travel<30.f){
            const float px=in->pointer_origin[0]
                          +travel*in->pointer_direction[0];
            const float py=in->pointer_origin[1]
                          +travel*in->pointer_direction[1];
            if(std::isfinite(px)&&std::isfinite(py)){
                if(m->pointerFocus.take(px,py,padMoving)){
                MenuHit hit;
                bool changed=false;
                if(m->inYard){
                    hit=reverie::gumball::YardHit(px,py);
                    pointed=hit;
                    if(hit.id>=0&&(m->yardCursor.x!=hit.x
                                    ||m->yardCursor.y!=hit.y)){
                        m->yardCursor.pointAt(hit.x,hit.y);
                        changed=true;
                    }
                }else if(m->inSupport){
                    hit=reverie::gumball::SupportHit(px,py);
                    pointed=hit;
                    if(hit.id>=0&&m->supportSelected!=hit.id){
                        m->supportSelected=hit.id;changed=true;
                    }
                }else if(m->inLoadout){
                    hit=reverie::gumball::LoadoutHit(px,py);
                    pointed=hit;
                    if(hit.id>=0&&m->loadoutSelected!=hit.id){
                        m->loadoutSelected=hit.id;changed=true;
                    }
                }else{
                    hit=reverie::gumball::ChartHit(px,py);
                    pointed=hit;
                    if(hit.id>=0&&m->selected!=hit.id){
                        m->selected=hit.id;changed=true;
                    }
                }
                if(changed){
                    ReverieNativeRequestFeedbackV1(
                        m->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
                    Rebuild(m);
                }
                }
            }
        }
    }
    const bool mayClick=reverie::gumball::MenuClickAllowed(
        pointerRay,m->pointerFocus.padFocus,padMoving,pointed);
    const bool clickEdge=pressed&&!m->primary;
    if(phase==Phase::Chart&&clickEdge&&!mayClick)
        ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
    if(phase==Phase::Flight){
        const uint32_t kills=m->sim.kills();const int hp=m->sim.hp();
        const int shieldBefore=m->sim.shield();
        const uint32_t fired=m->sim.shots_fired();
        const uint32_t usedKits=m->sim.kit_activations();
        // The actual Daydream motion controller owns flight positioning.
        // Touchpad click fires independently; the directional pad is only
        // a fallback when no valid tracked controller aim is available.
        reverie::gumball::FlightAim aim{};
        if(ReverieNativeInputHasPointerV1(in)
           &&in->pointer_kind==REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER)
            aim=reverie::gumball::FlightAimFromRay(
                in->pointer_origin,in->pointer_direction);
        m->flightPointer=aim.valid;
        if(aim.valid){
            m->flightTargetX=aim.x;
            m->flightTargetY=aim.y;
            const auto axes=reverie::gumball::FlightAxesToTarget(
                m->sim.player_x(),m->sim.player_y(),aim,dt,
                m->sim.pointer_move_speed());
            m->sim.update(dt,axes.x,axes.y,pressed,true);
        }else{
            m->sim.update(dt,in->move_x,in->move_y,pressed);
        }
        if(kills!=m->sim.kills())ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        if(hp>m->sim.hp()||shieldBefore>m->sim.shield())ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
        if(m->sim.shots_fired()>fired)
            ReverieNativeRequestSoundV1(m->host,m->sim.weapon()==Weapon::RockCannon?
                REVERIE_NATIVE_SOUND_WEAPON_HEAVY:REVERIE_NATIVE_SOUND_WEAPON_MACHINE);
        if(m->sim.phase()!=Phase::Flight||m->sim.kit_activations()!=usedKits)
            Save(m); // consuming a paid kit is durable before a process crash
        if(m->sim.kit_activations()!=usedKits)
            ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        Rebuild(m);
    }else if(phase==Phase::Chart&&m->inLoadout){
        if(reverie::gumball::PrepSecondaryBack(m->inLoadout,m->inSupport,
                                               m->inYard,in->secondary_down!=0,
                                               m->secondary)){
            m->inLoadout=false;m->navTime=0.f;
            m->pointerFocus.screenChanged();
            ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
            Rebuild(m);
        }else{
        m->navTime-=dt;
        const float x=in->move_x,y=in->move_y;
        if(m->navTime<=0.f&&(std::fabs(x)>.6f||std::fabs(y)>.6f)){
            m->loadoutSelected=LoadoutNavigate(m->loadoutSelected,
                std::fabs(x)>std::fabs(y)?(x>0.f?1:-1):0,
                std::fabs(y)>=std::fabs(x)?(y>0.f?-1:1):0);
            m->navTime=.18f;
            ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
            Rebuild(m);
        }
        if(clickEdge&&mayClick){
            bool good=false;
            if(m->loadoutSelected<4){
                good=m->sim.buy(static_cast<Upgrade>(m->loadoutSelected));
                if(good)Save(m);
            }else if(m->loadoutSelected<7){
                const Weapon weapon=static_cast<Weapon>(m->loadoutSelected-4);
                good=m->sim.owns_weapon(weapon)?
                    m->sim.select_weapon(weapon):m->sim.buy_weapon(weapon);
                if(good)Save(m);
            }else{
                m->inLoadout=false;m->pointerFocus.screenChanged();good=true;
            }
            ReverieNativeRequestFeedbackV1(m->host,good?
                REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
            Rebuild(m);
        }
        }
    }else if(phase==Phase::Chart&&m->inSupport){
        if(reverie::gumball::PrepSecondaryBack(m->inLoadout,m->inSupport,
                                               m->inYard,in->secondary_down!=0,
                                               m->secondary)){
            m->inSupport=false;m->navTime=0.f;
            m->pointerFocus.screenChanged();
            ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
            Rebuild(m);
        }else{
        m->navTime-=dt;
        if(m->navTime<=0.f&&std::fabs(in->move_y)>.55f){
            m->supportSelected=std::max(0,std::min(8,
                 m->supportSelected+(in->move_y>0.f?-1:1)));
            m->navTime=.17f;
            ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
            Rebuild(m);
        }
        if(clickEdge&&mayClick){
            bool good=false;
            if(m->supportSelected==8){
                m->inSupport=false;m->pointerFocus.screenChanged();good=true;
            }
            else if(m->supportSelected<2){
                good=m->sim.hire_support(m->supportSelected==0?
                    SupportKind::CapitaineEclair:SupportKind::MademoiselleMacaron);
                if(good)Save(m);
            }else if(m->supportSelected<6){
                good=m->sim.upgrade_support(
                    static_cast<SupportSkill>(m->supportSelected-2));
                if(good)Save(m);
            }else{
                good=m->sim.buy_support_kit(static_cast<SupportKit>(m->supportSelected-6));
                if(good)Save(m);
            }
            ReverieNativeRequestFeedbackV1(m->host,good?
                REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
            Rebuild(m);
        }
        }
    }else if(phase==Phase::Chart&&m->inYard){
        m->navTime-=dt;
        const float x=in->move_x,y=in->move_y;
        // App/Minus instantly moves focus between the part shelf and the ship.
        // The D-pad alone can still reach every cell and every shelf button.
        if(in->secondary_down!=0&&!m->secondary){
            m->yardCursor.switchFocus();
            std::snprintf(m->yardFeedback,sizeof(m->yardFeedback),
                          m->yardCursor.palette()?"CHOOSE A PART, THEN CLICK":
                          "CLICK A CELL TO CHANGE IT");
            ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
            Rebuild(m);
        }
        if(m->navTime<=0.f&&(std::fabs(x)>.6f||std::fabs(y)>.6f)){
            const int dx=std::fabs(x)>std::fabs(y)?(x>0.f?1:-1):0;
            // Screen-up must move the cursor UP, not down the grid/list.
            const int dy=std::fabs(y)>=std::fabs(x)?(y>0.f?-1:1):0;
            m->yardCursor.move(dx,dy);
            m->navTime=.15f;
            ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
            Rebuild(m);
        }
        if(clickEdge&&mayClick){
            bool good=false;
            if(!m->yardCursor.palette()){
                const auto result=m->yard.edit(
                    m->yardCursor.x,m->yardCursor.y,m->brush);
                good=result==reverie::gumball::ShipEdit::Placed
                    ||result==reverie::gumball::ShipEdit::Replaced
                    ||result==reverie::gumball::ShipEdit::Removed;
                std::snprintf(m->yardFeedback,sizeof(m->yardFeedback),
                              "%s",reverie::gumball::ShipEditMessage(result));
                if(good){
                    m->sim.configure_ship(m->yard);
                    SaveShipyard(m);
                }
            }else{
                const int choice=m->yardCursor.toolIndex();
                if(m->yardCursor.chosenPart()!=Part::Empty){
                    const Part candidate=m->yardCursor.chosenPart();
                    m->brush=candidate; // empty stock is still valid for removal
                    m->yardCursor.toolChosen(); // immediately back to ship
                    m->pointerFocus.padFocus=true; // preserve grid focus until ray moves
                    std::snprintf(m->yardFeedback,sizeof(m->yardFeedback),
                        m->ship.stock(candidate)>0u?
                            "ARMED %s: CLICK CELL TO EDIT":
                            "ARMED %s: REMOVE FITTED PARTS",
                        YardPartName(candidate));
                    good=true;
                }else if(choice==9){
                    m->inYard=false;m->pointerFocus.screenChanged();good=true;
                }
            }
            ReverieNativeRequestFeedbackV1(m->host,good?
                REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
            Rebuild(m);
        }
    }else if(phase==Phase::Chart){
        m->navTime-=dt;
        const float x=in->move_x,y=in->move_y;
        if(m->navTime<=0.f&&(std::fabs(x)>.6f||std::fabs(y)>.6f)){
            m->selected=ChartNavigate(m->selected,
                std::fabs(x)>std::fabs(y)?(x>0.f?1:-1):0,
                std::fabs(y)>=std::fabs(x)?(y>0.f?-1:1):0);
            m->navTime=.19f;
            ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
            Rebuild(m);
        }
        if(clickEdge&&mayClick){
            bool good=false;
            if(m->selected>=0&&m->selected<3){
                good=m->sim.launch(static_cast<unsigned>(m->selected));
                if(good)Save(m); // consume pending escort exactly once
            }else if(m->selected==10){
                m->inYard=true;m->yardCursor=YardCursor{};
                m->pointerFocus.screenChanged();
                std::snprintf(m->yardFeedback,sizeof(m->yardFeedback),
                              "SELECT PART ON RIGHT / CLICK SHIP TO EDIT");
                good=true;
            }else if(m->selected==11){
                m->inSupport=true;m->supportSelected=0;
                m->pointerFocus.screenChanged();good=true;
            }else if(m->selected==12){
                m->inLoadout=true;m->loadoutSelected=0;
                m->pointerFocus.screenChanged();good=true;
            }
            ReverieNativeRequestFeedbackV1(m->host,good?
                REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
            Rebuild(m);
        }
    }else if(pressed&&!m->primary&&m->sim.recover()){
        m->pointerFocus.screenChanged();
        Save(m);Rebuild(m);
        ReverieNativeRequestFeedbackV1(m->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
    }
    m->primary=pressed;
    m->secondary=in->secondary_down!=0;
}
int32_t Render(void* p,const ReverieNativeEyeV1* eye){
    auto* m=static_cast<Module*>(p);
    if(!m||!ReverieNativeEyeRenderableV1(eye)||!m->program||!m->vbo)return 0;
    ReverieNativeGlStateV1 state{};
    ReverieNativeGlAttribStateV1 a{},b{},c{};
    ReverieNativeGlStateCaptureV1(&state);
    ReverieNativeGlAttribCaptureV1(&a,static_cast<GLuint>(m->pos));
    ReverieNativeGlAttribCaptureV1(&b,static_cast<GLuint>(m->col));
    ReverieNativeGlAttribCaptureV1(&c,2u);
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
    const bool chartArt=m->sim.phase()==Phase::Chart
        &&!m->inLoadout&&!m->inSupport&&!m->inYard;
    const bool allyArt=m->sim.phase()==Phase::Flight&&m->sim.guest_active()
        &&(m->sim.guest()==CastGuest::PipParfait
            ||m->sim.guest()==CastGuest::LollyWren);
    if(m->routeProgram&&m->routeVbo&&(chartArt||allyArt)){
        glUseProgram(m->routeProgram);
        glBindBuffer(GL_ARRAY_BUFFER,m->routeVbo);
        glEnableVertexAttribArray(2u);
        glVertexAttribPointer(2u,2,GL_FLOAT,GL_FALSE,2*sizeof(float),
                              reinterpret_cast<const void*>(0));
        glUniformMatrix4fv(m->routeMvp,1,GL_FALSE,mvp);
        glUniform1i(m->routeSampler,0);
        glActiveTexture(GL_TEXTURE0);
        if(chartArt){
            for(unsigned lane=0;lane<3u;++lane){
                const unsigned image=reverie::gumball::Mix(
                    m->sim.lane(lane).id)%kRouteArtworkCount;
                const GLuint texture=m->routeTextures[image];
                if(!texture)continue;
                glBindTexture(GL_TEXTURE_2D,texture);
                glUniform4f(m->routeRect,
                            reverie::gumball::kChartRouteX[lane]+.50f,
                            .56f,.26f,.24f);
                glDrawArrays(GL_TRIANGLES,0,6);
            }
        }else if(allyArt){
            // These are real expression crops from each named companion's
            // approved sheet. The independent combat actor remains its ship.
            const unsigned image=m->sim.guest()==CastGuest::PipParfait
                ?kRouteArtworkCount:kRouteArtworkCount+1u;
            const GLuint texture=m->routeTextures[image];
            if(texture){
                glBindTexture(GL_TEXTURE_2D,texture);
                glUniform4f(m->routeRect,-1.72f,-.73f,.35f,.32f);
                glDrawArrays(GL_TRIANGLES,0,6);
            }
        }
    }
    const GLenum error=glGetError();
    ReverieNativeGlAttribRestoreV1(&c);
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
