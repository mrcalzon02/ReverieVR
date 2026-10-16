#include "red_ledger_trade.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace reverie::redledger;
int main(){
    trade::Trade state;
    assert(state.stock_drink(1)==1&&state.stock_snack(1)==1);
    const auto chosen=trade::RateService(PatronKind::RailWorker,1,1);
    const auto wrong=trade::RateService(PatronKind::RailWorker,0,-1);
    assert(chosen.cents>wrong.cents&&chosen.reputation>wrong.reputation);
    assert(chosen.favor==4&&wrong.favor<0);
    assert(state.SelectDrink(6)&&state.drink()==1);
    assert(state.SelectSnack()&&state.snack()==1);
    assert(state.ConsumeDrink()&&state.stock_drink(1)==0);
    assert(state.SnackAvailable());
    state.ConsumeSnack();
    assert(state.stock_snack(1)==0);
    state.RecordService(PatronKind::RailWorker);
    assert(state.favor(0)==4&&state.drink()==0&&state.snack()==-1);
    assert(state.CycleOffer()&&state.offer()==1);
    const int cost=state.offer_cost();
    assert(cost==trade::kDrinkCatalog[1].wholesale_cents);
    assert(state.AcceptStock()&&state.stock_drink(1)==3);
    assert(state.Enter(trade::Room::Stockroom));
    assert(!state.Enter(trade::Room::Stockroom));
    std::vector<uint8_t> bytes(trade::Trade::kSerializedSize+2u,0xC5u);
    assert(!state.Serialize(nullptr,trade::Trade::kSerializedSize));
    assert(!state.Serialize(bytes.data(),trade::Trade::kSerializedSize-1u));
    assert(state.Serialize(bytes.data(),trade::Trade::kSerializedSize));
    assert(bytes[trade::Trade::kSerializedSize]==0xC5u);
    trade::Trade restored;
    assert(restored.Deserialize(bytes.data(),trade::Trade::kSerializedSize));
    assert(restored.favor(0)==4&&restored.stock_drink(1)==3);
    assert(restored.room()==trade::Room::Stockroom);
    const int favor_before=restored.favor(0);
    bytes[19]^=0x7fu;
    assert(!restored.Deserialize(bytes.data(),trade::Trade::kSerializedSize));
    assert(restored.favor(0)==favor_before);
    assert(!restored.Deserialize(bytes.data(),trade::Trade::kSerializedSize-1u));
    std::cout<<"Red Ledger trade: faction menus, preferences, checksum and migration state OK\n";
}
