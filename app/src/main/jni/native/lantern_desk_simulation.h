#ifndef REVERIE_LANTERN_DESK_SIMULATION_H
#define REVERIE_LANTERN_DESK_SIMULATION_H

#include <array>
#include <cstdint>

namespace reverie { namespace recon { class Checkpoint; } }
namespace reverie { namespace lantern {

enum class Sector : uint8_t { RailApproach = 0, Orchard = 1, RiverCrossing = 2 };
enum class Assessment : uint8_t {
    NoContact = 0, PossibleContact = 1, VehicleColumn = 2, BridgeDamage = 3
};
enum class Phase : uint8_t { Briefing = 0, InFlight, PhotoReady, Filed };

// Observation only. No World Truth or correctness field crosses this boundary.
struct Photograph {
    Sector sector = Sector::RailApproach;
    uint8_t clarity = 0;
    std::array<uint16_t, 16> pixels{};
};
struct ReportResult {
    int32_t credibility_delta = 0;
    int32_t operational_effect = 0;
};

class Simulation {
public:
    bool AssignRecon(Sector sector);
    bool CompleteRecon();
    const Photograph *photo() const;
    bool SubmitReport(Assessment assessment, ReportResult *result);
    bool NextSortie();
    const Photograph *evidence_photo() const;
    bool previous_exposure(Photograph *output) const;
    const ReportResult *last_report() const;
    Phase phase() const { return phase_; }
    // Exposure order is capture provenance; it does not reveal the
    // observed unit type, report answer or a classified world-truth flag.
    bool photo_follow_up() const {
        return (phase_==Phase::PhotoReady||phase_==Phase::Filed)&&
            (follow_up_sectors_ &
             static_cast<uint8_t>(1u<<static_cast<unsigned>(selected_)))!=0u;
    }
    int32_t credibility() const { return credibility_; }
    int32_t operational_effect() const { return operational_effect_; }
private:
    friend class reverie::recon::Checkpoint;
    Phase phase_ = Phase::Briefing;
    Sector selected_ = Sector::RailApproach;
    // A filed sortie advances only its own sector when the next sortie opens.
    uint8_t follow_up_sectors_ = 0;
    uint8_t filed_sectors_ = 0;
    Photograph photo_{};
    ReportResult last_report_{};
    int32_t credibility_ = 0;
    int32_t operational_effect_ = 0;
};

}}  // namespace reverie::lantern
#endif
