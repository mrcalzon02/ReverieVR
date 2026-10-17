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
    assert(market.SelectDrink(sim.beer_units())&&market.drink()==4);
    assert(market.SelectDrink(sim.beer_units())&&market.drink()==7);
    assert(market.SelectSnack()&&market.snack()==0);
    assert(market.SelectSnack()&&market.snack()==2);
    const int beer_before=sim.beer_units();
    const int cash_before=sim.cash_cents();
    const int favor_before=market.favor(2);
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
    uint8_t premium_pending[Simulation::kSerializedSize]={};
    size_t pending_size=0u;
    assert(sim.Serialize(premium_pending,sizeof(premium_pending),&pending_size));
    Simulation checked;
    assert(!checked.Deserialize(premium_pending,pending_size,false));
    assert(checked.Deserialize(premium_pending,pending_size,true));
    assert(market.favor(2)==favor_before+quote.favor);
    assert(sim.CollectPayment());
    assert(sim.cash_cents()==cash_before+base+quote.cents);
    assert(sim.patrons_remaining()==3);
    assert(market.CycleOffer()&&market.offer()==1);
    const int wholesale=market.offer_cost();
    const int previous_stock=market.stock_drink(1);
    assert(market.CanAcceptStock());
    assert(sim.SpendSpecialStock(wholesale));
    assert(market.AcceptStock());
    assert(market.stock_drink(1)-previous_stock==trade::kDrinkCatalog[1].units);
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
    // The current standalone simulation can still validate its own segment.
    assert(loaded.Deserialize(packed,Simulation::kSerializedSize,false));
    // Pre-1.0 migrations are intentionally dropped; new v3 state stays atomic.
    Simulation fresh;
    uint8_t current_bytes[Simulation::kSerializedSize]={};
    size_t current_size=0u;
    assert(fresh.Serialize(current_bytes,sizeof(current_bytes),&current_size));
    Simulation loaded_current;
    assert(loaded_current.Deserialize(current_bytes,current_size));
    // No wholesale purchasing may silently exceed the available wallet.
    assert(!loaded_current.SpendSpecialStock(loaded_current.cash_cents()+1));
    std::cout<<"Red Ledger integrated trade: six-faction sales, wholesale and v3 save OK\n";
}
