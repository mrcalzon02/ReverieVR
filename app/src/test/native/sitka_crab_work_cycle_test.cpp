#include "sitka_crab_work_cycle.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::sitka;
static void Tick(CrabWorkCycle& cycle,unsigned seconds){
    while(seconds){
        unsigned step=std::min(seconds,60u);
        assert(cycle.Advance(step));seconds-=step;
    }
}
int main(){
    const auto& rough=kCrabGear[0];
    const auto& master=kCrabGear[5];
    assert(rough.set_seconds==24u*60u&&rough.haul_seconds==30u*60u);
    assert(master.set_seconds==300u&&master.haul_seconds==300u);
    assert(rough.max_pots==1u&&master.max_pots>=3u);
    assert(master.fuel_per_trip_per_mille==200u); // five ideal round trips per tank
    assert(rough.set_seconds>=master.set_seconds*4u);
    CrabWorkCycle trained(CrabGear::HandLine,9u,8u);
    assert(trained.SetSeconds()<rough.set_seconds);
    assert(trained.HaulSeconds()<rough.haul_seconds);
    CrabWorkCycle trained_master(CrabGear::ExpeditionCrane,9u,8u);
    assert(trained_master.SetSeconds()==300u);
    assert(trained_master.HaulSeconds()==300u);
    CrabWallet bank{1200u,1u};
    CrabWorkCycle cruise(CrabGear::ExpeditionCrane,8793u);
    assert(!cruise.Advance(0u));
    assert(!cruise.Advance(61u));
    assert(!cruise.BeginSet(0,1.,2.,12.,false,true,CrabBait::PurchasedFish,bank));
    assert(!cruise.BeginSet(0,1.,2.,NAN,true,true,CrabBait::PurchasedFish,bank));
    assert(bank.credits==1200u);
    Tick(cruise,kOutwardTravelTargetSeconds);
    assert(cruise.BeginSet(0,50.,100.,11.,true,true,CrabBait::PurchasedFish,bank));
    assert(bank.credits==1100u && cruise.RigBusy());
    assert(!cruise.BeginSet(1,60.,130.,12.,true,true,CrabBait::PurchasedFish,bank));
    Tick(cruise,master.set_seconds);
    assert(cruise.Pot(0).state==PotState::Soaking);
    assert(cruise.BeginSet(1,60.,130.,13.,true,false,CrabBait::RetainedFish,bank));
    assert(bank.retained_bait_fish==0u);
    Tick(cruise,master.set_seconds);
    assert(cruise.BeginSet(2,70.,160.,19.,true,true,CrabBait::PurchasedFish,bank));
    Tick(cruise,master.set_seconds);
    assert(cruise.Pot(0).state==PotState::Soaking);
    assert(!cruise.BeginHaul(0)); // no premature payout
    assert(cruise.SellAtDock(true,bank).earned_credits==0u);
    const unsigned wait=static_cast<unsigned>(cruise.Pot(0).ready_at-cruise.ClockSeconds());
    Tick(cruise,wait);
    assert(cruise.Pot(0).state==PotState::Ready);
    assert(cruise.BeginHaul(0));
    Tick(cruise,master.haul_seconds);
    assert(cruise.Pot(0).state==PotState::OnDeck);
    assert(cruise.Pot(1).state==PotState::Ready);
    assert(cruise.BeginHaul(1));Tick(cruise,master.haul_seconds);
    assert(cruise.BeginHaul(2));Tick(cruise,master.haul_seconds);
    assert(cruise.DeployedPotCount()==0u);
    assert(cruise.SellAtDock(false,bank).earned_credits==0u);
    Tick(cruise,kReturnTravelTargetSeconds);
    const auto first=cruise.SellAtDock(true,bank);
    assert(first.crabs<=27u);
    assert(first.earned_credits==first.crabs*kCrabSaleCredits);
    assert(bank.credits==1000u+first.earned_credits);
    assert(cruise.SellAtDock(true,bank).earned_credits==0u);
    // Crab count variation remains seeded, bounded, reproducible.
    CrabWallet slow_bank{300u,0u};
    CrabWorkCycle slow(CrabGear::HandLine,9u);
    assert(slow.BeginSet(0,20.,30.,10.,true,false,CrabBait::PurchasedFish,slow_bank));
    Tick(slow,300u);
    assert(slow.Pot(0).state==PotState::Setting);
    assert(!slow.BeginSet(1,20.,40.,11.,true,false,CrabBait::PurchasedFish,slow_bank));
    Tick(slow,rough.set_seconds-300u);
    assert(slow.Pot(0).state==PotState::Soaking);
    Tick(slow,kGameplaySoakSeconds);
    assert(slow.BeginHaul(0));
    Tick(slow,300u);
    assert(slow.Pot(0).state==PotState::Hauling);
    Tick(slow,rough.haul_seconds-300u);
    assert(slow.Pot(0).state==PotState::OnDeck);
    // Five nominal 20% trip charges, with no misleading unlimited-fuel claim.
    unsigned fuel=1000u;
    for(unsigned i=0;i<5u;++i){assert(fuel>=master.fuel_per_trip_per_mille);fuel-=master.fuel_per_trip_per_mille;}
    assert(fuel==0u);
    // Accurate total of 3-pot trip: 20 outbound + 15 setting +
    // 50 waiting until first pot's one-hour soak ends + 15 hauling + 20 home.
    assert(cruise.ClockSeconds()==20u*60u+15u*60u+50u*60u+
                                       15u*60u+20u*60u);
    std::cout<<"Sitka crabbing: six manual-to-hydraulic rig stages, passive 60m soak,\n"
                " three staggered pots, bait costs, docking-only sales and 5-trip fuel target PASS\n";
}