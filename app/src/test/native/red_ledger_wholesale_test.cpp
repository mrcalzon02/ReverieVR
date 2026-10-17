#include "red_ledger_wholesale.h"
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace reverie::redledger;
int main() {
    Simulation sim;
    trade::Trade market;
    const int32_t cash=sim.cash_cents();
    assert(!trade::BuyWholesale(nullptr,&market));
    assert(!trade::BuyWholesale(&sim,nullptr));
    assert(!trade::BuyWholesale(&sim,&market)); // house lager uses legacy crate
    assert(sim.cash_cents()==cash);
    assert(market.SelectOffer(1));
    const int initial=market.stock_drink(1);
    assert(trade::BuyWholesale(&sim,&market));
    assert(sim.cash_cents()==cash-trade::kDrinkCatalog[1].wholesale_cents);
    assert(market.stock_drink(1)==initial+8);
    const int32_t after_purchase=sim.cash_cents();
    assert(market.SelectOffer(3)); // original Concordant whiskey is costly
    const int whiskey_before=market.stock_drink(3);
    assert(!trade::BuyWholesale(&sim,&market));
    assert(sim.cash_cents()==after_purchase);
    assert(market.stock_drink(3)==whiskey_before);

    // Force each of 24 source products to its exact max capacity without
    // altering cash. No stock overflow or partial money debits are allowed.
    for(int offer=1;offer<trade::kOffers;++offer){
        Simulation wallet;
        trade::Trade inventory;
        assert(inventory.SelectOffer(offer));
        while(inventory.CanAcceptStock())assert(inventory.AcceptStock());
        const int32_t before=wallet.cash_cents();
        const int stock_before=offer<trade::kDrinks?
            inventory.stock_drink(offer):
            inventory.stock_snack(offer-trade::kDrinks);
        assert(!trade::BuyWholesale(&wallet,&inventory));
        assert(wallet.cash_cents()==before);
        assert((offer<trade::kDrinks?
            inventory.stock_drink(offer):
            inventory.stock_snack(offer-trade::kDrinks))==stock_before);
    }
    std::cout<<"Red Ledger wholesale: 24 atomic wallet/inventory capacity rejections OK\n";
}
