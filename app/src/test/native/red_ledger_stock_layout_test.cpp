#include "red_ledger_stock_layout.h"
#include "red_ledger_trade.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::redledger;

int main() {
    static_assert(stock::kCells==24,"The original catalog is six by four");
    std::array<bool,trade::kOffers> seen{};
    trade::Trade market;
    for(int slot=0;slot<stock::kCells;++slot) {
        assert(stock::IsValid(slot));
        const stock::Cell c=stock::At(slot);
        assert(c.faction==slot%stock::kSuppliers);
        assert(c.group==slot/stock::kSuppliers);
        assert(stock::SupplierCode(c.faction)==101*(c.faction+1));
        assert(c.offer>=1 && c.offer<trade::kOffers && !seen[c.offer]);
        seen[c.offer]=true;
        const int supplier=c.group<3 ?
            trade::kDrinkCatalog[c.offer].faction :
            trade::kSnackCatalog[c.offer-trade::kDrinks].faction;
        assert(supplier==c.faction);
        assert(c.x-stock::kHalfWidth>=-3.0f);
        assert(c.x+stock::kHalfWidth<=3.0f);
        assert(c.y-stock::kHalfHeight>=-1.5f);
        assert(c.y+stock::kHalfHeight<=1.5f);
        assert(std::sqrt(c.x*c.x+c.y*c.y+c.z*c.z)<4.25f);
        for(int other=slot+1;other<stock::kCells;++other){
            const stock::Cell d=stock::At(other);
            assert(std::abs(c.x-d.x)>=2*stock::kHalfWidth ||
                   std::abs(c.y-d.y)>=2*stock::kHalfHeight);
        }
        // Directly choosing a card must choose its actual purchasable SKU,
        // not just change a decorative caption or shared hover state.
        if(market.offer()!=c.offer)assert(market.SelectOffer(c.offer));
        else assert(!market.SelectOffer(c.offer));
        assert(market.offer()==c.offer);
        assert(market.offer_cost()>0);
    }
    for(int offer=1;offer<trade::kOffers;++offer)assert(seen[offer]);
    assert(!market.SelectOffer(-1));
    assert(!market.SelectOffer(trade::kOffers));
    assert(!stock::IsValid(-1) && !stock::IsValid(stock::kCells));
    std::cout<<"Red Ledger: 24 direct stock cards, unique SKUs, hit bounds and wallet quotes OK\n";
}
