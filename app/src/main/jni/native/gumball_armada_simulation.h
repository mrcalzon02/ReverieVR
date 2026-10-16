#ifndef REVERIE_GUMBALL_ARMADA_SIMULATION_H
#define REVERIE_GUMBALL_ARMADA_SIMULATION_H
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace reverie { namespace gumball {
constexpr uint32_t kLaneCount=4096;
constexpr unsigned kEnemySlots=24;
enum class Phase : uint8_t { Chart,Flight,Won,Lost,KingdomSaved };
enum class Upgrade : uint8_t { Blaster,Hull,Thruster,Shield };
struct Enemy { float x=0,y=0,age=0,firing=0; int hp=0; bool active=false,boss=false; };
struct Shot { float x=0,y=0,vy=0; bool active=false,hostile=false; };
struct Lane { uint32_t id=0; uint8_t hazard=0,reward=0; bool fortress=false; };
struct Snapshot {
    uint32_t magic=0x474D4231u, version=1, seed=0, node=0;
    uint32_t visited=0,cash=0,kills=0,bosses=0;
    uint16_t levels[4]={0,0,0,0};uint32_t checksum=0;
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
    int level(Upgrade u)const{return levels_[static_cast<unsigned>(u)];}
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
        px_=0;py_=-.85f;combo_=0;combo_clock_=0;cooldown_=0;
        spawned_=0;defeated_=0;bossKilled_=false;spawn_clock_=0;rng_=Mix(seed_^destination_.id^visited_);
        const uint32_t sector=destination_.id/1024u;
        target_=6+destination_.hazard*2+static_cast<int>(sector)*2;
        if(destination_.fortress&&((bosses_>>sector)&1u)==0u)++target_;
        enemies_={};shots_={};return true;
    }
    void update(float dt,float sx,float sy,bool fire){
        if(phase_!=Phase::Flight||!std::isfinite(dt)||dt<=0.f)return;
        dt=std::min(dt,.05f);
        sx=std::isfinite(sx)?std::max(-1.f,std::min(1.f,sx)):0.f;
        sy=std::isfinite(sy)?std::max(-1.f,std::min(1.f,sy)):0.f;
        const float speed=1.55f+.16f*levels_[2];
        px_=std::max(-1.9f,std::min(1.9f,px_+sx*dt*speed));
        py_=std::max(-1.15f,std::min(.65f,py_-sy*dt*speed));
        cooldown_-=dt;combo_clock_-=dt;
        if(combo_clock_<=0.f)combo_=0;
        if(fire&&cooldown_<=0.f){
            for(auto& s:shots_)if(!s.active){s={px_,py_+.12f,3.75f,true,false};break;}
            cooldown_=std::max(.10f,.34f-.022f*levels_[0]);
        }
        spawn_clock_-=dt;
        if(spawned_<target_&&spawn_clock_<=0.f){
            for(auto& e:enemies_)if(!e.active){
                const bool boss=destination_.fortress&&spawned_==target_-1
                    &&((bosses_>>(destination_.id/1024u))&1u)==0u;
                const uint32_t roll=next();
                e={-1.65f+static_cast<float>(roll%331u)*.01f,1.35f,0.f,.75f,
                   boss?(22+levels_[0]/2):(1+static_cast<int>(destination_.hazard/3)),true,boss};
                ++spawned_;break;
            }
            spawn_clock_=.56f;
        }
        for(auto& e:enemies_)if(e.active){
            e.age+=dt;e.y-=dt*(e.boss?.22f:(.23f+.018f*destination_.hazard));
            e.x+=std::sin(e.age*2.4f+e.y)*dt*(e.boss?.22f:.37f);
            e.firing-=dt;
            if(e.firing<=0.f){
                for(auto& s:shots_)if(!s.active){s={e.x,e.y-.14f,-1.3f,true,true};break;}
                e.firing=e.boss?.48f:1.6f;
            }
            if(e.y< -1.65f){
                e.active=false;++defeated_;combo_=0;
                if(!e.boss)damage(10+3*static_cast<int>(destination_.hazard));
                if(e.boss){phase_=Phase::Lost;return;}
            }
        }
        for(auto& s:shots_)if(s.active){
            s.y+=s.vy*dt;
            if(s.y>1.75f||s.y< -1.4f){s.active=false;continue;}
            if(s.hostile){
                if(std::fabs(s.x-px_)<.17f&&std::fabs(s.y-py_)<.16f){
                    s.active=false;damage(11+static_cast<int>(destination_.hazard));
                }
            }else{
                for(auto& e:enemies_)if(e.active
                    &&std::fabs(s.x-e.x)<(e.boss?.32f:.19f)
                    &&std::fabs(s.y-e.y)<(e.boss?.25f:.16f)){
                    s.active=false;e.hp-=1+levels_[0]/3;
                    if(e.hp<=0){
                        if(e.boss)bossKilled_=true;
                        e.active=false;++defeated_;++kills_;++combo_;combo_clock_=2.65f;
                        cash_+=static_cast<uint32_t>((e.boss?125u:9u)+(combo_/3)*4+destination_.reward*2);
                    }
                    break;
                }
            }
        }
        if(hp_<=0){phase_=Phase::Lost;return;}
        if(spawned_==target_&&defeated_==target_){
            node_=destination_.id;++visited_;
            if(destination_.fortress&&bossKilled_)bosses_|=1u<<(node_/1024u);
            phase_=bosses_==15u?Phase::KingdomSaved:Phase::Won;
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
        s.checksum=SnapshotChecksum(s);return s;
    }
    bool restore(const Snapshot& s){
        if(s.magic!=0x474D4231u||s.version!=1u||s.seed==0u||s.node>=kLaneCount
            ||s.bosses>15u||s.cash>100000000u||s.visited>10000000u
            ||s.kills>100000000u||s.checksum!=SnapshotChecksum(s))return false;
        for(unsigned i=0;i<4;++i)if(s.levels[i]>12u)return false;
        seed_=s.seed;node_=s.node;visited_=s.visited;cash_=s.cash;
        kills_=s.kills;bosses_=s.bosses;
        for(unsigned i=0;i<4;++i)levels_[i]=s.levels[i];
        phase_=bosses_==15u?Phase::KingdomSaved:Phase::Chart;
        return true;
    }
private:
    uint32_t next(){rng_=Mix(rng_+0x9E3779B9u);return rng_;}
    void damage(int amount){
        const int absorbed=std::min(shield_,amount);
        shield_-=absorbed;hp_-=amount-absorbed;combo_=0;combo_clock_=0;
    }
    Phase phase_=Phase::Chart;
    uint32_t seed_=1,node_=0,visited_=0,cash_=100,kills_=0,bosses_=0,rng_=1;
    Lane destination_{};
    uint16_t levels_[4]={0,0,0,0};
    int hp_=90,shield_=20,combo_=0,spawned_=0,defeated_=0,target_=0;
    bool bossKilled_=false;
    float px_=0,py_=-.85f,spawn_clock_=0,cooldown_=0,combo_clock_=0;
    std::array<Enemy,kEnemySlots> enemies_{};
    std::array<Shot,48> shots_{};
};
}}
#endif
