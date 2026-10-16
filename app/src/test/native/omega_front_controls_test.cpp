#include "omega_front_controls.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
static void Tap(Controller &c,bool primary,bool secondary=false){
    c.Input({0,0,0,false,false,-1,-1});
    c.Input({0,0,0,primary,secondary,-1,-1});
}
int main(){
    Controller c(104), b(104);
    assert(c.Input({0,0,0,false,false,4,11}));
    assert(c.cursor_x==4 && c.cursor_y==11);
    Tap(c,true);
    assert(c.selected==0);
    Tap(c,false,true);
    assert(c.mode==CommandMode::Attack);
    assert(!c.Input({0,0,0,false,false,-1,-1}));
    Tap(c,false,true);assert(c.mode==CommandMode::Repair);
    Tap(c,false,true);assert(c.mode==CommandMode::Resupply);
    Tap(c,false,true);assert(c.mode==CommandMode::Move);
    c.Input({0,0,0,false,false,7,11});
    Tap(c,true);
    assert(c.battle.queued(0)==1);
    c.Input({.1f,0,0,false,false,-1,-1});
    c.Input({.1f,0,0,false,false,-1,-1});
    c.Input({.1f,0,0,false,false,-1,-1});
    assert(c.battle.ticks()==1);
    assert(c.battle.unit(0)->x==5);
    // Axis moves cursor without a held select/mouse button.
    const int original=c.cursor_y;
    c.Input({.01f,0,1.f,false,false,-1,-1});
    assert(c.cursor_y==original-1);
    c.Input({0,0,0,false,false,-1,-1});
    c.selected=-1;
    Tap(c,false,true); assert(c.paused);
    auto frozen=c.battle.Checksum();
    for(int i=0;i<30;++i)c.Input({.1f,0,0,false,false,-1,-1});
    assert(c.battle.Checksum()==frozen);
    Tap(c,false,true);assert(!c.paused);
    Tap(c,true); assert(c.rejected); // not a friendly soldier at cursor
    // Separate replay from exact equivalent command sequence.
    const ControlsInput steps[]={{0,0,0,false,false,4,11},{0,0,0,true,false,-1,-1},
    {0,0,0,false,false,8,11},{0,0,0,false,false,-1,-1},{0,0,0,true,false,-1,-1}};
    for(auto v:steps)b.Input(v);
    Controller d(104); for(auto v:steps)d.Input(v);
    for(int i=0;i<40;++i) {
        b.Input({.05f,0,0,false,false,-1,-1});
        d.Input({.05f,0,0,false,false,-1,-1});
        assert(b.battle.Checksum()==d.battle.Checksum());
    }
    std::cout << "Omega Front controller-to-tactics regressions passed\n";
}
