#include "omega_front_checkpoint.h"
#include <cassert>
#include <iostream>

using namespace reverie::omega;

// Commit one deliberate tactical pulse. Rendering time alone never advances
// these orders and a held turn button must not create a second action.
static void CommitTurn(Controller &controller) {
    const uint32_t prior = controller.battle.ticks();
    assert(controller.Input({0, 0, 0, false, false, -1, -1, true}));
    assert(controller.battle.ticks() == prior + 1);
    controller.Input({0, 0, 0, false, false, -1, -1, false});
}

int main() {
    Controller original(73);
    Battle &battle = original.battle;
    battle.ClearUnits();
    assert(battle.ConfigureMission(Mission::CaptureOrchard));

    // The first two heavies can together kill this nine-HP rifleman.
    // The third defender's attack is valid when queued, but its target has
    // already died by the time its own action executes in the same turn.
    assert(battle.AddUnit(Side::Hearthwell, Class::Heavy, 2, 3) == 0);
    assert(battle.AddUnit(Side::Hearthwell, Class::Heavy, 2, 4) == 1);
    assert(battle.AddUnit(Side::Hearthwell, Class::Rifle, 2, 5) == 2);
    assert(battle.AddUnit(Side::Raiders, Class::Rifle, 4, 4) == 3);
    // A second living enemy preserves a real active battle after the kill.
    assert(battle.AddUnit(Side::Raiders, Class::Rifle, 20, 20) == 4);
    assert(battle.QueueAttack(0, 3));
    assert(battle.QueueAttack(1, 3));
    assert(battle.QueueAttack(2, 3));
    assert(battle.QueueMove(2, 3, 5));
    assert(battle.queued(0) == 1 && battle.queued(1) == 1);
    assert(battle.queued(2) == 2);
    assert(battle.unit(0)->ammo == 5 && battle.unit(1)->ammo == 5);
    assert(battle.unit(2)->ammo == 8 && battle.ammo_stock() == 18);
    const uint64_t queued_hash = battle.Checksum();

    // Save while the later rifleman's still-valid order is queued. On
    // restoration the *real* turn resolver must revalidate the target after
    // the earlier actors kill it, with no phantom shot or spent ammunition.
    Checkpoint::Bytes before{};
    assert(Checkpoint::Encode(original, &before));
    Controller restored(901);
    assert(Checkpoint::Decode(&restored, before.data(), before.size()));
    assert(restored.battle.Checksum() == queued_hash);
    assert(restored.battle.queued(2) == 2);

    CommitTurn(original);
    CommitTurn(restored);
    assert(battle.Checksum() == restored.battle.Checksum());
    assert(battle.ticks() == 1 && battle.result() == Result::Active);
    assert(!battle.unit(3)->alive() && battle.unit(4)->alive());
    assert(battle.unit(0)->ammo == 4 && battle.unit(1)->ammo == 4);
    assert(battle.unit(2)->ammo == 8);  // invalid at EXECUTION, not at issue
    assert(battle.ammo_stock() == 18 && battle.components() == 4);
    assert(battle.queued(1) == 0 && battle.queued(2) == 1);
    assert(battle.last_event().actor == 1 && battle.last_event().target == 3);
    assert(battle.unit(4)->x != 20 || battle.unit(4)->y != 20);

    // The next legal move is still queued. A second actual checkpoint and
    // third independent controller must not revive the invalid attack.
    Checkpoint::Bytes after{};
    assert(Checkpoint::Encode(restored, &after));
    Controller second_restore(19);
    assert(Checkpoint::Decode(&second_restore, after.data(), after.size()));
    assert(second_restore.battle.Checksum() == battle.Checksum());
    assert(second_restore.battle.queued(2) == 1);
    for (int turn = 2; turn <= 4; ++turn) {
        CommitTurn(original);
        CommitTurn(restored);
        CommitTurn(second_restore);
        assert(battle.ticks() == static_cast<uint32_t>(turn));
        assert(battle.result() == Result::Active);
        assert(battle.Checksum() == restored.battle.Checksum());
        assert(battle.Checksum() == second_restore.battle.Checksum());
        for (const Battle *scene : {&battle, &restored.battle,
                                     &second_restore.battle}) {
            assert(!scene->unit(3)->alive() && scene->unit(4)->alive());
            assert(scene->unit(0)->ammo == 4 && scene->unit(1)->ammo == 4);
            assert(scene->unit(2)->ammo == 8);
            assert(scene->ammo_stock() == 18 && scene->components() == 4);
            assert(scene->queued(2) == 0);
            assert(scene->unit(2)->x == 3 && scene->unit(2)->y == 5);
        }
    }
    Checkpoint::Bytes final_save{};
    assert(Checkpoint::Encode(second_restore, &final_save));
    Controller final_restore(123);
    assert(Checkpoint::Decode(&final_restore, final_save.data(), final_save.size()));
    assert(final_restore.battle.Checksum() == battle.Checksum());
    assert(final_restore.battle.unit(2)->ammo == 8);
    assert(final_restore.battle.queued(2) == 0);
    std::cout << "Omega Front stale target order, finite ammo and checkpoint replay PASS\n";
}
