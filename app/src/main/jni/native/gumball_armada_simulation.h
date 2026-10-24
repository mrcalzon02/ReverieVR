#ifndef REVERIE_GUMBALL_ARMADA_SIMULATION_H
#define REVERIE_GUMBALL_ARMADA_SIMULATION_H
#include "gumball_arcade_objectives.h"
#include "gumball_cast_encounters.h"
#include "gumball_shipyard.h"
#include "gumball_wave_formations.h"
#include "gumball_flight_control.h"
#include "gumball_support_contracts.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace reverie { namespace gumball {
constexpr uint32_t kLaneCount=4096;
constexpr unsigned kEnemySlots=24;
enum class Phase : uint8_t { Chart,Flight,Won,Lost,ArmadaDefeated };
enum class FailureReason : uint8_t { None, HullDestroyed, Escapes, EliteEscaped, Deadline, SalvageLost };
enum class Upgrade : uint8_t { Blaster,Hull,Thruster,Shield };
enum class EnemyKind : uint8_t { Scout, Interceptor, Bomber, Ace, Fortress, Courier, Blort, Nib, Biff, Baff, Cinnamon, Sydney, Jules };
enum class Weapon : uint8_t { PeaRepeater, RockCannon, MintSpread };
struct Enemy {
    float x=0,y=0,age=0,firing=0;
    int hp=0; bool active=false,boss=false;
    EnemyKind kind=EnemyKind::Scout;
    WaveFormation formation=WaveFormation::Column;
    uint8_t formation_slot=0;
};
struct Shot { float x=0,y=0,vy=0; bool active=false,hostile=false; float vx=0.f; int damage=1; bool ally=false,bomb=false,mine=false,zone=false; float warning=0.f; };
struct Lane { uint32_t id=0; uint8_t hazard=0,reward=0; bool fortress=false; };
struct SnapshotV1 {
    uint32_t magic=0x474D4231u, version=1, seed=0, node=0;
    uint32_t visited=0,cash=0,kills=0,bosses=0;
    uint16_t levels[4]={0,0,0,0};uint32_t checksum=0;
};
struct SnapshotV2 {
    uint32_t magic=0x474D4231u, version=2, seed=0, node=0;
    uint32_t visited=0,cash=0,kills=0,bosses=0;
    uint16_t levels[4]={0,0,0,0};
    uint32_t owned_weapons=1u,active_weapon=0u,checksum=0u;
};
struct Snapshot {
    uint32_t magic=0x474D4231u, version=3, seed=0, node=0;
    uint32_t visited=0,cash=0,kills=0,bosses=0;
    uint16_t levels[4]={0,0,0,0};
    uint32_t owned_weapons=1u,active_weapon=0u,support_state=0u,checksum=0u;
};
inline uint32_t Mix(uint32_t x){
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);
}
inline uint32_t SnapshotChecksum(const Snapshot& s){
    uint32_t h=2166136261u;
    Snapshot copy=s;copy.checksum=0;
    const auto* b=reinterpret_cast<const uint8_t*>(&copy);
    for(unsigned i=0;i<sizeof(copy);++i)h=(h^b[i])*16777619u;
    return h;
}
inline uint32_t SnapshotV2Checksum(const SnapshotV2& s){
    uint32_t h=2166136261u;SnapshotV2 copy=s;copy.checksum=0;
    const auto* b=reinterpret_cast<const uint8_t*>(&copy);
    for(unsigned i=0;i<sizeof(copy);++i)h=(h^b[i])*16777619u;
    return h;
}
inline uint32_t SnapshotV1Checksum(const SnapshotV1& s){
    uint32_t h=2166136261u;SnapshotV1 copy=s;copy.checksum=0;
    const auto* b=reinterpret_cast<const uint8_t*>(&copy);
    for(unsigned i=0;i<sizeof(copy);++i)h=(h^b[i])*16777619u;
    return h;
}
static_assert(sizeof(SnapshotV1)==44,"Legacy save binary shape must remain frozen");
static_assert(sizeof(SnapshotV2)==52,"V2 save binary shape must remain frozen");
static_assert(sizeof(Snapshot)==56,"V3 campaign save must preserve 56-byte layout");
class Simulation {
public:
    explicit Simulation(uint32_t seed=0xC4A0D1u):seed_(seed?seed:1u){}
    Phase phase()const{return phase_;}
    bool configure_ship(const Shipyard& ship) {
        if(phase_!=Phase::Chart)return false;
        ship_ready_=ship.ready();
        if(ship_ready_)ship_profile_=ship.profile();
        return ship_ready_;
    }
    // 3D ShipAssembly supplies the same physical profile contract as the
    // legacy flat yard; one simulation path powers both player builds.
    bool configure_ship_profile(const ShipProfile& profile,bool ready) {
        if(phase_!=Phase::Chart)return false;
        ship_ready_=ready;
        if(ready)ship_profile_=profile;
        return ready;
    }
    bool ship_ready()const{return ship_ready_;}
    const SupportContracts& support()const{return support_;}
    bool hire_support(SupportKind kind){
        if(phase_!=Phase::Chart)return false;
        return support_.hire(kind,cash_);
    }
    bool upgrade_support(SupportSkill skill){
        if(phase_!=Phase::Chart)return false;
        return support_.improve(skill,cash_);
    }
    bool buy_support_kit(SupportKit kind){
        if(phase_!=Phase::Chart)return false;
        return support_.buy_kit(kind,cash_);
    }
    uint32_t kit_activations()const{return kit_activations_;}
    SupportKind deployed_support()const{return deployed_support_;}
    uint32_t support_deployments()const{return support_deployments_;}
    int support_pulses()const{return support_pulses_;}
    float support_x()const{return support_x_;}
    bool slipstream_active()const{return slipstream_active_;}
    bool left_fast()const{return static_cast<int>(elapsed_/3.f)%2==0;}
    float movement_multiplier()const{
        if(!slipstream_active_||std::fabs(px_)<.15f)return 1.f;
        return ((px_<0.f)==left_fast())?1.48f:.52f;
    }
    float move_speed()const{
        return std::max(.7f,1.55f+.16f*levels_[2]+ship_profile_.speed)
               *movement_multiplier();
    }
    float pointer_move_speed()const{
        return move_speed()*kFlightPointerSpeedFactor;
    }
    CastGuest route_guest(unsigned choice)const{
        if(choice>=3)return CastGuest::None;
        const Lane target=lane(choice);
        return ChooseCastGuest(seed_,target.id,visited_,target.hazard);
    }
    ShipProfile ship_profile()const{return ship_profile_;}
    FailureReason failure()const{return failure_;}
    CastGuest guest()const{return cast_.kind();}
    bool guest_active()const{return cast_.active();}
    int guest_pulses()const{return cast_.pulses();}
    uint32_t ally_shots()const{return ally_shots_;}
    float ally_x()const{return ally_x_;}
    uint32_t bombs_defused()const{return bombs_defused_;}
    uint32_t mines_defused()const{return mines_defused_;}
    uint32_t twin_bonus_paid()const{return twins_.awarded()?32u:0u;}
    uint32_t node()const{return node_;}
    uint32_t cash()const{return cash_;}
    uint32_t kills()const{return kills_;}
    uint32_t visited()const{return visited_;}
    uint32_t bosses()const{return bosses_;}
    int hp()const{return hp_;}
    int shield()const{return shield_;}
    int combo()const{return combo_;}
    MissionRules mission()const{return mission_;}
    const SalvageField& salvage()const{return salvage_;}
    int salvage_collected()const{return salvage_.collected();}
    float remaining_time()const{
        return mission_.time_limit>0.f?std::max(0.f,mission_.time_limit-elapsed_):0.f;
    }
    int max_combo()const{return max_combo_;}
    int escaped()const{return escaped_;}
    int defeated()const{return defeated_;}
    int target()const{return target_;}
    int spawned()const{return spawned_;}
    int wave()const{return target_>0?std::min(3,1+(spawned_*3)/target_):0;}
    WaveFormation current_formation()const{
        return FormationFor(seed_,destination_.id,spawned_>0?spawned_-1:0);
    }
    int assault()const{return spawned_>0?1+(spawned_-1)/5:1;}
    int assault_count()const{return (target_+4)/5;}
    uint32_t sortie_cash()const{return sortie_cash_;}
    uint32_t completion_bonus()const{return completion_bonus_;}
    uint32_t shots_fired()const{return shots_fired_;}
    uint32_t shots_hit()const{return shots_hit_;}
    float elapsed()const{return elapsed_;}
    float hurt_flash()const{return hurt_flash_;}
    int level(Upgrade u)const{return levels_[static_cast<unsigned>(u)];}
    Weapon weapon()const{return weapon_;}
    bool owns_weapon(Weapon w)const{
        const unsigned id=static_cast<unsigned>(w);
        return id<3u&&(owned_weapons_&(1u<<id))!=0u;
    }
    int weapon_price(Weapon w)const{
        const unsigned id=static_cast<unsigned>(w);
        if(id>=3u||owns_weapon(w))return 0;
        return id==1u?120:160;
    }
    bool select_weapon(Weapon w){
        if(phase_!=Phase::Chart||!owns_weapon(w))return false;
        weapon_=w;return true;
    }
    bool buy_weapon(Weapon w){
        const unsigned id=static_cast<unsigned>(w);
        if(id>=3u||phase_!=Phase::Chart||owns_weapon(w))return false;
        const int cost=weapon_price(w);
        if(cash_<static_cast<uint32_t>(cost))return false;
        cash_-=static_cast<uint32_t>(cost);owned_weapons_|=1u<<id;weapon_=w;return true;
    }
    int command_rank()const{return 1+static_cast<int>(kills_/25)+static_cast<int>(bosses_)*4;}
    float player_x()const{return px_;}
    float player_y()const{return py_;}
    const std::array<Enemy,kEnemySlots>& enemies()const{return enemies_;}
    const std::array<Shot,48>& shots()const{return shots_;}
    Lane lane(unsigned choice)const{
        if(choice>=3)return {};
        const uint32_t sector=node_/1024u,origin=node_%1024u;
        const uint32_t step=37u+(Mix(seed_^node_^(choice*0x9E3779B9u))%69u);
        uint32_t target=sector*1024u+std::min(1023u,origin+step);
        if(origin==1023u&&sector<3u)target=(sector+1u)*1024u;
        const uint32_t mix=Mix(seed_^target^(choice<<19));
        Lane result;result.id=target;
        result.hazard=static_cast<uint8_t>(1u+(mix%5u)+sector);
        result.reward=static_cast<uint8_t>(1u+(mix>>8)%5u);
        result.fortress=target%1024u==1023u;
        return result;
    }
    int price(Upgrade u)const{
        const unsigned t=static_cast<unsigned>(u);
        return t<4?70+static_cast<int>(levels_[t])*95:0;
    }
    bool buy(Upgrade u){
        const unsigned t=static_cast<unsigned>(u);
        if(t>=4||phase_!=Phase::Chart||levels_[t]>=12||cash_<static_cast<uint32_t>(price(u)))return false;
        cash_-=static_cast<uint32_t>(price(u));++levels_[t];return true;
    }
    bool launch(unsigned choice){
        if(phase_!=Phase::Chart||choice>=3||!ship_ready_)return false;
        destination_=lane(choice);phase_=Phase::Flight;failure_=FailureReason::None;
        mission_=MissionFor(choice,destination_.hazard);salvage_.clear();
        cast_.begin(seed_,destination_.id,visited_,destination_.hazard);
        deployed_support_=support_.consume();
        support_timer_=3.2f+static_cast<float>(Mix(seed_^destination_.id)%5u)*.4f;
        support_pulses_=0;support_x_=-1.5f;slipstream_active_=false;
        kit_activations_=0;
        if(deployed_support_!=SupportKind::None)++support_deployments_;
        ally_shots_=0;bombs_defused_=0;mines_defused_=0;ally_x_=-1.3f;
        twins_.reset();
        hp_=90+levels_[1]*20+ship_profile_.hull;
        shield_=20+levels_[3]*18+ship_profile_.shield;
        px_=0;py_=-.85f;combo_=0;max_combo_=0;combo_clock_=0;cooldown_=0;
        escaped_=0;sortie_cash_=0;completion_bonus_=0;shots_fired_=0;shots_hit_=0;elapsed_=0;hurt_flash_=0;invulnerable_=0;regen_delay_=0;regen_credit_=0;
        spawned_=0;defeated_=0;bossKilled_=false;spawn_clock_=0;rng_=Mix(seed_^destination_.id^visited_);
        const uint32_t sector=destination_.id/1024u;
        target_=6+destination_.hazard*2+static_cast<int>(sector)*2;
        if(destination_.fortress&&((bosses_>>sector)&1u)==0u)++target_;
        enemies_={};shots_={};return true;
    }
    void update(float dt,float sx,float sy,bool fire,bool pointerGuided=false){
        if(phase_!=Phase::Flight||!std::isfinite(dt)||dt<=0.f)return;
        dt=std::min(dt,.05f);
        elapsed_+=dt;
        if(mission_.time_limit>0.f&&elapsed_>mission_.time_limit){
            failure_=FailureReason::Deadline;phase_=Phase::Lost;return;
        }
        // Use genuine pre-purchased supplies only on a meaningful damage
        // threshold. No effect or charge consumption if hull/shield is full.
        const int hullCap=90+levels_[1]*20+ship_profile_.hull;
        const int shieldCap=20+levels_[3]*18+ship_profile_.shield;
        const uint8_t used=support_.apply_emergency(hp_,hullCap,shield_,shieldCap);
        kit_activations_+=static_cast<uint32_t>((used&1u)!=0u)
                         +static_cast<uint32_t>((used&2u)!=0u);
        hurt_flash_=std::max(0.f,hurt_flash_-dt);
        invulnerable_=std::max(0.f,invulnerable_-dt);
        regen_delay_=std::max(0.f,regen_delay_-dt);
        if(regen_delay_<=0.f){
            regen_credit_+=dt*2.f;
            const int restored=static_cast<int>(regen_credit_);
            if(restored>0){
                shield_=std::min(20+levels_[3]*18+ship_profile_.shield,shield_+restored);
                regen_credit_-=static_cast<float>(restored);
            }
        }
        sx=std::isfinite(sx)?std::max(-1.f,std::min(1.f,sx)):0.f;
        sy=std::isfinite(sy)?std::max(-1.f,std::min(1.f,sy)):0.f;
        slipstream_active_=false;
        for(const auto& enemy:enemies_)if(enemy.active&&enemy.kind==EnemyKind::Sydney)
            slipstream_active_=true;
        const float speed=move_speed()*
            (pointerGuided?kFlightPointerSpeedFactor:1.f);
        px_=std::max(-1.9f,std::min(1.9f,px_+sx*dt*speed));
        py_=std::max(-1.15f,std::min(.65f,py_-sy*dt*speed));
        cooldown_-=dt;combo_clock_-=dt;
        if(combo_clock_<=0.f)combo_=0;
        if(fire&&cooldown_<=0.f){
            const int pellets=weapon_==Weapon::MintSpread?3:1;
            bool emitted=false;
            const int mounts=std::max(1,std::min(2,ship_profile_.mounts));
            for(int mount=0;mount<mounts;++mount)for(int n=0;n<pellets;++n){
                for(auto& shot:shots_)if(!shot.active){
                    const float vx=pellets==3?(n-1)*.55f:0.f;
                    const int damage=weapon_==Weapon::RockCannon?
                        4+levels_[0]/2:(weapon_==Weapon::MintSpread?
                        1+levels_[0]/4:1+levels_[0]/3);
                    shot={px_+ship_profile_.muzzle_x[mount]+(n-1)*.028f,
                        py_+.12f+ship_profile_.muzzle_y[mount],
                        weapon_==Weapon::RockCannon?2.55f:3.75f,
                        true,false,vx,damage};
                    ++shots_fired_;emitted=true;break;
                }
            }
            if(emitted)cooldown_=weapon_==Weapon::RockCannon?
                std::max(.41f,.79f-.016f*levels_[0]):
                (weapon_==Weapon::MintSpread?std::max(.30f,.55f-.014f*levels_[0]):
                std::max(.10f,.34f-.022f*levels_[0]));
        }
        spawn_clock_-=dt;
        if(spawned_<target_&&spawn_clock_<=0.f){
            for(auto& e:enemies_)if(!e.active){
                const bool boss=destination_.fortress&&spawned_==target_-1
                    &&((bosses_>>(destination_.id/1024u))&1u)==0u;
                const bool courier=!boss&&mission_.kind==MissionKind::Intercept
                    &&spawned_==target_-1;
                const bool ace=!boss&&!courier&&spawned_==target_-1;
                const bool blort=!boss&&!courier&&!ace&&cast_.kind()==CastGuest::BlortBlister
                    &&spawned_==target_/2;
                const bool nib=!boss&&!courier&&!ace
                    &&cast_.kind()==CastGuest::NibNoxious&&spawned_==target_/2;
                const bool biff=!boss&&!courier&&!ace
                    &&cast_.kind()==CastGuest::CrumbclankTwins&&spawned_==target_/2;
                const bool baff=!boss&&!courier&&!ace
                    &&cast_.kind()==CastGuest::CrumbclankTwins&&spawned_==target_/2+1;
                const bool cinnamon=!boss&&!courier&&!ace
                    &&cast_.kind()==CastGuest::CinnamonCharles&&spawned_==target_/2;
                const bool sydney=!boss&&!courier&&!ace
                    &&cast_.kind()==CastGuest::SydneySlipstream&&spawned_==target_/2;
                const bool jules=!boss&&!courier&&!ace
                    &&cast_.kind()==CastGuest::JawbreakerJules&&spawned_==target_/2;
                const unsigned rollType=rollKind(spawned_);
                EnemyKind kind=EnemyKind::Scout;
                if(boss)kind=EnemyKind::Fortress;
                else if(courier)kind=EnemyKind::Courier;
                else if(ace)kind=EnemyKind::Ace;
                else if(blort)kind=EnemyKind::Blort;
                else if(nib)kind=EnemyKind::Nib;
                else if(biff)kind=EnemyKind::Biff;
                else if(baff)kind=EnemyKind::Baff;
                else if(cinnamon)kind=EnemyKind::Cinnamon;
                else if(sydney)kind=EnemyKind::Sydney;
                else if(jules)kind=EnemyKind::Jules;
                else if(rollType==1u)kind=EnemyKind::Interceptor;
                else if(rollType==2u)kind=EnemyKind::Bomber;
                int hull=2;
                switch(kind){
                case EnemyKind::Scout: hull=2;break;
                case EnemyKind::Interceptor: hull=3;break;
                case EnemyKind::Bomber: hull=4;break;
                case EnemyKind::Blort: hull=5;break;
                case EnemyKind::Nib: hull=4;break;
                case EnemyKind::Biff: hull=5;break;
                case EnemyKind::Baff: hull=5;break;
                case EnemyKind::Cinnamon: hull=6;break;
                case EnemyKind::Sydney: hull=6;break;
                case EnemyKind::Jules: hull=11;break;
                case EnemyKind::Ace: hull=7;break;
                case EnemyKind::Courier: hull=6;break;
                case EnemyKind::Fortress: hull=22+static_cast<int>(destination_.id/1024u)*4;break;
                }
                const uint32_t roll=next();
                const WaveEntry waveEntry=PlanWaveEntry(
                    seed_,destination_.id,spawned_,target_,roll);
                e={waveEntry.x,waveEntry.y,0.f,
                   kind==EnemyKind::Baff?2.12f:1.45f,hull,true,boss,kind,
                   waveEntry.formation,static_cast<uint8_t>(spawned_%5)};
                ++spawned_;
                spawn_clock_=waveEntry.next_delay;
                break;
            }
        }
        for(auto& e:enemies_)if(e.active){
            e.age+=dt;
            // Named enemies and bosses keep their custom steering/attacks.
            const bool ordinary=e.kind==EnemyKind::Scout
                ||e.kind==EnemyKind::Interceptor||e.kind==EnemyKind::Bomber;
            const WaveTactics tactics=ordinary?
                TacticsFor(e.formation,e.formation_slot,e.age,e.x,px_):
                WaveTactics{};
            const float speed=e.kind==EnemyKind::Courier?.66f:
                (e.kind==EnemyKind::Interceptor?.53f:
                (e.kind==EnemyKind::Bomber||e.kind==EnemyKind::Blort
                   ||e.kind==EnemyKind::Nib||e.kind==EnemyKind::Biff
                   ||e.kind==EnemyKind::Baff||e.kind==EnemyKind::Cinnamon
                   ||e.kind==EnemyKind::Jules?.18f:
                (e.kind==EnemyKind::Ace?.43f:(e.boss?.15f:.31f))));
            e.y-=dt*(speed+.01f*destination_.hazard);
            if(e.kind==EnemyKind::Interceptor){
                const float dx=std::max(-1.f,std::min(1.f,px_-e.x));
                e.x+=dt*dx*.58f;
            }else if(e.kind==EnemyKind::Sydney){
                e.x+=dt*std::sin(e.age*2.8f)*.67f;
            }else if(e.kind==EnemyKind::Bomber||e.kind==EnemyKind::Blort
                      ||e.kind==EnemyKind::Nib||e.kind==EnemyKind::Biff
                      ||e.kind==EnemyKind::Baff||e.kind==EnemyKind::Cinnamon
                      ||e.kind==EnemyKind::Jules){
                e.x+=dt*std::sin(e.age*1.1f)*.09f;
            }else if(e.kind==EnemyKind::Courier){
                e.x+=dt*std::sin(e.age*4.1f+e.y)*.90f;
            }else if(e.kind==EnemyKind::Ace){
                e.x+=dt*std::sin(e.age*3.6f)*.82f;
            }else e.x+=std::sin(e.age*2.4f+e.y)*dt*(e.boss?.22f:.37f);
            if(ordinary)e.x+=dt*tactics.lateral_speed;
            e.x=std::max(-1.85f,std::min(1.85f,e.x));
            e.firing-=dt;
            if(e.firing<=0.f&&e.kind!=EnemyKind::Courier){
                const bool bombing=e.kind==EnemyKind::Blort
                    ||e.kind==EnemyKind::Biff||e.kind==EnemyKind::Baff;
                const bool mining=e.kind==EnemyKind::Nib;
                const bool raining=e.kind==EnemyKind::Cinnamon;
                const bool heavy=e.kind==EnemyKind::Jules;
                const int count=raining?3:(e.boss?3:
                    ((e.kind==EnemyKind::Ace||bombing||heavy)?2:1));
                const float aim=(e.kind==EnemyKind::Interceptor?
                    std::max(-.50f,std::min(.50f,(px_-e.x)*.32f)):0.f)
                    +tactics.shot_aim;
                for(int n=0;n<count;++n)for(auto& shot:shots_)if(!shot.active){
                    const float spread=e.boss?(n-1)*.43f:
                        (e.kind==EnemyKind::Ace?(n*2-1)*.24f:
                         (e.kind==EnemyKind::Biff?(-.38f+n*.22f):
                          (e.kind==EnemyKind::Baff?(.38f-n*.22f):
                           (heavy?(n*2-1)*.24f:
                            (bombing?(n*2-1)*.32f:0.f)))));
                    const float zoneX=raining?(-1.20f+1.20f*
                        static_cast<float>((next()+static_cast<uint32_t>(n))%3u)):e.x;
                    shot={zoneX,e.y-.14f,mining?-.22f:(raining?-1.70f:
                          (bombing||heavy?-.82f:
                           (e.kind==EnemyKind::Bomber?-1.05f:-1.3f))),
                          true,true,raining?0.f:aim+spread,
                          mining?10:(raining?9:(bombing||heavy?15:1)),
                          false,bombing||heavy,mining,raining,
                          raining?.68f:0.f};
                    break;
                }
                e.firing=e.boss?.70f:(e.kind==EnemyKind::Ace?1.18f:
                    (mining?1.95f:(raining?2.35f:(heavy?2.35f:
                     (bombing?1.78f:
                     (e.kind==EnemyKind::Bomber?1.85f:2.15f))))));
                e.firing*=tactics.fire_interval_scale;
            }
            if(e.y< -1.65f){
                e.active=false;++defeated_;++escaped_;combo_=0;
                if(escaped_>mission_.permitted_escapes){
                    failure_=(e.boss||e.kind==EnemyKind::Ace||e.kind==EnemyKind::Courier)?
                        FailureReason::EliteEscaped:FailureReason::Escapes;
                    phase_=Phase::Lost;return;
                }
                if(!e.boss)damage(10+3*static_cast<int>(destination_.hazard));
                if(e.boss||e.kind==EnemyKind::Ace||e.kind==EnemyKind::Courier){
                    failure_=FailureReason::EliteEscaped;phase_=Phase::Lost;return;
                }
            }
        }
        if(hp_<=0){failure_=FailureReason::HullDestroyed;phase_=Phase::Lost;return;}
        cast_.update(dt);
        support_timer_-=dt;
        if(deployed_support_!=SupportKind::None&&elapsed_<16.f
           &&support_pulses_<(deployed_support_==SupportKind::CapitaineEclair?
                               support_.volley_count():4)
           &&support_timer_<=0.f){
            bool helped=false;
            if(deployed_support_==SupportKind::CapitaineEclair){
                const Enemy* target=nullptr;
                for(const auto& enemy:enemies_)if(enemy.active&&enemy.y>-.65f)
                    if(!target||enemy.y<target->y)target=&enemy;
                if(target){
                    for(auto& bullet:shots_)if(!bullet.active){
                        support_x_=target->x;
                        bullet={support_x_,-.80f,3.65f,true,false,0.f,
                                support_.volley_damage(),true,false,false,false,0.f};
                        helped=true;break;
                    }
                }
            }else{
                const int cap=20+levels_[3]*18+ship_profile_.shield;
                const int hullCap=90+levels_[1]*20+ship_profile_.hull;
                const int restored=std::min(support_.shield_repair(),cap-shield_);
                const int patched=std::min(support_.hull_repair(),hullCap-hp_);
                const uint32_t pulled=salvage_.tractor(px_,py_,support_.tractor_reach());
                if(restored>0||patched>0||pulled>0u){
                    shield_+=restored;hp_+=patched;cash_+=pulled;sortie_cash_+=pulled;
                    support_x_=px_+.26f;helped=true;
                }
            }
            if(helped){++support_pulses_;support_timer_=1.8f;}
        }
        if(cast_.pulse_ready()){
            if(cast_.kind()==CastGuest::PipParfait){
                const Enemy* target=nullptr;
                for(const auto& enemy:enemies_)if(enemy.active&&enemy.y>-.55f)
                    if(!target||enemy.y<target->y)target=&enemy;
                if(target){
                    for(auto& bullet:shots_)if(!bullet.active){
                        ally_x_=target->x;
                        bullet={ally_x_,-.79f,3.6f,true,false,0.f,2,true,false};
                        ++ally_shots_;cast_.claim_pulse();break;
                    }
                }
            }else if(cast_.kind()==CastGuest::LollyWren){
                const int cap=20+levels_[3]*18+ship_profile_.shield;
                const int restored=std::min(8,cap-shield_);
                const uint32_t pulled=salvage_.tractor(px_,py_,1.25f);
                if(restored>0||pulled>0u){
                    shield_+=restored;cash_+=pulled;sortie_cash_+=pulled;
                    ally_x_=px_-.25f;
                    cast_.claim_pulse();
                }
            }
        }
        for(auto& s:shots_)if(s.active){
            if(s.warning>0.f){
                s.warning=std::max(0.f,s.warning-dt);
                continue; // warning period is non-damaging and stationary
            }
            s.y+=s.vy*dt;s.x+=s.vx*dt;
            if(s.y>1.75f||s.y< -1.4f){s.active=false;continue;}
            if(s.hostile){
                if(s.bomb||s.mine){
                    // Interceptable hostile hazards: only an actual friendly
                    // projectile removes one. No fictional cash award.
                    for(auto& bullet:shots_)if(bullet.active&&!bullet.hostile
                        &&std::fabs(bullet.x-s.x)<.16f
                        &&std::fabs(bullet.y-s.y)<.18f){
                        bullet.active=false;s.active=false;
                        if(s.mine)++mines_defused_;else ++bombs_defused_;
                        break;
                    }
                    if(!s.active)continue;
                }
                if(std::fabs(s.x-px_)<(s.zone?.23f:
                   (s.mine?.21f:(s.bomb?.20f:.17f)))
                   &&std::fabs(s.y-py_)<(s.mine?.19f:(s.bomb?.19f:.16f))){
                    s.active=false;damage((s.bomb||s.mine||s.zone)?
                        s.damage+static_cast<int>(destination_.hazard):
                        7+static_cast<int>(destination_.hazard));
                }
            }else{
                for(auto& e:enemies_)if(e.active
                    &&std::fabs(s.x-e.x)<(e.boss?.32f:.19f)
                    &&std::fabs(s.y-e.y)<(e.boss?.25f:.16f)){
                    s.active=false;if(!s.ally)++shots_hit_;e.hp-=s.damage;
                    if(e.hp<=0){
                        if(e.boss)bossKilled_=true;
                        e.active=false;++defeated_;++kills_;++combo_;max_combo_=std::max(max_combo_,combo_);combo_clock_=2.65f;
                        const uint32_t duoBonus=twins_.destroyed(e.kind==EnemyKind::Biff,
                                                                 e.kind==EnemyKind::Baff);
                        cash_+=duoBonus;sortie_cash_+=duoBonus;
                        const uint32_t reward=static_cast<uint32_t>((e.boss?125u:9u)+(combo_/3)*4+destination_.reward*2);
                        cash_+=reward;sortie_cash_+=reward;
                        salvage_.drop(e.x,e.y,static_cast<uint16_t>(
                            e.boss?42:(e.kind==EnemyKind::Ace?30:12)));
                    }
                    break;
                }
            }
        }
        const uint32_t scavenged=salvage_.update(dt,px_,py_,ship_profile_.pickup_radius);
        cash_+=scavenged;sortie_cash_+=scavenged;
        if(hp_<=0){failure_=FailureReason::HullDestroyed;phase_=Phase::Lost;return;}
        if(spawned_==target_&&defeated_==target_){
            if(salvage_.collected()<mission_.required_salvage){
                if(salvage_.empty()){
                    failure_=FailureReason::SalvageLost;phase_=Phase::Lost;
                }
                return;
            }
            completion_bonus_=mission_.bonus+
                static_cast<uint32_t>(destination_.reward*12+max_combo_*2);
            cash_+=completion_bonus_;sortie_cash_+=completion_bonus_;
            node_=destination_.id;++visited_;
            if(destination_.fortress&&bossKilled_)bosses_|=1u<<(node_/1024u);
            phase_=bosses_==15u?Phase::ArmadaDefeated:Phase::Won;
        }
    }
    bool recover(){
        if(phase_==Phase::Won||phase_==Phase::Lost||phase_==Phase::ArmadaDefeated){
            phase_=Phase::Chart;return true;
        }
        return false;
    }
    Snapshot snapshot()const{
        Snapshot s;s.seed=seed_;s.node=node_;s.visited=visited_;s.cash=cash_;
        s.kills=kills_;s.bosses=bosses_;
        for(unsigned i=0;i<4;++i)s.levels[i]=levels_[i];
        s.owned_weapons=owned_weapons_;s.active_weapon=static_cast<uint32_t>(weapon_);
        uint8_t supportLevels[4]={};support_.export_levels(supportLevels);
        for(unsigned i=0;i<4;++i)s.support_state|=static_cast<uint32_t>(supportLevels[i])<<(i*2u);
        s.support_state|=static_cast<uint32_t>(support_.booked())<<8u;
        s.support_state|=static_cast<uint32_t>(support_.kit_mask())<<10u;
        s.checksum=SnapshotChecksum(s);return s;
    }
    bool restore(const Snapshot& s){
        if(s.magic!=0x474D4231u||s.version!=3u||s.seed==0u||s.node>=kLaneCount
            ||s.bosses>15u||s.cash>100000000u||s.visited>10000000u
            ||s.kills>100000000u||s.checksum!=SnapshotChecksum(s))return false;
        for(unsigned i=0;i<4;++i)if(s.levels[i]>12u)return false;
        if((s.owned_weapons&~7u)!=0u||(s.owned_weapons&1u)==0u
            ||s.active_weapon>2u||(s.owned_weapons&(1u<<s.active_weapon))==0u)
            return false;
        if((s.support_state&~0xfffu)!=0u)return false;
        uint8_t supportLevels[4]={};
        for(unsigned i=0;i<4;++i)supportLevels[i]=static_cast<uint8_t>((s.support_state>>(i*2u))&3u);
        SupportContracts restored;
        if(!restored.restore(supportLevels,
            static_cast<uint8_t>((s.support_state>>8u)&3u),
            static_cast<uint8_t>((s.support_state>>10u)&3u)))return false;
        seed_=s.seed;node_=s.node;visited_=s.visited;cash_=s.cash;
        kills_=s.kills;bosses_=s.bosses;
        for(unsigned i=0;i<4;++i)levels_[i]=s.levels[i];
        owned_weapons_=s.owned_weapons;weapon_=static_cast<Weapon>(s.active_weapon);
        support_=restored;deployed_support_=SupportKind::None;support_pulses_=0;
        phase_=Phase::Chart; // Post-general progress remains revisitable, not a sticky result.
        return true;
    }
    bool restore(const SnapshotV2& legacy){
        if(legacy.magic!=0x474D4231u||legacy.version!=2u||
            legacy.seed==0u||legacy.node>=kLaneCount
            ||legacy.checksum!=SnapshotV2Checksum(legacy))return false;
        Snapshot migrated{};
        migrated.seed=legacy.seed;migrated.node=legacy.node;
        migrated.visited=legacy.visited;migrated.cash=legacy.cash;
        migrated.kills=legacy.kills;migrated.bosses=legacy.bosses;
        for(unsigned i=0;i<4;++i)migrated.levels[i]=legacy.levels[i];
        migrated.owned_weapons=legacy.owned_weapons;
        migrated.active_weapon=legacy.active_weapon;
        migrated.checksum=SnapshotChecksum(migrated);
        return restore(migrated);
    }
    bool restore(const SnapshotV1& legacy){
        if(legacy.magic!=0x474D4231u||legacy.version!=1u
            ||legacy.seed==0u||legacy.node>=kLaneCount
            ||legacy.bosses>15u||legacy.cash>100000000u
            ||legacy.visited>10000000u||legacy.kills>100000000u
            ||legacy.checksum!=SnapshotV1Checksum(legacy))return false;
        Snapshot migrated{};
        migrated.seed=legacy.seed;migrated.node=legacy.node;
        migrated.visited=legacy.visited;migrated.cash=legacy.cash;
        migrated.kills=legacy.kills;migrated.bosses=legacy.bosses;
        for(unsigned i=0;i<4;++i)migrated.levels[i]=legacy.levels[i];
        migrated.checksum=SnapshotChecksum(migrated);
        return restore(migrated);
    }
private:
    uint32_t next(){rng_=Mix(rng_+0x9E3779B9u);return rng_;}
    unsigned rollKind(int spawn)const{
        return Mix(seed_^destination_.id^(static_cast<uint32_t>(spawn)*0xA511E9B3u))%3u;
    }
    void damage(int amount){
        if(invulnerable_>0.f)return;
        invulnerable_=.36f;regen_delay_=2.4f;regen_credit_=0.f;
        const int absorbed=std::min(shield_,amount);
        shield_-=absorbed;hp_-=amount-absorbed;combo_=0;combo_clock_=0;hurt_flash_=.28f;
    }
    Phase phase_=Phase::Chart;
    FailureReason failure_=FailureReason::None;
    uint32_t seed_=1,node_=0,visited_=0,cash_=100,kills_=0,bosses_=0,rng_=1;
    Lane destination_{};
    MissionRules mission_{};
    SalvageField salvage_{};
    SupportContracts support_{};
    SupportKind deployed_support_=SupportKind::None;
    float support_timer_=0.f;
    int support_pulses_=0;
    float support_x_=-1.5f;
    bool slipstream_active_=false;
    uint32_t support_deployments_=0,kit_activations_=0;
    CastEncounter cast_{};
    uint32_t ally_shots_=0,bombs_defused_=0,mines_defused_=0;
    CrumbclankContract twins_{};
    float ally_x_=-1.3f;
    ShipProfile ship_profile_{};
    bool ship_ready_=true;
    uint16_t levels_[4]={0,0,0,0};
    uint32_t owned_weapons_=1u;
    Weapon weapon_=Weapon::PeaRepeater;
    int hp_=90,shield_=20,combo_=0,spawned_=0,defeated_=0,target_=0;
    bool bossKilled_=false;
    int escaped_=0,max_combo_=0;
    uint32_t sortie_cash_=0,completion_bonus_=0,shots_fired_=0,shots_hit_=0;
    float elapsed_=0,hurt_flash_=0,invulnerable_=0,regen_delay_=0,regen_credit_=0;
    float px_=0,py_=-.85f,spawn_clock_=0,cooldown_=0,combo_clock_=0;
    std::array<Enemy,kEnemySlots> enemies_{};
    std::array<Shot,48> shots_{};
};
}}
#endif
