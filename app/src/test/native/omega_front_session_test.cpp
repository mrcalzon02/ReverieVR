#include "omega_front_session.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
static void Tap(Session &s,bool primary,bool secondary=false){
    s.Input({0,0,0,false,false,-1,-1});
    s.Input({0,0,0,primary,secondary,-1,-1});
}
int main(){
    Session a(414),b(414);
    assert(a.Checksum()==b.Checksum());
    assert(a.view==FrontMode::SectorMap && a.frontier.location()==6);
    // Pointer selects an actual grid cell. No movement without a real order.
    a.Input({0,0,0,false,false,2,2});
    assert(a.focused==10 && a.frontier.location()==6);
    Tap(a,true);assert(a.frontier.location()==10);
    assert(a.view==FrontMode::SectorMap && a.frontier.day()==2);
    // The player has to be at a target, and pay the deployment cost.
    Tap(a,true);
    assert(a.view==FrontMode::Tactical && a.frontier.deployed());
    assert(a.tactical.battle.unit_count()==7);
    assert(a.frontier.rations()==21);
    assert(a.tactical.battle.ticks()==0);
    // Ended tactical battle returns to the map and resolves exactly once.
    for(int i=0;i<160 && a.tactical.battle.result()==Result::Active;++i)
        a.Input({.1f,0,0,false,false,-1,-1});
    assert(a.tactical.battle.result()!=Result::Active);
    const auto prior=a.frontier.Checksum();
    Tap(a,true);
    assert(a.view==FrontMode::SectorMap && !a.frontier.deployed());
    assert(a.frontier.last_report().outcome!=Result::Active);
    assert(a.frontier.Checksum()!=prior);
    const int saved=a.frontier.salvage();
    Tap(a,false);
    assert(a.frontier.salvage()==saved);
    assert(a.frontier.location()==10);
    // Map secondary cycles real options; no idle or decorative actions.
    Tap(a,false,true);assert(a.action==FieldAction::Forage);
    const int food_before=a.frontier.sector(10)->food;
    assert(a.focused==10);
    Tap(a,true);
    assert(a.frontier.sector(10)->food==food_before);
    assert(a.rejected); // damaged pump sector has insufficient food
    Tap(a,false,true);assert(a.action==FieldAction::Recruit);
    Tap(a,true);assert(a.rejected); // recruit only at Hearthwell
    Tap(a,false,true);assert(a.action==FieldAction::Repair);
    Tap(a,false,true);assert(a.action==FieldAction::Wait);
    const int day=a.frontier.day();Tap(a,true);assert(a.frontier.day()==day+1);
    // A valid tactical win returns exactly one strategic outcome, not a
    // separately fabricated victory reward. Use genuine queued AI targets.
    Session winner(414);
    winner.Input({0,0,0,false,false,2,2});Tap(winner,true);Tap(winner,true);
    assert(winner.frontier.deployed());
    for(int step=0;step<160 && winner.tactical.battle.result()==Result::Active;++step){
        Battle &fight=winner.tactical.battle;
        for(int i=0;i<4;++i){
            if(!fight.unit(i)->alive() || fight.queued(i)>0)continue;
            for(int j=4;j<fight.unit_count();++j){
                if(fight.unit(j)->alive() && fight.QueueAttack(i,j))break;
            }
        }
        winner.Input({.10f,0,0,false,false,-1,-1});
        winner.Input({.10f,0,0,false,false,-1,-1});
        winner.Input({.05f,0,0,false,false,-1,-1});
    }
    assert(winner.tactical.battle.result()==Result::Defended);
    const int before_award=winner.frontier.salvage();
    Tap(winner,true);
    assert(winner.frontier.operation()==1);
    assert(winner.frontier.sector(10)->owner==kIndependentOwner);
    assert(winner.frontier.salvage()>before_award);
    const int once=winner.frontier.salvage();
    Tap(winner,true); // no duplicate report (deploy would require S006)
    assert(winner.frontier.salvage()==once);
    // Independent replay across identical sequences, including pointer/axes.
    const ControlsInput moves[]={{0,0,0,false,false,2,2},
        {0,0,0,true,false,-1,-1}, {0,0,0,false,false,-1,-1},
        {0,.85f,0,false,false,-1,-1}, {0,0,0,false,false,-1,-1}};
    for(auto v:moves)b.Input(v);
    Session c(414);for(auto v:moves)c.Input(v);
    assert(b.Checksum()==c.Checksum());
    assert(b.focused==c.focused);
    std::cout<<"Omega Front strategic map / combat transition regressions passed\n";
}
