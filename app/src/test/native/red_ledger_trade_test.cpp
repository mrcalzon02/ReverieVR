#include "red_ledger_trade.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

using namespace reverie::redledger;

int main() {
    static_assert(trade::kFactions==6, "Six supplier groups are required");
    static_assert(trade::kDrinks==19, "18 original drinks plus the house tap");
    static_assert(trade::kSnacks==6, "Six original snacks");
    static_assert(trade::kOffers==25, "24 original SKUs plus house tap");
    static_assert(trade::Trade::kSerializedSize==152u,"v3 stock must be versioned");

    constexpr std::array<PatronKind,6> people={{
        PatronKind::RailWorker,PatronKind::NightClerk,
        PatronKind::MilitiaVeteran,PatronKind::Mechanic,
        PatronKind::DirectorateCourier,PatronKind::SalihorskHauler
    }};
    std::array<int,trade::kFactions> seen{};
    for(PatronKind patron:people){
        const int faction=trade::FactionFor(patron);
        assert(faction>=0&&faction<trade::kFactions);
        ++seen[static_cast<size_t>(faction)];
        const int drink=trade::FavoriteDrink(patron);
        const int snack=trade::FavoriteSnack(patron);
        assert(drink>0&&drink<trade::kDrinks);
        assert(snack>=0&&snack<trade::kSnacks);
        assert(trade::kDrinkCatalog[drink].faction==faction);
        assert(trade::kSnackCatalog[snack].faction==faction);
        const auto chosen=trade::RateService(patron,drink,snack);
        const auto wrong=trade::RateService(patron,0,-1);
        assert(chosen.cents>wrong.cents && chosen.reputation>wrong.reputation);
        assert(chosen.favor==4 && wrong.favor<0);
    }
    for(int count:seen)assert(count==1);
    for(int i=0;i<trade::kFactions;++i){
        const int first=1+i*3;
        const int catalog_prices[6][4]={
            {800,1000,3200,600}, {700,900,3000,500},
            {900,1100,3300,700}, {800,1000,3400,600},
            {1000,1200,3500,800}, {900,1100,3600,700}
        };
        for(int d=0;d<3;++d){
            assert(trade::kDrinkCatalog[first+d].faction==i);
            assert(trade::kDrinkCatalog[first+d].wholesale_cents==catalog_prices[i][d]);
        }
        assert(trade::kSnackCatalog[i].faction==i);
        assert(trade::kSnackCatalog[i].wholesale_cents==catalog_prices[i][3]);
    }
    assert(std::strcmp(trade::kDrinkCatalog[1].name,"BLUE HARBOR DRAFT")==0);
    assert(std::strcmp(trade::kDrinkCatalog[18].name,"MAZR CRYSTAL VODKA")==0);
    assert(std::strcmp(trade::kSnackCatalog[5].name,"BEANS CHICKEN AND POTATO")==0);

    // Unreal buy/sell prices are item-level source metadata; these
    // pre-1.0 order lot yields prevent a bottle of spirits being purchased
    // at $32 and sold as just three $5 drinks.
    for(int faction=0;faction<6;++faction){
        const int first=1+faction*3;
        assert(trade::kDrinkCatalog[first].units==8);
        assert(trade::kDrinkCatalog[first+1].units==6);
        assert(trade::kDrinkCatalog[first+2].units==12);
        assert(trade::kSnackCatalog[faction].units==6);
    }
    trade::Trade state;
    assert(state.stock_drink(1)==1 && state.stock_drink(7)==1);
    assert(state.stock_snack(2)==1);
    assert(state.SelectDrink(6) && state.drink()==1);
    assert(state.SelectDrink(6) && state.drink()==4);
    assert(state.SelectDrink(6) && state.drink()==7);
    assert(state.SelectSnack() && state.snack()==0);
    assert(state.SelectSnack() && state.snack()==2);
    assert(state.ConsumeDrink() && state.stock_drink(7)==0);
    assert(state.SnackAvailable());
    state.ConsumeSnack();
    assert(state.stock_snack(2)==0);
    state.RecordService(PatronKind::RailWorker);
    assert(state.favor(2)==4 && state.drink()==0 && state.snack()==-1);
    assert(state.CycleOffer() && state.offer()==1);
    assert(state.offer_cost()==trade::kDrinkCatalog[1].wholesale_cents);
    assert(state.CanAcceptStock());
    assert(state.AcceptStock() && state.stock_drink(1)==9);
    assert(state.Enter(trade::Room::Stockroom));
    assert(!state.Enter(trade::Room::Stockroom));

    std::vector<uint8_t> bytes(trade::Trade::kSerializedSize+2u,0xC5u);
    assert(!state.Serialize(nullptr,trade::Trade::kSerializedSize));
    assert(!state.Serialize(bytes.data(),trade::Trade::kSerializedSize-1u));
    assert(state.Serialize(bytes.data(),trade::Trade::kSerializedSize));
    assert(bytes[trade::Trade::kSerializedSize]==0xC5u);
    trade::Trade restored;
    assert(restored.Deserialize(bytes.data(),trade::Trade::kSerializedSize));
    assert(restored.favor(2)==4 && restored.stock_drink(1)==9);
    assert(restored.room()==trade::Room::Stockroom);

    // Every SKU must stop at the serialized stock cap, never silently
    // overflow it or mutate on an unfulfillable case order.
    for(int sku=1;sku<trade::kOffers;++sku){
        trade::Trade inventory;
        assert(inventory.SelectOffer(sku));
        const int stock_units=sku<trade::kDrinks?
            trade::kDrinkCatalog[sku].units:
            trade::kSnackCatalog[sku-trade::kDrinks].units;
        int prev=sku<trade::kDrinks?inventory.stock_drink(sku):
            inventory.stock_snack(sku-trade::kDrinks);
        int orders=0;
        while(inventory.CanAcceptStock()){
            assert(inventory.AcceptStock());
            const int now=sku<trade::kDrinks?inventory.stock_drink(sku):
                inventory.stock_snack(sku-trade::kDrinks);
            assert(now-prev==stock_units && now<=100000);
            prev=now;
            assert(++orders<=100000);
        }
        assert(prev>100000-stock_units);
        assert(!inventory.AcceptStock());
        const int after=sku<trade::kDrinks?inventory.stock_drink(sku):
            inventory.stock_snack(sku-trade::kDrinks);
        assert(after==prev);
        std::array<uint8_t,trade::Trade::kSerializedSize> cap_save{};
        assert(inventory.Serialize(cap_save.data(),cap_save.size()));
        trade::Trade cap_loaded;
        assert(cap_loaded.Deserialize(cap_save.data(),cap_save.size()));
    }
    const int favor_before=restored.favor(2);
    bytes[19]^=0x7fu;
    assert(!restored.Deserialize(bytes.data(),trade::Trade::kSerializedSize));
    assert(restored.favor(2)==favor_before);
    assert(!restored.Deserialize(bytes.data(),trade::Trade::kSerializedSize-1u));
    std::array<uint8_t,100u> obsolete={};
    assert(!restored.Deserialize(obsolete.data(),obsolete.size()));
    std::cout<<"Red Ledger: all six suppliers, 24 original SKUs and v3 save integrity OK\n";
}
