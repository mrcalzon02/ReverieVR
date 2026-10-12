#include "breakwater_campaign.h"
#include "breakwater_silhouettes.h"
#include "breakwater_terrain.h"
#include "breakwater_damage_feedback.h"
#include "breakwater_aircraft_feedback.h"
#include "breakwater_combat_input.h"
#include "breakwater_impacts.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"

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
    bool held = false, secondary_held = false, scene_dirty = true, gpu_dirty = true;
    float flash = 0.0f, elapsed = 0.0f, reload_ready_flash = 0.0f;
    float turret_flash = 0.0f, infantry_flash = 0.0f;
    float gunboat_salvo_flash = 0.0f, gunboat_salvo_x = 0.0f;
    float gunboat_salvo_z = 0.0f;
    float infantry_flash_x = 0.0f, infantry_flash_z = 0.0f;
    float gun_direction[3] = {0.0f,0.0f,-1.0f};
    float shot_direction[3] = {0.0f,0.0f,-1.0f};
    ImpactEffects impacts;
    BatteryDamageFeedback battery_damage;
    AircraftAimFeedback aircraft_aim;
    AircraftHitFeedback aircraft_hit;
    CombatInputLatch combat_input;
    bool armory_page=false, sam_armed=false, ghost_on=false;
    float ghost_position[3]={}, shot_origin[3]={}, shot_gravity=0;
    std::vector<float> mesh;
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
const char *Glyph(char c) {
    // Five rows of three pixels, a deliberately tiny generated DOS-era font.
    switch(c) {
        case 'A':return "010101111101101"; case 'B':return "110101110101110";
        case 'C':return "011100100100011"; case 'D':return "110101101101110";
        case 'E':return "111100110100111"; case 'F':return "111100110100100";
        case 'G':return "011100101101011"; case 'H':return "101101111101101";
        case 'I':return "111010010010111"; case 'J':return "001001001101010";
        case 'K':return "101101110101101"; case 'L':return "100100100100111";
        case 'M':return "101111111101101"; case 'N':return "101111111111101";
        case 'O':return "111101101101111"; case 'P':return "110101110100100";
        case 'Q':return "111101101111011"; case 'R':return "110101110101101";
        case 'S':return "011100010001110"; case 'T':return "111010010010010";
        case 'U':return "101101101101111"; case 'V':return "101101101101010";
        case 'W':return "101101111111101"; case 'X':return "101101010101101";
        case 'Y':return "101101010010010"; case 'Z':return "111001010100111";
        case '0':return "111101101101111"; case '1':return "010110010010111";
        case '2':return "110001111100111"; case '3':return "110001110001110";
        case '4':return "101101111001001"; case '5':return "111100110001110";
        case '6':return "011100111101111"; case '7':return "111001010010010";
        case '8':return "111101111101111"; case '9':return "111101111001110";
        case '-':return "000000111000000"; case '/':return "001001010100100";
        case ':':return "000010000010000"; default:return "000000000000000";
    }
}
void Text(State *s,const char *text,float x,float y,float z,float scale,
          float r,float g,float b) {
    if (!text) return;
    const size_t len=std::strlen(text);
    float px=x-static_cast<float>(len)*scale*2.0f;
    for(size_t i=0;i<len;i++) {
        const char ascii=text[i];
        const char upper=(ascii>='a' && ascii<='z') ?
            static_cast<char>(ascii-'a'+'A') : ascii;
        const char *pixels=Glyph(upper);
        for(int bit=0;bit<15;bit++) {
            if(pixels[bit]!='1')continue;
            const int cx=bit%3, cy=bit/3;
            Panel(s,px+(cx+.5f)*scale,y+(2.5f-cy)*scale,z,
                  scale*.93f,scale*.93f,r,g,b);
        }
        px+=4*scale;
    }
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
    float x=0,y=0;
    if(AimAtPlane(input,kShopZ,&x,&y)<0)return -1;
    for(int n=0;n<4;n++)
        if(std::abs(x-(-2.1f+n*1.4f))<.62f && std::abs(y+.3f)<.27f)
            return n;
    return -1;
}
int ShopTarget(const ReverieNativeInputV1 &input) {
    float x=0,y=0;
    if(AimAtPlane(input,kShopZ,&x,&y)<0)return -1;
    for(int n=0;n<8;n++) {
        const int col=n%4,row=n/4;
        if(std::abs(x-(-2.1f+col*1.4f))<.62f &&
           std::abs(y-(.05f-row*.65f))<.27f)return n;
    }
    if(y<-.82f && y>-1.40f) {
        if(x>.08f && x<1.88f)return 8;
        if(x<-.08f && x>-1.88f)return 9;
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
    return index;
}
constexpr char kSaveSlot[]="breakwater-v1.bin";
bool SaveState(State *s) {
    if(!ReverieNativeHostSupportsSaveV1(s->host))return false;
    uint8_t bytes[Campaign::kSerializedSize]={};
    if(!s->campaign.SerializeShop(bytes,sizeof(bytes)))return false;
    return s->host->write_save(kSaveSlot,bytes,
        static_cast<uint32_t>(sizeof(bytes)))==REVERIE_NATIVE_SAVE_OK;
}
void LoadState(State *s) {
    if(!ReverieNativeHostSupportsSaveV1(s->host))return;
    uint8_t bytes[Campaign::kSerializedSize]={};
    uint32_t size=0;
    const int32_t status=s->host->read_save(kSaveSlot,bytes,
        static_cast<uint32_t>(sizeof(bytes)),&size);
    if(status==REVERIE_NATIVE_SAVE_NOT_FOUND)return;
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
            s->battery_damage=BatteryDamageFeedback{};
            s->reload_ready_flash=0.0f;
            s->turret_flash=0.0f;
            s->gunboat_salvo_flash=0.0f;
            s->aircraft_aim={};
            s->aircraft_hit=AircraftHitFeedback{};
            s->sam_armed=false;s->armory_page=false;s->ghost_on=false;
            game.Start();
            SaveState(s);
            Feedback(s,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        } else Feedback(s,REVERIE_NATIVE_FEEDBACK_FAILURE);
    } else if(game.phase()==Phase::Shop) {
        if(!edge)return;
        const int target=ShopTarget(input);
        bool done=false;
        if(target==9) {
            s->armory_page=!s->armory_page;
            done=true;
        } else if(target==8) {
            done=game.BeginWave();
            if(done) {
                s->combat_input.CaptureUiPress();
                s->battery_damage=BatteryDamageFeedback{};
                s->infantry_flash=0.0f;
                s->gunboat_salvo_flash=0.0f;
                s->aircraft_aim={};
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
            if(ok)s->impacts.RecordSupportDamage(before,game.enemies(),false);
            Feedback(s,ok?REVERIE_NATIVE_FEEDBACK_ARTILLERY:REVERIE_NATIVE_FEEDBACK_FAILURE);
            return;
        }
        if(route==CombatInputRoute::Airstrike){
            const auto before=game.enemies();
            const bool ok=game.CallAirstrike();
            if(ok)s->impacts.RecordSupportDamage(before,game.enemies(),true);
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
        const int16_t targetHp=enemyTarget?game.enemies()[target].hp:0;
        const Enemy enemyBefore=enemyTarget?game.enemies()[target]:Enemy{};
        const float gravity=game.stats().projectile_gravity;
        if(enemyTarget)
            AimAtEnemy(input,game.enemies()[target],gravity,missile,&impactDepth);
        else if(target>=static_cast<int>(Campaign::kMaxEnemies)) {
            const Infantry &trooper=game.infantry_units()[
                target-Campaign::kMaxEnemies];
            const float center[]={trooper.x,-.66f,trooper.z};
            ProjectileHitsSphere(input.pointer_origin,input.pointer_direction,
                                 gravity,center,.30f,&impactDepth);
        }
        bool shot=false;
        if(target>=0 && target<static_cast<int>(Campaign::kMaxEnemies))
            shot=game.FireAt(static_cast<uint8_t>(target));
        else if(target>=static_cast<int>(Campaign::kMaxEnemies))
            shot=game.FireAtInfantry(static_cast<uint8_t>(target-Campaign::kMaxEnemies));
        else if(GunWithinTraverse(input.pointer_direction,
                                  game.selected_weapon()))
            shot=game.FireMiss();
        if(shot){
            if(enemyTarget)
                s->aircraft_hit.Record(enemyBefore,game.enemies()[target],true);
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
                } else s->impacts.RecordHit(hit,down,.001f,ImpactKind::Sand);
            } else if(target<0 && !missile)
                BallisticWaterImpact(s,input,gravity);
            for(int k=0;k<3;k++) {
                s->shot_direction[k]=input.pointer_direction[k];
                s->shot_origin[k]=input.pointer_origin[k];
            }
            s->shot_gravity=missile?0.0f:gravity;
            s->flash=.09f;
            // Use fixed host-owned gameplay sound cues, not the UI confirm beep.
            Feedback(s,REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE |
                (game.reloading()?REVERIE_NATIVE_FEEDBACK_RELOAD_START:0u));
        } else if(edge)Feedback(s,REVERIE_NATIVE_FEEDBACK_FAILURE);
    } else if(edge && (game.phase()==Phase::Defeat || game.phase()==Phase::Victory)) {
        game=Campaign(Difficulty::Regular,1049u);
        s->impacts=ImpactEffects{};
        s->battery_damage=BatteryDamageFeedback{};
        s->reload_ready_flash=0.0f;
        s->turret_flash=0.0f;
        s->infantry_flash=0.0f;
        s->gunboat_salvo_flash=0.0f;
        s->aircraft_aim={};
        s->aircraft_hit=AircraftHitFeedback{};
        s->sam_armed=false;s->armory_page=false;s->ghost_on=false;
        Feedback(s,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
    }
}
void BuildScene(State *s) {
    auto &game=s->campaign;
    s->mesh.clear();
    const bool night=game.period()==Period::Night;
    // Generated sky bands and large sparse sea strips, no image assets.
    for(int row=0;row<10;row++) {
        const float bottom=-.2f+row*1.7f;
        Panel(s,0,bottom+.86f,-26.5f,42.0f,1.73f,
              night?.025f+.008f*row:.13f+.035f*row,
              night?.045f+.010f*row:.24f+.035f*row,
              night?.08f+.012f*row:.36f+.03f*row);
    }
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

    // Shore parapet, mounted barrel and enemy silhouettes.
    Box(s,0,-1.24f,-.95f,7.8f,.55f,.7f,.37f,.38f,.36f);
    // Gun carriage stays world-anchored. The light two-strip barrel aims
    // with the hand, not with headset rotation or the scene camera.
    Box(s,0,-.83f,-.64f,.68f,.42f,.46f,.27f,.29f,.31f);
    const float *aim=s->gun_direction;
    const float horizontal=std::sqrt(aim[0]*aim[0]+aim[2]*aim[2]);
    const float rightX=horizontal>.01f?-aim[2]/horizontal:1.0f;
    const float rightZ=horizontal>.01f?aim[0]/horizontal:0.0f;
    const float sx=0.0f,sy=-.78f,sz=-.88f;
    const float ex=sx+aim[0]*1.45f;
    const float ey=sy+aim[1]*1.45f;
    const float ez=sz+aim[2]*1.45f;
    Triangle(s,sx+rightX*.07f,sy,sz+rightZ*.07f,
             sx-rightX*.07f,sy,sz-rightZ*.07f,
             ex+rightX*.055f,ey,ez+rightZ*.055f,.31f,.34f,.33f);
    Triangle(s,ex+rightX*.055f,ey,ez+rightZ*.055f,
             sx-rightX*.07f,sy,sz-rightZ*.07f,
             ex-rightX*.055f,ey,ez-rightZ*.055f,.31f,.34f,.33f);
    Triangle(s,sx,sy+.065f,sz,sx,sy-.065f,sz,
             ex,ey+.050f,ez,.48f,.48f,.43f);
    Triangle(s,ex,ey+.050f,ez,sx,sy-.065f,sz,
             ex,ey-.050f,ez,.48f,.48f,.43f);
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
    for(const Enemy &e:game.enemies())if(e.active) {
        const bool aircraft=e.type==UnitType::Aircraft;
        const float y=aircraft?1.6f:-.68f;
        if(aircraft) {
            const bool bomber=e.aircraft_pattern==AircraftPattern::Bomber;
            Box(s,e.x,y,e.z,.34f,.28f,bomber?1.65f:1.20f,
                bomber?.54f:.67f,bomber?.48f:.58f,bomber?.43f:.48f);
            // Lightweight swept wings have actual banked tip vertices. Their
            // roll angle comes from the simulation's seeded flight track,
            // never wall-clock animation or a separate stereo-eye update.
            const float halfSpan=bomber?1.225f:.925f;
            const float tipRise=std::sin(e.bank)*halfSpan;
            for(int side : {-1,1}) {
                const float sx=static_cast<float>(side);
                const float rootFront=e.z+(bomber?.22f:.30f);
                const float rootRear=e.z-(bomber?.38f:.23f);
                const float tipZ=e.z-(bomber?.28f:.07f);
                const float tipX=e.x+sx*halfSpan;
                const float tipY=y+sx*tipRise;
                Triangle(s,e.x+sx*.15f,y+.02f,rootFront,
                         tipX,tipY,tipZ,
                         e.x+sx*.17f,y+.02f,rootRear,
                         bomber?.42f:.45f,bomber?.43f:.47f,bomber?.40f:.44f);
                // Parallel bright leading-edge streak makes the silhouette
                // readable without a normal map or additional texture.
                Triangle(s,e.x+sx*.16f,y+.035f,rootFront,
                         tipX,tipY+.025f,tipZ,
                         e.x+sx*.32f,y+.035f,rootFront-.045f,
                         bomber?.64f:.70f,bomber?.59f:.67f,bomber?.51f:.56f);
            }
            if(bomber) {
                for(int side : {-1,1}) {
                    const float sx=static_cast<float>(side);
                    Box(s,e.x+sx*.64f,y+sx*tipRise*.52f,e.z+.04f,
                        .23f,.20f,.40f,.30f,.32f,.31f);
                }
                Box(s,e.x,y-.17f,e.z+.12f,.58f,.22f,.62f,.31f,.31f,.29f);
            } else {
                Box(s,e.x,y+.02f,e.z+.63f,.24f,.18f,.26f,.49f,.51f,.46f);
            }
        } else {
            // Deterministic boat recipes share one mesh, built once for both eyes.
            EmitVesselSilhouette(e,
                [s](auto... arguments) { Box(s,arguments...); },
                [s](auto... arguments) { Triangle(s,arguments...); });
            // This flash belongs to the real gunboat attack cooldown.
            if(e.type==UnitType::Gunboat && e.attack_timer>2.64f)
                Box(s,e.x,-.42f,e.z+.76f,.15f,.13f,.17f,
                    .98f,.68f,.24f);
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
    for(const Infantry &trooper:game.infantry_units())if(trooper.active) {
        // Authoritative live soldier location, never an extra combat target.
        EmitInfantrySilhouette(trooper,
            [s](auto... arguments) { Box(s,arguments...); },
            [s](auto... arguments) { Triangle(s,arguments...); });
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
    if(s->infantry_flash>0.0f) {
        // One cheap bright muzzle flash at an actual soldier firing position.
        // Its short lifetime is updated once, not separately per stereo eye.
        const float x=s->infantry_flash_x,z=s->infantry_flash_z+.22f;
        Box(s,x,-.60f,z,.19f,.16f,.14f,1.0f,.78f,.25f);
        Triangle(s,x-.16f,-.60f,z,x,-.44f,z,x+.16f,-.60f,z,
                 1.0f,.87f,.40f);
    }
    if(game.phase()==Phase::Briefing) {
        Panel(s,0,.40f,-3.06f,6.35f,3.6f,.08f,.12f,.16f);
        Text(s,"BREAKWATER",0,1.45f,-3.0f,.082f,.98f,.86f,.58f);
        Text(s,"CHOOSE DIFFICULTY",0,.78f,-3.0f,.060f,.85f,.87f,.79f);
        const char *choices[]={"CADET","REGULAR","VETERAN","SIEGE"};
        for(int n=0;n<4;n++) {
            const float x=-2.1f+n*1.4f;
            Panel(s,x,-.30f,-3.0f,1.23f,.49f,.22f,.33f,.30f);
            Text(s,choices[n],x,-.30f,-2.97f,.049f,.97f,.93f,.74f);
        }
        Text(s,"SELECT A LEVEL",0,-1.12f,-2.97f,.067f,.90f,.83f,.66f);
    } else if(game.phase()==Phase::Shop) {
        Panel(s,0,.4f,-3.06f,6.35f,3.75f,.08f,.12f,.16f);
        Text(s,"BREAKWATER",0,1.67f,-3.0f,.073f,.92f,.86f,.58f);
        Text(s,Campaign::Locations()[game.location()].name,0,1.28f,-3.0f,
             .055f,.69f,.78f,.76f);
        Text(s,night?"NIGHT":"DAY",-1.55f,.88f,-3.0f,.065f,.93f,.81f,.62f);
        char digits[32];std::snprintf(digits,sizeof(digits),"D%u W%u/%u",
               static_cast<unsigned>(game.day()),
               static_cast<unsigned>(game.wave_index()),
               static_cast<unsigned>(game.waves_this_period()));
        Text(s,digits,1.05f,.88f,-3.0f,.053f,.90f,.89f,.82f);
        const char *labels[]={"FIRE","POWER","CLIP","ANTI","ARTY","AIR","TURRET","REPAIR"};
        for(int n=0;n<8;n++) {
            if(s->armory_page && n>=6)continue;
            const float x=-2.1f+(n%4)*1.4f,y=.05f-(n/4)*.65f;
            const WeaponKind weapon=static_cast<WeaponKind>(n);
            const bool equipment=s->armory_page && n<kWeaponCount;
            const bool owned=equipment && game.owns_weapon(weapon);
            const int price=s->armory_page?
                (n==5?(game.sam_charges()<3?540:-1):
                 equipment?(owned?-1:WeaponFor(weapon).purchase_cost):-1):
                game.price(static_cast<Upgrade>(n));
            Panel(s,x,y,-3.0f,1.23f,.49f,owned?.18f:price<0?.20f:.21f,
                  owned?.40f:price<0?.21f:.32f,price<0?.23f:.26f);
            char tier_label[16];
            if(!s->armory_page && n==3)
                std::snprintf(tier_label,sizeof(tier_label),"AA %u/3",
                    static_cast<unsigned>(game.level(Upgrade::AntiAir)));
            Text(s,s->armory_page?(n==5?"SAM":WeaponFor(weapon).name):
                 n==3?tier_label:labels[n],
                 x,y+.10f,-2.98f,.045f,.94f,.92f,.72f);
            char amount[20];
            if(owned)std::snprintf(amount,sizeof(amount),
                        game.selected_weapon()==weapon?"ACTIVE":"SELECT");
            else if(price<0)std::snprintf(amount,sizeof(amount),"MAX");
            else std::snprintf(amount,sizeof(amount),"%d",price);
            Text(s,amount,x,y-.10f,-2.98f,.048f,.89f,.89f,.84f);
        }
        Panel(s,.95f,-1.11f,-3.0f,1.72f,.48f,.27f,.45f,.33f);
        Text(s,"START",.95f,-1.10f,-2.97f,.070f,1,.97f,.78f);
        Panel(s,-.95f,-1.11f,-3.0f,1.72f,.48f,.24f,.29f,.42f);
        Text(s,s->armory_page?"UPGRADES":"ARMORY",
             -.95f,-1.10f,-2.97f,.055f,.84f,.90f,1.0f);
        char credits[32];std::snprintf(credits,sizeof(credits),"CREDITS %d",game.credits());
        Text(s,credits,0,-1.7f,-2.96f,.062f,.90f,.84f,.62f);
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
        Text(s,support,-2.07f,.97f,-1.79f,.064f,.89f,.81f,.42f);
        std::snprintf(support,sizeof(support),"AIR %u",
            static_cast<unsigned>(game.airstrike_charges()));
        Text(s,support,2.10f,.97f,-1.79f,.064f,.89f,.81f,.42f);
        std::snprintf(support,sizeof(support),"SAM %u",
            static_cast<unsigned>(game.sam_charges()));
        Panel(s,0,.97f,-1.82f,1.22f,.32f,
              s->sam_armed?.42f:.18f,s->sam_armed?.38f:.28f,.14f);
        Text(s,support,0,.97f,-1.79f,.046f,1.0f,.90f,.63f);
        for(uint8_t id=0;id<kWeaponCount;id++) {
            const WeaponKind weapon=static_cast<WeaponKind>(id);
            const float x=-2.40f+1.20f*id;
            const bool selected=game.selected_weapon()==weapon;
            const bool owned=game.owns_weapon(weapon);
            Panel(s,x,-1.44f,-1.82f,1.04f,.42f,
                  selected?.24f:.13f,selected?.44f:.22f,owned?.37f:.15f);
            Text(s,owned?WeaponFor(weapon).name:"LOCKED",
                 x,-1.44f,-1.79f,.034f,
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
    s->vbo=0;s->program=0;
    s->scene_dirty=true;s->gpu_dirty=true;
}
void *Create(const ReverieNativeHostV1 *host) {
    if(!ReverieNativeHostSupportsLogV1(host))return nullptr;
    State *s=new(std::nothrow) State();
    if(!s)return nullptr;
    s->host=host;
    s->mesh.reserve(81920);
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
    const bool okay=s->program && s->vbo && s->position>=0 &&
        s->color>=0 && s->matrix>=0;
    if(!okay)DestroyGl(s);
    s->scene_dirty=true;s->gpu_dirty=true;
    ReverieNativeGlStateRestoreV1(&previous);
    return okay?1:0;
}
void GlRelease(void *instance) {
    State *s=static_cast<State *>(instance);if(s)DestroyGl(s);
}
void Resume(void *) {}
void Pause(void *) {}
void Update(void *instance,const ReverieNativeInputV1 *input) {
    State *s=static_cast<State *>(instance);
    if(!s || !ReverieNativeInputHasPointerV1(input))return;
    ReverieNativeInputV1 copy=*input;
    ReverieNativeSanitizeBaseInputV1(&copy);
    ReverieNativeSanitizePointerV1(&copy);
    const bool down=copy.primary_down!=0;
    const bool secondary=copy.secondary_down!=0;
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
    if(s->campaign.phase()==Phase::Combat) {
        const bool was_reloading=s->campaign.reloading();
        const uint32_t turret_before=s->campaign.turret_shot().sequence;
        const uint32_t volley_before=s->campaign.infantry_volley().sequence;
        const uint32_t air_before=s->campaign.aircraft_attack().sequence;
        const uint32_t gunboat_before=s->campaign.gunboat_salvo().sequence;
        const uint32_t collision_before=s->campaign.gunboat_collision().sequence;
        const int32_t hull_before=s->campaign.integrity();
        s->campaign.Tick(copy.delta_seconds);
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
            s->infantry_flash_x=volley.x;
            s->infantry_flash_z=volley.z;
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
            s->battery_damage=BatteryDamageFeedback{};
            s->infantry_flash=0.0f;
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
    s->aircraft_aim={};
    if(s->campaign.phase()==Phase::Combat &&
       copy.pointer_kind!=REVERIE_NATIVE_POINTER_NONE)
        s->aircraft_aim=InspectAircraftAim(s->campaign,CombatTarget(
            copy,s->campaign,s->sam_armed ||
            WeaponFor(s->campaign.selected_weapon()).uses_ghost));
    s->held=down;s->secondary_held=secondary;
    s->scene_dirty=true;
}
int32_t Draw(void *instance,const ReverieNativeEyeV1 *eye) {
    State *s=static_cast<State *>(instance);
    if(!s || !ReverieNativeEyeRenderableV1(eye) || !s->program || !s->vbo)return 0;
    if(s->scene_dirty) {
        BuildScene(s);
        s->scene_dirty=false;s->gpu_dirty=true;
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
