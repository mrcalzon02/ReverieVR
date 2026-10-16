#ifndef REVERIE_GUMBALL_ARMADA_SIMULATION_H
#define REVERIE_GUMBALL_ARMADA_SIMULATION_H
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace reverie { namespace gumball {
constexpr uint32_t kLaneCount=4096;
constexpr unsigned kEnemySlots=24;
enum class Phase : uint8_t { Chart,Flight,Won,Lost,ArmadaDefeated };
enum class Upgrade : uint8_t { Blaster,Hull,Thruster,Shield };
enum class EnemyKind : uint8_t { Scout, Interceptor, Bomber, Ace, Fortress };
enum class Weapon : uint8_t { PeaRepeater, RockCannon, MintSpread };
struct Enemy { float x=0,y=0,age=0,firing=0; int hp=0; bool active=false,boss=false; EnemyKind kind=EnemyKind::Scout; };
struct Shot { float x=0,y=0,vy=0; bool active=false,hostile=false; float vx=0.f; int damage=1; };
struct Lane { uint32_t id=0; uint8_t hazard=0,reward=0; bool fortress=false; };
struct SnapshotV1 {
    uint32_t magic=0x474D4231u, version=1, seed=0, node=0;
    uint32_t visited=0,cash=0,kills=0,bosses=0;
    uint16_t levels[4]={0,0,0,0};uint32_t checksum=0;
};
struct Snapshot {
    uint32_t magic=0x474D4231u, version=2, seed=0, node=0;
    uint32_t visited=0,cash=0,kills=0,bosses=0;
    uint16_t levels[4]={0,0,0,0};
    uint32_t owned_weapons=1u,active_weapon=0u,checksum=0u;
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
inline uint32_t SnapshotV1Checksum(const SnapshotV1& s){
    uint32_t h=2166136261u;SnapshotV1 copy=s;copy.checksum=0;
    const auto* b=reinterpret_cast<const uint8_t*>(&copy);
    for(unsigned i=0;i<sizeof(copy);++i)h=(h^b[i])*16777619u;
    return h;
}
static_assert(sizeof(SnapshotV1)==44,"Legacy save binary shape must remain frozen");
class Simulation {
public:
    explicit Simulation(uint32_t seed=0xC4A0D1u):seed_(seed?seed:1u){}
    Phase phase()const{return phase_;}
    uint32_t node()const{return node_;}
    uint32_t cash()const{return cash_;}
    uint32_t kills()const{return kills_;}
    uint32_t visited()const{return visited_;}
    uint32_t bosses()const{return bosses_;}
    int hp()const{return hp_;}
    int shield()const{return shield_;}
    int combo()const{return combo_;}
    int max_combo()const{return max_combo_;}
    int escaped()const{return escaped_;}
    int defeated()const{return defeated_;}
    int target()const{return target_;}
    int spawned()const{return spawned_;}
    int wave()const{return target_>0?std::min(3,1+(spawned_*3)/target_):0;}
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
        if(phase_!=Phase::Chart||choice>=3)return false;
        destination_=lane(choice);phase_=Phase::Flight;
        hp_=90+levels_[1]*20;shield_=20+levels_[3]*18;
        px_=0;py_=-.85f;combo_=0;max_combo_=0;combo_clock_=0;cooldown_=0;
        escaped_=0;sortie_cash_=0;completion_bonus_=0;shots_fired_=0;shots_hit_=0;elapsed_=0;hurt_flash_=0;invulnerable_=0;regen_delay_=0;regen_credit_=0;
        spawned_=0;defeated_=0;bossKilled_=false;spawn_clock_=0;rng_=Mix(seed_^destination_.id^visited_);
        const uint32_t sector=destination_.id/1024u;
        target_=6+destination_.hazard*2+static_cast<int>(sector)*2;
        if(destination_.fortress&&((bosses_>>sector)&1u)==0u)++target_;
        enemies_={};shots_={};return true;
    }
    void update(float dt,float sx,float sy,bool fire){
        if(phase_!=Phase::Flight||!std::isfinite(dt)||dt<=0.f)return;
        dt=std::min(dt,.05f);
        elapsed_+=dt;hurt_flash_=std::max(0.f,hurt_flash_-dt);
        invulnerable_=std::max(0.f,invulnerable_-dt);
        regen_delay_=std::max(0.f,regen_delay_-dt);
        if(regen_delay_<=0.f){
            regen_credit_+=dt*2.f;
            const int restored=static_cast<int>(regen_credit_);
            if(restored>0){
                shield_=std::min(20+levels_[3]*18,shield_+restored);
                regen_credit_-=static_cast<float>(restored);
            }
        }
        sx=std::isfinite(sx)?std::max(-1.f,std::min(1.f,sx)):0.f;
        sy=std::isfinite(sy)?std::max(-1.f,std::min(1.f,sy)):0.f;
        const float speed=1.55f+.16f*levels_[2];
        px_=std::max(-1.9f,std::min(1.9f,px_+sx*dt*speed));
        py_=std::max(-1.15f,std::min(.65f,py_-sy*dt*speed));
        cooldown_-=dt;combo_clock_-=dt;
        if(combo_clock_<=0.f)combo_=0;
        if(fire&&cooldown_<=0.f){
            const int pellets=weapon_==Weapon::MintSpread?3:1;
            bool emitted=false;
            for(int n=0;n<pellets;++n){
                for(auto& shot:shots_)if(!shot.active){
                    const float vx=pellets==3?(n-1)*.55f:0.f;
                    const int damage=weapon_==Weapon::RockCannon?
                        4+levels_[0]/2:(weapon_==Weapon::MintSpread?
                        1+levels_[0]/4:1+levels_[0]/3);
                    shot={px_+(n-1)*.028f,py_+.12f,
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
                const bool ace=!boss&&spawned_==target_-1;
                const unsigned rollType=rollKind(spawned_);
                const EnemyKind kind=boss?EnemyKind::Fortress:(ace?EnemyKind::Ace:
                    (rollType==0u?EnemyKind::Scout:
                    (rollType==1u?EnemyKind::Interceptor:EnemyKind::Bomber)));
                const uint32_t roll=next();
                e={-1.65f+static_cast<float>(roll%331u)*.01f,1.35f,0.f,1.45f,
                   boss?(22+static_cast<int>(destination_.id/1024u)*4):(kind==EnemyKind::Ace?7:
                   (kind==EnemyKind::Bomber?4:(kind==EnemyKind::Interceptor?3:2))),true,boss,kind};
                ++spawned_;break;
            }
            spawn_clock_=(spawned_%5==0?2.00f:.56f);
        }
        for(auto& e:enemies_)if(e.active){
            e.age+=dt;
            const float speed=e.kind==EnemyKind::Interceptor?.53f:
                (e.kind==EnemyKind::Bomber?.18f:
                (e.kind==EnemyKind::Ace?.43f:(e.boss?.15f:.31f)));
            e.y-=dt*(speed+.01f*destination_.hazard);
            if(e.kind==EnemyKind::Interceptor){
                const float dx=std::max(-1.f,std::min(1.f,px_-e.x));
                e.x+=dt*dx*.58f;
            }else if(e.kind==EnemyKind::Bomber){
                e.x+=dt*std::sin(e.age*1.1f)*.09f;
            }else if(e.kind==EnemyKind::Ace){
                e.x+=dt*std::sin(e.age*3.6f)*.82f;
            }else e.x+=std::sin(e.age*2.4f+e.y)*dt*(e.boss?.22f:.37f);
            e.x=std::max(-1.85f,std::min(1.85f,e.x));
            e.firing-=dt;
            if(e.firing<=0.f){
                const int count=e.boss?3:(e.kind==EnemyKind::Ace?2:1);
                const float aim=e.kind==EnemyKind::Interceptor?
                    std::max(-.50f,std::min(.50f,(px_-e.x)*.32f)):0.f;
                for(int n=0;n<count;++n)for(auto& shot:shots_)if(!shot.active){
                    const float spread=e.boss?(n-1)*.43f:
                        (e.kind==EnemyKind::Ace?(n*2-1)*.24f:0.f);
                    shot={e.x,e.y-.14f,e.kind==EnemyKind::Bomber?-1.05f:-1.3f,
                          true,true,aim+spread};
                    break;
                }
                e.firing=e.boss?.70f:(e.kind==EnemyKind::Ace?1.18f:
                    (e.kind==EnemyKind::Bomber?1.85f:2.15f));
            }
            if(e.y< -1.65f){
                e.active=false;++defeated_;++escaped_;combo_=0;
                if(escaped_>=3){phase_=Phase::Lost;return;}
                if(!e.boss)damage(10+3*static_cast<int>(destination_.hazard));
                if(e.boss||e.kind==EnemyKind::Ace){phase_=Phase::Lost;return;}
            }
        }
        for(auto& s:shots_)if(s.active){
            s.y+=s.vy*dt;s.x+=s.vx*dt;
            if(s.y>1.75f||s.y< -1.4f){s.active=false;continue;}
            if(s.hostile){
                if(std::fabs(s.x-px_)<.17f&&std::fabs(s.y-py_)<.16f){
                    s.active=false;damage(7+static_cast<int>(destination_.hazard));
                }
            }else{
                for(auto& e:enemies_)if(e.active
                    &&std::fabs(s.x-e.x)<(e.boss?.32f:.19f)
                    &&std::fabs(s.y-e.y)<(e.boss?.25f:.16f)){
                    s.active=false;++shots_hit_;e.hp-=s.damage;
                    if(e.hp<=0){
                        if(e.boss)bossKilled_=true;
                        e.active=false;++defeated_;++kills_;++combo_;max_combo_=std::max(max_combo_,combo_);combo_clock_=2.65f;
                        const uint32_t reward=static_cast<uint32_t>((e.boss?125u:9u)+(combo_/3)*4+destination_.reward*2);
                        cash_+=reward;sortie_cash_+=reward;
                    }
                    break;
                }
            }
        }
        if(hp_<=0){phase_=Phase::Lost;return;}
        if(spawned_==target_&&defeated_==target_){
            completion_bonus_=static_cast<uint32_t>(20+destination_.reward*12+max_combo_*2);
            cash_+=completion_bonus_;sortie_cash_+=completion_bonus_;
            node_=destination_.id;++visited_;
            if(destination_.fortress&&bossKilled_)bosses_|=1u<<(node_/1024u);
            phase_=bosses_==15u?Phase::ArmadaDefeated:Phase::Won;
        }
    }
    bool recover(){
        if(phase_==Phase::Won||phase_==Phase::Lost){phase_=Phase::Chart;return true;}
        return false;
    }
    Snapshot snapshot()const{
        Snapshot s;s.seed=seed_;s.node=node_;s.visited=visited_;s.cash=cash_;
        s.kills=kills_;s.bosses=bosses_;
        for(unsigned i=0;i<4;++i)s.levels[i]=levels_[i];
        s.owned_weapons=owned_weapons_;s.active_weapon=static_cast<uint32_t>(weapon_);
        s.checksum=SnapshotChecksum(s);return s;
    }
    bool restore(const Snapshot& s){
        if(s.magic!=0x474D4231u||s.version!=2u||s.seed==0u||s.node>=kLaneCount
            ||s.bosses>15u||s.cash>100000000u||s.visited>10000000u
            ||s.kills>100000000u||s.checksum!=SnapshotChecksum(s))return false;
        for(unsigned i=0;i<4;++i)if(s.levels[i]>12u)return false;
        if((s.owned_weapons&~7u)!=0u||(s.owned_weapons&1u)==0u
            ||s.active_weapon>2u||(s.owned_weapons&(1u<<s.active_weapon))==0u)
            return false;
        seed_=s.seed;node_=s.node;visited_=s.visited;cash_=s.cash;
        kills_=s.kills;bosses_=s.bosses;
        for(unsigned i=0;i<4;++i)levels_[i]=s.levels[i];
        owned_weapons_=s.owned_weapons;weapon_=static_cast<Weapon>(s.active_weapon);
        phase_=bosses_==15u?Phase::ArmadaDefeated:Phase::Chart;
        return true;
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
    uint32_t seed_=1,node_=0,visited_=0,cash_=100,kills_=0,bosses_=0,rng_=1;
    Lane destination_{};
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
