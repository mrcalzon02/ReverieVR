#include "omega_front_checkpoint.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
static void Reseal(Checkpoint::Bytes &data){
    uint64_t h=UINT64_C(14695981039346656037);
    for(size_t i=0;i<data.size()-8;++i){h^=data[i];h*=UINT64_C(1099511628211);}
    for(size_t i=data.size()-8;i<data.size();++i){data[i]=static_cast<uint8_t>(h);h>>=8;}
}
int main(){
    Controller original(104);
    Checkpoint::Bytes snapshot{};
    assert(Checkpoint::Encode(original,&snapshot));
    Controller resumed(77);
    assert(Checkpoint::Decode(&resumed,snapshot.data(),snapshot.size()));
    assert(resumed.battle.Checksum()==original.battle.Checksum());
    assert(!resumed.paused && resumed.selected==-1);
    assert(resumed.cursor_x==4 && resumed.cursor_y==12);
    // Nontrivial movement, active order queue and fractional battle clock.
    original.Input({0,0,0,false,false,4,11});
    original.Input({0,0,0,true,false,-1,-1});
    original.Input({0,0,0,false,false,8,11});
    original.Input({0,0,0,true,false,-1,-1});
    assert(original.battle.queued(0)==1);
    original.Input({.1f,0,0,false,false,-1,-1});
    assert(original.battle.ticks()==0);
    assert(Checkpoint::Encode(original,&snapshot));
    assert(Checkpoint::Decode(&resumed,snapshot.data(),snapshot.size()));
    assert(resumed.battle.Checksum()==original.battle.Checksum());
    assert(resumed.selected==original.selected);
    assert(resumed.battle.queued(0)==1);
    for(int i=0;i<35;++i){
        original.Input({.05f,0,0,false,false,-1,-1});
        resumed.Input({.05f,0,0,false,false,-1,-1});
        assert(resumed.battle.Checksum()==original.battle.Checksum());
        assert(resumed.battle.ticks()==original.battle.ticks());
    }
    // Paused tactical planning must remain paused, but orders remain accepted.
    original.selected=-1;
    original.Input({0,0,0,false,true,-1,-1});
    assert(original.paused);
    assert(Checkpoint::Encode(original,&snapshot));
    assert(Checkpoint::Decode(&resumed,snapshot.data(),snapshot.size()));
    assert(resumed.paused && resumed.selected==-1);
    const auto frozen=resumed.battle.Checksum();
    for(int i=0;i<10;++i)resumed.Input({.1f,0,0,false,false,-1,-1});
    assert(resumed.battle.Checksum()==frozen);
    // Loss must persist and freeze on reload; victory remains a result too.
    Battle &battle=original.battle;
    for(int i=0;i<200 && battle.result()==Result::Active;++i)battle.Step();
    assert(battle.result()!=Result::Active);
    assert(Checkpoint::Encode(original,&snapshot));
    assert(Checkpoint::Decode(&resumed,snapshot.data(),snapshot.size()));
    assert(resumed.battle.result()==original.battle.result());
    assert(resumed.battle.Checksum()==original.battle.Checksum());
    // Selected defenders can die before the next input. Their pending
    // selected ID must not invalidate the entire otherwise valid checkpoint.
    Controller casualty(99);
    casualty.battle.ClearUnits();
    assert(casualty.battle.AddUnit(Side::Hearthwell,Class::Rifle,2,13)==0);
    assert(casualty.battle.AddUnit(Side::Raiders,Class::Heavy,3,13)==1);
    assert(casualty.battle.AddUnit(Side::Raiders,Class::Heavy,4,13)==2);
    casualty.Input({0,0,0,false,false,2,13});
    casualty.Input({0,0,0,true,false,-1,-1});
    assert(casualty.selected==0);
    for(int i=0;i<10 && casualty.battle.unit(0)->alive();++i) casualty.battle.Step();
    assert(!casualty.battle.unit(0)->alive());
    assert(casualty.selected==0);
    assert(Checkpoint::Encode(casualty,&snapshot));
    Controller casualty_restore(14);
    assert(Checkpoint::Decode(&casualty_restore,snapshot.data(),snapshot.size()));
    assert(casualty_restore.battle.Checksum()==casualty.battle.Checksum());
    assert(casualty_restore.selected==0);
    // Subsequent input safely deselects the dead soldier.
    casualty_restore.Input({0,0,0,false,false,-1,-1});
    casualty_restore.Input({0,0,0,true,false,-1,-1});
    assert(casualty_restore.selected==-1 || casualty_restore.battle.result()!=Result::Active);
    // Rejection never mutates an existing healthy in-memory battle.
    const uint64_t prior=resumed.battle.Checksum();
    auto invalid=snapshot;invalid[35]^=1;
    assert(!Checkpoint::Decode(&resumed,invalid.data(),invalid.size()));
    assert(resumed.battle.Checksum()==prior);
    assert(!Checkpoint::Decode(&resumed,snapshot.data(),snapshot.size()-1));
    invalid=snapshot;invalid[8]=99;Reseal(invalid); // schema version
    assert(!Checkpoint::Decode(&resumed,invalid.data(),invalid.size()));
    invalid=snapshot;invalid[16]=255;Reseal(invalid); // cursor X
    assert(!Checkpoint::Decode(&resumed,invalid.data(),invalid.size()));
    invalid=snapshot;invalid[36]=255;Reseal(invalid); // tick high byte
    assert(!Checkpoint::Decode(&resumed,invalid.data(),invalid.size()));
    // A restored checkpoint can continue faithfully when ended and restarted.
    resumed.Reset();original.Reset();
    assert(resumed.battle.Checksum()==original.battle.Checksum());
    // Defensive invalid API calls.
    assert(!Checkpoint::Encode(original,nullptr));
    assert(!Checkpoint::Decode(nullptr,snapshot.data(),snapshot.size()));
    assert(!Checkpoint::Decode(&resumed,nullptr,snapshot.size()));
    std::cout<<"Omega Front versioned battle checkpoint regressions passed\n";
}
