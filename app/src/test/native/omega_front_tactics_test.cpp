#include "omega_front_tactics.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;

int main() {
    Battle a(104), b(104);
    assert(a.unit_count() == 8 && a.pump_hp() == 18);
    assert(a.queued(0) == 0);
    assert(!a.QueueMove(4, 1, 1));  // hostile cannot receive player commands
    assert(!a.QueueMove(0, -1, 7));
    assert(!a.QueueMove(0, 9, 11)); // wall
    assert(!a.QueueAttack(0, 0)); // friendlies not targetable
    assert(!a.QueueAttack(0, 123));
    assert(a.QueueMove(0, 8, 11));
    assert(b.QueueMove(0, 8, 11));
    assert(!a.QueueAttack(1,4)); // unseen foe cannot be tracked
    assert(!b.QueueAttack(1,4));
    for(int t=0;t<25;++t) {
        a.Step();b.Step();
        assert(a.Checksum()==b.Checksum());
        assert(a.pump_hp()==b.pump_hp());
    }
    Battle noOrder(104);
    const auto checksum = noOrder.Checksum();
    assert(checksum == noOrder.Checksum());
    assert(noOrder.ticks()==0);

    Battle movement(17);
    movement.ClearUnits();
    int id=movement.AddUnit(Side::Hearthwell,Class::Rifle,0,0);
    movement.AddUnit(Side::Raiders,Class::Rifle,23,23);
    assert(id==0);
    assert(movement.SetGround(5,0,Ground::DeepWater));
    assert(movement.QueueMove(id, 4, 0));
    for(int i=0;i<4;++i)movement.Step();
    assert(movement.unit(id)->x==4 && movement.unit(id)->y==0);
    assert(movement.queued(id)==0);
    assert(!movement.SetGround(6,0,Ground::Wall)); // no terrain edit mid-battle
    assert(!movement.QueueMove(0,5,0));

    // Finite repair parts make a measurable difference during a pump assault.
    Battle repair(6), withoutRepair(6);
    repair.ClearUnits(); withoutRepair.ClearUnits();
    for (Battle *scene : {&repair, &withoutRepair}) {
        scene->AddUnit(Side::Hearthwell,Class::Engineer,2,13);
        scene->AddUnit(Side::Raiders,Class::Rifle,8,12);
        scene->Step();
    }
    assert(repair.pump_hp()<18);
    assert(repair.QueueRepair(0));
    int stock=repair.components();
    repair.Step(); withoutRepair.Step();
    assert(repair.pump_hp()==withoutRepair.pump_hp()+3);
    assert(repair.components()==stock-1);

    Battle contested(5);
    contested.ClearUnits();
    contested.AddUnit(Side::Hearthwell,Class::Heavy,4,12);
    contested.AddUnit(Side::Raiders,Class::Rifle,7,12);
    assert(contested.QueueAttack(0,1));
    for(int i=0;i<10 && contested.result()==Result::Active;++i)contested.Step();
    assert(contested.result()==Result::Defended);
    assert(contested.pump_hp()>0);
    uint64_t victory=contested.Checksum();
    contested.Step();
    assert(victory==contested.Checksum());

    Battle overrun(9);
    overrun.ClearUnits();
    overrun.AddUnit(Side::Hearthwell,Class::Engineer,1,1);
    overrun.AddUnit(Side::Raiders,Class::Heavy,3,12);
    overrun.AddUnit(Side::Raiders,Class::Heavy,4,12);
    overrun.AddUnit(Side::Raiders,Class::Heavy,3,13);
    for(int i=0;i<20 && overrun.result()==Result::Active;++i)overrun.Step();
    assert(overrun.result()==Result::Overrun);
    assert(overrun.pump_hp()==0);

    Battle cap(123);
    cap.ClearUnits();
    cap.AddUnit(Side::Hearthwell,Class::Rifle,0,0);
    cap.AddUnit(Side::Raiders,Class::Engineer,23,23);
    for (int i=0;i<kOrdersPerUnit;++i) assert(cap.QueueMove(0,5,5));
    assert(!cap.QueueMove(0,6,6));
    assert(cap.CancelOrders(0));
    assert(cap.queued(0)==0);
    assert(!cap.CancelOrders(1));
    for(int i=0;i<kUnitLimit+2;++i)cap.AddUnit(Side::Raiders,Class::Rifle,10+i%kWidth,1);
    assert(cap.unit_count()<=kUnitLimit);

    std::cout << "Omega Front deterministic Hearthwell tactics regressions passed\n";
}
