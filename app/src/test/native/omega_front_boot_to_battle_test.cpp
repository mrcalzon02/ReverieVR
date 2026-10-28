#include "omega_front_campaign_save.h"
#include <cassert>
#include <iostream>

// Exercise the normal 2D strategy-to-battle command route without editing
// Battle directly, adding developer cheats, or depending on a VR headset.
// Android GL initialization and headset input remain a separate device gate.
using namespace reverie::omega;
static void Tap(Session &game) {
    game.Input({0,0,0,false,false,-1,-1});
    game.Input({0,0,0,true,false,-1,-1});
}
int main() {
    Session game(414);
    assert(game.view==FrontMode::SectorMap);
    assert(game.frontier.location()==6);
    assert(!game.frontier.deployed());
    // Point at sector 11, then travel by pressing the real primary input.
    game.Input({0,0,0,false,false,2,2});
    assert(game.focused==10);
    Tap(game);
    assert(game.frontier.location()==10);
    assert(game.view==FrontMode::SectorMap);
    Tap(game); // Deploy from actual selected/current sector.
    assert(game.view==FrontMode::Tactical);
    assert(game.frontier.deployed());
    assert(game.tactical.battle.unit_count()==7);
    // Nothing advances on VR frames. Only a deliberate END TURN command
    // resolves the queued orders and enemy response.
    assert(game.tactical.battle.ticks()==0);
    const Unit *soldier=game.tactical.battle.unit(0);
    assert(soldier && soldier->x==3 && soldier->y==11);
    game.Input({0,0,0,false,false,3,11});
    Tap(game);
    assert(game.tactical.selected==0);
    game.Input({0,0,0,false,false,7,11});
    Tap(game);
    assert(game.tactical.battle.queued(0)==1);
    for(int i=0;i<300;++i)game.Input({.10f,0,0,false,false,-1,-1});
    assert(game.tactical.battle.ticks()==0);
    assert(game.Input({0,0,0,false,false,-1,-1,true}));
    assert(game.tactical.battle.ticks()==1);
    assert(game.tactical.battle.unit(0)->x==4);
    const auto checkpoint=game.tactical.battle.Checksum();
    CampaignSave::Bytes bytes{};
    assert(CampaignSave::Encode(game,&bytes));
    Session resumed(888);
    assert(CampaignSave::Decode(&resumed,bytes.data(),bytes.size()));
    assert(resumed.view==FrontMode::Tactical);
    assert(resumed.frontier.deployed());
    assert(resumed.tactical.battle.Checksum()==checkpoint);
    assert(resumed.tactical.battle.unit(0)->x==4);
    assert(resumed.tactical.battle.queued(0)==1);
    for(int i=0;i<300;++i)resumed.Input({.10f,0,0,false,false,-1,-1});
    assert(resumed.tactical.battle.ticks()==1);
    assert(resumed.Input({0,0,0,false,false,-1,-1,true}));
    assert(resumed.tactical.battle.ticks()==2);
    std::cout<<"PASS Omega Front: sector map -> travel -> deploy -> unit order -> update -> save/load -> gameplay\n";
}
