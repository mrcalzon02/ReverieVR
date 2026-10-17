#include "red_ledger_simulation.h"
#include "red_ledger_trade.h"
#include "red_ledger_renovations.h"
#include <array>
#include <algorithm>
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
    // Genuine living-quarters renovation loop: paid upgrades affect later
    // patron receipts and survive the new composite save section.
    property::Renovations upgrades;
    Simulation investor;
    trade::Trade vendor;
    assert(upgrades.category()==property::Walls);
    assert(upgrades.tier(property::Walls)==0);
    assert(upgrades.cost(vendor)==1600);
    const int32_t opening=investor.cash_cents();
    assert(upgrades.buy(&investor,vendor));
    assert(upgrades.tier(property::Walls)==1);
    assert(investor.cash_cents()==opening-1600);
    assert(upgrades.service_premium(7,2)>0);
    assert(!upgrades.buy(&investor,vendor)); // second tier costs 3200
    assert(upgrades.tier(property::Walls)==1);
    for(int category=1;category<property::Count;++category){
        assert(upgrades.next());
        assert(upgrades.category()==category);
    }
    assert(upgrades.next()&&upgrades.category()==property::Walls);
    std::array<uint8_t,property::Renovations::kSerializedSize> renoBytes{};
    assert(!upgrades.serialize(nullptr,renoBytes.size()));
    assert(upgrades.serialize(renoBytes.data(),renoBytes.size()));
    property::Renovations reload;
    assert(reload.deserialize(renoBytes.data(),renoBytes.size()));
    assert(reload.tier(property::Walls)==1);
    assert(reload.category()==property::Walls);
    const int previous=reload.tier(property::Walls);
    renoBytes[4]=6u; // valid hash will still reject tier 6
    uint32_t digest=2166136261u;
    for(size_t i=0;i<16u;++i)digest=(digest^renoBytes[i])*16777619u;
    for(int i=0;i<4;++i)renoBytes[16+i]=static_cast<uint8_t>(digest>>(i*8));
    assert(!reload.deserialize(renoBytes.data(),renoBytes.size()));
    assert(reload.tier(property::Walls)==previous);
    renoBytes[4]=1u;
    assert(!reload.deserialize(renoBytes.data(),renoBytes.size())); // bad checksum
    assert(!reload.deserialize(renoBytes.data(),renoBytes.size()-1u));
    Simulation visitor;
    assert(visitor.TakeCleanCup()&&visitor.FillSpecialDrink());
    const auto *buyer=visitor.current_patron();
    assert(buyer!=nullptr);
    const auto quote=trade::RateService(buyer->kind,7,2);
    assert(visitor.ServeHeldCup());
    const int expected=buyer->drink_price_cents+quote.cents+
                       upgrades.service_premium(7,2);
    assert(visitor.ApplyServicePremium(
        quote.cents+upgrades.service_premium(7,2),
        std::min(4,quote.reputation+upgrades.reputation_bonus())));
    assert(visitor.pending_payment_cents()==expected);
    assert(visitor.CollectPayment());
    assert(visitor.cash_cents()==2200+expected);
    std::cout<<"Red Ledger trade and v4 renovations: paid upgrades, revenue and save OK\n";
}
