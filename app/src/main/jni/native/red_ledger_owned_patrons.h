#ifndef REVERIE_RED_LEDGER_OWNED_PATRONS_H
#define REVERIE_RED_LEDGER_OWNED_PATRONS_H
// Six game-specific PSX patron skins. The mesh/UV atlas binaries are curated
// privately from the author's model library; never publish raw third-party
// assets to this source repository. A missing optional skin uses the existing
// fallback without changing the customer's simulation identity or hitbox.
// RLBP is a static evaluated pose; full skeletal animation is a separate task.
#include "red_ledger_simulation.h"
#include <cstddef>

namespace reverie { namespace redledger { namespace owned_patrons {
constexpr size_t kCount=6u;
enum Model : size_t {
    RailWorker=0u, NightClerk=1u, MilitiaVeteran=2u,
    Mechanic=3u, DirectorateCourier=4u, SalihorskHauler=5u
};
constexpr const char *kPaths[kCount]={
    "native/red-ledger/beckenburg/patron_rail_worker.rlprop",
    "native/red-ledger/beckenburg/patron_night_clerk.rlprop",
    "native/red-ledger/beckenburg/patron_militia_veteran.rlprop",
    "native/red-ledger/beckenburg/patron_mechanic.rlprop",
    "native/red-ledger/beckenburg/patron_directorate_courier.rlprop",
    "native/red-ledger/beckenburg/patron_salihorsk_hauler.rlprop"
};
constexpr Model ForPatron(PatronKind kind) {
    switch(kind) {
        case PatronKind::RailWorker: return RailWorker;
        case PatronKind::NightClerk: return NightClerk;
        case PatronKind::MilitiaVeteran: return MilitiaVeteran;
        case PatronKind::Mechanic: return Mechanic;
        case PatronKind::DirectorateCourier: return DirectorateCourier;
        case PatronKind::SalihorskHauler: return SalihorskHauler;
    }
    return RailWorker;
}
constexpr float kPatronX=-1.30f;
constexpr float kPatronY=-0.47f;
constexpr float kPatronZ=-2.05f;
static_assert(kCount==6u,"A PSX variant must exist for each patron archetype");
}}}
#endif
