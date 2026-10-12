#include "breakwater_campaign.h"
#include "breakwater_damage_feedback.h"
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
    float infantry_flash_x = 0.0f, infantry_flash_z = 0.0f;
    float gun_direction[3] = {0.0f,0.0f,-1.0f};
    float shot_direction[3] = {0.0f,0.0f,-1.0f};
    ImpactEffects impacts;
    BatteryDamageFeedback battery_damage;
    CombatInputLatch combat_input;
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
    if(std::abs(x)<.88f && y<-.82f && y>-1.40f)return 8;
    return -1;
}
bool AimAtEnemy(const ReverieNativeInputV1 &input,const Enemy &enemy,
                float *distance) {
    if(!enemy.active)return false;
    const float center[]={enemy.x,
        enemy.type==UnitType::Aircraft?1.6f:-.63f,enemy.z};
    const float radius=enemy.type==UnitType::Rowboat?.62f:
         enemy.type==UnitType::LandingCraft?.82f:.78f;
    return RaySphereSurfaceDistance(input.pointer_origin,
        input.pointer_direction,center,radius,distance);
}
bool GunWithinTraverse(const float *direction) {
    // Bounded rail-mounted gun: approximately 68 degrees horizontal
    // traverse each side, with a generous but finite elevation arc.
    return direction[2]<-0.18f &&
        std::abs(direction[0]) <= -direction[2]*2.5f &&
        std::abs(direction[1]) <= -direction[2]*1.6f;
}
int CombatTarget(const ReverieNativeInputV1 &input,const Campaign &game) {
    if(input.pointer_kind==REVERIE_NATIVE_POINTER_NONE ||
       !GunWithinTraverse(input.pointer_direction))return -1;
    float best=1e9f;int index=-1;
    for(uint8_t i=0;i<game.enemies().size();i++) {
        float depth=0;
        if(AimAtEnemy(input,game.enemies()[i],&depth) && depth<best){
            best=depth; index=i;
        }
    }
    for(uint8_t i=0;i<game.infantry_units().size();i++) {
        float depth=0;
        if(RayHitsInfantry(game.infantry_units()[i],input.pointer_origin,
                           input.pointer_direction,&depth) && depth<best) {
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
            game.Start();
            SaveState(s);
            Feedback(s,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        } else Feedback(s,REVERIE_NATIVE_FEEDBACK_FAILURE);
    } else if(game.phase()==Phase::Shop) {
        if(!edge)return;
        const int target=ShopTarget(input);
        bool done=false;
        if(target==8) {
            done=game.BeginWave();
            if(done) {
                s->combat_input.CaptureUiPress();
                s->battery_damage=BatteryDamageFeedback{};
                s->infantry_flash=0.0f;
            }
        }
        else if(target>=0 && target<8){
            done=game.Purchase(static_cast<Upgrade>(target));
            if(done)SaveState(s);
        }
        Feedback(s,done?REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
    } else if(game.phase()==Phase::Combat) {
        float x=0,y=0;
        const CombatHudTarget hud = AimAtPlane(input,-1.8f,&x,&y)>0 ?
            HitCombatSupportHud(x,y) : CombatHudTarget::None;
        // Capture support presses until release, even if the aim drifts off
        // the HUD. Shop START presses also remain captured across phases.
        const CombatInputRoute route = s->combat_input.Route(hud,edge);
        if(route==CombatInputRoute::NoAction)return;
        if(route==CombatInputRoute::Artillery){
            const auto before=game.enemies();
            const bool ok=game.CallArtillery();
            if(ok)s->impacts.RecordSupportDamage(before,game.enemies(),false);
            Feedback(s,ok?REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
            return;
        }
        if(route==CombatInputRoute::Airstrike){
            const auto before=game.enemies();
            const bool ok=game.CallAirstrike();
            if(ok)s->impacts.RecordSupportDamage(before,game.enemies(),true);
            Feedback(s,ok?REVERIE_NATIVE_FEEDBACK_ACTIVATION:REVERIE_NATIVE_FEEDBACK_FAILURE);
            return;
        }
        const int target=CombatTarget(input,game);
        // Resolve the impact before a lethal hit deactivates the target.
        float impactDepth=0.0f;
        const bool enemyTarget=target>=0 &&
            target<static_cast<int>(Campaign::kMaxEnemies);
        const int16_t targetHp=enemyTarget?game.enemies()[target].hp:0;
        if(enemyTarget)
            AimAtEnemy(input,game.enemies()[target],&impactDepth);
        else if(target>=static_cast<int>(Campaign::kMaxEnemies))
            RayHitsInfantry(game.infantry_units()[target-Campaign::kMaxEnemies],
                           input.pointer_origin,input.pointer_direction,&impactDepth);
        bool shot=false;
        if(target>=0 && target<static_cast<int>(Campaign::kMaxEnemies))
            shot=game.FireAt(static_cast<uint8_t>(target));
        else if(target>=static_cast<int>(Campaign::kMaxEnemies))
            shot=game.FireAtInfantry(static_cast<uint8_t>(target-Campaign::kMaxEnemies));
        else if(GunWithinTraverse(input.pointer_direction))
            shot=game.FireMiss();
        if(shot){
            if(impactDepth>0.0f) {
                if(enemyTarget)
                    s->impacts.RecordDamageHit(input.pointer_origin,input.pointer_direction,
                        impactDepth,targetHp,game.enemies()[target].hp,
                        game.enemies()[target].active);
                else
                    s->impacts.RecordHit(input.pointer_origin,input.pointer_direction,
                                         impactDepth,ImpactKind::Sand);
            } else if(target<0)
                s->impacts.RecordWaterMiss(input.pointer_origin,input.pointer_direction);
            for(int k=0;k<3;k++)s->shot_direction[k]=input.pointer_direction[k];
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
        const float nearX=sx+shot[0]*1.30f;
        const float nearY=sy+shot[1]*1.30f;
        const float nearZ=sz+shot[2]*1.30f;
        const float farX=sx+shot[0]*7.5f;
        const float farY=sy+shot[1]*7.5f;
        const float farZ=sz+shot[2]*7.5f;
        Triangle(s,nearX+shotRightX*.018f,nearY,nearZ+shotRightZ*.018f,
                 nearX-shotRightX*.018f,nearY,nearZ-shotRightZ*.018f,
                 farX+shotRightX*.010f,farY,farZ+shotRightZ*.010f,
                 .98f,.72f,.25f);
        Triangle(s,farX+shotRightX*.010f,farY,farZ+shotRightZ*.010f,
                 nearX-shotRightX*.018f,nearY,nearZ-shotRightZ*.018f,
                 farX-shotRightX*.010f,farY,farZ-shotRightZ*.010f,
                 .98f,.72f,.25f);
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
            Box(s,e.x,y+.02f,e.z,bomber?2.45f:1.85f,.10f,.30f,
                bomber?.42f:.45f,bomber?.43f:.47f,bomber?.40f:.44f);
            if(bomber)
                Box(s,e.x,y-.17f,e.z+.12f,.58f,.22f,.62f,.31f,.31f,.29f);
        } else {
            const float width=e.type==UnitType::Rowboat?.7f:
                e.type==UnitType::LandingCraft?1.1f:1.25f;
            Box(s,e.x,y,e.z,width,.29f,1.0f,.40f,.36f,.26f);
            Box(s,e.x,y+.20f,e.z,.31f,.17f,.48f,.25f,.25f,.24f);
            // The short-lived cannon flash is derived from simulation state,
            // not updated per eye; it identifies an actual gunboat salvo.
            if(e.type==UnitType::Gunboat && e.attack_timer>2.64f)
                Box(s,e.x,y+.32f,e.z+.38f,.18f,.15f,.23f,
                    .98f,.68f,.24f);
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
        // Rendering and combat share the same live soldier position.
        Box(s,trooper.x,-.76f,trooper.z,.18f,.38f,.17f,.51f,.45f,.35f);
        Box(s,trooper.x,-.47f,trooper.z,.15f,.16f,.15f,.68f,.58f,.42f);
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
            const float x=-2.1f+(n%4)*1.4f,y=.05f-(n/4)*.65f;
            const int price=game.price(static_cast<Upgrade>(n));
            Panel(s,x,y,-3.0f,1.23f,.49f,price<0?.20f:.21f,
                  price<0?.21f:.32f,price<0?.23f:.26f);
            Text(s,labels[n],x,y+.10f,-2.98f,.051f,.94f,.92f,.72f);
            char amount[20];
            if(price<0)std::snprintf(amount,sizeof(amount),"MAX");
            else std::snprintf(amount,sizeof(amount),"%d",price);
            Text(s,amount,x,y-.10f,-2.98f,.053f,.89f,.89f,.84f);
        }
        Panel(s,0,-1.11f,-3.0f,1.72f,.48f,.27f,.45f,.33f);
        Text(s,"START",0,-1.10f,-2.97f,.080f,1,.97f,.78f);
        char credits[32];std::snprintf(credits,sizeof(credits),"CREDITS %d",game.credits());
        Text(s,credits,0,-1.7f,-2.96f,.062f,.90f,.84f,.62f);
    } else if(game.phase()==Phase::Combat) {
        char digits[32];std::snprintf(digits,sizeof(digits),"HULL %d",game.integrity());
        Text(s,digits,0,1.20f,-2.0f,.075f,.92f,.92f,.74f);
        char ammo[32];
        std::snprintf(ammo,sizeof(ammo),"CLIP %u",
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
        if(s->battery_damage.active()) {
            char damage[28];
            std::snprintf(damage,sizeof(damage),"HULL HIT -%d",
                          s->battery_damage.damage());
            Panel(s,0,-.06f,-1.82f,2.60f,.33f,.42f,.11f,.12f);
            Text(s,damage,0,-.06f,-1.79f,.042f,1.0f,.82f,.77f);
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
       GunWithinTraverse(copy.pointer_direction)) {
        for(int k=0;k<3;k++)s->gun_direction[k]=copy.pointer_direction[k];
    }
    s->elapsed+=copy.delta_seconds;
    s->flash=std::max(0.0f,s->flash-copy.delta_seconds);
    s->reload_ready_flash=std::max(0.0f,
        s->reload_ready_flash-copy.delta_seconds);
    s->turret_flash=std::max(0.0f,s->turret_flash-copy.delta_seconds);
    s->infantry_flash=std::max(0.0f,s->infantry_flash-copy.delta_seconds);
    s->battery_damage.Tick(copy.delta_seconds);
    s->impacts.Tick(copy.delta_seconds);
    if(s->campaign.phase()==Phase::Combat) {
        const bool was_reloading=s->campaign.reloading();
        const uint32_t turret_before=s->campaign.turret_shot().sequence;
        const uint32_t volley_before=s->campaign.infantry_volley().sequence;
        const int32_t hull_before=s->campaign.integrity();
        s->campaign.Tick(copy.delta_seconds);
        s->battery_damage.Record(hull_before,s->campaign.integrity());
        const TurretShotEvent turret=s->campaign.turret_shot();
        if(turret.sequence!=turret_before) {
            s->impacts.RecordTurretImpact(turret.x,turret.z,turret.destroyed);
            s->turret_flash=.10f;
        }
        const InfantryVolleyEvent volley=s->campaign.infantry_volley();
        if(volley.sequence!=volley_before && volley.shooters>0) {
            s->infantry_flash_x=volley.x;
            s->infantry_flash_z=volley.z;
            s->infantry_flash=.15f;
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
            if(down)s->combat_input.CaptureUiPress();
        }
    }
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
