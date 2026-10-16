#include "gumball_support_contracts.h"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace reverie::gumball;
int main(){
    SupportContracts s;uint32_t cash=500;
    assert(s.booked()==SupportKind::None&&s.volley_damage()==2&&s.shield_repair()==7);
    assert(!s.hire(SupportKind::None,cash));
    assert(s.hire(SupportKind::CapitaineEclair,cash)&&cash==455);
    assert(!s.hire(SupportKind::MademoiselleMacaron,cash));
    assert(s.consume()==SupportKind::CapitaineEclair&&s.consume()==SupportKind::None);
    assert(s.improve(SupportSkill::EclairVolley,cash)&&s.volley_damage()==3&&cash==365);
    assert(s.improve(SupportSkill::EclairDuration,cash)&&s.volley_count()==4);
    assert(s.improve(SupportSkill::MacaronRelief,cash)&&s.shield_repair()==10);
    assert(s.hull_repair()==7);
    assert(s.improve(SupportSkill::MacaronTractor,cash)&&s.tractor_reach()>1.f);
    uint8_t levels[4];s.export_levels(levels);
    SupportContracts saved;assert(saved.restore(levels,0));
    assert(saved.volley_damage()==s.volley_damage());
    assert(saved.tractor_reach()==s.tractor_reach());
    uint8_t invalid[4]={0,4,0,0};assert(!saved.restore(invalid,0));
    assert(!saved.restore(levels,3));
    uint32_t broke=0;assert(!s.hire(SupportKind::MademoiselleMacaron,broke));
    assert(!s.improve(SupportSkill::EclairVolley,broke));
    uint32_t rich=10000;
    for(int i=1;i<3;++i)assert(saved.improve(SupportSkill::EclairVolley,rich));
    assert(saved.level(SupportSkill::EclairVolley)==3);
    assert(!saved.improve(SupportSkill::EclairVolley,rich));
    assert(saved.hire(SupportKind::MademoiselleMacaron,rich));
    saved.export_levels(levels);
    SupportContracts restored;
    assert(restored.restore(levels,static_cast<uint8_t>(saved.booked())));
    assert(restored.booked()==SupportKind::MademoiselleMacaron);
    assert(restored.consume()==SupportKind::MademoiselleMacaron);
    assert(restored.booked()==SupportKind::None);
    // Limited independent emergency supplies, with actual debit, no free
    // duplicate purchase, and persistence separate from escort booking.
    uint32_t kitCash=150;
    SupportContracts kits;
    assert(kits.buy_kit(SupportKit::HullPatch,kitCash)&&kitCash==85u);
    assert(!kits.buy_kit(SupportKit::HullPatch,kitCash)&&kitCash==85u);
    assert(kits.buy_kit(SupportKit::ShieldTonic,kitCash)&&kitCash==40u);
    assert(kits.kit_mask()==3u);
    uint8_t kitLevels[4];kits.export_levels(kitLevels);
    SupportContracts kitRestored;
    assert(kitRestored.restore(kitLevels,0,kits.kit_mask()));
    assert(kitRestored.has_kit(SupportKit::HullPatch));
    assert(kitRestored.has_kit(SupportKit::ShieldTonic));
    assert(!kitRestored.restore(kitLevels,0,4u));
    assert(kitRestored.kit_mask()==3u);
    assert(kitRestored.use_kit(SupportKit::HullPatch));
    assert(!kitRestored.use_kit(SupportKit::HullPatch));
    assert(kitRestored.kit_mask()==2u);
    assert(kitRestored.use_kit(SupportKit::ShieldTonic));
    assert(!kitRestored.use_kit(SupportKit::ShieldTonic));
    assert(!kitRestored.buy_kit(static_cast<SupportKit>(9),kitCash));
    assert(std::strcmp(SupportKitName(SupportKit::HullPatch),"HULL PATCH")==0);
    assert(std::strcmp(SupportName(SupportKind::CapitaineEclair),"CAPITAINE ECLAIR")==0);
    std::cout<<"support contract purchase, skill caps and persistence passed\n";
}
