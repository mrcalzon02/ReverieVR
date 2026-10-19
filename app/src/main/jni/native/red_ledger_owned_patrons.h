#ifndef REVERIE_RED_LEDGER_OWNED_PATRONS_H
#define REVERIE_RED_LEDGER_OWNED_PATRONS_H
// REJECTED: previous character LODs failed visual inspection; none load.
// decimated with UV seam preservation and 64px derived source JPEG palettes.
// These RLBP *static poses* have no skin, no joints and NO animation.
// Compatible with the existing optional patron interaction/hitbox.
#include "red_ledger_simulation.h"
#include <cstddef>
namespace reverie { namespace redledger { namespace owned_patrons {
constexpr size_t kCount=2u;
enum Model : size_t {Gopnick=0u,PinkDress=1u};
constexpr const char *kPaths[kCount]={
    "native/red-ledger/rejected-assets/never-shipped-gopnick.rlprop",
    "native/red-ledger/rejected-assets/never-shipped-pink-dress.rlprop"
};
inline Model ForPatron(PatronKind kind) {
    // Consistent, deterministic variants; never infer actual NPC gender
    // from faction, and never let the choice change the simulation identity.
    switch(kind) {
        case PatronKind::NightClerk:
        case PatronKind::DirectorateCourier:
        case PatronKind::RailWorker:
            return PinkDress;
        default:
            return Gopnick;
    }
}
constexpr float kPatronX=-1.30f;
constexpr float kPatronY=-.47f;
constexpr float kPatronZ=-2.05f;
constexpr int kSourceGopnickTriangles=746654;
constexpr int kSourcePinkTriangles=784444;
constexpr int kDerivedGopnickTriangles=6792;
constexpr int kDerivedPinkTriangles=6918;
static_assert(kCount==2u,"Owner character manifest mismatch");
}}}
#endif
