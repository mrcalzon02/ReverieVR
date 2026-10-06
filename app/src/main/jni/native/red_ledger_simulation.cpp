#include "red_ledger_simulation.h"

#include <algorithm>

namespace reverie {
namespace redledger {

namespace {

const std::array<PatronDefinition, 4> kPatrons = {{
    {PatronKind::RailWorker, "Rail worker", 280, 1},
    {PatronKind::NightClerk, "Night clerk", 240, 1},
    {PatronKind::MilitiaVeteran, "Militia veteran", 330, 2},
    {PatronKind::Mechanic, "Mechanic", 300, 1}
}};

const std::array<SupplierOffer, 2> kSupplies = {{
    {SupplierItem::BeerCrate, "Eight-bottle beer crate", 900, 8, 0},
    {SupplierItem::CupSet, "Four mismatched cups", 320, 0, 4}
}};

EventType EventForDay(int32_t day) {
    const int32_t phase = (std::max(day, 1) - 1) % 3;
    switch (phase) {
        case 0:
            return EventType::ProtectionDemand;
        case 1:
            return EventType::Inspection;
        case 2:
        default:
            return EventType::SupplyInterruption;
    }
}

}  // namespace

Simulation::Simulation() {
    Reset();
}

void Simulation::Reset() {
    day_ = 1;
    cash_cents_ = 2200;
    debt_cents_ = 20000;
    beer_units_ = 6;
    clean_cups_ = 3;
    dirty_cups_ = 0;
    reputation_ = 0;
    pressure_ = 0;
    last_ledger_ = {};
    OpenDay();
}

int32_t Simulation::day() const { return day_; }
int32_t Simulation::cash_cents() const { return cash_cents_; }
int32_t Simulation::debt_cents() const { return debt_cents_; }
int32_t Simulation::beer_units() const { return beer_units_; }
int32_t Simulation::clean_cups() const { return clean_cups_; }
int32_t Simulation::dirty_cups() const { return dirty_cups_; }
int32_t Simulation::reputation() const { return reputation_; }
int32_t Simulation::pressure() const { return pressure_; }
int32_t Simulation::patrons_remaining() const {
    return std::max(0, kPatronsPerDay - next_patron_index_);
}
bool Simulation::day_open() const { return day_open_; }
EventType Simulation::current_event() const { return event_; }
bool Simulation::protection_paid() const { return protection_paid_; }

const PatronDefinition *Simulation::current_patron() const {
    if (!day_open_ || next_patron_index_ >= kPatronsPerDay) {
        return nullptr;
    }

    const PatronKind kind = patron_queue_[next_patron_index_];
    for (const PatronDefinition &patron : kPatrons) {
        if (patron.kind == kind) {
            return &patron;
        }
    }
    return nullptr;
}

const DayLedger &Simulation::last_ledger() const {
    return last_ledger_;
}

bool Simulation::ServeNextPatron() {
    const PatronDefinition *patron = current_patron();
    if (patron == nullptr) {
        return false;
    }

    if (beer_units_ <= 0 || clean_cups_ <= 0) {
        ++lost_today_;
        --reputation_;
        --day_reputation_delta_;
        ++pressure_;
        ++day_pressure_delta_;
        ++next_patron_index_;
        return false;
    }

    --beer_units_;
    --clean_cups_;
    ++dirty_cups_;
    cash_cents_ += patron->drink_price_cents;
    sales_cents_ += patron->drink_price_cents;
    ++served_today_;
    reputation_ += patron->reputation_on_service;
    day_reputation_delta_ += patron->reputation_on_service;
    ++next_patron_index_;
    return true;
}

bool Simulation::WashOneCup() {
    if (!day_open_ || dirty_cups_ <= 0) {
        return false;
    }

    --dirty_cups_;
    ++clean_cups_;
    return true;
}

bool Simulation::BuySupply(SupplierItem item) {
    if (!day_open_ || event_ == EventType::SupplyInterruption) {
        return false;
    }

    const SupplierOffer *offer = FindOffer(item);
    if (offer == nullptr || cash_cents_ < offer->cost_cents) {
        return false;
    }

    cash_cents_ -= offer->cost_cents;
    supply_spend_cents_ += offer->cost_cents;
    beer_units_ += offer->beer_units;
    clean_cups_ += offer->clean_cups;
    return true;
}

bool Simulation::PayProtection() {
    if (!day_open_
        || event_ != EventType::ProtectionDemand
        || protection_paid_
        || cash_cents_ < kProtectionCostCents) {
        return false;
    }

    cash_cents_ -= kProtectionCostCents;
    protection_spend_cents_ += kProtectionCostCents;
    protection_paid_ = true;
    if (pressure_ > 0) {
        --pressure_;
        --day_pressure_delta_;
    }
    return true;
}

DayLedger Simulation::CloseDay() {
    if (!day_open_) {
        return last_ledger_;
    }

    if (event_ == EventType::ProtectionDemand && !protection_paid_) {
        pressure_ += 3;
        day_pressure_delta_ += 3;
        reputation_ -= 1;
        day_reputation_delta_ -= 1;
    }

    if (event_ == EventType::Inspection && dirty_cups_ > clean_cups_) {
        const int32_t fine = std::min(cash_cents_, kInspectionFineCents);
        cash_cents_ -= fine;
        inspection_fines_cents_ += fine;
        ++pressure_;
        ++day_pressure_delta_;
    }

    const int32_t debt_payment =
        std::min(cash_cents_, kDailyDebtServiceCents);
    cash_cents_ -= debt_payment;
    debt_cents_ = std::max(0, debt_cents_ - debt_payment);

    last_ledger_.day = day_;
    last_ledger_.opening_cash_cents = opening_cash_cents_;
    last_ledger_.sales_cents = sales_cents_;
    last_ledger_.supply_spend_cents = supply_spend_cents_;
    last_ledger_.protection_spend_cents = protection_spend_cents_;
    last_ledger_.inspection_fines_cents = inspection_fines_cents_;
    last_ledger_.debt_service_cents = debt_payment;
    last_ledger_.closing_cash_cents = cash_cents_;
    last_ledger_.patrons_served = served_today_;
    last_ledger_.patrons_lost = lost_today_;
    last_ledger_.ending_beer_units = beer_units_;
    last_ledger_.ending_clean_cups = clean_cups_;
    last_ledger_.ending_dirty_cups = dirty_cups_;
    last_ledger_.reputation_delta = day_reputation_delta_;
    last_ledger_.pressure_delta = day_pressure_delta_;

    day_open_ = false;
    ++day_;
    OpenDay();
    return last_ledger_;
}

const std::array<PatronDefinition, 4> &Simulation::PatronCatalog() {
    return kPatrons;
}

const std::array<SupplierOffer, 2> &Simulation::SupplierCatalog() {
    return kSupplies;
}

void Simulation::OpenDay() {
    day_open_ = true;
    event_ = EventForDay(day_);
    protection_paid_ = false;
    opening_cash_cents_ = cash_cents_;
    sales_cents_ = 0;
    supply_spend_cents_ = 0;
    protection_spend_cents_ = 0;
    inspection_fines_cents_ = 0;
    served_today_ = 0;
    lost_today_ = 0;
    day_reputation_delta_ = 0;
    day_pressure_delta_ = 0;
    next_patron_index_ = 0;
    BuildPatronQueue();
}

void Simulation::BuildPatronQueue() {
    const int32_t rotation = (day_ - 1) % static_cast<int32_t>(kPatrons.size());
    for (int32_t index = 0; index < kPatronsPerDay; ++index) {
        patron_queue_[index] = kPatrons[
            static_cast<size_t>((rotation + index) % static_cast<int32_t>(kPatrons.size()))
        ].kind;
    }
}

const SupplierOffer *Simulation::FindOffer(SupplierItem item) const {
    for (const SupplierOffer &offer : kSupplies) {
        if (offer.item == item) {
            return &offer;
        }
    }
    return nullptr;
}

}  // namespace redledger
}  // namespace reverie
