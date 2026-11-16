#include "lantern_desk_simulation.h"

#include <cstddef>
#include <limits>

namespace reverie { namespace lantern {
namespace {
void Pixel(Photograph *p, int x, int y) {
    if (x >= 0 && x < 16 && y >= 0 && y < 16)
        p->pixels[static_cast<size_t>(y)] |= static_cast<uint16_t>(1u << x);
}
void Line(Photograph *p, int x0, int x1, int y) {
    for (int x = x0; x <= x1; ++x) Pixel(p, x, y);
}
uint8_t SectorBit(Sector sector) {
    return static_cast<uint8_t>(1u << static_cast<unsigned>(sector));
}
Assessment ObservedCase(Sector sector, bool follow_up) {
    switch (sector) {
    case Sector::RailApproach: return follow_up ? Assessment::NoContact : Assessment::VehicleColumn;
    case Sector::Orchard: return follow_up ? Assessment::VehicleColumn : Assessment::NoContact;
    case Sector::RiverCrossing: return follow_up ? Assessment::NoContact : Assessment::BridgeDamage;
    }
    return Assessment::NoContact;
}
void VehiclePair(Photograph* p) {
    Line(p, 3, 8, 6); Line(p, 4, 7, 5);
    Line(p, 10, 13, 8); Line(p, 11, 12, 7);
    Pixel(p, 4, 7); Pixel(p, 7, 7);
    Pixel(p, 10, 9); Pixel(p, 13, 9);
}
void MakePhotograph(Sector sector, bool follow_up, Photograph *p) {
    p->sector = sector;
    p->pixels.fill(0u);
    // A low-resolution, reproducible camera capture. The silhouettes are
    // evidence, never a hidden truth label supplied to the analyst.
    const auto observed = ObservedCase(sector, follow_up);
    if (sector == Sector::RailApproach) {
        p->clarity = 94;
        Line(p, 1, 14, 12); Line(p, 1, 14, 13);
    } else if (sector == Sector::Orchard) {
        p->clarity = 76;
        Pixel(p, 4, 5); Pixel(p, 5, 4); Pixel(p, 6, 5);
        Pixel(p, 11, 3); Pixel(p, 12, 4);
        Line(p, 5, 5, 8); Line(p, 11, 11, 8);
        // A small elevated civilian structure is a plausible false contact.
        if (observed == Assessment::NoContact) {
            Line(p, 7, 10, 6); Line(p, 7, 10, 7);
            Pixel(p, 7, 8); Pixel(p, 10, 8);
        }
    } else {
        p->clarity = 81;
        Line(p, 1, 5, 9); Line(p, 10, 14, 9);
        Line(p, 1, 5, 10); Line(p, 10, 14, 10);
        Line(p, 0, 15, 2); Line(p, 0, 15, 14);
        if (observed == Assessment::NoContact) {
            Line(p, 6, 9, 9); Line(p, 6, 9, 10);
        }
    }
    if (observed == Assessment::VehicleColumn) VehiclePair(p);
    // Deterministic sensor grain; no random device-dependent state.
    uint32_t seed = 0x63a9f221u + static_cast<uint32_t>(sector) * 173u;
    for (int i = 0; i < 13; ++i) {
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        Pixel(p, static_cast<int>(seed & 15u),
              static_cast<int>((seed >> 8) & 15u));
    }
}
bool KnownSector(Sector sector) {
    return sector == Sector::RailApproach || sector == Sector::Orchard
        || sector == Sector::RiverCrossing;
}
}  // namespace

bool Simulation::AssignRecon(Sector sector) {
    if (phase_ != Phase::Briefing || !KnownSector(sector)) return false;
    selected_ = sector;
    phase_ = Phase::InFlight;
    return true;
}
bool Simulation::CompleteRecon() {
    if (phase_ != Phase::InFlight) return false;
    MakePhotograph(selected_, (follow_up_sectors_ & SectorBit(selected_)) != 0, &photo_);
    phase_ = Phase::PhotoReady;
    return true;
}
const Photograph *Simulation::photo() const {
    return phase_ == Phase::PhotoReady ? &photo_ : nullptr;
}
bool Simulation::SubmitReport(Assessment assessment, ReportResult *result) {
    if (phase_ != Phase::PhotoReady || result == nullptr
        || static_cast<unsigned>(assessment)>static_cast<unsigned>(Assessment::BridgeDamage)) return false;
    int32_t trust = -3;
    int32_t effect = -2;
    const auto observed = ObservedCase(selected_, (follow_up_sectors_ & SectorBit(selected_)) != 0);
    if (assessment == observed) {
        trust = observed == Assessment::NoContact ? 2 : 3;
        effect = observed == Assessment::NoContact ? 1 : 2;
    } else if (assessment == Assessment::PossibleContact) {
        trust = observed == Assessment::NoContact ? -1 : 1;
        effect = observed == Assessment::VehicleColumn ? 1 : 0;
    }
    const auto accumulate=[](int32_t total,int32_t delta){
        const int64_t next=static_cast<int64_t>(total)+delta;
        if(next>std::numeric_limits<int32_t>::max())return std::numeric_limits<int32_t>::max();
        if(next<std::numeric_limits<int32_t>::min())return std::numeric_limits<int32_t>::min();
        return static_cast<int32_t>(next);
    };
    credibility_ = accumulate(credibility_,trust);
    operational_effect_ = accumulate(operational_effect_,effect);
    last_report_ = {trust, effect};
    *result = last_report_;
    phase_ = Phase::Filed;
    return true;
}
bool Simulation::NextSortie() {
    if(phase_!=Phase::Filed)return false;
    follow_up_sectors_ ^= SectorBit(selected_);
    phase_=Phase::Briefing;selected_=Sector::RailApproach;
    photo_=Photograph{};last_report_=ReportResult{};
    return true;
}
const Photograph *Simulation::evidence_photo() const {
    return phase_==Phase::PhotoReady||phase_==Phase::Filed?&photo_:nullptr;
}
const ReportResult *Simulation::last_report() const {
    return phase_==Phase::Filed?&last_report_:nullptr;
}
}}  // namespace reverie::lantern
