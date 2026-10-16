#ifndef REVERIE_GUMBALL_SUPPORT_CONTRACTS_H
#define REVERIE_GUMBALL_SUPPORT_CONTRACTS_H
#include <cstdint>
namespace reverie { namespace gumball {
enum class SupportKind : uint8_t { None, CapitaineEclair, MademoiselleMacaron };
enum class SupportSkill : uint8_t { EclairVolley, EclairDuration, MacaronRelief, MacaronTractor };
inline const char* SupportName(SupportKind kind){
    switch(kind){
    case SupportKind::CapitaineEclair:return "CAPITAINE ECLAIR";
    case SupportKind::MademoiselleMacaron:return "MLLE MACARON";
    default:return "NO CONTRACT";
    }
}
class SupportContracts {
public:
    static constexpr uint8_t kMaxLevel=3;
    uint8_t level(SupportSkill skill)const{
        const unsigned i=static_cast<unsigned>(skill);
        return i<4?levels_[i]:0;
    }
    uint32_t upgrade_cost(SupportSkill skill)const{
        const unsigned i=static_cast<unsigned>(skill);
        return i<4&&levels_[i]<kMaxLevel?90u+45u*levels_[i]:0u;
    }
    bool improve(SupportSkill skill,uint32_t& cash){
        const unsigned i=static_cast<unsigned>(skill);
        const uint32_t cost=upgrade_cost(skill);
        if(i>=4||cost==0||cash<cost)return false;
        cash-=cost;++levels_[i];return true;
    }
    uint32_t hire_price(SupportKind kind)const{
        return kind==SupportKind::CapitaineEclair?45u:
               (kind==SupportKind::MademoiselleMacaron?55u:0u);
    }
    bool hire(SupportKind kind,uint32_t& cash){
        const uint32_t price=hire_price(kind);
        if(booked_!=SupportKind::None||price==0u||cash<price)return false;
        cash-=price;booked_=kind;return true;
    }
    SupportKind booked()const{return booked_;}
    SupportKind consume(){
        const SupportKind kind=booked_;booked_=SupportKind::None;return kind;
    }
    int volley_damage()const{return 2+static_cast<int>(levels_[0]);}
    int volley_count()const{return 3+static_cast<int>(levels_[1]);}
    int shield_repair()const{return 7+3*static_cast<int>(levels_[2]);}
    int hull_repair()const{return 5+2*static_cast<int>(levels_[2]);}
    float tractor_reach()const{return 1.00f+.18f*static_cast<float>(levels_[3]);}
    bool restore(const uint8_t (&levels)[4],uint8_t hired){
        for(uint8_t l:levels)if(l>kMaxLevel)return false;
        if(hired>static_cast<uint8_t>(SupportKind::MademoiselleMacaron))return false;
        for(unsigned i=0;i<4;++i)levels_[i]=levels[i];
        booked_=static_cast<SupportKind>(hired);return true;
    }
    void export_levels(uint8_t (&levels)[4])const{
        for(unsigned i=0;i<4;++i)levels[i]=levels_[i];
    }
private:
    uint8_t levels_[4]={0,0,0,0};
    SupportKind booked_=SupportKind::None;
};
}}
#endif
