#include "gumball_armada_simulation.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::gumball;
int main(){
    Simulation game(42), replay(42);
    assert(game.node()==0 && game.cash()==100 && kLaneCount==4096);
    for(unsigned i=0;i<3;++i){
        const auto a=game.lane(i), b=replay.lane(i);
        assert(a.id==b.id && a.hazard==b.hazard && a.id<kLaneCount);
    }
    assert(game.buy(Upgrade::Blaster));
    assert(game.level(Upgrade::Blaster)==1 && game.cash()==30);
    assert(!game.buy(Upgrade::Blaster));
    const auto save=game.snapshot();
    assert(replay.restore(save));
    assert(replay.cash()==30 && replay.level(Upgrade::Blaster)==1);
    auto tampered=save;tampered.cash=9999;
    assert(!replay.restore(tampered));

    // Steering must not depend on the fire button.
    Simulation pad(9001);assert(pad.launch(0));
    const float startX=pad.player_x();
    pad.update(.05f,1.0f,0.0f,false);
    assert(pad.player_x()>startX && pad.kills()==0);
    // Fortress routes cannot be completed by slipping past the boss.
    Simulation fortress(7);
    auto edge=fortress.snapshot();
    edge.node=1022;edge.checksum=SnapshotChecksum(edge);
    assert(fortress.restore(edge));
    for(unsigned i=0;i<3;++i)
        assert(fortress.lane(i).id==1023 && fortress.lane(i).fortress);
    assert(fortress.launch(0));
    for(int i=0;i<24000&&fortress.phase()==Phase::Flight;++i)
        fortress.update(.05f,0,0,false);
    assert(fortress.phase()==Phase::Lost);
    assert(fortress.bosses()==0u && fortress.node()==1022u);
    edge=fortress.snapshot();
    edge.node=1023;edge.bosses=1u;edge.checksum=SnapshotChecksum(edge);
    assert(fortress.restore(edge));
    assert(fortress.lane(1).id==1024u);
    assert(!game.launch(3));
    assert(game.launch(0) && replay.launch(0));
    assert(!game.buy(Upgrade::Shield));
    for(int i=0;i<200;++i){
        const float x=i%23<11?.5f:-.5f;
        game.update(.025f,x,0,true);
        replay.update(.025f,x,0,true);
        assert(game.hp()==replay.hp() && game.cash()==replay.cash());
        assert(game.phase()==replay.phase());
        assert(std::fabs(game.player_x()-replay.player_x())<0.0001f);
    }
    for(int i=0;i<30000 && game.phase()==Phase::Flight;++i)
        game.update(.05f,0,0,false);
    assert(game.phase()==Phase::Lost || game.phase()==Phase::Won);
    assert(game.recover() && game.phase()==Phase::Chart);
    assert(!game.recover());
    Simulation idle(321);assert(idle.launch(0));
    for(int i=0;i<12000 && idle.phase()==Phase::Flight;++i)
        idle.update(.05f,0,0,false);
    assert(idle.phase()==Phase::Lost);
    const auto finalSave=game.snapshot();
    assert(replay.restore(finalSave));
    assert(replay.node()==game.node() && replay.bosses()==game.bosses());
    assert(!replay.restore(tampered));
    std::cout<<"Gumball Armada deterministic combat and save tests passed\n";
}
