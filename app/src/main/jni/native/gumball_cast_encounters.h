#ifndef REVERIE_GUMBALL_CAST_ENCOUNTERS_H
#define REVERIE_GUMBALL_CAST_ENCOUNTERS_H
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace reverie { namespace gumball {
enum class CastGuest : uint8_t { None, PipParfait, LollyWren, BlortBlister, NibNoxious, CrumbclankTwins, CinnamonCharles, SydneySlipstream, JawbreakerJules };
inline uint32_t CastHash(uint32_t value){
    value^=value>>16;value*=0x7feb352du;
    value^=value>>15;value*=0x846ca68bu;
    return value^(value>>16);
}
inline CastGuest ChooseCastGuest(uint32_t seed,uint32_t destination,
                                 uint32_t completed,uint32_t risk){
    if(completed==0u)return CastGuest::None;
    const uint32_t choice=CastHash(seed^destination^(completed*0x9e3779b9u)^(risk<<16))%9u;
    return static_cast<CastGuest>(choice);
}
inline const char* CastGuestName(CastGuest id){
    switch(id){
    case CastGuest::PipParfait:return "PIP PARFAIT";
    case CastGuest::LollyWren:return "LOLLY WREN";
    case CastGuest::BlortBlister:return "BLORT BLISTER";
    case CastGuest::NibNoxious:return "NIB NOXIOUS";
    case CastGuest::CrumbclankTwins:return "BIFF & BAFF";
    case CastGuest::CinnamonCharles:return "CINNAMON CHARLES";
    case CastGuest::SydneySlipstream:return "SYDNEY SLIPSTREAM";
    case CastGuest::JawbreakerJules:return "JAWBREAKER JULES";
    default:return "";
    }
}
// First content slice: one seeded guest per sortie, at most five real ally
// pulses, and no heap allocation, network, runtime scripting or save slot.
// Shared pair-reward policy. Only real destruction callbacks should feed it;
// escapes, merely meeting the twins and repeated callbacks yield no bonus.
class CrumbclankContract {
public:
    void reset(){biff_=false;baff_=false;paid_=false;}
    uint32_t destroyed(bool isBiff,bool isBaff){
        if(isBiff)biff_=true;
        if(isBaff)baff_=true;
        if(!paid_&&biff_&&baff_){
            paid_=true;return 32u;
        }
        return 0u;
    }
    bool awarded()const{return paid_;}
    bool both_destroyed()const{return biff_&&baff_;}
private:
    bool biff_=false,baff_=false,paid_=false;
};
class CastEncounter {
public:
    void begin(uint32_t seed,uint32_t destination,uint32_t completed,uint32_t risk){
        kind_=ChooseCastGuest(seed,destination,completed,risk);
        elapsed_=0.f;active_left_=0.f;cooldown_=0.f;pulses_=0;
        started_=false;
        start_time_=3.5f+static_cast<float>(CastHash(seed^destination)%5u)*.45f;
    }
    void update(float dt){
        if(!std::isfinite(dt)||dt<=0.f)return;
        dt=std::min(dt,.05f);elapsed_+=dt;
        if(kind_!=CastGuest::PipParfait&&kind_!=CastGuest::LollyWren)return;
        if(!started_&&elapsed_>=start_time_){
            started_=true;active_left_=5.5f;cooldown_=0.f;
        }
        if(started_&&active_left_>0.f){
            active_left_=std::max(0.f,active_left_-dt);
            cooldown_=std::max(0.f,cooldown_-dt);
        }
    }
    bool pulse_ready()const{return active()&&cooldown_<=0.f&&pulses_<5;}
    bool claim_pulse(){
        if(!pulse_ready())return false;
        ++pulses_;cooldown_=1.0f;return true;
    }
    CastGuest kind()const{return kind_;}
    bool active()const{return active_left_>0.f;}
    int pulses()const{return pulses_;}
    bool triggered()const{return started_;}
private:
    CastGuest kind_=CastGuest::None;
    float elapsed_=0.f,start_time_=0.f,active_left_=0.f,cooldown_=0.f;
    int pulses_=0;
    bool started_=false;
};
}}
#endif
