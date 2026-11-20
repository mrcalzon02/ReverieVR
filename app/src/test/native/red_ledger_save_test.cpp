#include "red_ledger_save.h"
#include <cassert>
#include <iostream>
using namespace reverie::redledger;
int main(){
    save::State game;
    assert(game.simulation.TakeCleanCup()&&game.simulation.FillHeldCup()
        &&game.simulation.ServeHeldCup()&&game.simulation.CollectPayment());
    save::Checkpoint::Bytes bytes{};
    assert(save::Checkpoint::Encode(game,&bytes));
    save::State restored;
    assert(save::Checkpoint::Decode(&restored,bytes.data(),bytes.size())==save::Checkpoint::Format::Checked);
    assert(restored.simulation.cash_cents()==game.simulation.cash_cents());
    save::Checkpoint::Bytes roundtrip{};
    assert(save::Checkpoint::Encode(restored,&roundtrip)&&roundtrip==bytes);
    for(size_t i=0;i<bytes.size();++i){
        auto broken=bytes;broken[i]^=1u;
        assert(save::Checkpoint::Decode(&restored,broken.data(),broken.size())==save::Checkpoint::Format::Invalid);
        assert(save::Checkpoint::Encode(restored,&roundtrip)&&roundtrip==bytes);
    }
    for(size_t size=0;size<bytes.size();++size){
        assert(save::Checkpoint::Decode(&restored,bytes.data(),size)==save::Checkpoint::Format::Invalid);
    }
    // Preserve exactly the supported previous composite shapes; the old
    // base has no retrospective checksum guarantee, which the record states.
    for(size_t size:{save::Checkpoint::kLegacySize,save::Checkpoint::kPreviousPayloadSize}){
        assert(save::Checkpoint::Decode(&restored,bytes.data()+save::Checkpoint::kHeaderSize,size)
            ==save::Checkpoint::Format::Legacy);
        assert(restored.simulation.cash_cents()==game.simulation.cash_cents());
    }
    assert(!save::Checkpoint::Encode(game,nullptr));
    assert(save::Checkpoint::Decode(nullptr,bytes.data(),bytes.size())==save::Checkpoint::Format::Invalid);
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
    std::cout<<"PASS Red Ledger whole-envelope bit corruption, transactional rejection and supported composite import\n";
}
