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

constexpr uint32_t kSaveMagic = 0x31534c52u;
constexpr uint32_t kSaveVersion = 1u;

void WriteU32(
    uint8_t *buffer,
    size_t *offset,
    uint32_t value
) {
    buffer[*offset + 0u] = static_cast<uint8_t>(value & 0xffu);
    buffer[*offset + 1u] = static_cast<uint8_t>((value >> 8u) & 0xffu);
    buffer[*offset + 2u] = static_cast<uint8_t>((value >> 16u) & 0xffu);
    buffer[*offset + 3u] = static_cast<uint8_t>((value >> 24u) & 0xffu);
    *offset += 4u;
}

bool ReadU32(
    const uint8_t *buffer,
    size_t size,
    size_t *offset,
    uint32_t *value
) {
    if (buffer == nullptr
        || offset == nullptr
        || value == nullptr
        || *offset > size
        || size - *offset < 4u) {
        return false;
    }

    *value =
        static_cast<uint32_t>(buffer[*offset + 0u])
        | (static_cast<uint32_t>(buffer[*offset + 1u]) << 8u)
        | (static_cast<uint32_t>(buffer[*offset + 2u]) << 16u)
        | (static_cast<uint32_t>(buffer[*offset + 3u]) << 24u);
    *offset += 4u;
    return true;
}

bool InRange(
    int32_t value,
    int32_t minimum,
    int32_t maximum
) {
    return value >= minimum && value <= maximum;
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

bool Simulation::Serialize(
    uint8_t *buffer,
    size_t capacity,
    size_t *out_size
) const {
    if (out_size == nullptr) {
        return false;
    }
    *out_size = kSerializedSize;
    if (buffer == nullptr || capacity < kSerializedSize) {
        return false;
    }

    size_t offset = 0u;
    auto write_int = [&](int32_t value) {
        WriteU32(buffer, &offset, static_cast<uint32_t>(value));
    };

    WriteU32(buffer, &offset, kSaveMagic);
    WriteU32(buffer, &offset, kSaveVersion);

    write_int(day_);
    write_int(cash_cents_);
    write_int(debt_cents_);
    write_int(beer_units_);
    write_int(clean_cups_);
    write_int(dirty_cups_);
    write_int(reputation_);
    write_int(pressure_);
    write_int(static_cast<int32_t>(event_));
    write_int(day_open_ ? 1 : 0);
    write_int(protection_paid_ ? 1 : 0);
    write_int(opening_cash_cents_);
    write_int(sales_cents_);
    write_int(supply_spend_cents_);
    write_int(protection_spend_cents_);
    write_int(inspection_fines_cents_);
    write_int(served_today_);
    write_int(lost_today_);
    write_int(day_reputation_delta_);
    write_int(day_pressure_delta_);
    write_int(next_patron_index_);

    write_int(last_ledger_.day);
    write_int(last_ledger_.opening_cash_cents);
    write_int(last_ledger_.sales_cents);
    write_int(last_ledger_.supply_spend_cents);
    write_int(last_ledger_.protection_spend_cents);
    write_int(last_ledger_.inspection_fines_cents);
    write_int(last_ledger_.debt_service_cents);
    write_int(last_ledger_.closing_cash_cents);
    write_int(last_ledger_.patrons_served);
    write_int(last_ledger_.patrons_lost);
    write_int(last_ledger_.ending_beer_units);
    write_int(last_ledger_.ending_clean_cups);
    write_int(last_ledger_.ending_dirty_cups);
    write_int(last_ledger_.reputation_delta);
    write_int(last_ledger_.pressure_delta);

    return offset == kSerializedSize;
}

bool Simulation::Deserialize(
    const uint8_t *buffer,
    size_t size
) {
    if (buffer == nullptr || size != kSerializedSize) {
        return false;
    }

    size_t offset = 0u;
    uint32_t magic = 0u;
    uint32_t version = 0u;
    if (!ReadU32(buffer, size, &offset, &magic)
        || !ReadU32(buffer, size, &offset, &version)
        || magic != kSaveMagic
        || version != kSaveVersion) {
        return false;
    }

    auto read_int = [&](int32_t *value) {
        uint32_t raw = 0u;
        if (!ReadU32(buffer, size, &offset, &raw)) {
            return false;
        }
        *value = static_cast<int32_t>(raw);
        return true;
    };

    int32_t day = 0;
    int32_t cash = 0;
    int32_t debt = 0;
    int32_t beer = 0;
    int32_t clean = 0;
    int32_t dirty = 0;
    int32_t reputation = 0;
    int32_t pressure = 0;
    int32_t event = 0;
    int32_t day_open = 0;
    int32_t protection_paid = 0;
    int32_t opening_cash = 0;
    int32_t sales = 0;
    int32_t supply_spend = 0;
    int32_t protection_spend = 0;
    int32_t inspection_fines = 0;
    int32_t served = 0;
    int32_t lost = 0;
    int32_t reputation_delta = 0;
    int32_t pressure_delta = 0;
    int32_t next_patron = 0;
    DayLedger ledger = {};

    if (!read_int(&day)
        || !read_int(&cash)
        || !read_int(&debt)
        || !read_int(&beer)
        || !read_int(&clean)
        || !read_int(&dirty)
        || !read_int(&reputation)
        || !read_int(&pressure)
        || !read_int(&event)
        || !read_int(&day_open)
        || !read_int(&protection_paid)
        || !read_int(&opening_cash)
        || !read_int(&sales)
        || !read_int(&supply_spend)
        || !read_int(&protection_spend)
        || !read_int(&inspection_fines)
        || !read_int(&served)
        || !read_int(&lost)
        || !read_int(&reputation_delta)
        || !read_int(&pressure_delta)
        || !read_int(&next_patron)
        || !read_int(&ledger.day)
        || !read_int(&ledger.opening_cash_cents)
        || !read_int(&ledger.sales_cents)
        || !read_int(&ledger.supply_spend_cents)
        || !read_int(&ledger.protection_spend_cents)
        || !read_int(&ledger.inspection_fines_cents)
        || !read_int(&ledger.debt_service_cents)
        || !read_int(&ledger.closing_cash_cents)
        || !read_int(&ledger.patrons_served)
        || !read_int(&ledger.patrons_lost)
        || !read_int(&ledger.ending_beer_units)
        || !read_int(&ledger.ending_clean_cups)
        || !read_int(&ledger.ending_dirty_cups)
        || !read_int(&ledger.reputation_delta)
        || !read_int(&ledger.pressure_delta)
        || offset != kSerializedSize) {
        return false;
    }

    if (!InRange(day, 1, 1000000)
        || !InRange(cash, 0, 1000000000)
        || !InRange(debt, 0, 1000000000)
        || !InRange(beer, 0, 100000)
        || !InRange(clean, 0, 100000)
        || !InRange(dirty, 0, 100000)
        || !InRange(reputation, -100000, 100000)
        || !InRange(pressure, 0, 100000)
        || !InRange(event,
            static_cast<int32_t>(EventType::None),
            static_cast<int32_t>(EventType::SupplyInterruption))
        || !InRange(day_open, 0, 1)
        || !InRange(protection_paid, 0, 1)
        || !InRange(opening_cash, 0, 1000000000)
        || !InRange(sales, 0, 1000000000)
        || !InRange(supply_spend, 0, 1000000000)
        || !InRange(protection_spend, 0, 1000000000)
        || !InRange(inspection_fines, 0, 1000000000)
        || !InRange(served, 0, kPatronsPerDay)
        || !InRange(lost, 0, kPatronsPerDay)
        || !InRange(reputation_delta, -100000, 100000)
        || !InRange(pressure_delta, -100000, 100000)
        || !InRange(next_patron, 0, kPatronsPerDay)
        || event != static_cast<int32_t>(EventForDay(day))) {
        return false;
    }

    if (!InRange(ledger.day, 0, day)
        || !InRange(ledger.opening_cash_cents, 0, 1000000000)
        || !InRange(ledger.sales_cents, 0, 1000000000)
        || !InRange(ledger.supply_spend_cents, 0, 1000000000)
        || !InRange(ledger.protection_spend_cents, 0, 1000000000)
        || !InRange(ledger.inspection_fines_cents, 0, 1000000000)
        || !InRange(ledger.debt_service_cents, 0, 1000000000)
        || !InRange(ledger.closing_cash_cents, 0, 1000000000)
        || !InRange(ledger.patrons_served, 0, kPatronsPerDay)
        || !InRange(ledger.patrons_lost, 0, kPatronsPerDay)
        || !InRange(ledger.ending_beer_units, 0, 100000)
        || !InRange(ledger.ending_clean_cups, 0, 100000)
        || !InRange(ledger.ending_dirty_cups, 0, 100000)
        || !InRange(ledger.reputation_delta, -100000, 100000)
        || !InRange(ledger.pressure_delta, -100000, 100000)) {
        return false;
    }

    day_ = day;
    cash_cents_ = cash;
    debt_cents_ = debt;
    beer_units_ = beer;
    clean_cups_ = clean;
    dirty_cups_ = dirty;
    reputation_ = reputation;
    pressure_ = pressure;
    event_ = static_cast<EventType>(event);
    day_open_ = day_open != 0;
    protection_paid_ = protection_paid != 0;
    opening_cash_cents_ = opening_cash;
    sales_cents_ = sales;
    supply_spend_cents_ = supply_spend;
    protection_spend_cents_ = protection_spend;
    inspection_fines_cents_ = inspection_fines;
    served_today_ = served;
    lost_today_ = lost;
    day_reputation_delta_ = reputation_delta;
    day_pressure_delta_ = pressure_delta;
    next_patron_index_ = next_patron;
    last_ledger_ = ledger;
    BuildPatronQueue();
    return true;
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
