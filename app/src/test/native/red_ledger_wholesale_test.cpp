#include "red_ledger_wholesale.h"
#include "red_ledger_save.h"
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

    // The live checkout uses a two-party wallet/inventory transaction.
    // A checked whole-game restart must preserve both halves, including an
    // unaffordable offer (no charge), one purchased lot, and no free replay.
    {
        save::State original{};
        assert(original.market.SelectOffer(1));
        save::Checkpoint::Bytes opening{};
        assert(save::Checkpoint::Encode(original,&opening));
        save::State resumed{};
        assert(save::Checkpoint::Decode(
            &resumed,opening.data(),opening.size())==save::Checkpoint::Format::Checked);
        const int32_t opening_cash=resumed.simulation.cash_cents();
        const int opening_stock=resumed.market.stock_drink(1);
        const int lot=trade::kDrinkCatalog[1].units;
        assert(opening_cash>=resumed.market.offer_cost());

        // An expensive offer must reject without debiting any of the checked
        // gameplay state, not merely leave the selected SKU's stock unchanged.
        assert(resumed.market.SelectOffer(3));
        save::Checkpoint::Bytes expensive_before{},expensive_after{};
        assert(save::Checkpoint::Encode(resumed,&expensive_before));
        assert(!trade::BuyWholesale(&resumed.simulation,&resumed.market));
        assert(save::Checkpoint::Encode(resumed,&expensive_after));
        assert(expensive_after==expensive_before);
        assert(resumed.market.SelectOffer(1));
        const int price=resumed.market.offer_cost();
        assert(trade::BuyWholesale(&resumed.simulation,&resumed.market));
        assert(resumed.simulation.cash_cents()==opening_cash-price);
        assert(resumed.market.stock_drink(1)==opening_stock+lot);

        save::Checkpoint::Bytes purchased{},roundtrip{};
        assert(save::Checkpoint::Encode(resumed,&purchased));
        save::State reopened{};
        assert(save::Checkpoint::Decode(
            &reopened,purchased.data(),purchased.size())==save::Checkpoint::Format::Checked);
        assert(save::Checkpoint::Encode(reopened,&roundtrip));
        assert(roundtrip==purchased);
        assert(reopened.simulation.cash_cents()==opening_cash-price);
        assert(reopened.market.stock_drink(1)==opening_stock+lot);
        assert(reopened.market.SelectOffer(3));
        save::Checkpoint::Bytes failed_before{},failed_after{};
        assert(save::Checkpoint::Encode(reopened,&failed_before));
        assert(!trade::BuyWholesale(&reopened.simulation,&reopened.market));
        assert(save::Checkpoint::Encode(reopened,&failed_after));
        assert(failed_before==failed_after);
        assert(reopened.simulation.cash_cents()==opening_cash-price);
        assert(reopened.market.stock_drink(1)==opening_stock+lot);
    }

    // A full SKU restored from a checked checkpoint must not collect money,
    // even when the buyer has enough cash for the displayed price.
    {
        save::State full{};
        assert(full.market.SelectOffer(1));
        while(full.market.CanAcceptStock())assert(full.market.AcceptStock());
        assert(full.simulation.cash_cents()>=full.market.offer_cost());
        save::Checkpoint::Bytes capped{},after{};
        assert(save::Checkpoint::Encode(full,&capped));
        save::State restored_full{};
        assert(save::Checkpoint::Decode(
            &restored_full,capped.data(),capped.size())==save::Checkpoint::Format::Checked);
        assert(!trade::BuyWholesale(&restored_full.simulation,&restored_full.market));
        assert(save::Checkpoint::Encode(restored_full,&after));
        assert(capped==after);
    }

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
    // The live checkout derives the lot limit from the purchased category,
    // not the serialization safety ceiling. All 24 branded products must
    // gain exactly one usable lot when their own storage is improved.
    for(int offer=1;offer<trade::kOffers;++offer){
        property::Renovations storage;
        Simulation investor;
        trade::Trade inventory;
        assert(inventory.SelectOffer(offer));
        const int category=offer>=trade::kDrinks?property::Snacks:
            ((offer-1)%3)==0?property::Draft:((offer-1)%3)==1?property::Bottles:property::Spirits;
        const int lot=offer>=trade::kDrinks?trade::kSnackCatalog[offer-trade::kDrinks].units:
            trade::kDrinkCatalog[offer].units;
        const int cap=storage.stock_capacity(offer);
        assert(cap==2*lot);
        while(inventory.CanAcceptStock(cap))assert(inventory.AcceptStock(cap));
        const int before=investor.cash_cents();
        assert(!trade::BuyWholesale(&investor,&inventory,cap));
        assert(investor.cash_cents()==before);
        for(int i=0;i<category;++i)assert(storage.next());
        assert(storage.buy(&investor,inventory));
        assert(investor.cash_cents()==before-1600);
        assert(storage.stock_capacity(offer)==cap+lot);
        assert(inventory.CanAcceptStock(storage.stock_capacity(offer)));
        assert(inventory.AcceptStock(storage.stock_capacity(offer)));
        assert(!inventory.CanAcceptStock(storage.stock_capacity(offer)));
        assert(storage.stock_capacity(0)==(category==property::Draft?24:16));
        save::State checkpoint{};
        checkpoint.simulation=investor;
        checkpoint.market=inventory;
        checkpoint.renovations=storage;
        save::Checkpoint::Bytes bytes{};
        assert(save::Checkpoint::Encode(checkpoint,&bytes));
        save::State loaded{};
        assert(save::Checkpoint::Decode(&loaded,bytes.data(),bytes.size())==save::Checkpoint::Format::Checked);
        assert(loaded.renovations.stock_capacity(offer)==cap+lot);
        assert(!loaded.market.CanAcceptStock(loaded.renovations.stock_capacity(offer)));
    }
    std::cout<<"Red Ledger wholesale: 24 atomic wallet/inventory capacity rejections and paid storage expansions OK\n";
}
