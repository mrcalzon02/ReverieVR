#include "breakwater_campaign.h"
#include "breakwater_checkpoint.h"
#include "breakwater_silhouettes.h"
#include "breakwater_sprite_atlas.h"
#include "breakwater_terrain.h"
#include "breakwater_cover.h"
#include "breakwater_sky.h"
#include "breakwater_damage_feedback.h"
#include "breakwater_aircraft_feedback.h"
#include "breakwater_surface_aim.h"
#include "breakwater_combat_input.h"
#include "breakwater_pointer_focus.h"
#include "breakwater_impacts.h"
#include "breakwater_wrecks.h"
#include "breakwater_gun_mesh.h"
#include "breakwater_menu_layout.h"
#include "breakwater_shop_feedback.h"
#include "breakwater_scene_refresh.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include "reverie_native_vector_text.h"

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
    GLuint sprite_program = 0, sprite_vbo = 0, sprite_texture = 0;
    GLint sprite_matrix = -1, sprite_position = -1, sprite_uv = -1;
    GLint sprite_sampler = -1, sprite_tint = -1;
    bool sprite_gpu_dirty = true;
    bool held = false, secondary_held = false, scene_dirty = true, gpu_dirty = true;
    float flash = 0.0f, elapsed = 0.0f, reload_ready_flash = 0.0f;
    float turret_flash = 0.0f, infantry_flash = 0.0f;
    float gunboat_salvo_flash = 0.0f, gunboat_salvo_x = 0.0f;
    float gunboat_salvo_z = 0.0f;
    uint64_t infantry_firing_mask=0u;
    float gun_direction[3] = {0.0f,0.0f,-1.0f};
    float shot_direction[3] = {0.0f,0.0f,-1.0f};
    ImpactEffects impacts;
    WreckEffects wrecks;
    BatteryDamageFeedback battery_damage;
    AircraftAimFeedback aircraft_aim;
    SurfaceAimFeedback surface_aim;
    int cover_aim_index=-1;
    AircraftHitFeedback aircraft_hit;
    CombatInputLatch combat_input;
    PointerFocusGate pointer_focus;
    bool armory_page=false, sam_armed=false, ghost_on=false;
    int menu_focus=-1;
    bool pointer_visible=false;
    float ghost_position[3]={}, shot_origin[3]={}, shot_gravity=0;
    std::vector<float> mesh;
    std::vector<float> sprite_mesh;
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
void Text(State *s,const char *text,float x,float y,float z,float scale,
          float r,float g,float b) {
    reverie::nativeart::StrokeText(s->mesh,text,x,y,scale,r,g,b,z);
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
int DifficultyTarget(const ReverieNativeInputV1 &input) {
    float x=0.f,y=0.f;
    if(AimAtPlane(input,kShopZ,&x,&y)<0.f)return -1;
    return menu::HitDifficulty(x,y);
}
int ShopTarget(const ReverieNativeInputV1 &input,bool armory) {
    float x=0.f,y=0.f;
    if(AimAtPlane(input,kShopZ,&x,&y)<0.f)return -1;
    const int target=menu::HitShop(x,y);
    // Armory only has five guns plus SAM: empty 6/7 cells are not buttons.
    return armory && target>=6 && target<8 ? -1 : target;
}
int ShopDisplayPrice(const State *s,int index) {
    if(!s || index<0 || index>=8) return -1;
    const Campaign &game=s->campaign;
    if(!s->armory_page)
        return game.price(static_cast<Upgrade>(index));
    if(index==5) return game.sam_charges()<3?540:-1;
    if(index<kWeaponCount) {
        const WeaponKind weapon=static_cast<WeaponKind>(index);
        return game.owns_weapon(weapon)?-1:WeaponFor(weapon).purchase_cost;
    }
    return -1;
}
bool AimAtEnemy(const ReverieNativeInputV1 &input,const Enemy &enemy,
                float gravity,bool ghost,float *distance) {
    if(!enemy.active)return false;
    const float center[]={enemy.x,
        enemy.type==UnitType::Aircraft?1.6f:-.63f,enemy.z};
    const float radius=enemy.type==UnitType::Rowboat?.62f:
         enemy.type==UnitType::LandingCraft?.82f:.78f;
    return ghost?RaySphereSurfaceDistance(input.pointer_origin,
        input.pointer_direction,center,radius,distance):
        ProjectileHitsSphere(input.pointer_origin,input.pointer_direction,
                             gravity,center,radius,distance);
}
bool GunWithinTraverse(const float *direction,WeaponKind weapon) {
    return WeaponWithinTraverse(direction,weapon);
}
int CombatTarget(const ReverieNativeInputV1 &input,const Campaign &game,
                 bool ghost=false) {
    if(input.pointer_kind==REVERIE_NATIVE_POINTER_NONE ||
       (!ghost && !GunWithinTraverse(input.pointer_direction,
                                    game.selected_weapon())))return -1;
    float best=1e9f;int index=-1;
    for(uint8_t i=0;i<game.enemies().size();i++) {
        float depth=0;
        if(AimAtEnemy(input,game.enemies()[i],
                      game.stats().projectile_gravity,ghost,&depth) && depth<best){
            best=depth; index=i;
        }
    }
    if(WeaponFor(game.selected_weapon()).anti_infantry && !ghost)
        for(uint8_t i=0;i<game.infantry_units().size();i++) {
            const Infantry &trooper=game.infantry_units()[i];
            if(!trooper.active)continue;
            const float center[]={trooper.x,-.66f,trooper.z};
            float depth=0;
            if(ProjectileHitsSphere(input.pointer_origin,input.pointer_direction,
                                    game.stats().projectile_gravity,
                                    center,.30f,&depth) && depth<best) {
                best=depth;
                index=static_cast<int>(Campaign::kMaxEnemies)+i;
            }
        }
    // Physical cover occludes only when its exact ballistic hit is closer.
    if(!ghost) {
        float wallDepth=0.0f;
        const int wall=CoverTarget(game,input.pointer_origin,
            input.pointer_direction,game.stats().projectile_gravity,
            &wallDepth);
        if(wall>=0 && wallDepth<best)
            index=kCoverTargetBase+wall;
    }
    return index;
}
bool SaveState(State *s) {
    if(!ReverieNativeHostSupportsSaveV1(s->host))return false;
    uint8_t bytes[Campaign::kSerializedSize]={};
    if(!s->campaign.SerializeShop(bytes,sizeof(bytes)))return false;
    return s->host->write_save(kCheckpointSlot,bytes,
        static_cast<uint32_t>(sizeof(bytes)))==REVERIE_NATIVE_SAVE_OK;
}
void LoadState(State *s) {
    if(!ReverieNativeHostSupportsSaveV1(s->host))return;
    uint8_t bytes[Campaign::kSerializedSize]={};
    uint32_t size=0;
    const int32_t status=s->host->read_save(kCheckpointSlot,bytes,
        static_cast<uint32_t>(sizeof(bytes)),&size);
    if(status==REVERIE_NATIVE_SAVE_NOT_FOUND || IsClearedCheckpoint(status,size))return;
    if(status!=REVERIE_NATIVE_SAVE_OK || size!=sizeof(bytes) ||
       !s->campaign.DeserializeShop(bytes,size)) {
        ReverieNativeLogV1(s->host,REVERIE_NATIVE_LOG_WARN,"Breakwater",
            "Save rejected; keeping fresh campaign instead of resetting partially.");
    }
}
void BallisticWaterImpact(State *s,const ReverieNativeInputV1 &input,
                          float gravity) {
    if(input.pointer_direction[2]>=-.0001f)return;
    float prev[3]={},at[3]={};
    if(!ProjectilePosition(input.pointer_origin,input.pointer_direction,
                           .01f,gravity,prev))return;
    for(float t=.26f;t<=39.76f;t+=.25f) {
        if(!ProjectilePosition(input.pointer_origin,input.pointer_direction,
                               t,gravity,at))return;
        if(prev[1]>-1.10f && at[1]<=-1.10f) {
            const float down[]={0.0f,-1.0f,0.0f};
            s->impacts.RecordHit(at,down,.001f,ImpactKind::Water);
            return;
        }
        for(int i=0;i<3;i++)prev[i]=at[i];
    }
}
void Interact(State *s,const ReverieNativeInputV1 &input,bool edge) {
    Campaign &game=s->campaign;
    if(game.phase()==Phase::Briefing) {
        if(!edge)return;
        const int choice=DifficultyTarget(input);
        if(choice>=0 && choice<4) {
            game=Campaign(static_cast<Difficulty>(choice),1049u);
            s->impacts=ImpactEffects{};
            s->wrecks.Clear();
            s->battery_damage=BatteryDamageFeedback{};
            s->reload_ready_flash=0.0f;
            s->turret_flash=0.0f;
            s->gunboat_salvo_flash=0.0f;
            s->aircraft_aim={};s->surface_aim={};s->cover_aim_index=-1;
            s->aircraft_hit=AircraftHitFeedback{};
            s->sam_armed=false;s->armory_page=false;s->ghost_on=false;
            game.Start();
            SaveState(s);
            Feedback(s,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        } else Feedback(s,REVERIE_NATIVE_FEEDBACK_FAILURE);
    } else if(game.phase()==Phase::Shop) {
        if(!edge)return;
        const int target=ShopTarget(input,s->armory_page);
        bool done=false;
        if(target==9) {
            s->armory_page=!s->armory_page;
            done=true;
        } else if(target==8) {
            done=game.BeginWave();
            if(done) {
                s->combat_input.CaptureUiPress();
                s->wrecks.Clear();
                s->battery_damage=BatteryDamageFeedback{};
                s->infantry_flash=0.0f;
                s->infantry_firing_mask=0u;
                s->gunboat_salvo_flash=0.0f;
                s->aircraft_aim={};s->surface_aim={};s->cover_aim_index=-1;
                s->aircraft_hit=AircraftHitFeedback{};
                s->sam_armed=false;
            }
        }
        else if(target>=0 && target<8){
            if(s->armory_page) {
                if(target<kWeaponCount)
                    done=game.PurchaseWeapon(static_cast<WeaponKind>(target));
                else if(target==5)done=game.PurchaseSam();
            } else done=game.Purchase(static_cast<Upgrade>(target));
        }
        if(done && game.phase()==Phase::Shop)SaveState(s);
        Feedback(s,done?REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
    } else if(game.phase()==Phase::Combat) {
        float x=0,y=0;
        const CombatHudTarget hud = AimAtPlane(input,-1.8f,&x,&y)>0 ?
            HitCombatSupportHud(x,y) : CombatHudTarget::None;
        // Capture support presses until release, even if the aim drifts off
        // the HUD. Shop START presses also remain captured across phases.
        const CombatInputRoute route = s->combat_input.Route(hud,edge);
        if(route==CombatInputRoute::NoAction)return;
        if(route>=CombatInputRoute::Weapon0 &&
           route<=CombatInputRoute::Weapon4) {
            const WeaponKind kind=static_cast<WeaponKind>(
                static_cast<uint8_t>(route)-static_cast<uint8_t>(CombatInputRoute::Weapon0));
            const bool ok=game.SelectWeapon(kind);
            if(ok)s->sam_armed=false;
            Feedback(s,ok?REVERIE_NATIVE_FEEDBACK_ACTIVATION:
                          REVERIE_NATIVE_FEEDBACK_FAILURE);
            return;
        }
        if(route==CombatInputRoute::Sam) {
            const bool ok=game.sam_charges()>0 || s->sam_armed;
            if(ok)s->sam_armed=!s->sam_armed;
            Feedback(s,ok?REVERIE_NATIVE_FEEDBACK_ACTIVATION:
                          REVERIE_NATIVE_FEEDBACK_FAILURE);
            return;
        }
        if(route==CombatInputRoute::Artillery){
            const auto before=game.enemies();
            const bool ok=game.CallArtillery();
            if(ok) {
                s->impacts.RecordSupportDamage(before,game.enemies(),false);
                s->wrecks.RecordTransitions(before,game.enemies());
            }
            Feedback(s,ok?REVERIE_NATIVE_FEEDBACK_ARTILLERY:REVERIE_NATIVE_FEEDBACK_FAILURE);
            return;
        }
        if(route==CombatInputRoute::Airstrike){
            const auto before=game.enemies();
            const bool ok=game.CallAirstrike();
            if(ok) {
                s->impacts.RecordSupportDamage(before,game.enemies(),true);
                s->wrecks.RecordTransitions(before,game.enemies());
            }
            Feedback(s,ok?REVERIE_NATIVE_FEEDBACK_AIRSTRIKE:REVERIE_NATIVE_FEEDBACK_FAILURE);
            return;
        }
        if(s->sam_armed) {
            // SAM targeting is one attempt per Select press. Capture both
            // valid launches and rejected locks so a held trigger cannot
            // launch after aim drift or spill into ordinary gunfire.
            if(!s->combat_input.CaptureOneShotPress(edge))return;
            const int lock=CombatTarget(input,game,true);
            const bool valid=lock>=0 &&
                lock<static_cast<int>(Campaign::kMaxEnemies) &&
                game.enemies()[lock].type==UnitType::Aircraft;
            const Enemy source=valid?game.enemies()[lock]:Enemy{};
            const bool ok=valid && game.CallSam(static_cast<uint8_t>(lock));
            if(ok) {
                const float origin[]={source.x,1.6f,source.z};
                const float down[]={0.0f,-1.0f,0.0f};
                s->impacts.RecordHit(origin,down,.001f,ImpactKind::AirstrikeBlast);
                s->aircraft_hit.Record(source,game.enemies()[lock],true);
                s->wrecks.RecordDestroyed(source,game.enemies()[lock]);
                s->sam_armed=false;
            }
            Feedback(s,ok?REVERIE_NATIVE_FEEDBACK_ACTIVATION:
                          REVERIE_NATIVE_FEEDBACK_FAILURE);
            return;
        }
        const bool missile=WeaponFor(game.selected_weapon()).uses_ghost;
        const int target=CombatTarget(input,game,missile);
        // Resolve the impact before a lethal hit deactivates the target.
        float impactDepth=0.0f;
        const bool enemyTarget=target>=0 &&
            target<static_cast<int>(Campaign::kMaxEnemies);
        const bool infantryTarget=target>=static_cast<int>(
            Campaign::kMaxEnemies) && target<kCoverTargetBase;
        const bool coverTarget=target>=kCoverTargetBase &&
            target<kCoverTargetEnd;
        const int16_t targetHp=enemyTarget?game.enemies()[target].hp:0;
        const Enemy enemyBefore=enemyTarget?game.enemies()[target]:Enemy{};
        const float gravity=game.stats().projectile_gravity;
        if(enemyTarget)
            AimAtEnemy(input,game.enemies()[target],gravity,missile,&impactDepth);
        else if(infantryTarget) {
            const Infantry &trooper=game.infantry_units()[
                target-Campaign::kMaxEnemies];
            const float center[]={trooper.x,-.66f,trooper.z};
            ProjectileHitsSphere(input.pointer_origin,input.pointer_direction,
                                 gravity,center,.30f,&impactDepth);
        } else if(coverTarget) {
            ProjectileHitsCover(input.pointer_origin,
                input.pointer_direction,gravity,
                game.shore_cover()[target-kCoverTargetBase],&impactDepth);
        }
        bool shot=false;
        if(target>=0 && target<static_cast<int>(Campaign::kMaxEnemies))
            shot=game.FireAt(static_cast<uint8_t>(target));
        else if(infantryTarget)
            shot=game.FireAtInfantry(static_cast<uint8_t>(
                target-Campaign::kMaxEnemies));
        else if(coverTarget)
            shot=game.FireAtCover(static_cast<uint8_t>(
                target-kCoverTargetBase));
        else if(GunWithinTraverse(input.pointer_direction,
                                  game.selected_weapon()))
            shot=game.FireMiss();
        if(shot){
            if(enemyTarget) {
                s->aircraft_hit.Record(enemyBefore,game.enemies()[target],true);
                s->wrecks.RecordDestroyed(enemyBefore,game.enemies()[target]);
            }
            float hit[3]={};
            if(impactDepth>0.0f &&
               ProjectilePosition(input.pointer_origin,input.pointer_direction,
                                  impactDepth,missile?0.0f:gravity,hit)) {
                const float down[]={0.0f,-1.0f,0.0f};
                if(enemyTarget) {
                    if(game.enemies()[target].hp<targetHp)
                        s->impacts.RecordHit(hit,down,.001f,
                            game.enemies()[target].active?
                                ImpactKind::Metal:ImpactKind::Explosion);
                } else if(infantryTarget || coverTarget)
                    s->impacts.RecordHit(hit,down,.001f,
                        coverTarget && game.location()==1u ?
                            ImpactKind::Metal:ImpactKind::Sand);
            } else if(target<0 && !missile)
                BallisticWaterImpact(s,input,gravity);
            for(int k=0;k<3;k++) {
                s->shot_direction[k]=input.pointer_direction[k];
                s->shot_origin[k]=input.pointer_origin[k];
            }
            s->shot_gravity=missile?0.0f:gravity;
            s->flash=.09f;
            // A successful shot emits exactly one typed, ordered audio
            // event. Older ABI-v1 hosts continue using the original report.
            const uint32_t cue=REVERIE_NATIVE_SOUND_WEAPON_MACHINE+
                static_cast<uint32_t>(game.selected_weapon());
            if(!ReverieNativeRequestSoundV1(s->host,cue))
                Feedback(s,REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE);
            if(game.reloading())
                Feedback(s,REVERIE_NATIVE_FEEDBACK_RELOAD_START);
        } else if(edge)Feedback(s,REVERIE_NATIVE_FEEDBACK_FAILURE);
    } else if(edge && (game.phase()==Phase::Defeat || game.phase()==Phase::Victory)) {
        // A restart must not resurrect the old shop checkpoint after app exit.
        // Keep the in-memory restart usable on hosts without save support.
        const bool checkpoint_cleared =
            !ReverieNativeHostSupportsSaveV1(s->host) || ClearCheckpoint(s->host);
        if(!checkpoint_cleared)
            ReverieNativeLogV1(s->host,REVERIE_NATIVE_LOG_WARN,"Breakwater",
                "Restarted in memory, but clearing the old checkpoint failed.");
        game=Campaign(Difficulty::Regular,1049u);
        s->impacts=ImpactEffects{};
        s->wrecks.Clear();
        s->battery_damage=BatteryDamageFeedback{};
        s->reload_ready_flash=0.0f;
        s->turret_flash=0.0f;
        s->infantry_flash=0.0f;
        s->infantry_firing_mask=0u;
        s->gunboat_salvo_flash=0.0f;
        s->aircraft_aim={};s->surface_aim={};s->cover_aim_index=-1;
        s->aircraft_hit=AircraftHitFeedback{};
        s->sam_armed=false;s->armory_page=false;s->ghost_on=false;
        Feedback(s,checkpoint_cleared?REVERIE_NATIVE_FEEDBACK_ACTIVATION:
                          REVERIE_NATIVE_FEEDBACK_FAILURE);
    }
}
void BuildScene(State *s) {
    auto &game=s->campaign;
    s->mesh.clear();
    s->sprite_mesh.clear();
    const bool night=game.period()==Period::Night;
    // 360-degree layered horizon: no frontal backdrop edges when head turns.
    // Fixed panorama, clouds, sun/moon and distant smoke are shared by both
    // eyes and stay farther away than every gameplay target and sea strip.
    EmitSkyPanorama(game.location(),night,
        [s](auto... arguments) { Triangle(s,arguments...); });
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
    // World-space shore geometry is generated alongside the existing
    // simulation mesh. The beach ends at the real transport landing line;
    // only Campaign owns targets, collision and landfall behavior.
    EmitCoastalTerrain(game.location(),night,
        [s](auto... arguments) { Box(s,arguments...); },
        [s](auto... arguments) { Triangle(s,arguments...); });
    for(const ShoreCover &barrier:game.shore_cover())
        EmitShoreCover(game.location(),barrier,night,
            [s](auto... arguments) { Box(s,arguments...); });

    // Near-field gun geometry must NOT block world-space menus at z=-3.
    // The old parapet covered the credits line despite correct text/layout.
    // Show the actual 3D carriage while fighting; menu phases stay legible.
    if(game.phase()==Phase::Combat) {
        // The shore platform, breech, collars, recessed muzzle and wheel
        // meshes are one deterministic XYZRGB triangle batch per scene.
        Box(s,0,-1.24f,-.95f,7.8f,.55f,.7f,.37f,.38f,.36f);
        Box(s,0,-1.06f,-.88f,.62f,.22f,.47f,.29f,.30f,.30f);
        const float *aim=s->gun_direction;
        const WeaponKind equipped=game.selected_weapon();
        gunmesh::Build([s](auto... vertices){Triangle(s,vertices...);},
                       equipped,aim,s->flash);
        if(s->flash>0.0f)
            gunmesh::Flash([s](auto... vertices){Triangle(s,vertices...);},
                           equipped,aim,s->flash);
    }
    if(s->flash>0.0f) {
        const float *shot=s->shot_direction;
        const float shotHorizontal=std::sqrt(shot[0]*shot[0]+shot[2]*shot[2]);
        const float shotRightX=shotHorizontal>.01f?-shot[2]/shotHorizontal:1.0f;
        const float shotRightZ=shotHorizontal>.01f?shot[0]/shotHorizontal:0.0f;
        for(int segment=0;segment<9;segment++) {
            const float t=1.2f+segment*1.3f;
            float a[3]={},b[3]={};
            if(!ProjectilePosition(s->shot_origin,shot,t,s->shot_gravity,a) ||
               !ProjectilePosition(s->shot_origin,shot,t+1.3f,s->shot_gravity,b))
                break;
            const float w=.018f-.001f*segment;
            Triangle(s,a[0]+shotRightX*w,a[1],a[2]+shotRightZ*w,
                     a[0]-shotRightX*w,a[1],a[2]-shotRightZ*w,
                     b[0]+shotRightX*w*.7f,b[1],b[2]+shotRightZ*w*.7f,
                     .98f,.72f,.25f);
            Triangle(s,b[0]+shotRightX*w*.7f,b[1],b[2]+shotRightZ*w*.7f,
                     a[0]-shotRightX*w,a[1],a[2]-shotRightZ*w,
                     b[0]-shotRightX*w*.7f,b[1],b[2]-shotRightZ*w*.7f,
                     .98f,.72f,.25f);
        }
    }
    if(s->ghost_on) {
        const float x=s->ghost_position[0],y=s->ghost_position[1],
                    z=s->ghost_position[2];
        const float r=s->sam_armed?.96f:.52f;
        const float g=s->sam_armed?.84f:.94f;
        const float b=s->sam_armed?.34f:1.0f;
        const float w=.13f;
        Triangle(s,x-w,y,z,x,y+w,z,x+w,y,z,r,g,b);
        Triangle(s,x-w,y,z,x,y-w,z,x+w,y,z,r*.68f,g*.68f,b*.68f);
        Box(s,x,y,z,.025f,.025f,.025f,1,1,1);
    }
    if(game.stats().turret) {
        Box(s,2.15f,-.76f,-1.40f,.42f,.32f,1.14f,.36f,.44f,.39f);
        if(s->turret_flash>0.0f)
            Box(s,2.15f,-.76f,-2.06f,.20f,.20f,.18f,1.0f,.77f,.32f);
    }
    // One atlas card per live enemy: the campaign continues owning all
    // movement, world positions, hit volumes, attacks, landfall and score.
    // Forward-facing world cards use the SAME mesh for both stereo eyes.
    for(const Enemy &e:game.enemies())if(e.active) {
        // Ship ramp/oars, gunboat flashes and bank-aware aircraft art are
        // selected purely from the live simulation pose, with no new
        // animation clocks or per-eye scene changes.
        const SpriteId sprite=EnemySprite(e);
        const bool aircraft=e.type==UnitType::Aircraft;
        float width=.99f,height=.71f,y=-.66f,bank=0.0f;
        if(e.type==UnitType::LandingCraft) {
            width=1.24f;height=.86f;y=-.70f;
        } else if(e.type==UnitType::Gunboat) {
            width=1.37f;height=.96f;y=-.68f;
        } else if(aircraft) {
            const bool bomber=e.aircraft_pattern==AircraftPattern::Bomber;
            width=bomber?2.55f:2.02f;
            height=bomber?1.35f:1.08f;
            y=1.6f;
            bank=std::isfinite(e.bank)?e.bank:0.0f;
        }
        AppendSprite(s->sprite_mesh,sprite,e.x,y,e.z,width,height,bank);
    }
    if(s->surface_aim.visible) {
        // Same exact CombatTarget ray as weapon damage. These small world-
        // space corner marks are UI only; they do not widen the hit sphere.
        const SurfaceAimFeedback &target=s->surface_aim;
        const float x=target.x,z=target.z+.045f;
        const float span=target.half_width;
        const float top=target.infantry?-.31f:-.20f;
        const float bottom=target.infantry?-.99f:-1.02f;
        const float red=target.vulnerable?.47f:1.0f;
        const float green=target.vulnerable?.92f:.53f;
        const float blue=target.vulnerable?.53f:.27f;
        for(int side : {-1,1}) {
            const float edge=x+side*span;
            const float tip=edge-side*.105f;
            Triangle(s,edge,top,z,tip,top,z,edge,top-.13f,z,
                     red,green,blue);
            Triangle(s,edge,bottom,z,tip,bottom,z,edge,bottom+.13f,z,
                     red,green,blue);
        }
        if(!target.infantry) {
            // A two-triangle status strip reports actual remaining hull,
            // without adding fonts over tiny moving boat silhouettes.
            const float fraction=SurfaceHullFraction(target);
            const float width=span*1.80f;
            const float barY=top+.14f;
            Panel(s,x,barY,z+.004f,width,.065f,.10f,.13f,.14f);
            if(fraction>0.0f) {
                const float fill=width*fraction;
                Panel(s,x-width*.5f+fill*.5f,barY,z+.008f,
                      fill,.044f,red,green,blue);
            }
        }
    }
    if(s->aircraft_aim.visible) {
        // Four tiny world-space brackets surround only the exact ray-selected
        // moving aircraft. The bracket is NOT aim assistance or a wide hitbox.
        const float x=s->aircraft_aim.x,z=s->aircraft_aim.z;
        const float span=s->aircraft_aim.pattern==AircraftPattern::Bomber?
            1.49f:1.18f;
        const float r=s->aircraft_aim.equipped?.36f:1.0f;
        const float g=s->aircraft_aim.equipped?1.0f:.67f;
        const float b=s->aircraft_aim.equipped?.58f:.23f;
        for(int side : {-1,1}) {
            const float edge=x+static_cast<float>(side)*span;
            Triangle(s,edge,1.91f,z,edge,1.72f,z,
                     edge-static_cast<float>(side)*.15f,1.91f,z,r,g,b);
            Triangle(s,edge,1.28f,z,edge,1.47f,z,
                     edge-static_cast<float>(side)*.15f,1.28f,z,r,g,b);
        }
    }
    // A destroyed enemy is already gone from the authoritative simulation,
    // but its last recorded position carries a brief 3D wreck silhouette.
    // Shared scene geometry is built once, identically for both eyes.
    for(const WreckPulse &wreck:s->wrecks.pulses())
        if(wreck.remaining>0.0f)
            EmitWreckFragments(wreck,[s](auto... vertices) {
                Triangle(s,vertices...);
            });
    // CPU-updated, fixed-capacity impact pulses render identically in both eyes.
    for(const ImpactPulse &impact:s->impacts.pulses())if(impact.remaining>0.0f) {
        const bool artillery=impact.kind==ImpactKind::ArtilleryBlast;
        const bool airstrike=impact.kind==ImpactKind::AirstrikeBlast;
        const bool support=artillery || airstrike;
        const bool explosion=impact.kind==ImpactKind::Explosion || support;
        const float lifetime=ImpactEffects::LifetimeFor(impact.kind);
        const float progress=std::max(0.0f,1.0f-impact.remaining/lifetime);
        const float size=support?.30f+.83f*progress:
                         explosion?.19f+.54f*progress:.07f+.17f*progress;
        const float x=impact.x,y=impact.y,z=impact.z+.035f;
        if(explosion) {
            // Three crossed, expanding vertex-color blast petals darken to smoke.
            // No textures, heap particles or per-eye simulation updates.
            const float fade=1.0f-progress;
            // Artillery burns amber at the waterline; airstrikes flash pale
            // around aircraft as well as ships, then fade into dark smoke.
            const float r=.20f+(airstrike?.70f:.76f)*fade;
            const float g=.16f+(airstrike?.69f:artillery?.48f:.46f)*fade;
            const float b=.12f+(airstrike?.72f:artillery?.14f:.12f)*fade;
            Triangle(s,x-size,y,z,x,y+size*.9f,z,x+size,y,z,r,g,b);
            Triangle(s,x,y-size*.7f,z,x+size*.8f,y,z,x,y+size*.7f,z,
                     r*.84f,g*.78f,b*.80f);
            Triangle(s,x,y-size*.65f,z-size*.6f,x,y+size*.85f,z,
                     x,y-size*.65f,z+size*.6f,r*.80f,g*.86f,b*.85f);
        } else if(impact.kind==ImpactKind::Water) {
            Triangle(s,x-size,y+.01f,z-size,x+size,y+.01f,z-size,
                     x,y+size*1.5f,z,.67f,.85f,.96f);
            Triangle(s,x-size*.7f,y+.01f,z+size,x+size*.7f,y+.01f,z+size,
                     x,y+size*1.2f,z,.41f,.70f,.87f);
        } else {
            const bool sand=impact.kind==ImpactKind::Sand;
            const float r=sand?.79f:1.0f,g=sand?.67f:.73f,b=sand?.46f:.24f;
            Triangle(s,x-size,y,z,x,y+size,z,x+size,y,z,r,g,b);
            Triangle(s,x-size*.7f,y,z,x,y-size,z,x+size*.7f,y,z,
                     r*.75f,g*.75f,b*.75f);
        }
    }
    for(size_t slot=0;slot<game.infantry_units().size();++slot) {
        const Infantry &trooper=game.infantry_units()[slot];
        if(!trooper.active)continue;
        // The snapshot identifies actual ready shooters from a confirmed
        // volley. Only their existing 2D cards change to fire frames.
        const bool firing=s->infantry_flash>0.0f &&
            InfantryFiringPose(trooper,s->infantry_firing_mask,slot);
        const SpriteId pose=InfantrySprite(trooper,firing);
        AppendSprite(s->sprite_mesh,pose,trooper.x,-.67f,trooper.z,
                     .40f,.70f);
    }
    if(s->gunboat_salvo_flash>0.0f) {
        // A brief world-space tracer starts at the vessel that actually fired.
        // It persists if that vessel sinks before the next stereo render.
        const float x=s->gunboat_salvo_x,z=s->gunboat_salvo_z+.38f;
        Triangle(s,x-.035f,-.33f,z,x+.035f,-.33f,z,
                 0.0f,-.75f,-.94f,1.0f,.77f,.31f);
        Triangle(s,x,-.28f,z,0.0f,-.70f,-.94f,
                 0.0f,-.80f,-.94f,.95f,.56f,.22f);
    }
    // Infantry muzzle flashes live in the firing sprite tiles above;
    // do not draw a duplicate generic 3D flash at one arbitrary shooter.
    if(game.phase()==Phase::Briefing) {
        const menu::Rect frame=menu::kDifficultyPanel;
        Panel(s,frame.x,frame.y,-3.06f,frame.width,frame.height,
              .075f,.105f,.13f);
        Text(s,"BREAKWATER",0,1.34f,-2.97f,.078f,.97f,.87f,.60f);
        Text(s,"SELECT DIFFICULTY",0,.74f,-2.97f,.059f,.83f,.88f,.83f);
        const char *choices[]={"CADET","REGULAR","VETERAN","SIEGE"};
        for(int n=0;n<4;n++) {
            const menu::Rect tile=menu::DifficultyCard(n);
            const bool hover=s->menu_focus==n;
            Panel(s,tile.x,tile.y,-3.0f,tile.width,tile.height,
                  hover?.29f:.14f,hover?.40f:.21f,hover?.38f:.27f);
            Panel(s,tile.x,tile.y+tile.height*.45f,-2.99f,
                  tile.width-.08f,.014f,.77f,.77f,.61f);
            Text(s,choices[n],tile.x,tile.y,-2.96f,
                 menu::FittedScale(choices[n],tile.width,.048f),
                 hover?1.f:.90f,hover?.99f:.92f,.76f);
        }
        const char *difficulty_hint=menu::DifficultyHint(s->menu_focus);
        Text(s,difficulty_hint,0,.31f,-2.96f,
             menu::FittedScale(difficulty_hint,5.65f,.045f),
             .90f,.93f,.75f);
        Text(s,"POINT AND SELECT",0,-.91f,-2.96f,.049f,.86f,.88f,.79f);
    } else if(game.phase()==Phase::Shop) {
        const menu::Rect frame=menu::kPanel;
        Panel(s,frame.x,frame.y,-3.06f,frame.width,frame.height,
              .065f,.095f,.12f);
        Text(s,"BREAKWATER",0,1.46f,-2.97f,.069f,.95f,.84f,.59f);
        Text(s,Campaign::Locations()[game.location()].name,0,1.16f,-2.97f,
             .050f,.73f,.84f,.79f);
        char header[50];
        std::snprintf(header,sizeof(header),"%s  D%u  W%u/%u  %d CR",
            night?"NIGHT":"DAY",static_cast<unsigned>(game.day()),
            static_cast<unsigned>(game.wave_index()),
            static_cast<unsigned>(game.waves_this_period()),
            game.credits());
        Text(s,header,0,menu::kShopStatusY,-2.97f,
             menu::FittedScale(header,5.75f,.048f),.91f,.90f,.77f);
        // Focus-dependent inspection and purchase eligibility are
        // shown as real vector text; no extra tabs or inert details panel.
        const int focused=s->menu_focus;
        const bool focus_weapon=s->armory_page &&
            focused>=0 && focused<kWeaponCount;
        const WeaponKind focused_kind=focus_weapon?
            static_cast<WeaponKind>(focused):WeaponKind::MachineGun;
        const bool focus_owned=focus_weapon &&
            game.owns_weapon(focused_kind);
        const bool focus_active=focus_owned &&
            game.selected_weapon()==focused_kind;
        char hint[80]={};
        shopfeedback::Describe(hint,sizeof(hint),s->armory_page,focused,
            ShopDisplayPrice(s,focused),game.credits(),
            focus_owned,focus_active);
        Text(s,hint,0,menu::kShopHintY,-2.972f,
             menu::FittedScale(hint,5.65f,menu::kShopHintScale),
             .87f,.96f,.85f);
        // Everything interactive fits within the dark board and uses
        // exactly the same menu::Rect as the pointer hit-test.
        const char *labels[]={"FIRE","POWER","CLIP","ANTI","ARTY","AIR","TURRET","REPAIR"};
        for(int n=0;n<8;n++) {
            if(s->armory_page && n>=6)continue;
            const menu::Rect tile=menu::ShopCard(n);
            const float x=tile.x,y=tile.y;
            const WeaponKind weapon=static_cast<WeaponKind>(n);
            const bool equipment=s->armory_page && n<kWeaponCount;
            const bool owned=equipment && game.owns_weapon(weapon);
            const bool hover=s->menu_focus==n;
            const int price=ShopDisplayPrice(s,n);
            const bool unaffordable=price>game.credits();
            const float r=owned?.15f:price<0?.15f:
                          unaffordable?.23f:.13f;
            const float g=owned?.30f:price<0?.19f:
                          unaffordable?.16f:.22f;
            const float b=price<0?.20f:unaffordable?.18f:.24f;
            Panel(s,x,y,-3.0f,tile.width,tile.height,
                  r+(hover?.13f:0.f),g+(hover?.13f:0.f),
                  b+(hover?.12f:0.f));
            Panel(s,x,y+tile.height*.47f,-2.99f,
                  tile.width-.08f,.012f,
                  hover?.99f:.46f,hover?.92f:.55f,hover?.64f:.52f);
            char tier_label[16];
            if(!s->armory_page && n==3)
                std::snprintf(tier_label,sizeof(tier_label),"AA %u/3",
                    static_cast<unsigned>(game.level(Upgrade::AntiAir)));
            const char *title=s->armory_page?
                 (n==5?"SAM":WeaponFor(weapon).name):
                 (n==3?tier_label:labels[n]);
            Text(s,title,x,y+.085f,-2.973f,
                 menu::FittedScale(title,tile.width,.043f),
                 .97f,.94f,.76f);
            char amount[20];
            if(owned)std::snprintf(amount,sizeof(amount),
                        game.selected_weapon()==weapon?"ACTIVE":"SELECT");
            else if(price<0)std::snprintf(amount,sizeof(amount),"MAX");
            else std::snprintf(amount,sizeof(amount),"%d",price);
            const SpriteId icon=ShopSupportSprite(s->armory_page,
                static_cast<uint8_t>(n));
            if(icon!=SpriteId::Count)
                AppendSprite(s->sprite_mesh,icon,
                             x-.45f,y-.11f,-2.952f,.155f,.155f);
            const float amount_x=x+(icon!=SpriteId::Count?.095f:0.0f);
            Text(s,amount,amount_x,y-.109f,-2.973f,
                 menu::FittedScale(amount,tile.width-(icon!=SpriteId::Count?.22f:0.f),.041f),
                 .89f,.93f,.87f);
        }
        for(int action=8;action<=9;++action) {
            const menu::Rect tile=menu::ShopAction(action);
            const bool hover=s->menu_focus==action;
            Panel(s,tile.x,tile.y,-3.0f,tile.width,tile.height,
                  hover?.28f:.16f,hover?.44f:.29f,hover?.39f:.32f);
            const char *title=action==8?"START":
                (s->armory_page?"UPGRADES":"ARMORY");
            Text(s,title,tile.x,tile.y,-2.974f,
                 menu::FittedScale(title,tile.width,.052f),
                 .96f,.96f,.81f);
        }
        char summary[40];
        std::snprintf(summary,sizeof(summary),"CREDITS %d",game.credits());
        Text(s,summary,0,menu::kShopCreditsY,-2.967f,
             menu::FittedScale(summary,3.50f,menu::kShopCreditsScale),.92f,.86f,.64f);
    } else if(game.phase()==Phase::Combat) {
        char digits[32];std::snprintf(digits,sizeof(digits),"HULL %d",game.integrity());
        Text(s,digits,0,1.20f,-2.0f,.075f,.92f,.92f,.74f);
        char ammo[32];
        std::snprintf(ammo,sizeof(ammo),"%s %u",
            WeaponFor(game.selected_weapon()).name,
            static_cast<unsigned>(game.magazine_left()));
        Text(s,ammo,0,.93f,-1.79f,.055f,.88f,.9f,.68f);
        if(game.reloading()) {
            // Bounded progress bar: computed once during update, rendered
            // as the same scene geometry for both eyes.
            const float progress=game.reload_fraction();
            Panel(s,0,.72f,-1.79f,1.60f,.12f,.24f,.28f,.30f);
            if(progress>0.0f) {
                const float fill=1.52f*progress;
                Panel(s,-.76f+fill*.5f,.72f,-1.78f,fill,.09f,
                      .93f,.74f,.34f);
            }
            Text(s,"RELOADING",0,.52f,-1.79f,.038f,.92f,.79f,.54f);
        } else if(s->reload_ready_flash>0.0f) {
            Text(s,"READY",0,.61f,-1.79f,.048f,.57f,.96f,.58f);
        } else if(game.magazine_left()<game.stats().magazine) {
            Text(s,"SECONDARY RELOAD",0,.52f,-1.79f,.034f,.72f,.83f,.74f);
        }
        char support[24];
        std::snprintf(support,sizeof(support),"ARTY %u",
            static_cast<unsigned>(game.artillery_charges()));
        AppendSprite(s->sprite_mesh,SpriteId::ArtillerySupport,
                     -2.99f,.97f,-1.76f,.26f,.26f);
        Text(s,support,-2.02f,.97f,-1.79f,.064f,.89f,.81f,.42f);
        std::snprintf(support,sizeof(support),"AIR %u",
            static_cast<unsigned>(game.airstrike_charges()));
        AppendSprite(s->sprite_mesh,SpriteId::AirstrikeSupport,
                     1.14f,.97f,-1.76f,.26f,.26f);
        Text(s,support,2.13f,.97f,-1.79f,.064f,.89f,.81f,.42f);
        std::snprintf(support,sizeof(support),"SAM %u",
            static_cast<unsigned>(game.sam_charges()));
        Panel(s,0,.97f,-1.82f,1.22f,.32f,
              s->sam_armed?.42f:.18f,s->sam_armed?.38f:.28f,.14f);
        AppendSprite(s->sprite_mesh,SpriteId::SamSupport,
                     -.45f,.97f,-1.76f,.19f,.19f);
        Text(s,support,.16f,.97f,-1.79f,.046f,1.0f,.90f,.63f);
        for(uint8_t id=0;id<kWeaponCount;id++) {
            const WeaponKind weapon=static_cast<WeaponKind>(id);
            const float x=-2.40f+1.20f*id;
            const bool selected=game.selected_weapon()==weapon;
            const bool owned=game.owns_weapon(weapon);
            Panel(s,x,-1.44f,-1.82f,1.04f,.42f,
                  selected?.24f:.13f,selected?.44f:.22f,owned?.37f:.15f);
            // HUD reuses the very same equipment art. No HUD-only icon
            // cache, texture upload or extra draw call is necessary.
            AppendSprite(s->sprite_mesh,WeaponSprite(weapon),
                         x,-1.33f,-1.77f,.27f,.18f);
            Text(s,owned?WeaponFor(weapon).name:"LOCKED",
                 x,-1.54f,-1.79f,.033f,
                 owned?.90f:.56f,owned?.92f:.59f,owned?.69f:.57f);
        }
        if(s->sam_armed)Text(s,"SAM AIM SELECT",0,-1.05f,-1.79f,
                             .042f,1.0f,.80f,.50f);
        // Threat predictions come from live attack timers and actual firing
        // windows, not a decorative alarm or an unrelated wall-clock timer.
        const IncomingThreats threats=game.incoming_threats();
        if(threats.aircraft_seconds>=0.0f && threats.aircraft_seconds<=6.0f) {
            char warning[24];
            std::snprintf(warning,sizeof(warning),"AIR RAID %uS",
                static_cast<unsigned>(std::ceil(threats.aircraft_seconds)));
            Panel(s,-1.58f,.30f,-1.82f,2.20f,.27f,.44f,.15f,.13f);
            Text(s,warning,-1.58f,.30f,-1.79f,.039f,1.0f,.81f,.68f);
        }
        if(threats.gunboat_seconds>=0.0f && threats.gunboat_seconds<=6.0f) {
            char warning[24];
            std::snprintf(warning,sizeof(warning),"GUNBOAT %uS",
                static_cast<unsigned>(std::ceil(threats.gunboat_seconds)));
            Panel(s,1.58f,.30f,-1.82f,2.20f,.27f,.42f,.29f,.12f);
            Text(s,warning,1.58f,.30f,-1.79f,.039f,1.0f,.90f,.64f);
        }
        // First landing parties are a real danger, but their warning must
        // not start until living soldiers can reach and use the firing line.
        // Keep it clear of the shell's other two threat indicators.
        if(threats.infantry_seconds>=0.0f && threats.infantry_seconds<=6.0f) {
            char warning[24];
            std::snprintf(warning,sizeof(warning),"SHORE %uS",
                static_cast<unsigned>(std::ceil(threats.infantry_seconds)));
            Panel(s,0.0f,-.78f,-1.82f,1.86f,.24f,.31f,.28f,.15f);
            Text(s,warning,0.0f,-.78f,-1.79f,.038f,.98f,.93f,.68f);
        }
        if(s->battery_damage.active()) {
            char damage[28];
            std::snprintf(damage,sizeof(damage),"HULL HIT -%d",
                          s->battery_damage.damage());
            Panel(s,0,-.06f,-1.82f,2.60f,.33f,.42f,.11f,.12f);
            Text(s,damage,0,-.06f,-1.79f,.042f,1.0f,.82f,.77f);
        }
        if(s->aircraft_aim.visible) {
            // The user must actually have the target under the shared combat
            // ray. Do not label nearby but unselected aircraft as locked.
            char label[40];
            if(!s->aircraft_aim.equipped) {
                std::snprintf(label,sizeof(label),"AA REQUIRED");
                Panel(s,0,-.43f,-1.82f,2.65f,.31f,.38f,.15f,.13f);
                Text(s,label,0,-.43f,-1.79f,.046f,1.0f,.78f,.53f);
            } else {
                std::snprintf(label,sizeof(label),"%s HP %d",
                    s->aircraft_aim.pattern==AircraftPattern::Bomber?
                    "BOMBER":"STRAFER",
                    static_cast<int>(s->aircraft_aim.hp));
                Panel(s,0,-.43f,-1.82f,3.00f,.31f,.12f,.32f,.23f);
                Text(s,label,0,-.43f,-1.79f,.046f,.69f,1.0f,.78f);
            }
        }
        if(s->cover_aim_index>=0 &&
           s->cover_aim_index<static_cast<int>(Campaign::kMaxCover)) {
            const ShoreCover &wall=game.shore_cover()[
                static_cast<size_t>(s->cover_aim_index)];
            if(wall.hp>0) {
                char label[36];
                std::snprintf(label,sizeof(label),"COVER %d/%d",
                    static_cast<int>(wall.hp),static_cast<int>(wall.max_hp));
                Panel(s,0,-.43f,-1.82f,3.02f,.31f,.12f,.29f,.24f);
                Text(s,label,0,-.43f,-1.79f,.040f,.95f,.89f,.64f);
            }
        }
        if(s->surface_aim.visible) {
            const SurfaceAimFeedback &target=s->surface_aim;
            char label[36];
            if(target.infantry)
                std::snprintf(label,sizeof(label),
                    target.vulnerable?"INFANTRY":"INFANTRY NO SHOT");
            else
                std::snprintf(label,sizeof(label),"%s HP %d/%d",
                    target.type==UnitType::Rowboat?"ROWBOAT":
                    target.type==UnitType::LandingCraft?"LANDER":"GUNBOAT",
                    static_cast<int>(target.hp),
                    static_cast<int>(target.max_hp));
            Panel(s,0,-.43f,-1.82f,3.02f,.31f,.12f,.29f,.24f);
            Text(s,label,0,-.43f,-1.79f,.040f,
                 target.vulnerable?.75f:1.0f,
                 target.vulnerable?1.0f:.68f,.70f);
        }
        if(s->aircraft_hit.active()) {
            char hit[36];
            if(s->aircraft_hit.destroyed())
                std::snprintf(hit,sizeof(hit),"AIRCRAFT DOWN");
            else
                std::snprintf(hit,sizeof(hit),"AIR HIT -%d",s->aircraft_hit.damage());
            Panel(s,0,-.83f,-1.82f,2.70f,.32f,.16f,.33f,.18f);
            Text(s,hit,0,-.83f,-1.79f,.043f,.76f,1.0f,.62f);
        }
        if(s->flash>0)Panel(s,0,-.65f,-1.85f,.28f,.20f,1,.82f,.25f);
    } else {
        Panel(s,0,0,-3.0f,4.2f,1.6f,.07f,.10f,.14f);
        Text(s,game.phase()==Phase::Victory?"VICTORY":"BATTERY LOST",
             0,.3f,-2.95f,.09f,.9f,.81f,.50f);
        Text(s,"FIRE TO RESTART",0,-.35f,-2.95f,.06f,.87f,.82f,.69f);
        if(game.phase()==Phase::Defeat && s->battery_damage.active()) {
            char damage[28];
            std::snprintf(damage,sizeof(damage),"LAST HIT -%d",
                          s->battery_damage.damage());
            Text(s,damage,0,-.75f,-2.95f,.044f,1.0f,.65f,.62f);
        }
    }
}
void DestroyGl(State *s) {
    if(s->vbo)glDeleteBuffers(1,&s->vbo);
    if(s->program)glDeleteProgram(s->program);
    if(s->sprite_vbo)glDeleteBuffers(1,&s->sprite_vbo);
    if(s->sprite_program)glDeleteProgram(s->sprite_program);
    if(s->sprite_texture)glDeleteTextures(1,&s->sprite_texture);
    s->vbo=0;s->program=0;
    s->sprite_vbo=0;s->sprite_program=0;s->sprite_texture=0;
    s->scene_dirty=true;s->gpu_dirty=true;s->sprite_gpu_dirty=true;
}
void *Create(const ReverieNativeHostV1 *host) {
    if(!ReverieNativeHostSupportsLogV1(host))return nullptr;
    State *s=new(std::nothrow) State();
    if(!s)return nullptr;
    s->host=host;
    s->mesh.reserve(81920);
    s->sprite_mesh.reserve(3000);
    // New players choose their difficulty. Returning players restore shop state.
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
    s->sprite_vbo=0;s->sprite_program=0;s->sprite_texture=0;
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
    // A single opaque-alpha atlas keeps textured enemies to one draw call
    // per eye; the old untextured world remains a separate shared mesh.
    const char *svs="uniform mat4 u_Mvp;attribute vec3 a_Pos;"
        "attribute vec2 a_Uv;varying vec2 v_Uv;"
        "void main(){gl_Position=u_Mvp*vec4(a_Pos,1.0);v_Uv=a_Uv;}";
    const char *sfs="precision mediump float;uniform sampler2D u_Atlas;"
        "uniform vec3 u_Tint;varying vec2 v_Uv;"
        "void main(){vec4 pixel=texture2D(u_Atlas,v_Uv);"
        "if(pixel.a<0.5)discard;gl_FragColor=vec4(pixel.rgb*u_Tint,1.0);}";
    const ReverieNativeGlAttributeBindingV1 spriteBindings[]={
        {0u,"a_Pos"},{1u,"a_Uv"}};
    s->sprite_program=ReverieNativeBuildProgram(
        svs,sfs,spriteBindings,2);
    if(s->sprite_program) {
        s->sprite_position=glGetAttribLocation(s->sprite_program,"a_Pos");
        s->sprite_uv=glGetAttribLocation(s->sprite_program,"a_Uv");
        s->sprite_matrix=glGetUniformLocation(s->sprite_program,"u_Mvp");
        s->sprite_sampler=glGetUniformLocation(s->sprite_program,"u_Atlas");
        s->sprite_tint=glGetUniformLocation(s->sprite_program,"u_Tint");
        glGenBuffers(1,&s->sprite_vbo);
        glGenTextures(1,&s->sprite_texture);
        if(s->sprite_texture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,s->sprite_texture);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            const SpritePixels atlas=BuildSpriteAtlas();
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,kSpriteAtlasWidth,
                kSpriteAtlasHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,atlas.data());
        }
    }
    const bool okay=s->program && s->vbo && s->position>=0 &&
        s->color>=0 && s->matrix>=0 && s->sprite_program &&
        s->sprite_vbo && s->sprite_texture &&
        s->sprite_position>=0 && s->sprite_uv>=0 &&
        s->sprite_matrix>=0 && s->sprite_sampler>=0 &&
        s->sprite_tint>=0 && glGetError()==GL_NO_ERROR;
    if(!okay)DestroyGl(s);
    s->scene_dirty=true;s->gpu_dirty=true;s->sprite_gpu_dirty=true;
    ReverieNativeGlStateRestoreV1(&previous);
    return okay?1:0;
}
void GlRelease(void *instance) {
    State *s=static_cast<State *>(instance);if(s)DestroyGl(s);
}
void Resume(void *) {}
void Pause(void *instance) {
    State *s=static_cast<State *>(instance);
    if(!s)return;
    s->pointer_focus.RequireRelease();
    s->pointer_visible=false;
    s->menu_focus=-1;
    s->ghost_on=false;
    s->held=false;
    s->secondary_held=false;
    s->combat_input.Release();
    s->infantry_flash=0.0f;
    s->infantry_firing_mask=0u;
    s->surface_aim={};s->aircraft_aim={};s->cover_aim_index=-1;
    s->scene_dirty=true;
}
void Update(void *instance,const ReverieNativeInputV1 *input) {
    State *s=static_cast<State *>(instance);
    if(!s || !ReverieNativeInputHasPointerV1(input))return;
    ReverieNativeInputV1 copy=*input;
    ReverieNativeSanitizeBaseInputV1(&copy);
    ReverieNativeSanitizePointerV1(&copy);
    bool down=copy.primary_down!=0;
    bool secondary=copy.secondary_down!=0;
    const Phase phase_before=s->campaign.phase();
    const bool pointer_was_visible=s->pointer_visible;
    const int previous_focus=s->menu_focus;
    // Quick Menu/temporary controller loss withdraws the gameplay ray.
    // Do not advance enemies, reloads, effects, or score while defenseless.
    if(!s->pointer_focus.Filter(
            copy.pointer_kind!=REVERIE_NATIVE_POINTER_NONE,down,secondary)) {
        s->held=false;
        s->secondary_held=false;
        s->combat_input.Release();
        s->infantry_flash=0.0f;
        s->infantry_firing_mask=0u;
        s->surface_aim={};s->aircraft_aim={};s->cover_aim_index=-1;
        s->pointer_visible=false;
        s->menu_focus=-1;
        s->ghost_on=false;
        if(scene::RefreshNeeded(phase_before,phase_before,
                previous_focus!=-1,false,pointer_was_visible))
            s->scene_dirty=true;
        return;
    }
    // Only fresh post-focus button presses can enter any game action.
    copy.primary_down=down?1u:0u;
    copy.secondary_down=secondary?1u:0u;
    s->pointer_visible=true;
    // Hover feedback corresponds precisely to the raycast menus, including
    // disabled armory slots. Only mark the scene dirty when focus changes.
    int next_focus=-1;
    if(s->campaign.phase()==Phase::Briefing)
        next_focus=DifficultyTarget(copy);
    else if(s->campaign.phase()==Phase::Shop)
        next_focus=ShopTarget(copy,s->armory_page);
    if(next_focus!=s->menu_focus) {
        s->menu_focus=next_focus;
        s->scene_dirty=true;
    }
    if(copy.pointer_kind!=REVERIE_NATIVE_POINTER_NONE &&
       !WeaponFor(s->campaign.selected_weapon()).uses_ghost &&
       GunWithinTraverse(copy.pointer_direction,
                         s->campaign.selected_weapon())) {
        for(int k=0;k<3;k++)s->gun_direction[k]=copy.pointer_direction[k];
    }
    s->elapsed+=copy.delta_seconds;
    s->flash=std::max(0.0f,s->flash-copy.delta_seconds);
    s->reload_ready_flash=std::max(0.0f,
        s->reload_ready_flash-copy.delta_seconds);
    s->turret_flash=std::max(0.0f,s->turret_flash-copy.delta_seconds);
    s->infantry_flash=std::max(0.0f,s->infantry_flash-copy.delta_seconds);
    s->gunboat_salvo_flash=std::max(0.0f,
        s->gunboat_salvo_flash-copy.delta_seconds);
    s->battery_damage.Tick(copy.delta_seconds);
    s->aircraft_hit.Tick(copy.delta_seconds);
    s->impacts.Tick(copy.delta_seconds);
    s->wrecks.Tick(copy.delta_seconds);
    if(s->campaign.phase()==Phase::Combat) {
        const bool was_reloading=s->campaign.reloading();
        const uint32_t turret_before=s->campaign.turret_shot().sequence;
        const uint32_t volley_before=s->campaign.infantry_volley().sequence;
        const uint32_t air_before=s->campaign.aircraft_attack().sequence;
        const uint32_t gunboat_before=s->campaign.gunboat_salvo().sequence;
        const uint32_t collision_before=s->campaign.gunboat_collision().sequence;
        const int32_t hull_before=s->campaign.integrity();
        const auto enemies_before=s->campaign.enemies();
        s->campaign.Tick(copy.delta_seconds);
        s->wrecks.RecordTransitions(enemies_before,s->campaign.enemies());
        s->battery_damage.Record(hull_before,s->campaign.integrity());
        const TurretShotEvent turret=s->campaign.turret_shot();
        if(turret.sequence!=turret_before) {
            s->impacts.RecordTurretImpact(turret.x,turret.z,turret.destroyed);
            s->turret_flash=.10f;
        }
        const GunboatSalvoEvent salvo=s->campaign.gunboat_salvo();
        if(salvo.sequence!=gunboat_before && salvo.damage>0) {
            s->gunboat_salvo_x=salvo.x;
            s->gunboat_salvo_z=salvo.z;
            s->gunboat_salvo_flash=.15f;
            const float parapet[]={0.0f,-.70f,-.94f};
            const float down[]={0.0f,-1.0f,0.0f};
            s->impacts.RecordHit(parapet,down,.12f,ImpactKind::Metal);
        }
        const GunboatCollisionEvent collision=s->campaign.gunboat_collision();
        if(collision.sequence!=collision_before && collision.damage>0)
            s->impacts.RecordGunboatCollision(collision.x,collision.z,
                                              collision.damage);
        const AircraftAttackEvent air=s->campaign.aircraft_attack();
        if(air.sequence!=air_before && air.damage>0) {
            // A real bomber hit blasts the parapet; strafers spark against it.
            // No effect is generated for approaching or intercepted aircraft.
            const float origin[]={air.x,-.77f,-.93f};
            const float down[]={0.0f,-1.0f,0.0f};
            s->impacts.RecordHit(origin,down,.10f,
                air.pattern==AircraftPattern::Bomber?
                    ImpactKind::Explosion:ImpactKind::Metal);
        }
        const InfantryVolleyEvent volley=s->campaign.infantry_volley();
        if(volley.sequence!=volley_before && volley.damage>0) {
            s->infantry_firing_mask=volley.firing_mask;
            s->infantry_flash=.15f;
            // The actual volley also strikes the battery, not just the
            // shooter's muzzle. Distribute the cheap spark across the
            // parapet using the real firing position.
            const float parapet[]={std::max(-.8f,std::min(.8f,volley.x*.25f)),
                                   -.76f,-.92f};
            const float down[]={0.0f,-1.0f,0.0f};
            s->impacts.RecordHit(parapet,down,.10f,ImpactKind::Metal);
        }
        if(was_reloading && !s->campaign.reloading() &&
           s->campaign.phase()==Phase::Combat)
        {
            s->reload_ready_flash=.55f;
            Feedback(s,REVERIE_NATIVE_FEEDBACK_RELOAD_READY);
        }
        if(s->campaign.phase()==Phase::Shop)SaveState(s);
    }
    bool started_reload=false;
    if(secondary && !s->secondary_held &&
       s->campaign.phase()==Phase::Combat) {
        started_reload=s->campaign.RequestReload();
        if(started_reload)s->reload_ready_flash=0.0f;
        Feedback(s,started_reload?REVERIE_NATIVE_FEEDBACK_RELOAD_START:
                  REVERIE_NATIVE_FEEDBACK_FAILURE);
    }
    // Release restores gun ownership; no held UI press can become gunfire.
    if(!down)s->combat_input.Release();
    // Reload takes priority if both controls are pressed on the same frame.
    if(down && !started_reload)Interact(s,copy,!s->held);
    if(secondary&&!s->secondary_held && s->campaign.phase()==Phase::Shop) {
        if(s->campaign.BeginWave()) {
            s->wrecks.Clear();
            s->battery_damage=BatteryDamageFeedback{};
            s->infantry_flash=0.0f;
            s->infantry_firing_mask=0u;
            s->gunboat_salvo_flash=0.0f;
            if(down)s->combat_input.CaptureUiPress();
        }
    }
    // The phantom sight follows the controller rather than a barrel that
    // can no longer physically elevate to the target.
    s->ghost_on=false;
    if(s->campaign.phase()==Phase::Combat &&
       copy.pointer_kind!=REVERIE_NATIVE_POINTER_NONE) {
        s->ghost_on=(s->sam_armed ||
            WeaponFor(s->campaign.selected_weapon()).uses_ghost ||
            !GunWithinTraverse(copy.pointer_direction,
                               s->campaign.selected_weapon())) &&
            GhostAimPoint(copy.pointer_origin,copy.pointer_direction,
                          s->ghost_position);
    }
    s->aircraft_aim={};s->surface_aim={};s->cover_aim_index=-1;
    if(s->campaign.phase()==Phase::Combat &&
       copy.pointer_kind!=REVERIE_NATIVE_POINTER_NONE) {
        const int target=CombatTarget(copy,s->campaign,s->sam_armed ||
            WeaponFor(s->campaign.selected_weapon()).uses_ghost);
        s->cover_aim_index=target>=kCoverTargetBase &&
            target<kCoverTargetEnd?target-kCoverTargetBase:-1;
        s->aircraft_aim=InspectAircraftAim(s->campaign,target);
        // SAM targeting accepts aircraft only; never advertise a
        // valid surface lock while the special missile mode is armed.
        if(!s->sam_armed)
            s->surface_aim=InspectSurfaceAim(s->campaign,target);
    }
    const bool ui_pressed=
        (down && !s->held && !started_reload &&
         (phase_before==Phase::Briefing || phase_before==Phase::Shop ||
          phase_before==Phase::Victory || phase_before==Phase::Defeat)) ||
        (secondary && !s->secondary_held && phase_before==Phase::Shop);
    if(scene::RefreshNeeded(phase_before,s->campaign.phase(),
                           s->menu_focus!=previous_focus,
                           ui_pressed,!pointer_was_visible))
        s->scene_dirty=true;
    s->held=down;s->secondary_held=secondary;
}
int32_t Draw(void *instance,const ReverieNativeEyeV1 *eye) {
    State *s=static_cast<State *>(instance);
    if(!s || !ReverieNativeEyeRenderableV1(eye) || !s->program || !s->vbo ||
       !s->sprite_program || !s->sprite_vbo || !s->sprite_texture)return 0;
    if(s->scene_dirty) {
        BuildScene(s);
        s->scene_dirty=false;s->gpu_dirty=true;s->sprite_gpu_dirty=true;
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
    if(!s->sprite_mesh.empty()) {
        glUseProgram(s->sprite_program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,s->sprite_texture);
        glUniform1i(s->sprite_sampler,0);
        const bool night=s->campaign.period()==Period::Night;
        glUniform3f(s->sprite_tint,night?.52f:1.0f,
            night?.63f:1.0f,night?.72f:1.0f);
        glBindBuffer(GL_ARRAY_BUFFER,s->sprite_vbo);
        if(s->sprite_gpu_dirty) {
            glBufferData(GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(s->sprite_mesh.size()*sizeof(float)),
                s->sprite_mesh.data(),GL_DYNAMIC_DRAW);
            s->sprite_gpu_dirty=false;
        }
        glEnableVertexAttribArray(static_cast<GLuint>(s->sprite_position));
        glEnableVertexAttribArray(static_cast<GLuint>(s->sprite_uv));
        glVertexAttribPointer(static_cast<GLuint>(s->sprite_position),3,
            GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(0));
        glVertexAttribPointer(static_cast<GLuint>(s->sprite_uv),2,
            GL_FLOAT,GL_FALSE,5*sizeof(float),
            reinterpret_cast<void*>(3*sizeof(float)));
        glUniformMatrix4fv(s->sprite_matrix,1,GL_FALSE,vp);
        glDrawArrays(GL_TRIANGLES,0,
            static_cast<GLsizei>(s->sprite_mesh.size()/5));
    }
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
