#include "red_ledger_simulation.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

using namespace reverie::redledger;

namespace {

void ServeAndCollect(Simulation *simulation) {
    assert(simulation != nullptr);
    const PatronDefinition *patron =
        simulation->current_patron();
    assert(patron != nullptr);

    assert(simulation->TakeCleanCup());
    assert(
        simulation->work_drink_state()
            == DrinkState::EmptyCup
    );
    assert(simulation->FillHeldCup());
    assert(
        simulation->work_drink_state()
            == DrinkState::FilledCup
    );

    const int32_t expected_payment =
        patron->drink_price_cents;
    assert(simulation->ServeHeldCup());
    assert(
        simulation->work_drink_state()
            == DrinkState::None
    );
    assert(
        simulation->pending_payment_cents()
            == expected_payment
    );
    assert(simulation->CollectPayment());
    assert(
        simulation->pending_payment_cents()
            == 0
    );
}

}  // namespace

int main() {
    Simulation sim;

    assert(sim.day() == 1);
    assert(
        sim.current_event()
            == EventType::ProtectionDemand
    );
    assert(sim.cash_cents() == 2200);
    assert(sim.beer_units() == 6);
    assert(sim.clean_cups() == 3);
    assert(sim.patrons_remaining() == 4);
    assert(sim.TakeCleanCup());
    assert(sim.ReturnHeldCup());
    assert(sim.clean_cups() == 3);
    assert(
        sim.work_drink_state()
            == DrinkState::None
    );

    ServeAndCollect(&sim);
    ServeAndCollect(&sim);
    ServeAndCollect(&sim);

    assert(sim.clean_cups() == 0);
    assert(sim.dirty_cups() == 3);
    assert(!sim.TakeCleanCup());
    assert(sim.WashOneCup());
    ServeAndCollect(&sim);

    assert(sim.patrons_remaining() == 0);
    assert(sim.pending_payment_cents() == 0);
    assert(sim.PayProtection());
    assert(sim.pressure() >= 0);

    DayLedger day1 = sim.CloseDay();
    assert(day1.day == 1);
    assert(day1.patrons_served == 4);
    assert(day1.patrons_lost == 0);
    assert(day1.sales_cents == 1150);
    assert(
        day1.protection_spend_cents
            == Simulation::kProtectionCostCents
    );
    assert(
        day1.debt_service_cents
            == Simulation::kDailyDebtServiceCents
    );
    assert(sim.day() == 2);
    assert(
        sim.current_event()
            == EventType::Inspection
    );

    assert(sim.BuySupply(
        SupplierItem::BeerCrate
    ));
    assert(sim.WashOneCup());
    assert(sim.TakeCleanCup());
    assert(sim.FillHeldCup());
    assert(sim.ServeHeldCup());
    assert(
        sim.pending_payment_cents() > 0
    );

    uint8_t save[
        Simulation::kSerializedSize
    ] = {};
    size_t save_size = 0u;
    assert(sim.Serialize(
        save,
        sizeof(save),
        &save_size
    ));
    assert(
        save_size
            == Simulation::kSerializedSize
    );

    Simulation resumed;
    assert(resumed.Deserialize(
        save,
        save_size
    ));
    assert(resumed.day() == sim.day());
    assert(
        resumed.cash_cents()
            == sim.cash_cents()
    );
    assert(
        resumed.debt_cents()
            == sim.debt_cents()
    );
    assert(
        resumed.beer_units()
            == sim.beer_units()
    );
    assert(
        resumed.clean_cups()
            == sim.clean_cups()
    );
    assert(
        resumed.dirty_cups()
            == sim.dirty_cups()
    );
    assert(
        resumed.pending_payment_cents()
            == sim.pending_payment_cents()
    );
    assert(
        resumed.work_drink_state()
            == DrinkState::None
    );
    assert(resumed.CollectPayment());

    // Pending payment is the 23rd payload field (offset 96). A
    // syntactically valid but mismatched patron amount must fail without
    // resetting the already-running game or changing its customer state.
    uint8_t invalid_payment[Simulation::kSerializedSize] = {};
    size_t invalid_size = 0;
    assert(sim.Serialize(invalid_payment, sizeof(invalid_payment), &invalid_size));
    assert(invalid_size == Simulation::kSerializedSize);
    invalid_payment[96] = 0xe7u; // 999 cents, no patron charges this
    invalid_payment[97] = 0x03u;
    invalid_payment[98] = 0u;
    invalid_payment[99] = 0u;
    const int32_t before_cash = resumed.cash_cents();
    const int32_t before_day = resumed.day();
    const int32_t before_remaining = resumed.patrons_remaining();
    assert(!resumed.Deserialize(invalid_payment, invalid_size));
    assert(resumed.cash_cents() == before_cash);
    assert(resumed.day() == before_day);
    assert(resumed.patrons_remaining() == before_remaining);

    save[0] ^= 0xffu;
    Simulation corrupt;
    assert(!corrupt.Deserialize(
        save,
        save_size
    ));

    while (
        resumed.patrons_remaining() > 0
    ) {
        if (resumed.clean_cups() == 0) {
            assert(resumed.WashOneCup());
        }
        ServeAndCollect(&resumed);
    }

    DayLedger day2 =
        resumed.CloseDay();
    assert(day2.day == 2);
    assert(day2.patrons_served == 4);
    assert(
        day2.inspection_fines_cents
            == Simulation::kInspectionFineCents
    );
    assert(resumed.day() == 3);
    assert(
        resumed.current_event()
            == EventType::SupplyInterruption
    );
    assert(!resumed.BuySupply(
        SupplierItem::BeerCrate
    ));

    const int32_t before_turn_away =
        resumed.patrons_remaining();
    assert(
        resumed.TurnAwayCurrentPatron()
    );
    assert(
        resumed.patrons_remaining()
            == before_turn_away - 1
    );

    std::cout
        << "red ledger manual service smoke passed\n";
    std::cout
        << "cash=" << resumed.cash_cents()
        << " debt=" << resumed.debt_cents()
        << " reputation=" << resumed.reputation()
        << " pressure=" << resumed.pressure()
        << "\n";
    return 0;
}
