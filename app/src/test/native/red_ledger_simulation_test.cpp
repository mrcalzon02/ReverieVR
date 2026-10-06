#include "red_ledger_simulation.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

using namespace reverie::redledger;

int main() {
    Simulation sim;

    assert(sim.day() == 1);
    assert(sim.current_event() == EventType::ProtectionDemand);
    assert(sim.cash_cents() == 2200);
    assert(sim.beer_units() == 6);
    assert(sim.clean_cups() == 3);
    assert(sim.patrons_remaining() == 4);

    assert(sim.ServeNextPatron());
    assert(sim.ServeNextPatron());
    assert(sim.ServeNextPatron());
    assert(sim.clean_cups() == 0);
    assert(!sim.ServeNextPatron());
    assert(sim.patrons_remaining() == 0);
    assert(sim.WashOneCup());
    assert(sim.PayProtection());
    assert(sim.pressure() >= 0);

    DayLedger day1 = sim.CloseDay();
    assert(day1.day == 1);
    assert(day1.patrons_served == 3);
    assert(day1.patrons_lost == 1);
    assert(day1.protection_spend_cents == Simulation::kProtectionCostCents);
    assert(day1.debt_service_cents == Simulation::kDailyDebtServiceCents);
    assert(sim.day() == 2);
    assert(sim.current_event() == EventType::Inspection);

    uint8_t save[Simulation::kSerializedSize] = {};
    size_t save_size = 0u;
    assert(sim.Serialize(save, sizeof(save), &save_size));
    assert(save_size == Simulation::kSerializedSize);

    Simulation resumed;
    assert(resumed.Deserialize(save, save_size));
    assert(resumed.day() == sim.day());
    assert(resumed.cash_cents() == sim.cash_cents());
    assert(resumed.debt_cents() == sim.debt_cents());
    assert(resumed.beer_units() == sim.beer_units());
    assert(resumed.clean_cups() == sim.clean_cups());
    assert(resumed.dirty_cups() == sim.dirty_cups());
    assert(resumed.reputation() == sim.reputation());
    assert(resumed.pressure() == sim.pressure());

    save[0] ^= 0xffu;
    Simulation corrupt;
    assert(!corrupt.Deserialize(save, save_size));

    assert(resumed.BuySupply(SupplierItem::BeerCrate));
    while (resumed.patrons_remaining() > 0) {
        if (!resumed.ServeNextPatron()) {
            resumed.WashOneCup();
        }
    }

    DayLedger day2 = resumed.CloseDay();
    assert(day2.day == 2);
    assert(day2.inspection_fines_cents == Simulation::kInspectionFineCents);
    assert(resumed.day() == 3);
    assert(resumed.current_event() == EventType::SupplyInterruption);
    assert(!resumed.BuySupply(SupplierItem::BeerCrate));

    std::cout << "red ledger simulation smoke passed\n";
    std::cout << "cash=" << resumed.cash_cents()
              << " debt=" << resumed.debt_cents()
              << " reputation=" << resumed.reputation()
              << " pressure=" << resumed.pressure() << "\n";
    return 0;
}
