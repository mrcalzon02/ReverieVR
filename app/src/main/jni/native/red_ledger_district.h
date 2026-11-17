#ifndef REVERIE_RED_LEDGER_DISTRICT_H
#define REVERIE_RED_LEDGER_DISTRICT_H

#include <algorithm>
#include "red_ledger_clientele.h"
#include "red_ledger_renovations.h"

namespace reverie { namespace redledger { namespace district {

struct GroceryAccount {
    static constexpr int kOpeningDay = 61;
    static constexpr int kOrderInterval = 7;
    static constexpr int kLot = 3;
    static constexpr int kMinimumServices = 4;
    static constexpr size_t kBytes = 12;
    uint8_t envelopes_paid = 0;
    bool incomplete_books = false;
    uint8_t selected = 0;
    uint8_t last_supplier = 0;
    uint32_t last_dispatch = 0;
    uint32_t deliveries = 0;

    void close_shift(const DayLedger &book) {
        if (book.day <= 7 && book.protection_spend_cents > 0) ++envelopes_paid;
    }
    int next_day() const {
        if (!last_dispatch) return kOpeningDay;
        return kOpeningDay + (1 + (static_cast<int>(last_dispatch) - kOpeningDay) /
                                  kOrderInterval) * kOrderInterval;
    }
    static bool improved(const property::Renovations &building) {
        return building.tier(property::Walls) || building.tier(property::Floor) ||
               building.tier(property::Ceiling) || building.tier(property::Seating);
    }
    int first_fee() const {
        // The old books cannot establish a missed payment, so no fee is imputed.
        return !incomplete_books && envelopes_paid < 3 ? 100 : 0;
    }
    int proceeds(int faction, bool first) const {
        return trade::kSnackCatalog[faction].wholesale_cents / 2 + 150 -
               (first ? first_fee() : 0);
    }
    bool dispatch(Simulation &sim, trade::Trade &market,
                  const property::Renovations &building, const clientele::State &customers) {
        if (sim.day() < next_day() || !improved(building) ||
            customers.services() < kMinimumServices || market.stock_snack(selected) < kLot)
            return false;
        Simulation settled = sim;
        if (!settled.RecordGrocerySale(proceeds(selected, deliveries == 0)) ||
            !market.SupplyGrocer(selected, kLot)) return false;
        sim = settled;
        last_supplier = selected;
        last_dispatch = static_cast<uint32_t>(sim.day());
        ++deliveries;
        return true;
    }
    void encode(uint8_t *p) const {
        p[0]=envelopes_paid; p[1]=incomplete_books; p[2]=selected; p[3]=last_supplier;
        for (unsigned i=0; i<4; ++i) {
            p[4+i]=static_cast<uint8_t>(last_dispatch >> (8*i));
            p[8+i]=static_cast<uint8_t>(deliveries >> (8*i));
        }
    }
    bool decode(const uint8_t *p, int day) {
        GroceryAccount next;
        if (p[0] > std::min(3, (day+1)/3) || p[1] > 1 ||
            p[2] >= trade::kFactions || p[3] >= trade::kFactions) return false;
        next.envelopes_paid=p[0]; next.incomplete_books=p[1]; next.selected=p[2]; next.last_supplier=p[3];
        for (unsigned i=0; i<4; ++i) {
            next.last_dispatch |= static_cast<uint32_t>(p[4+i]) << (8*i);
            next.deliveries |= static_cast<uint32_t>(p[8+i]) << (8*i);
        }
        if (!next.deliveries) {
            if (next.last_dispatch || next.last_supplier) return false;
        } else if (next.last_dispatch < kOpeningDay || next.last_dispatch > static_cast<uint32_t>(day) ||
                   next.deliveries > 1+(next.last_dispatch-kOpeningDay)/kOrderInterval) return false;
        *this=next;
        return true;
    }
};

}}}
#endif
