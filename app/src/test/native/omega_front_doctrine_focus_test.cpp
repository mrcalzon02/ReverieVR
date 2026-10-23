#include "omega_front_tactics.h"
#include <cassert>
#include <iostream>

using namespace reverie::omega;

static Battle MakeBattle(Doctrine doctrine){
    Battle b(521);
    b.ClearUnits();
    // Defender #0 is one tile closer but has a physical rubble position.
    // Defender #1 is exposed. All three positions are LOS-valid and legal.
    assert(b.SetGround(6,10,Ground::Rubble));
    assert(b.ConfigureDoctrine(doctrine));
    assert(b.AddUnit(Side::Hearthwell,Class::Rifle,6,10)==0);
    assert(b.AddUnit(Side::Hearthwell,Class::Engineer,6,11)==1);
    assert(b.AddUnit(Side::Raiders,Class::Rifle,8,10)==2);
    return b;
}
int main(){
    Battle disorganized=MakeBattle(Doctrine::Unaligned);
    disorganized.Step();
    assert(disorganized.last_event().actor==2);
    assert(disorganized.last_event().target==0); // old closest-target instinct
    Battle drilled=MakeBattle(Doctrine::Hearthward);
    drilled.Step();
    assert(drilled.last_event().actor==2);
    assert(drilled.last_event().target==1); // avoids covered rifle
    assert(drilled.unit(2)->ammo==7); // still spends its real bullet
    assert(disorganized.unit(2)->ammo==7);
    assert(drilled.unit(0)->hp==Battle::MaxHp(Class::Rifle));
    assert(drilled.unit(1)->hp<=Battle::MaxHp(Class::Engineer));
    // Same seed/inputs must lead to the same choices on replays/checkpoints.
    Battle replay=MakeBattle(Doctrine::Hearthward);
    replay.Step();
    assert(replay.Checksum()==drilled.Checksum());
    std::cout<<"PASS Omega Front doctrinal focus-fire: exposed support targets, "
                "legitimate line of sight, no cheated shots\n";
}
