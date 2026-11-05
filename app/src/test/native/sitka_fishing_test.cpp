#include "sitka_fishing.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::sitka;
int main() {
    Fishing f;
    assert(!f.Cast(false,4.f) && !f.Cast(true,NAN) && !f.Cast(true,0.f));
    assert(f.Cast(true,2.f) && !f.Cast(true,2.f));
    Fishing copy=f;
    for(int i=0;i<120 && f.phase!=FishingPhase::Bite;++i){f.Step(.1f,false,false);copy.Step(.1f,false,false);}
    assert(f.phase==FishingPhase::Bite && f.hooked==copy.hooked);
    f.Step(.05f,true,false);
    assert(f.phase==FishingPhase::Reeling);
    for(int i=0;i<1200 && f.phase!=FishingPhase::Landed;++i) {
        f.Step(.1f,i%8<4,false);
    }
    assert(f.phase==FishingPhase::Landed && f.journal.total==1u);
    assert(!f.Cast(true,3.f) && f.AcknowledgeCatch());
    for(int i=0;i<3;++i){
        assert(f.Cast(true,10.f));
        for(int j=0;j<120 && f.phase!=FishingPhase::Bite;++j)f.Step(.1f,false,false);
        assert(f.phase==FishingPhase::Bite);
        f.Step(.1f,true,false);
        for(int j=0;j<1200 && f.phase!=FishingPhase::Landed;++j)f.Step(.1f,j%8<4,false);
        assert(f.AcknowledgeCatch());
    }
    assert(f.journal.total==4u);
    uint32_t expected_credits=0u;
    for(unsigned k=0;k<4;++k)expected_credits+=
        f.journal.caught[k]*kFishCreditAwards[k];
    assert(f.journal.credits==expected_credits);
    assert(!f.UpgradeTackle()); // fish credits alone never unlock a shop tier
    assert(f.Discover(0u,BeachFind::Shell));
    assert(!f.Discover(0u,BeachFind::Shell));
    const auto offer=NextEquipment(f.journal,Equipment::Rod);
    assert(offer.available&&offer.unlocked&&offer.required_sites==1u);
    assert(f.UpgradeTackle()&&f.journal.gear==1u&&
           f.journal.credits==expected_credits-offer.credits);
    assert(!f.UpgradeTackle()); // another unique beach discovery needed
    assert(f.Discover(1u,BeachFind::Crab));
    const auto line=NextEquipment(f.journal,Equipment::Line);
    assert(line.unlocked && f.Purchase(Equipment::Line));
    assert(f.journal.line_level==1u);
    assert(!f.Discover(64u,BeachFind::Shell));
    assert(!f.Discover(2u,BeachFind::Count));
    auto saved=SaveJournal(f.journal);
    Journal restored;
    assert(LoadJournal(saved,restored) && SaveJournal(restored)==saved);
    const auto untouched=SaveJournal(restored);
    assert(!LoadJournal(saved+"x",restored) && SaveJournal(restored)==untouched);
    assert(!LoadJournal("SITKA-FISH-1\n1\n0\n0\n1\n0\n0\n0\n0\n",restored));
    assert(!LoadJournal("SITKA-FISH-2\n",restored));
    assert(f.Cast(true,5.f));f.Step(NAN,false,false);f.Step(99.f,false,true);
    assert(f.phase==FishingPhase::Ready);
    std::cout<<"Sitka fishing simulation, upgrades, save and rejection PASS\n";
}
