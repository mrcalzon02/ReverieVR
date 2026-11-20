#include "red_ledger_simulation.h"
#include "red_ledger_save.h"

#include <array>
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

// A checkpoint near the legal inventory ceiling is still a valid game, not
// a corrupted save. Use the production codec to seed those rare long-run
// levels; then exercise the real purchase and round-trip boundaries.
static Simulation StockLimitFixture(int32_t beer,int32_t cups,int32_t spent=0,
                                   int32_t dirty=0,int32_t held=0) {
    Simulation base;
    std::array<uint8_t,Simulation::kSerializedSize> bytes{};
    size_t count=0;
    assert(base.Serialize(bytes.data(),bytes.size(),&count));
    assert(count==bytes.size());
    const auto write=[&bytes](size_t offset,uint32_t value) {
        for(size_t b=0;b<4u;++b)
            bytes[offset+b]=static_cast<uint8_t>(value>>(b*8u));
    };
    write(20u,static_cast<uint32_t>(beer));
    write(24u,static_cast<uint32_t>(cups));
    write(28u,static_cast<uint32_t>(dirty));
    write(60u,static_cast<uint32_t>(spent));
    write(92u,static_cast<uint32_t>(held));
    Simulation loaded;
    assert(loaded.Deserialize(bytes.data(),bytes.size()));
    return loaded;
}
static void RoundTripStock(const Simulation& original) {
    std::array<uint8_t,Simulation::kSerializedSize> bytes{};
    size_t size=0;
    assert(original.Serialize(bytes.data(),bytes.size(),&size));
    Simulation restored;
    assert(restored.Deserialize(bytes.data(),size));
    assert(restored.cash_cents()==original.cash_cents());
    assert(restored.beer_units()==original.beer_units());
    assert(restored.clean_cups()==original.clean_cups());
    assert(restored.dirty_cups()==original.dirty_cups());
    assert(restored.work_drink_state()==original.work_drink_state());
}
int main() {
    static_assert(Simulation::kMaxStockUnits==100000,
                  "Purchased inventory must fit the same checked save bounds");
    Simulation nearBeer=StockLimitFixture(99993,3);
    assert(!nearBeer.BuySupply(SupplierItem::BeerCrate));
    assert(nearBeer.beer_units()==99993&&nearBeer.cash_cents()==2200);
    RoundTripStock(nearBeer);
    Simulation exactBeer=StockLimitFixture(99992,3);
    assert(exactBeer.BuySupply(SupplierItem::BeerCrate));
    assert(exactBeer.beer_units()==Simulation::kMaxStockUnits);
    assert(exactBeer.cash_cents()==1300);
    assert(!exactBeer.BuySupply(SupplierItem::BeerCrate));
    RoundTripStock(exactBeer);
    Simulation nearCups=StockLimitFixture(6,99997);
    assert(!nearCups.BuySupply(SupplierItem::CupSet));
    assert(nearCups.clean_cups()==99997&&nearCups.cash_cents()==2200);
    RoundTripStock(nearCups);
    Simulation exactCups=StockLimitFixture(6,99996);
    assert(exactCups.BuySupply(SupplierItem::CupSet));
    assert(exactCups.clean_cups()==Simulation::kMaxStockUnits);
    assert(exactCups.cash_cents()==1880);
    assert(!exactCups.BuySupply(SupplierItem::CupSet));
    RoundTripStock(exactCups);
    // All physical cups count, whether clean, dirty or temporarily held.
    Simulation combinedOverflow=StockLimitFixture(6,99996,0,1);
    assert(!combinedOverflow.BuySupply(SupplierItem::CupSet));
    assert(combinedOverflow.clean_cups()==99996&&combinedOverflow.dirty_cups()==1);
    RoundTripStock(combinedOverflow);
    Simulation combinedExact=StockLimitFixture(6,99994,0,2);
    assert(combinedExact.BuySupply(SupplierItem::CupSet));
    assert(combinedExact.clean_cups()==99998&&combinedExact.dirty_cups()==2);
    assert(combinedExact.WashOneCup()&&combinedExact.clean_cups()==99999);
    assert(combinedExact.WashOneCup()&&combinedExact.clean_cups()==100000);
    assert(!combinedExact.BuySupply(SupplierItem::CupSet));
    RoundTripStock(combinedExact);
    // Valid high-stock legacy saves must remain readable; unsafe transfer
    // attempts must leave the saved fields and held work state untouched.
    Simulation olderFullClean=StockLimitFixture(6,100000,0,1);
    assert(!olderFullClean.WashOneCup());
    RoundTripStock(olderFullClean);
    Simulation olderHeld=StockLimitFixture(6,100000,0,0,
                                          static_cast<int32_t>(DrinkState::EmptyCup));
    assert(!olderHeld.ReturnHeldCup());
    assert(olderHeld.work_drink_state()==DrinkState::EmptyCup);
    RoundTripStock(olderHeld);
    Simulation olderDirty=StockLimitFixture(6,3,0,100000,
                                           static_cast<int32_t>(DrinkState::FilledCup));
    assert(!olderDirty.ServeHeldCup());
    assert(olderDirty.pending_payment_cents()==0);
    assert(olderDirty.WashOneCup());
    assert(olderDirty.ServeHeldCup());
    RoundTripStock(olderDirty);
    // The checked daily supply ledger has the same billion-cent ceiling.
    Simulation spentCap=StockLimitFixture(6,3,999999700);
    assert(!spentCap.BuySupply(SupplierItem::BeerCrate));
    assert(!spentCap.BuySupply(SupplierItem::CupSet));
    assert(spentCap.cash_cents()==2200&&spentCap.beer_units()==6&&
           spentCap.clean_cups()==3);
    RoundTripStock(spentCap);
    std::cout<<"PASS Red Ledger house-stock and supply-ledger purchase caps remain reloadable\n";

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

    Simulation business;
    for(int day=1;day<=135;++day){
        if(business.beer_units()<8&&business.current_event()!=EventType::SupplyInterruption)
            assert(business.BuySupply(SupplierItem::BeerCrate));
        while(business.current_patron()){
            if(business.beer_units()==0){assert(business.TurnAwayCurrentPatron());continue;}
            if(business.clean_cups()==0)assert(business.WashOneCup());
            ServeAndCollect(&business);
        }
        const int owed=business.debt_cents();
        const auto closed=business.CloseDay();
        assert(closed.debt_service_cents==owed-business.debt_cents());
        if(day==134)assert(closed.debt_service_cents==50&&business.debt_cents()==0);
        if(day==135)assert(closed.debt_service_cents==0);
    }
    // The final installment is 50 cents on day 134. Saving and restoring
    // after *every* close must conserve the complete checked game state,
    // rather than collecting a second installment or losing inventory.
    save::State continuous;
    save::State daily_reloaded;
    int32_t continuous_paid = 0;
    int32_t reloaded_paid = 0;
    const auto run_house_shift = [](save::State* game) {
        Simulation &sim = game->simulation;
        if (sim.beer_units() < 8 &&
            sim.current_event() != EventType::SupplyInterruption)
            assert(sim.BuySupply(SupplierItem::BeerCrate));
        while (sim.current_patron()) {
            if (sim.beer_units() == 0) {
                assert(sim.TurnAwayCurrentPatron());
                continue;
            }
            if (sim.clean_cups() == 0) assert(sim.WashOneCup());
            assert(sim.TakeCleanCup());
            assert(sim.FillHeldCup());
            assert(sim.ServeHeldCup());
            assert(sim.CollectPayment());
        }
    };
    for (int day = 1; day <= 135; ++day) {
        run_house_shift(&continuous);
        run_house_shift(&daily_reloaded);
        const int32_t continuous_owed = continuous.simulation.debt_cents();
        const int32_t reloaded_owed = daily_reloaded.simulation.debt_cents();
        const DayLedger plain = continuous.simulation.CloseDay();
        const DayLedger restored = daily_reloaded.simulation.CloseDay();
        assert(plain.day == day && restored.day == day);
        assert(plain.debt_service_cents ==
               continuous_owed - continuous.simulation.debt_cents());
        assert(restored.debt_service_cents ==
               reloaded_owed - daily_reloaded.simulation.debt_cents());
        assert(plain.debt_service_cents == restored.debt_service_cents);
        assert(plain.patrons_served == restored.patrons_served);
        assert(plain.patrons_lost == restored.patrons_lost);
        continuous_paid += plain.debt_service_cents;
        reloaded_paid += restored.debt_service_cents;
        if (day == 134) {
            assert(plain.debt_service_cents == 50);
            assert(continuous.simulation.debt_cents() == 0);
        }
        if (day == 135) {
            assert(plain.debt_service_cents == 0);
            assert(continuous.simulation.debt_cents() == 0);
        }
        save::Checkpoint::Bytes expected{};
        save::Checkpoint::Bytes checked{};
        assert(save::Checkpoint::Encode(continuous, &expected));
        assert(save::Checkpoint::Encode(daily_reloaded, &checked));
        // Full envelope equality also covers cash, house stock, patron
        // progress, last-day books, trade, property and clientele fields.
        assert(checked == expected);
        save::State next;
        assert(save::Checkpoint::Decode(&next, checked.data(), checked.size())
               == save::Checkpoint::Format::Checked);
        save::Checkpoint::Bytes after_load{};
        assert(save::Checkpoint::Encode(next, &after_load));
        assert(after_load == expected);
        daily_reloaded = next;
        assert(daily_reloaded.simulation.day() == day + 1);
        assert(daily_reloaded.simulation.cash_cents() ==
               continuous.simulation.cash_cents());
        assert(daily_reloaded.simulation.debt_cents() ==
               continuous.simulation.debt_cents());
        assert(daily_reloaded.simulation.patrons_remaining() ==
               continuous.simulation.patrons_remaining());
    }
    assert(continuous_paid == 20000 && reloaded_paid == 20000);
    std::cout<<"PASS Red Ledger 135-day checked checkpoint parity, final 50-cent debt service and no repeat payment\n";
    std::cout<<"Red Ledger 135 closes: final 50-cent installment and paid-off balance verified.\n";
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
