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

    // A dead defender's queued attack is still serialized, but can never
    // execute after restoration. A second living defender proves that real
    // tactical turns continue, rather than the whole encounter freezing.
    Controller pending_casualty(211);
    Battle &pending_battle=pending_casualty.battle;
    pending_battle.ClearUnits();
    assert(pending_battle.ConfigureMission(Mission::CaptureOrchard));
    assert(pending_battle.AddUnit(Side::Hearthwell,Class::Rifle,2,13)==0);
    assert(pending_battle.AddUnit(Side::Hearthwell,Class::Engineer,0,0)==1);
    assert(pending_battle.AddUnit(Side::Raiders,Class::Heavy,3,13)==2);
    assert(pending_battle.AddUnit(Side::Raiders,Class::Heavy,4,13)==3);
    pending_casualty.Input({0,0,0,false,false,2,13});
    pending_casualty.Input({0,0,0,true,false,-1,-1});
    assert(pending_casualty.selected==0);
    assert(pending_battle.QueueAttack(0,2));
    assert(pending_battle.QueueMove(1,5,0));
    assert(pending_battle.queued(0)==1 && pending_battle.queued(1)==1);
    // Execute the *actual* enemy AI: heavy raiders kill the rifleman
    // after its first shot, before its still-live attack order is consumed.
    for(int i=0;i<3 && pending_battle.unit(0)->alive();++i)
        pending_battle.Step();
    assert(pending_battle.ticks()>0);
    assert(!pending_battle.unit(0)->alive());
    assert(pending_battle.unit(2)->alive());
    assert(pending_battle.result()==Result::Active);
    assert(pending_battle.queued(0)==1);
    assert(pending_casualty.selected==0); // stale selection also persisted
    const int dead_x=pending_battle.unit(0)->x;
    const int dead_y=pending_battle.unit(0)->y;
    const int dead_ammo=pending_battle.unit(0)->ammo;
    const int components=pending_battle.components();
    const int ammo_stock=pending_battle.ammo_stock();
    const uint32_t casualty_tick=pending_battle.ticks();
    // A dead actor cannot accept replacements or revoke its stored order.
    const auto death_hash=pending_battle.Checksum();
    assert(!pending_battle.QueueAttack(0,2));
    assert(!pending_battle.QueueMove(0,6,13));
    assert(pending_battle.Checksum()==death_hash);
    Checkpoint::Bytes pending_bytes{};
    assert(Checkpoint::Encode(pending_casualty,&pending_bytes));
    Controller casualty_replay(19);
    assert(Checkpoint::Decode(&casualty_replay,pending_bytes.data(),pending_bytes.size()));
    assert(casualty_replay.selected==0);
    assert(casualty_replay.battle.Checksum()==death_hash);
    assert(casualty_replay.battle.queued(0)==1);
    for(int turn=0;turn<4;++turn) {
        assert(pending_battle.result()==Result::Active);
        const ControlsInput release{0,0,0,false,false,-1,-1,false};
        const ControlsInput advance{0,0,0,false,false,-1,-1,true};
        pending_casualty.Input(release);
        casualty_replay.Input(release);
        assert(pending_casualty.Input(advance));
        assert(casualty_replay.Input(advance));
        assert(casualty_replay.battle.Checksum()==pending_battle.Checksum());
        assert(casualty_replay.battle.ticks()==pending_battle.ticks());
        assert(casualty_replay.selected==pending_casualty.selected);
        for(const Battle *scene:{&pending_battle,&casualty_replay.battle}) {
            const Unit *dead=scene->unit(0);
            assert(dead && !dead->alive());
            assert(dead->x==dead_x && dead->y==dead_y && dead->ammo==dead_ammo);
            assert(scene->queued(0)==1); // inert, not fired or recycled
            assert(scene->components()==components && scene->ammo_stock()==ammo_stock);
        }
    }
    assert(pending_battle.ticks()==casualty_tick+4);
    assert(pending_battle.unit(1)->alive());
    assert(pending_battle.unit(1)->x==5 && pending_battle.unit(1)->y==0);
    assert(pending_battle.queued(1)==0);
    assert(pending_casualty.selected==-1 && casualty_replay.selected==-1);
    Checkpoint::Bytes post_casualty_bytes{};
    assert(Checkpoint::Encode(casualty_replay,&post_casualty_bytes));
    Controller twice_loaded(23);
    assert(Checkpoint::Decode(&twice_loaded,post_casualty_bytes.data(),
                              post_casualty_bytes.size()));
    assert(twice_loaded.battle.Checksum()==casualty_replay.battle.Checksum());
    assert(!twice_loaded.battle.unit(0)->alive() && twice_loaded.battle.queued(0)==1);
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
