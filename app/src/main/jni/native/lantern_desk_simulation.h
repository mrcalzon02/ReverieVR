#ifndef REVERIE_LANTERN_DESK_SIMULATION_H
#define REVERIE_LANTERN_DESK_SIMULATION_H

#include <array>
#include <cstdint>

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
    Phase phase() const { return phase_; }
    int32_t credibility() const { return credibility_; }
    int32_t operational_effect() const { return operational_effect_; }
private:
    Phase phase_ = Phase::Briefing;
    Sector selected_ = Sector::RailApproach;
    Photograph photo_{};
    int32_t credibility_ = 0;
    int32_t operational_effect_ = 0;
};

}}  // namespace reverie::lantern
#endif
