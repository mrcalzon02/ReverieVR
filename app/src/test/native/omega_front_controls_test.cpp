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
    for(int i=0;i<300;++i)c.Input({.1f,0,0,false,false,-1,-1});
    // Neither the player nor AI may move on frame-time alone.
    assert(c.battle.ticks()==0);
    assert(c.battle.unit(0)->x==4);
    assert(c.Input({0,0,0,false,false,-1,-1,true}));
    assert(c.battle.ticks()==1);
    // A held "end turn" signal cannot resolve a second round.
    assert(!c.Input({.5f,0,0,false,false,-1,-1,true}));
    assert(c.battle.ticks()==1);
    c.Input({0,0,0,false,false,-1,-1});
    assert(c.battle.unit(0)->x==5);
    // Axis moves cursor without a held select/mouse button.
    const int original=c.cursor_y;
    c.Input({.01f,0,1.f,false,false,-1,-1});
    assert(c.cursor_y==original-1);
    c.Input({0,0,0,false,false,-1,-1});
    c.selected=-1;
    Tap(c,false,true); assert(c.paused);
    // Tactical pause accepts real orders without moving soldiers or enemies.
    c.cursor_x=c.battle.unit(0)->x;c.cursor_y=c.battle.unit(0)->y;
    Tap(c,true);
    assert(c.selected==0);
    Tap(c,false,true);assert(c.mode==CommandMode::Attack);
    Tap(c,false,true);assert(c.mode==CommandMode::Repair);
    assert(c.Input({0,0,0,false,false,2,12}));
    Tap(c,true);
    assert(c.battle.queued(0)>=1);
    auto frozen=c.battle.Checksum();
    for(int i=0;i<30;++i)c.Input({.1f,0,0,false,false,-1,-1});
    assert(c.battle.Checksum()==frozen);
    assert(c.Input({0,0,0,false,false,-1,-1,true}));
    assert(c.rejected);
    assert(c.battle.Checksum()==frozen);
    c.Input({0,0,0,false,false,-1,-1});
    c.selected=-1;
    Tap(c,false,true);assert(!c.paused);
    c.selected=-1;c.cursor_x=11;c.cursor_y=11;
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
    assert(b.battle.ticks()==0 && d.battle.ticks()==0);
    assert(b.Input({0,0,0,false,false,-1,-1,true}));
    assert(d.Input({0,0,0,false,false,-1,-1,true}));
    assert(b.battle.Checksum()==d.battle.Checksum());
    assert(b.battle.ticks()==1);
    std::cout << "Omega Front explicit-turn tactical controls passed\n";
}
