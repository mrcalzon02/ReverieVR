#include "red_ledger_simulation.h"
#include "red_ledger_trade.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
using namespace reverie::redledger;
int main() {
    Simulation sim;
    trade::Trade market;
    const PatronDefinition *patron=sim.current_patron();
    assert(patron&&patron->kind==PatronKind::RailWorker);
    assert(market.SelectDrink(sim.beer_units())&&market.drink()==1);
    assert(market.SelectSnack()&&market.snack()==1);
    const int beer_before=sim.beer_units();
    const int cash_before=sim.cash_cents();
    const int favor_before=market.favor(0);
    assert(sim.TakeCleanCup());
    assert(sim.FillSpecialDrink());
    assert(market.ConsumeDrink());
    assert(sim.beer_units()==beer_before); // spirits/special ale do not use house stock
    const auto quote=trade::RateService(patron->kind,market.drink(),market.snack());
    const int base=patron->drink_price_cents;
    assert(sim.ServeHeldCup());
    assert(sim.ApplyServicePremium(quote.cents,quote.reputation));
    market.ConsumeSnack();
    market.RecordService(patron->kind);
    assert(sim.pending_payment_cents()==base+quote.cents);
    assert(market.favor(0)==favor_before+quote.favor);
    assert(sim.CollectPayment());
    assert(sim.cash_cents()==cash_before+base+quote.cents);
    assert(sim.patrons_remaining()==3);
    assert(market.CycleOffer()&&market.offer()==1);
    const int wholesale=market.offer_cost();
    assert(sim.SpendSpecialStock(wholesale));
    assert(market.AcceptStock());
    assert(sim.cash_cents()==cash_before+base+quote.cents-wholesale);
    assert(market.Enter(trade::Room::Living));
    uint8_t packed[Simulation::kSerializedSize+trade::Trade::kSerializedSize]={};
    size_t sim_size=0u;
    assert(sim.Serialize(packed,Simulation::kSerializedSize,&sim_size));
    assert(sim_size==Simulation::kSerializedSize);
    assert(market.Serialize(packed+sim_size,trade::Trade::kSerializedSize));
    Simulation loaded;
    trade::Trade stored;
    assert(loaded.Deserialize(packed,Simulation::kSerializedSize,true));
    assert(stored.Deserialize(packed+sim_size,trade::Trade::kSerializedSize));
    assert(loaded.cash_cents()==sim.cash_cents());
    assert(stored.stock_drink(1)==market.stock_drink(1));
    assert(stored.room()==trade::Room::Living);
    // The legacy simulation parser must not accept the new premium unless
    // explicitly paired with the checked trade extension.
    assert(!loaded.Deserialize(packed,Simulation::kSerializedSize,false));
    // An untouched original V2 state is still independently restorable.
    Simulation legacy;
    uint8_t legacy_bytes[Simulation::kSerializedSize]={};
    size_t legacy_size=0u;
    assert(legacy.Serialize(legacy_bytes,sizeof(legacy_bytes),&legacy_size));
    Simulation migrated;
    assert(migrated.Deserialize(legacy_bytes,legacy_size));
    // No wholesale purchasing may silently exceed the available wallet.
    assert(!migrated.SpendSpecialStock(migrated.cash_cents()+1));
    std::cout<<"Red Ledger integrated trade: sales, favor, wholesale, legacy and composite save OK\n";
}
