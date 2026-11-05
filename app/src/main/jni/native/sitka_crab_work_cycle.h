#ifndef REVERIE_SITKA_CRAB_WORK_CYCLE_H
#define REVERIE_SITKA_CRAB_WORK_CYCLE_H
// CPU-only PLANNING prototype for Sitka's Tier-3 pot-crabbing career.
// Not yet connected to player save, boat movement, world pot props or sale UI.
// 60-minute soak is intentionally game-compressed, NOT actual Alaska fishing advice.
// Call Advance() once per game simulation clock, never once per stereo eye.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace reverie { namespace sitka {

enum class CrabGear : uint8_t {
    HandLine, HandCrank, DeckPuller, PoweredHauler, HydraulicGantry,
    ExpeditionCrane, Count
};
struct CrabGearSpec {
    const char* model_key;
    uint32_t set_seconds,haul_seconds,max_pots;
    uint32_t max_crabs_per_pot;
    uint32_t fuel_per_trip_per_mille;
};
// Time and yield values are TUNING TARGETS for a relaxing game, not field data.
inline constexpr std::array<CrabGearSpec,6> kCrabGear={{
    {"handline-buoy-pot",24u*60u,30u*60u,1u,3u,270u},
    {"manual-pot-puller",19u*60u,24u*60u,2u,4u,250u},
    {"deck-line-puller",14u*60u,18u*60u,3u,5u,240u},
    {"powered-pot-hauler",10u*60u,12u*60u,4u,6u,225u},
    {"hydraulic-pot-gantry",7u*60u,8u*60u,5u,7u,215u},
    {"expedition-crane-autoload",5u*60u,5u*60u,5u,9u,200u}
}};
constexpr uint32_t kGameplaySoakSeconds=60u*60u;
constexpr uint32_t kFishingBaitCostCredits=100u;
constexpr uint32_t kCrabSaleCredits=90u;
constexpr uint32_t kOutwardTravelTargetSeconds=20u*60u;
constexpr uint32_t kReturnTravelTargetSeconds=20u*60u;

enum class CrabBait : uint8_t { PurchasedFish, RetainedFish };
enum class PotState : uint8_t { Stowed, Setting, Soaking, Ready, Hauling, OnDeck };
struct CrabPot {
    PotState state=PotState::Stowed;
    double east=0.,north=0.,depth_metres=0.;
    bool source_depth=false; // provenance of bathymetry, not real-world crab distribution
    uint64_t work_finish=0,ready_at=0;
    uint32_t catch_count=0;
};
struct CrabWallet {
    uint32_t credits=0;
    uint32_t retained_bait_fish=0; // future fish-inventory input, NOT lifetime journal totals
};
struct CrabSale {
    uint32_t crabs=0,earned_credits=0;
};
class CrabWorkCycle {
public:
    explicit CrabWorkCycle(CrabGear equipment,uint32_t seed=0xCA861Au,
                           uint32_t deckwork_level=0u)
        :gear_(unsigned(equipment)<kCrabGear.size()?equipment:CrabGear::HandLine),
         seed_(seed?seed:0xCA861Au),
         deckwork_(std::min(8u,deckwork_level)){}
    const CrabGearSpec& Gear()const{return kCrabGear[unsigned(gear_)];}
    uint64_t ClockSeconds()const{return clock_;}
    // Player deckwork skill gives a bounded handling advantage early in
    // the chain. The master crane still targets five-minute set/haul.
    uint32_t SetSeconds()const{return HandlingSeconds(Gear().set_seconds);}
    uint32_t HaulSeconds()const{return HandlingSeconds(Gear().haul_seconds);}
    const CrabPot& Pot(unsigned id)const{return pots_.at(id);}
    uint32_t FuelPerTripPerMille()const{return Gear().fuel_per_trip_per_mille;}
    bool RigBusy()const{return working_<pots_.size();}
    unsigned DeployedPotCount()const{
        unsigned count=0;
        for(const auto& p:pots_)
            if(p.state==PotState::Setting||p.state==PotState::Soaking||
               p.state==PotState::Ready||p.state==PotState::Hauling)
                ++count;
        return count;
    }
    bool BeginSet(unsigned id,double east,double north,double depth_metres,
                  bool confirmed_water,bool surveyed_depth,
                  CrabBait bait,CrabWallet& wallet){
        if(id>=pots_.size()||RigBusy()||
           pots_[id].state!=PotState::Stowed||
           DeployedPotCount()>=Gear().max_pots||!confirmed_water||
           !std::isfinite(east)||!std::isfinite(north)||
           !std::isfinite(depth_metres)||depth_metres<=0.||
           depth_metres>2000.||std::abs(east)>1.e8||
           std::abs(north)>1.e8)return false;
        if(bait==CrabBait::PurchasedFish){
            if(wallet.credits<kFishingBaitCostCredits)return false;
            wallet.credits-=kFishingBaitCostCredits;
        }else if(bait==CrabBait::RetainedFish){
            if(wallet.retained_bait_fish==0u)return false;
            --wallet.retained_bait_fish;
        }else return false;
        pots_[id]={PotState::Setting,east,north,depth_metres,
                    surveyed_depth,clock_+SetSeconds(),0u,0u};
        working_=id;
        return true;
    }
    bool BeginHaul(unsigned id){
        if(id>=pots_.size()||RigBusy()||
           pots_[id].state!=PotState::Ready)return false;
        auto& p=pots_[id];p.state=PotState::Hauling;
        p.work_finish=clock_+HaulSeconds();
        working_=id;
        return true;
    }
    // The caller supplies elapsed in-game seconds. Bounded increments prevent
    // a single accidental frame hitch from skipping a full pot set/haul.
    bool Advance(uint32_t seconds){
        if(seconds==0u||seconds>60u||
           clock_>std::numeric_limits<uint64_t>::max()-seconds)return false;
        clock_+=seconds;
        if(RigBusy()){
            auto& p=pots_[working_];
            if(clock_>=p.work_finish){
                if(p.state==PotState::Setting){
                    p.state=PotState::Soaking;
                    p.ready_at=p.work_finish+kGameplaySoakSeconds;
                }else if(p.state==PotState::Hauling){
                    p.state=PotState::OnDeck;
                    p.catch_count=YieldFor(working_,p);
                }
                working_=unsigned(pots_.size());
            }
        }
        for(auto& p:pots_)
            if(p.state==PotState::Soaking&&clock_>=p.ready_at)
                p.state=PotState::Ready;
        return true;
    }
    // Fish may still be caught by hand while soaking; credits are ONLY paid
    // at the port sale. Calling again cannot duplicate a catch payout.
    CrabSale SellAtDock(bool at_authorized_port,CrabWallet& wallet){
        CrabSale sale;
        if(!at_authorized_port||RigBusy())return sale;
        for(auto& p:pots_){
            if(p.state!=PotState::OnDeck)continue;
            sale.crabs+=p.catch_count;
            p=CrabPot{};
        }
        const uint64_t gross=uint64_t(sale.crabs)*kCrabSaleCredits;
        sale.earned_credits=static_cast<uint32_t>(std::min<uint64_t>(
            gross,wallet.credits<1000000u?1000000u-wallet.credits:0u));
        wallet.credits+=sale.earned_credits;
        return sale;
    }
private:
    uint32_t HandlingSeconds(uint32_t base)const{
        const double factor=1.0-.025*double(deckwork_);
        return std::max(300u,static_cast<uint32_t>(std::ceil(base*factor)));
    }
    static uint32_t Hash(uint32_t x){
        x^=x>>16u;x*=0x7feb352du;x^=x>>15u;x*=0x846ca68bu;
        return x^(x>>16u);
    }
    uint32_t YieldFor(unsigned id,const CrabPot& p)const{
        const unsigned max=Gear().max_crabs_per_pot;
        const uint64_t loc_e=static_cast<uint64_t>(std::llround(std::abs(p.east)*2.));
        const uint64_t loc_n=static_cast<uint64_t>(std::llround(std::abs(p.north)*2.));
        const uint32_t random=Hash(seed_^uint32_t(loc_e)^uint32_t(loc_n>>8u)^
                                   (uint32_t(id+1u)*2654435761u));
        // Intentionally broad 0..max variance. Future authored ecology,
        // legally retained species/size and habitat quality will replace it.
        return random%(max+1u);
    }
    CrabGear gear_;
    uint32_t seed_;
    uint32_t deckwork_=0u;
    uint64_t clock_=0;
    std::array<CrabPot,5> pots_{};
    unsigned working_=5u;
};

}} // namespace reverie::sitka
#endif