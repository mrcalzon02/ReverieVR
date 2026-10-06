#ifndef REVERIE_RED_LEDGER_SIMULATION_H
#define REVERIE_RED_LEDGER_SIMULATION_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace reverie {
namespace redledger {

enum class EventType : uint8_t {
    None = 0,
    ProtectionDemand,
    Inspection,
    SupplyInterruption
};

enum class PatronKind : uint8_t {
    RailWorker = 0,
    NightClerk,
    MilitiaVeteran,
    Mechanic
};

enum class SupplierItem : uint8_t {
    BeerCrate = 0,
    CupSet
};

enum class DrinkState : uint8_t {
    None = 0,
    EmptyCup,
    FilledCup
};

struct PatronDefinition {
    PatronKind kind;
    const char *name;
    int32_t drink_price_cents;
    int32_t reputation_on_service;
};

struct SupplierOffer {
    SupplierItem item;
    const char *name;
    int32_t cost_cents;
    int32_t beer_units;
    int32_t clean_cups;
};

struct DayLedger {
    int32_t day = 0;
    int32_t opening_cash_cents = 0;
    int32_t sales_cents = 0;
    int32_t supply_spend_cents = 0;
    int32_t protection_spend_cents = 0;
    int32_t inspection_fines_cents = 0;
    int32_t debt_service_cents = 0;
    int32_t closing_cash_cents = 0;
    int32_t patrons_served = 0;
    int32_t patrons_lost = 0;
    int32_t ending_beer_units = 0;
    int32_t ending_clean_cups = 0;
    int32_t ending_dirty_cups = 0;
    int32_t reputation_delta = 0;
    int32_t pressure_delta = 0;
};

class Simulation {
public:
    static constexpr int32_t kDailyDebtServiceCents = 150;
    static constexpr int32_t kProtectionCostCents = 600;
    static constexpr int32_t kInspectionFineCents = 450;
    static constexpr size_t kSerializedSize = 160u;

    Simulation();

    void Reset();

    int32_t day() const;
    int32_t cash_cents() const;
    int32_t debt_cents() const;
    int32_t beer_units() const;
    int32_t clean_cups() const;
    int32_t dirty_cups() const;
    int32_t reputation() const;
    int32_t pressure() const;
    int32_t patrons_remaining() const;
    int32_t pending_payment_cents() const;
    bool day_open() const;
    EventType current_event() const;
    DrinkState work_drink_state() const;
    bool protection_paid() const;

    const PatronDefinition *current_patron() const;
    const DayLedger &last_ledger() const;

    bool Serialize(
        uint8_t *buffer,
        size_t capacity,
        size_t *out_size
    ) const;
    bool Deserialize(
        const uint8_t *buffer,
        size_t size
    );

    bool TakeCleanCup();
    bool ReturnHeldCup();
    bool FillHeldCup();
    bool ServeHeldCup();
    bool CollectPayment();
    bool TurnAwayCurrentPatron();
    bool WashOneCup();
    bool BuySupply(SupplierItem item);
    bool PayProtection();
    DayLedger CloseDay();

    static const std::array<PatronDefinition, 4> &PatronCatalog();
    static const std::array<SupplierOffer, 2> &SupplierCatalog();

private:
    static constexpr int32_t kPatronsPerDay = 4;

    void OpenDay();
    void BuildPatronQueue();
    const SupplierOffer *FindOffer(SupplierItem item) const;

    int32_t day_ = 1;
    int32_t cash_cents_ = 0;
    int32_t debt_cents_ = 0;
    int32_t beer_units_ = 0;
    int32_t clean_cups_ = 0;
    int32_t dirty_cups_ = 0;
    int32_t reputation_ = 0;
    int32_t pressure_ = 0;
    int32_t pending_payment_cents_ = 0;

    EventType event_ = EventType::None;
    DrinkState work_drink_state_ = DrinkState::None;
    bool day_open_ = false;
    bool protection_paid_ = false;

    int32_t opening_cash_cents_ = 0;
    int32_t sales_cents_ = 0;
    int32_t supply_spend_cents_ = 0;
    int32_t protection_spend_cents_ = 0;
    int32_t inspection_fines_cents_ = 0;
    int32_t served_today_ = 0;
    int32_t lost_today_ = 0;
    int32_t day_reputation_delta_ = 0;
    int32_t day_pressure_delta_ = 0;

    std::array<PatronKind, kPatronsPerDay> patron_queue_{};
    int32_t next_patron_index_ = 0;
    DayLedger last_ledger_{};
};

}  // namespace redledger
}  // namespace reverie

#endif
