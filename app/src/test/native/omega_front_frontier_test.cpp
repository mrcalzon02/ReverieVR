#include "omega_front_frontier.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;

static bool Win(Battle &b){
    for(int tick=0;tick<160 && b.result()==Result::Active;++tick){
        for(int i=0;i<4;++i){
            const Unit* u=b.unit(i);
            if(!u||!u->alive())continue;
            if(b.queued(i)>0)continue;
            for(int j=4;j<b.unit_count();++j){
                if(b.unit(j)->alive() && b.QueueAttack(i,j))break;
            }
        }
        b.Step();
    }
    return b.result()==Result::Defended;
}
int main(){
    Frontier a(414),b(414);
    assert(a.Checksum()==b.Checksum());
    assert(a.location()==6 && a.day()==1 && a.operation()==0);
    assert(a.operation_faction()==0 && a.target_sector()==10);
    assert(a.sector(6)->previous==Ecology::Wasteland);
    assert(a.sector(10)->ecology==Ecology::Eden);
    assert(a.sector(6)->owner==kIndependentOwner);
    assert(kMajorFactions==17 && kIndependentOwner==18);
    assert(a.sector(-1)==nullptr && a.sector(16)==nullptr);
    assert(!a.can_travel(15) && !a.can_travel(0));
    assert(!a.can_travel(10+4)); // non-adjacent
    assert(!a.StartOperation()); // target is S011, not Hearthwell
    assert(a.next_morphology_day()==4 && a.forecast_sector()==8);
    assert(a.Travel(10) && b.Travel(10));
    assert(a.location()==10 && a.day()==2);
    assert(!a.can_travel(14)); // viaduct is severed
    assert(!a.RepairBridge());
    assert(a.StartOperation() && b.StartOperation());
    assert(a.current_battle()!=nullptr && a.current_battle()->unit_count()==7);
    assert(a.rations()==21);
    assert(!a.StartOperation() && !a.Travel(6) && !a.AdvanceDay());
    assert(!a.ResolveOperation()); // live fight cannot award rewards
    assert(a.Checksum()==b.Checksum());
    const int before=a.salvage();
    Battle* first=a.current_battle();
    bool win=Win(*first);
    std::cerr << "first fight=" << (win?"defended":"overrun") << " ticks="
              << first->ticks() << " pump=" << first->pump_hp() << "\n";
    assert(a.ResolveOperation());
    assert(!a.ResolveOperation()); // one battle == one award
    assert(a.salvage()>=before);
    assert(a.guards()<=12);
    assert(a.last_report().outcome!=Result::Active);
    assert(a.operation()==(win?1:0));
    if(win)assert(a.operation_faction()==1 && a.target_sector()==5);
    if(!win){
        assert(a.trust()==50);
        assert(a.sector(10)->claims>8);
    } else {
        assert(a.sector(10)->owner==kIndependentOwner);
        assert(a.trust()==60);
    }
    assert(win); // First operation must genuinely be completable.
    assert(a.Travel(6) && a.Travel(5));
    assert(a.operation()==1 && a.StartOperation());
    assert(a.current_battle()->unit_count()==8);
    bool win2=Win(*a.current_battle());
    std::cerr<<"second fight="<<(win2?"defended":"overrun")<<" ticks="<<a.current_battle()->ticks()<<"\n";
    assert(a.ResolveOperation());
    if(win2){
        assert(a.operation()==2);
        assert(a.Travel(6) && a.Travel(10));
        assert(a.StartOperation());
        assert(a.current_battle()->unit_count()==9);
        bool win3=Win(*a.current_battle());
        std::cerr<<"third fight="<<(win3?"defended":"overrun")<<" ticks="<<a.current_battle()->ticks()<<"\n";
        assert(a.ResolveOperation());
        assert(!a.ResolveOperation());
        assert(a.operation()==(win3?3:2));
        if(win3)assert(a.target_sector()==-1);
        else assert(a.last_report().outcome==Result::Overrun);
    }
    // Morphological advance is forecast, deterministic and conserves stores.
    Frontier c(55),d(55);
    assert(c.Travel(5) && d.Travel(5));
    assert(c.AdvanceDay() && d.AdvanceDay());
    assert(c.sector(8)->ecology==Ecology::Wasteland);
    assert(c.AdvanceDay() && d.AdvanceDay());
    assert(c.day()==4 && c.sector(8)->ecology==Ecology::Eden);
    assert(c.sector(8)->previous==Ecology::Wasteland);
    assert(c.sector(8)->food==0 && c.sector(8)->water==0);
    assert(c.morphology_epoch()==1 && c.next_morphology_day()==-1);
    assert(c.Checksum()==d.Checksum());
    assert(!c.Recruit()); // only at home
    assert(c.Travel(6));
    assert(!c.Recruit()); // guards full
    assert(!c.RepairPump()); // not at pump
    // Meals and ammunition cannot duplicate: taking field rations consumes
    // physical stored food, and repairs require actual salvaged metal.
    Frontier barter(79);
    const auto food_before=barter.sector(6)->food;
    const int rations_before=barter.rations();
    assert(barter.ForageRations());
    assert(barter.sector(6)->food==food_before-6);
    assert(barter.rations()==rations_before+4);
    Battle configured(24);
    assert(configured.ConfigurePump(9));
    assert(configured.pump_hp()==9);
    configured.Step();
    assert(!configured.ConfigurePump(18));
    Frontier walk(79);
    assert(walk.Travel(10));
    assert(!walk.can_travel(14));
    assert(walk.Travel(11) && walk.Travel(15)==false); // barren inaccessible
    Frontier bridge(99);
    assert(bridge.Travel(10) && bridge.Travel(9) && bridge.Travel(13));
    assert(bridge.Travel(14));
    const int metal=bridge.salvage();
    assert(bridge.RepairBridge());
    assert(bridge.salvage()==metal-6);
    assert(!bridge.RepairBridge());
    assert(bridge.can_travel(10));
    // The campaign is not a generic arc-count unlock; three real operations.
    std::cout<<"Omega Front first-frontier strategic and battle regression passed\n";
}
