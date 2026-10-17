#ifndef REVERIE_RED_LEDGER_WHOLESALE_H
#define REVERIE_RED_LEDGER_WHOLESALE_H

#include "red_ledger_simulation.h"
#include "red_ledger_trade.h"

// Game-private two-party transaction. Failure must leave BOTH the live wallet
// and the live inventory unchanged. No pre-1.0 save compatibility is assumed.
namespace reverie { namespace redledger { namespace trade {
inline bool BuyWholesale(Simulation *simulation, Trade *market) {
    if(simulation==nullptr || market==nullptr || !market->CanAcceptStock())
        return false;
    Simulation cash=*simulation;
    Trade inventory=*market;
    const int cost=market->offer_cost();
    if(!cash.SpendSpecialStock(cost) || !inventory.AcceptStock())
        return false;
    *simulation=cash;
    *market=inventory;
    return true;
}
}}}
#endif
