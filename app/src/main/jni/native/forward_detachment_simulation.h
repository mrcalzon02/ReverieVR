#ifndef REVERIE_FORWARD_DETACHMENT_SIMULATION_H
#define REVERIE_FORWARD_DETACHMENT_SIMULATION_H

#include <cstdint>

namespace reverie { namespace forward {
enum class Route : uint8_t { MainRoad = 0, RidgeBypass = 1 };
enum class Phase : uint8_t { Briefing = 0, Travelling, Contact, Resolved, Returned, Debriefed };
enum class Decision : uint8_t { HaltAndReport = 0, Avoid = 1, Engage = 2 };

// Observable clues, not an authoritative contact classification.
struct ContactEvidence {
    bool disturbed_ground = false;
    bool civilian_silhouettes = false;
    bool vehicle_tracks = false;
    bool weapon_identified = false;
};
struct Debrief {
    int32_t information_value = 0;
    int32_t vehicle_condition = 100;
    int32_t fuel_remaining = 20;
    bool unnecessary_force = false;
};
class Simulation {
public:
    bool BeginPatrol(Route route);
    bool ReachContact();
    bool InspectContact(ContactEvidence *out) const;
    bool ResolveContact(Decision decision);
    bool ReturnToBase();
    bool FileDebrief(Debrief *out);
    Phase phase() const { return phase_; }
    int32_t vehicle_condition() const { return vehicle_condition_; }
    int32_t fuel_remaining() const { return fuel_remaining_; }
private:
    Phase phase_ = Phase::Briefing;
    Route route_ = Route::MainRoad;
    Decision decision_ = Decision::Avoid;
    int32_t vehicle_condition_ = 100;
    int32_t fuel_remaining_ = 20;
    int32_t information_value_ = 0;
    bool unnecessary_force_ = false;
};
}}  // namespace reverie::forward
#endif
