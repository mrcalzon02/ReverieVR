#include "red_ledger_simulation.h"
#include <cassert>
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

    assert(sim.BuySupply(SupplierItem::BeerCrate));
    while (sim.patrons_remaining() > 0) {
        if (!sim.ServeNextPatron()) {
            sim.WashOneCup();
        }
    }
    DayLedger day2 = sim.CloseDay();
    assert(day2.day == 2);
    assert(day2.inspection_fines_cents == Simulation::kInspectionFineCents);
    assert(sim.day() == 3);
    assert(sim.current_event() == EventType::SupplyInterruption);
    assert(!sim.BuySupply(SupplierItem::BeerCrate));

    std::cout << "red ledger simulation smoke passed\n";
    std::cout << "cash=" << sim.cash_cents()
              << " debt=" << sim.debt_cents()
              << " reputation=" << sim.reputation()
              << " pressure=" << sim.pressure() << "\n";
    return 0;
}
