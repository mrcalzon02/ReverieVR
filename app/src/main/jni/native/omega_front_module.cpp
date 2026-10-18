#include "omega_front_campaign_save.h"
#include "omega_front_texture_renderer.h"
#include "omega_front_sound.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include "reverie_native_flat_mesh.h"
#include "reverie_native_vector_text.h"
#include <GLES2/gl2.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

namespace {
namespace game = reverie::omega;
namespace omega_art = reverie::omega_art;
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
    reverie::omega_art::SpriteRenderer sprites;
    size_t base_vertices=0,unit_vertices=0;
};
void Rect(State *s,float x,float y,float w,float h,float z,float r,float g,float b){
    reverie::flatmesh::Rect(s->mesh,x,y,w,h,z,r,g,b);
}
void Line(State *s,float x,float y,float xx,float yy,float w,float z,float r,float g,float b){
    reverie::flatmesh::Segment(s->mesh,x,y,xx,yy,w,z,r,g,b);
}
// Native outline lettering; coordinates and font scaling are unchanged.
void Text(State *s,const char *msg,float center_y,float px,
          float r,float g,float b){
    reverie::nativeart::StrokeText(s->mesh,msg,0.f,center_y,px,r,g,b,kNear);
}
void TextAt(State *s,const char *msg,float cx,float center_y,float px,
            float r,float g,float b){
    reverie::nativeart::StrokeText(s->mesh,msg,cx,center_y,px,r,g,b,kNear);
}
void DrawMap(State *s){
    const game::Session &session=s->session;
    const game::Frontier &f=session.frontier;
    s->mesh.clear();s->mesh.reserve(14000);
    s->sprites.Clear();
    Rect(s,0,0,2.15f,2.3f,kBoardZ-.09f,.018f,.036f,.049f);
    Text(s,"OMEGA FRONT  HEARTHFALL",1.070f,.014f,.92f,.94f,.81f);
    char line[110];
    std::snprintf(line,sizeof(line),"DAY %d  GUARDS %d  RATIONS %d  SALVAGE %d",f.day(),f.guards(),f.rations(),f.salvage());
    Text(s,line,.916f,.009f,.77f,.85f,.88f);
    // Initial strategic view now uses the same packaged terrain textures
    // that tactical combat uses. Draw the permanent board first, then the
    // sector sprites, then owner/selection/text overlays in a later pass.
    s->base_vertices=s->mesh.size()/6u;
    s->unit_vertices=s->base_vertices;
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
        if(f.intelligence(i)==game::Intelligence::Unknown){
            r=.12f;g=.16f;b=.20f;
        }
        // Unexplored locations remain obscured: textures cannot leak
        // geography that the existing scouting/intelligence rules hide.
        const bool unknown=f.intelligence(i)==game::Intelligence::Unknown;
        // Shared material textures are drawn only for known geography.
        const int visual=sector->ecology==game::Ecology::Eden?
                         omega_art::SharedGrass:
                         sector->ecology==game::Ecology::Wasteland?
                         (i%3==0?omega_art::SharedRock:omega_art::SharedSand):
                         (i%2==0?omega_art::SharedRoad:omega_art::SharedConcrete);
        if(unknown || !s->sprites.Place(visual,x,y,span*.96f,
                                        span*.96f,kBoardZ+.001f))
            Rect(s,x,y,span*.96f,span*.96f,kBoardZ,r,g,b);
        else {
            // Keep the actual ecology/anomaly distinction legible even
            // with art underneath, instead of replacing it with decoration.
            Rect(s,x,y+span*.45f,span*.88f,span*.042f,
                 kBoardZ+.002f,r,g,b);
            if(sector->anomaly)
                Rect(s,x-span*.32f,y-span*.30f,span*.08f,span*.11f,
                     -1.851f,.76f,.43f,.91f);
        }
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
        // Unknown owners cannot be read from a distant, unseen sector.
        if(f.intelligence(i)!=game::Intelligence::Surveyed)continue;
        const bool independent=sector->owner==game::kIndependentOwner;
        Rect(s,x+span*.31f,y-span*.31f,span*.14f,span*.14f,-1.85f,
             independent?.25f:sector->owner?.91f:.60f,
             independent?.90f:sector->owner?.32f:.64f,
             independent?.94f:sector->owner?.28f:.67f);
        // Source-derived resource silhouettes on SURVEYED sectors only.
        // Never reveal unsurveyed inventory, ownership or scouting results.
        const int resource=sector->wreck_mass>0?omega_art::SupplyCrate:
                           sector->ecology==game::Ecology::Eden && sector->food>0?
                           omega_art::LivingTree:-1;
        if(resource>=0)
            s->sprites.Place(resource,x-span*.28f,y-span*.28f,
                             span*.26f,span*.26f,kBoardZ+.008f);
    }
    const auto *target=f.sector(session.focused);
    if(target){
        const bool surveyed=f.intelligence(session.focused)==game::Intelligence::Surveyed;
        const int counter=surveyed && target->claims>=45?
            f.reprisal_faction(session.focused):0;
        if(counter>0)
            std::snprintf(line,sizeof(line),"S%03d %s  REPRISAL F%02d",
                          session.focused+1,target->name,counter);
        else std::snprintf(line,sizeof(line),"S%03d %s",session.focused+1,
                           surveyed?target->name:"UNSURVEYED");
        Text(s,line,-.868f,.012f,counter>0?1.f:.94f,
             counter>0?.61f:.95f,counter>0?.38f:.81f);
        if(surveyed)
            std::snprintf(line,sizeof(line),"FOOD %u WATER %u WRECK %u CLAIM %u",
                target->food,target->water,target->wreck_mass,target->claims);
        else std::snprintf(line,sizeof(line),"INTEL %s  SCOUT ADJACENT",
            f.intelligence(session.focused)==game::Intelligence::Unknown?"UNKNOWN":"TERRAIN ONLY");
        const bool endangered=surveyed && target->claims>=60;
        Text(s,line,-.982f,.010f,endangered?1.f:.77f,
             endangered?.56f:.86f,endangered?.37f:.86f);
    }
    const char *actions[]={"DEPLOY","FORAGE","RECRUIT","REPAIR","WAIT","SCOUT","PARLEY","ACCORD"};
    if(session.focused!=f.location()){
        const bool recon=session.action==game::FieldAction::Scout;
        const bool parley=session.action==game::FieldAction::Parley;
        const bool accord=session.action==game::FieldAction::Accord;
        const bool convoy=session.action==game::FieldAction::Forage;
        const bool security=session.action==game::FieldAction::Repair;
        if(security){
            const bool ready=f.can_secure(session.focused);
            Text(s,ready?"SECURE ROUTE  2 RATIONS 1 SALVAGE":
                "SECURE BLOCKED  CHECK CLAIMS AND ESCORT",
                -1.098f,.009f,ready?.70f:1.f,
                ready?.95f:.43f,.63f);
        }else if(convoy){
            const bool available=f.can_convoy(session.focused);
            const int ambusher=available?f.ConvoyAmbusher(session.focused):0;
            if(ambusher>0){
                std::snprintf(line,sizeof(line),"CONVOY AMBUSH RISK F%02d  CLICK ESCORT",
                              ambusher);
                Text(s,line,-1.098f,.010f,1.f,.65f,.31f);
            }else Text(s,available?"CONVOY IN  UP TO 8 FOOD 6 WATER  CLICK":
                "CONVOY UNAVAILABLE  NEED ROUTE RATION ESCORT",
                -1.098f,.010f,available?.70f:1.f,
                available?.95f:.43f,.63f);
        }else if(recon||parley||accord){
            std::snprintf(line,sizeof(line),"%s  CLICK  MENU CHANGE",
                          recon?"SCOUT":parley?"PARLEY":"ACCORD");
            Text(s,line,-1.098f,.011f,.91f,.90f,.64f);
        }else{
            const bool possible=f.can_travel(session.focused);
            const bool licensed=possible && f.has_transit_right(session.focused);
            Text(s,!possible?"ROUTE BLOCKED":
                   licensed?"PAID PASS  FREE ENTRY":"CLICK TO MARCH",
                 -1.098f,.013f,possible?.71f:.96f,
                 possible?.98f:.43f,.49f);
        }
    }else {
        const bool remote=session.action==game::FieldAction::Scout||
                          session.action==game::FieldAction::Parley||
                          session.action==game::FieldAction::Accord;
        const bool infrastructure=(f.location()==10 && f.pump_health()<18) ||
                                  (f.location()==14 && !f.bridge_repaired());
        const bool securing=session.action==game::FieldAction::Repair &&
                            !infrastructure;
        const bool defend=session.action==game::FieldAction::Deploy &&
                          f.can_defend_outpost();
        const bool attack=session.action==game::FieldAction::Deploy &&
                          f.can_frontline_assault();
        if(defend){
            std::snprintf(line,sizeof(line),"DEFEND OUTPOST F%02d  2 RATIONS  CLICK",
                f.reprisal_faction(f.location()));
            Text(s,line,-1.098f,.010f,1.f,.72f,.40f);
        }else if(attack){
            std::snprintf(line,sizeof(line),"ASSAULT OUTPOST F%02u  2 RATIONS  CLICK",
                target?target->owner:0u);
            Text(s,line,-1.098f,.010f,1.f,.72f,.40f);
        }else{
            std::snprintf(line,sizeof(line),"%s  %s  MENU CHANGE",
                          securing?"SECURE":actions[static_cast<int>(session.action)],
                          remote?"SELECT NEIGHBOR":"CLICK EXECUTE");
            Text(s,line,-1.098f,.010f,.93f,.92f,.60f);
        }
    }
    if(f.operation()<game::kHearthfallOperations){
        std::snprintf(line,sizeof(line),"%d/%d %s  S%03d F%02u REL %d",
            f.operation()+1,game::kHearthfallOperations,
            game::Frontier::OperationName(f.operation()),f.target_sector()+1,
            f.operation_faction(),f.relation(f.operation_faction()));
        Text(s,line,-1.194f,.009f,.78f,.90f,.93f);
    }else Text(s,"HEARTHFALL COMPLETE  OUTPOST WAR CONTINUES",
                 -1.194f,.009f,.78f,.94f,.76f);
    if(f.last_frontline()){
        std::snprintf(line,sizeof(line),f.last_frontline_defense()?
            "LAST DEFENSE S%03d %s  LOST %u":
            "LAST ASSAULT S%03d %s  LOST %u",
            f.last_frontline_sector()+1,
            f.last_frontline_defense()?
                (f.last_report().outcome==game::Result::Defended?
                 "HELD":"LOST"):
                (f.last_report().outcome==game::Result::Defended?
                 "TAKEN":"REPULSED"),
            f.last_report().defenders_lost);
        Text(s,line,.73f,.009f,.97f,.75f,.48f);
    }
    if(f.last_convoy()){
        std::snprintf(line,sizeof(line),"LAST CONVOY S%03d %s  LOST %u",
            f.last_convoy_source()+1,
            f.last_report().outcome==game::Result::Defended?"SAVED":"RAIDED",
            f.last_report().defenders_lost);
        Text(s,line,.73f,.009f,.97f,.75f,.48f);
    }
    if(f.last_border_inspection()>=3 &&
       f.location()==game::Accord::Sector(f.last_border_inspection())){
        std::snprintf(line,sizeof(line),"BORDER INSPECTION M%02d",
                      f.last_border_inspection()+1);
        Text(s,line,.84f,.009f,1.f,.55f,.39f);
    }
    if(f.last_accord()>=3 && f.next_morphology_day()<0){
        const int op=f.last_accord();
        const game::Obligations::Status status=f.obligation_status(op);
        if(status==game::Obligations::Due)
            std::snprintf(line,sizeof(line),"PACT M%02d DUE DAY %d",
                          op+1,f.obligation_due_day(op));
        else
            std::snprintf(line,sizeof(line),"PACT M%02d %s",op+1,
                          status==game::Obligations::Honored?"HONORED":
                          status==game::Obligations::Broken?"BROKEN":"LEGACY");
        Text(s,line,.80f,.010f,
             status==game::Obligations::Broken?1.f:.60f,
             status==game::Obligations::Broken?.37f:.92f,.78f);
    }
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
// These are visual-only skins, never new simulation classes. Unit ID is
// stable across battle checkpoints, so visual variety survives save/load.
// Enemy doctrines choose recognizable specialist uniforms irrespective of ID.
int SoldierTexture(const game::Unit &u,game::Doctrine doctrine) {
    const bool ally=u.side==game::Side::Hearthwell;
    const bool alternate=(u.id&1u)!=0u;
    switch(u.role){
    case game::Class::Rifle:
        return ally?(alternate?omega_art::RifleHAlt:omega_art::RifleH):
                    (alternate?omega_art::RifleRAlt:omega_art::RifleR);
    case game::Class::Engineer:
        return ally?(alternate?omega_art::EngineerHAlt:omega_art::EngineerH):
                    ((alternate||doctrine==game::Doctrine::Meridian)?
                       omega_art::EngineerRAlt:omega_art::EngineerR);
    case game::Class::Salvager:
        return ally?(alternate?omega_art::SalvagerHAlt:omega_art::SalvagerH):
                    ((alternate||doctrine==game::Doctrine::Scrapline)?
                       omega_art::SalvagerRAlt:omega_art::SalvagerR);
    case game::Class::Sentinel:
        return ally?(alternate?omega_art::SentinelHAlt:omega_art::SentinelH):
                    ((alternate||doctrine==game::Doctrine::Meridian)?
                       omega_art::SentinelRAlt:omega_art::SentinelR);
    case game::Class::Legionary:
        return ally?(alternate?omega_art::LegionaryHAlt:omega_art::LegionaryH):
                    ((alternate||doctrine==game::Doctrine::LostNinth)?
                       omega_art::LegionaryRAlt:omega_art::LegionaryR);
    case game::Class::Heavy:return -1;
    }
    return -1;
}
// GPU sprites carry the main silhouette. These unobtrusive marks retain class
// equipment and faction recognition without replacing or obscuring the art.
void SpriteEquipment(State *s,const game::Unit &u,float x,float y,
                     game::Doctrine doctrine) {
    float r=.29f,g=.86f,b=.58f;
    if(u.side!=game::Side::Hearthwell) {
        r=.91f;g=.36f;b=.29f;
        if(doctrine==game::Doctrine::Meridian){r=.30f;g=.66f;b=.97f;}
        else if(doctrine==game::Doctrine::Scrapline){r=.98f;g=.58f;b=.22f;}
        else if(doctrine==game::Doctrine::Hearthward){r=.86f;g=.78f;b=.37f;}
    }
    Rect(s,x-kCell*.32f,y+kCell*.38f,kCell*.13f,kCell*.08f,
         -1.843f,r,g,b);
    if(u.role==game::Class::Engineer) {
        Line(s,x-kCell*.28f,y-kCell*.22f,x-kCell*.28f,y+kCell*.05f,
             kCell*.065f,-1.843f,.96f,.78f,.25f);
        Rect(s,x-kCell*.28f,y-kCell*.24f,kCell*.16f,kCell*.11f,
             -1.843f,.91f,.70f,.23f);
    }else if(u.role==game::Class::Salvager) {
        Rect(s,x-kCell*.28f,y-kCell*.03f,kCell*.16f,kCell*.23f,
             -1.843f,.91f,.52f,.18f);
        Line(s,x+kCell*.27f,y-kCell*.16f,x+kCell*.35f,y+kCell*.09f,
             kCell*.052f,-1.843f,.34f,.77f,.86f);
    }else if(u.role==game::Class::Sentinel) {
        Rect(s,x,y+kCell*.25f,kCell*.25f,kCell*.055f,
             -1.843f,.27f,.89f,.98f);
    }else if(u.role==game::Class::Legionary) {
        // Distinct melee-only shield and short sword. No invented firearm.
        Rect(s,x-kCell*.28f,y-kCell*.04f,kCell*.22f,kCell*.39f,
             -1.843f,.75f,.25f,.20f);
        Rect(s,x-kCell*.28f,y-kCell*.04f,kCell*.08f,kCell*.25f,
             -1.842f,.95f,.76f,.31f);
        Line(s,x+kCell*.28f,y-kCell*.22f,x+kCell*.28f,y+kCell*.22f,
             kCell*.070f,-1.843f,.83f,.81f,.66f);
    }
    const float hp=static_cast<float>(u.hp)/
                   static_cast<float>(game::Battle::MaxHp(u.role));
    Rect(s,x,y-kCell*.43f,kCell*.68f,kCell*.065f,
         -1.842f,.14f,.16f,.19f);
    Rect(s,x-kCell*.34f+hp*kCell*.34f,y-kCell*.43f,
         kCell*.68f*hp,kCell*.055f,-1.841f,.22f,.92f,.32f);
}
void Draw(State *s){
    if(s->session.view==game::FrontMode::SectorMap){DrawMap(s);return;}
    s->mesh.clear();s->mesh.reserve(75000);
    s->sprites.Clear();
    Rect(s,0,0,2.15f,2.3f,kBoardZ-.09f,.018f,.036f,.049f);
    Rect(s,0,kCenterY,2*kHalf+.035f,2*kHalf+.035f,kBoardZ-.04f,.21f,.28f,.20f);
    s->base_vertices=s->mesh.size()/6u;
    const game::Battle &battle=s->session.tactical.battle;
    const bool defendingConvoy=s->session.frontier.convoy_battle();
    const bool frontlineBattle=s->session.frontier.frontline_battle();
    const bool defendingOutpost=s->session.frontier.frontline_defense();
    const auto *location=s->session.frontier.sector(s->session.frontier.location());
    const bool verdant=location && location->ecology==game::Ecology::Eden;
    const bool wasted=location && location->ecology==game::Ecology::Wasteland;
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
        // Only appearance changes: the logical Ground and terrain costs do not.
        const int sprite=ground==game::Ground::Road?omega_art::SharedRoad:
                         ground==game::Ground::Rubble?omega_art::SharedRock:
                         ground==game::Ground::Wall?
                         ((x+y)%3?omega_art::SharedConcrete:omega_art::SharedSteel):
                         ground==game::Ground::DeepWater?omega_art::Water:
                         verdant?omega_art::SharedGrass:
                         wasted?((x+y)%4==0?omega_art::SharedRock:omega_art::SharedSand):
                         ((x+y)%5==0?omega_art::SharedSand:omega_art::SharedGrass);
        if(!s->sprites.Place(sprite,wx,wy,kCell*.965f,kCell*.965f,kBoardZ)){
            Rect(s,wx,wy,kCell*.965f,kCell*.965f,kBoardZ,r,g,b);
            if(ground==game::Ground::Rubble)
                Line(s,wx-kCell*.20f,wy-kCell*.13f,wx+kCell*.15f,wy+kCell*.13f,
                     kCell*.15f,kBoardZ+.008f,.64f,.55f,.47f);
            if(ground==game::Ground::Wall)
                Rect(s,wx,wy,kCell*.77f,kCell*.74f,kBoardZ+.015f,.36f,.39f,.39f);
        }
        // Low-poly deadwood marks occasional existing rubble/cover cells.
        // This decor never becomes phantom collision or an opaque UI overlay.
        if(ground==game::Ground::Rubble && (x*7+y*11)%13==0)
            s->sprites.Place(omega_art::DeadTree,wx,wy,kCell*.83f,
                             kCell*.83f,kBoardZ+.004f);
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
    // The inherited structure health represents a real pump in the authored
    // campaign missions, or the supply wagon in an intercepted field convoy.
    auto center=[](int x,int y,float &wx,float &wy){
        wx=(x+.5f)*kCell-kHalf;wy=kCenterY+kHalf-(y+.5f)*kCell;
    };
    float px=0,py=0;center(2,12,px,py);
    // PSX barrel = physical pump; cargo crate = convoy supply socket.
    // Successful sprites replace the large colored placeholder rectangles.
    const bool centerAsset=s->sprites.Place(
        defendingConvoy?omega_art::SupplyCrate:omega_art::MetalBarrel,
        px,py,kCell*.88f,kCell*.88f,kBoardZ+.006f);
    if(!centerAsset)
        Rect(s,px,py,kCell*.76f,kCell*.74f,-1.85f,
             defendingConvoy?.69f:defendingOutpost?.50f:.22f,
             defendingConvoy?.44f:defendingOutpost?.52f:.74f,
             defendingConvoy?.20f:defendingOutpost?.55f:.87f);
    if(defendingOutpost){
        Rect(s,px,py+kCell*.22f,kCell*.90f,kCell*.13f,
             -1.843f,.25f,.29f,.34f);
    }
    if(defendingConvoy && !centerAsset){
        Rect(s,px-kCell*.23f,py-kCell*.31f,kCell*.18f,kCell*.18f,
             -1.84f,.15f,.13f,.12f);
        Rect(s,px+kCell*.23f,py-kCell*.31f,kCell*.18f,kCell*.18f,
             -1.84f,.15f,.13f,.12f);
    }else if(!defendingConvoy && !centerAsset)
        Rect(s,px,py+kCell*.18f,kCell*.36f,kCell*.15f,
             -1.84f,.75f,.90f,.95f);
    center(5,12,px,py);
    // Live ammunition stock belongs at the existing supply socket.
    if(!s->sprites.Place(omega_art::AmmoBox,px,py,
                         kCell*.82f,kCell*.82f,kBoardZ+.006f))
        Rect(s,px,py,kCell*.78f,kCell*.72f,-1.85f,.65f,.44f,.15f);
    Line(s,px-kCell*.25f,py,px+kCell*.25f,py,kCell*.065f,-1.84f,.95f,.77f,.34f);
    // Mission marker is at the actual game-logic target; never an inert UI
    // ornament. Capture needs occupation, relay needs a living engineer and
    // two finite demolition charges from the team's field components.
    if(battle.mission()!=game::Mission::HoldPump){
        center(battle.objective_x(),battle.objective_y(),px,py);
        const bool orchard=battle.mission()==game::Mission::CaptureOrchard;
        // The real campaign objective is either an orchard or enemy relay.
        // The original PSX meshes were baked to overhead 2D sprite PNGs.
        const int objectiveArt=orchard?omega_art::LivingTree:
                               omega_art::RelayMonitor;
        if(!s->sprites.Place(objectiveArt,px,py,kCell*.90f,
                             kCell*.90f,kBoardZ+.007f)){
            Rect(s,px,py,kCell*.94f,kCell*.94f,-1.857f,
                 orchard?.91f:.65f,orchard?.81f:.48f,orchard?.25f:.95f);
            Rect(s,px,py,kCell*.58f,kCell*.58f,-1.854f,
                 orchard?.24f:.37f,orchard?.56f:.24f,orchard?.20f:.64f);
        }
        for(int i=0;i<battle.objective_progress();++i){
            Rect(s,px-kCell*.27f+kCell*.16f*i,py-kCell*.27f,
                 kCell*.12f,kCell*.10f,-1.842f,.93f,.99f,.41f);
        }
    }
    s->unit_vertices=s->mesh.size()/6u;
    for(int i=0;i<battle.unit_count();++i){
        const game::Unit *u=battle.unit(i);
        if(!u||!u->alive())continue;
        center(u->x,u->y,px,py);
        if(i==s->session.tactical.selected){
            const float h=kCell*.45f;
            Line(s,px-h,py-h,px+h,py-h,kCell*.048f,-1.842f,1.f,.90f,.26f);
            Line(s,px+h,py-h,px+h,py+h,kCell*.048f,-1.842f,1.f,.90f,.26f);
            Line(s,px+h,py+h,px-h,py+h,kCell*.048f,-1.842f,1.f,.90f,.26f);
            Line(s,px-h,py+h,px-h,py-h,kCell*.048f,-1.842f,1.f,.90f,.26f);
        }
        // Only the armored Heavy class receives Kenney tank hull artwork:
        // never represent engineers, salvagers or infantry as tanks.
        int hull=omega_art::IndependentHull;
        if(u->side==game::Side::Hearthwell)hull=omega_art::HearthwellHull;
        else if(battle.doctrine()==game::Doctrine::Meridian)hull=omega_art::MeridianHull;
        else if(battle.doctrine()==game::Doctrine::Scrapline)hull=omega_art::ScraplineHull;
        else if(battle.doctrine()==game::Doctrine::LostNinth)hull=omega_art::LostNinthHull;
        const int sprite=u->role==game::Class::Heavy?
                         hull:SoldierTexture(*u,battle.doctrine());
        const bool facing_left=u->side==game::Side::Raiders;
        if(s->sprites.Place(sprite,px,py,kCell*.90f,kCell*.90f,-1.85f,
                            facing_left))
            SpriteEquipment(s,*u,px,py,battle.doctrine());
        else UnitSprite(s,*u,px,py,battle.doctrine());
    }
    Rect(s,0,1.054f,2.05f,.30f,-1.92f,.07f,.13f,.19f);
    Text(s,"OMEGA FRONT  HEARTHWELL",1.076f,.013f,.91f,.93f,.78f);
    char status[96];
    if(defendingConvoy)
        std::snprintf(status,sizeof(status),"ESCORT WAGON HP %d  AMMO %d  TICK %u",
                      battle.pump_hp(),battle.ammo_stock(),battle.ticks());
    else if(defendingOutpost)
        std::snprintf(status,sizeof(status),"DEFEND OUTPOST S%03d  HP %d  TICK %u",
            s->session.frontier.location()+1,
            battle.pump_hp(),battle.ticks());
    else if(frontlineBattle)
        std::snprintf(status,sizeof(status),"TAKE OUTPOST S%03d  HOLD %d/3  TICK %u",
            s->session.frontier.location()+1,
            battle.objective_progress(),battle.ticks());
    else if(battle.mission()==game::Mission::CaptureOrchard) {
        const int op=s->session.frontier.operation();
        const char *site=op==4?"CHARTER GATE":op==5?"CARAVAN FORD":"ORCHARD";
        std::snprintf(status,sizeof(status),"HOLD %s %d/3  TICK %u",
                      site,battle.objective_progress(),battle.ticks());
    }
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
    if(battle.result()==game::Result::Defended)
        std::snprintf(action,sizeof(action),defendingConvoy?
                      "WAGON SAVED  CLICK REPORT":defendingOutpost?
                      "OUTPOST HELD  CLICK REPORT":frontlineBattle?
                      "OUTPOST TAKEN  CLICK REPORT":"DEFENDED  CLICK REPORT");
    else if(battle.result()==game::Result::Overrun)
        std::snprintf(action,sizeof(action),defendingConvoy?
                      "CONVOY LOST  CLICK REPORT":defendingOutpost?
                      "OUTPOST LOST  CLICK REPORT":frontlineBattle?
                      "ASSAULT LOST  CLICK REPORT":"OVERRUN  CLICK REPORT");
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
    s->sprites.Initialize(s->host);
    Draw(s);s->upload=true;
    return 1;
}
void GlEnd(void *p){
    State *s=static_cast<State *>(p);if(!s)return;
    if(s->vbo)glDeleteBuffers(1,&s->vbo);
    if(s->program)glDeleteProgram(s->program);
    s->sprites.Shutdown();
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
    const game::Result old_result=s->session.tactical.battle.result();
    if(s->session.Input(action)){
        const uint32_t tick=s->session.tactical.battle.ticks();
        const uint32_t new_orders=s->session.orders+s->session.tactical.issued;
        // The native sound bank is host-owned and mixed only for actual
        // simulation transitions during input/update, never per VR eye.
        if(old_view!=game::FrontMode::Tactical &&
           s->session.view==game::FrontMode::Tactical) {
            ReverieNativeRequestSoundV1(s->host,REVERIE_NATIVE_SOUND_WAVE_START);
        }else if(old_view==game::FrontMode::Tactical &&
                 s->session.view==game::FrontMode::Tactical) {
            const game::Battle &battle=s->session.tactical.battle;
            const uint32_t shot=reverie::omega_audio::TickFireCue(battle,old_tick);
            if(shot && !ReverieNativeRequestSoundV1(s->host,shot))
                ReverieNativeRequestFeedbackV1(s->host,
                                                 REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE);
            const uint32_t outcome=reverie::omega_audio::OutcomeCue(
                old_result,battle.result());
            if(outcome)ReverieNativeRequestSoundV1(s->host,outcome);
        }
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
    GLint src_rgb=GL_ONE,dst_rgb=GL_ZERO,src_alpha=GL_ONE,dst_alpha=GL_ZERO;
    glGetIntegerv(GL_BLEND_SRC_RGB,&src_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB,&dst_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,&src_alpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA,&dst_alpha);
    float matrix[16];ReverieNativeMat4Multiply(matrix,eye->projection,eye->view);
    glDisable(GL_CULL_FACE);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
    glBindBuffer(GL_ARRAY_BUFFER,s->vbo);
    if(s->upload){
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(s->mesh.size()*sizeof(float)),
                     s->mesh.data(),GL_DYNAMIC_DRAW);
        s->upload=false;
    }
    auto colored=[&](size_t first,size_t last){
        if(first>=last)return;
        glDisable(GL_BLEND);
        glUseProgram(s->program);glBindBuffer(GL_ARRAY_BUFFER,s->vbo);
        glEnableVertexAttribArray(static_cast<GLuint>(s->pos));
        glVertexAttribPointer(static_cast<GLuint>(s->pos),3,GL_FLOAT,GL_FALSE,
                              6*sizeof(float),reinterpret_cast<const void *>(0));
        glEnableVertexAttribArray(static_cast<GLuint>(s->color));
        glVertexAttribPointer(static_cast<GLuint>(s->color),3,GL_FLOAT,GL_FALSE,
                              6*sizeof(float),reinterpret_cast<const void *>(3*sizeof(float)));
        glUniformMatrix4fv(s->mvp,1,GL_FALSE,matrix);
        glDrawArrays(GL_TRIANGLES,static_cast<GLint>(first),
                     static_cast<GLsizei>(last-first));
    };
    auto sprites=[&](int first,int last){
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        s->sprites.DrawRange(matrix,first,last);
    };
    const size_t total=s->mesh.size()/6u;
    if(s->session.view==game::FrontMode::Tactical){
        colored(0,s->base_vertices);
        sprites(0,omega_art::kGroundCount);
        // After ground, objective/cover overlays must not erase the real
        // PSX props. Then draw actors over props, and UI over both.
        colored(s->base_vertices,s->unit_vertices);
        sprites(omega_art::kGroundCount,omega_art::kTerrainCount);
        sprites(omega_art::kTerrainCount,omega_art::Count);
        colored(s->unit_vertices,total);
    }else{
        // Front map is the initial screen, not an unrelated all-geometry
        // placeholder. Preserve text, fog-of-war, selection and click zones.
        colored(0,s->base_vertices);
        sprites(0,omega_art::kGroundCount);
        colored(s->base_vertices,total);
        // Resource art sits in the surveyed sector corner, above terrain
        // and below none of the central sector labels.
        sprites(omega_art::kGroundCount,omega_art::kTerrainCount);
    }
    s->sprites.upload=false;
    glBlendFuncSeparate(src_rgb,dst_rgb,src_alpha,dst_alpha);
    const GLenum error=glGetError();
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
