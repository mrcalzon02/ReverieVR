#ifndef REVERIE_GUMBALL_ARCADE_OBJECTIVES_H
#define REVERIE_GUMBALL_ARCADE_OBJECTIVES_H
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace reverie { namespace gumball {
enum class MissionKind : uint8_t { Sweep=0, Intercept=1, Recovery=2 };
struct MissionRules {
    MissionKind kind=MissionKind::Sweep;
    int permitted_escapes=2;       // Third escape fails a sweep.
    int required_salvage=0;
    float time_limit=0.f;          // Zero: untimed.
    uint32_t bonus=0u;
};
inline MissionRules MissionFor(unsigned route, unsigned danger) {
    const unsigned risk=std::min(danger,8u);
    switch(route) {
        case 0: return {MissionKind::Sweep,2,0,0.f,20u+5u*risk};
        case 1: return {MissionKind::Intercept,0,0,55.f+3.f*risk,40u+8u*risk};
        case 2: return {MissionKind::Recovery,2,3,0.f,30u+6u*risk};
        default: return {};
    }
}
inline const char* MissionName(MissionKind kind) {
    switch(kind) {
        case MissionKind::Sweep: return "SOUR SWEEP";
        case MissionKind::Intercept: return "COURIER INTERCEPT";
        case MissionKind::Recovery: return "SALVAGE RECOVERY";
    }
    return "UNKNOWN MISSION";
}
struct SalvagePickup {
    float x=0.f, y=0.f;
    uint16_t cash=0;
    bool active=false;
};
class SalvageField {
public:
    static constexpr unsigned kCapacity=24;
    bool drop(float x,float y,uint16_t cash) {
        if(!std::isfinite(x)||!std::isfinite(y)||cash==0u)return false;
        for(auto& p:items_)if(!p.active) {
            p={x,y,cash,true};++dropped_;return true;
        }
        return false;
    }
    // Updates every logical tick; no allocation, rendering, or save I/O.
    uint32_t update(float dt,float player_x,float player_y) {
        if(!std::isfinite(dt)||dt<=0.f||!std::isfinite(player_x)
            ||!std::isfinite(player_y))return 0u;
        dt=std::min(dt,.05f);
        uint32_t award=0u;
        for(auto& p:items_)if(p.active) {
            p.y-=dt*.24f;
            if(std::fabs(p.x-player_x)<.23f&&std::fabs(p.y-player_y)<.22f) {
                award+=p.cash;p.active=false;++collected_;
            } else if(p.y< -1.35f) {p.active=false;++expired_;}
        }
        return award;
    }
    bool empty()const {
        for(const auto& p:items_)if(p.active)return false;
        return true;
    }
    int collected()const{return collected_;}
    int expired()const{return expired_;}
    int dropped()const{return dropped_;}
    const std::array<SalvagePickup,kCapacity>& items()const{return items_;}
    void clear(){items_={};collected_=0;expired_=0;dropped_=0;}
private:
    std::array<SalvagePickup,kCapacity> items_{};
    int collected_=0,expired_=0,dropped_=0;
};
}}
#endif
