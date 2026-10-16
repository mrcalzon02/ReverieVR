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
    assert(std::strcmp(SupportName(SupportKind::CapitaineEclair),"CAPITAINE ECLAIR")==0);
    std::cout<<"support contract purchase, skill caps and persistence passed\n";
}
