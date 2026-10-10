#include "forward_detachment_simulation.h"

namespace reverie { namespace forward {
bool Simulation::BeginPatrol(Route route) {
    if (phase_ != Phase::Briefing ||
        (route != Route::MainRoad && route != Route::RidgeBypass)) return false;
    route_ = route;
    phase_ = Phase::Travelling;
    return true;
}
bool Simulation::ReachContact() {
    if (phase_ != Phase::Travelling) return false;
    // The shorter road has the greater hazard; the ridge costs fuel.
    fuel_remaining_ -= (route_ == Route::MainRoad ? 5 : 8);
    phase_ = Phase::Contact;
    return true;
}
bool Simulation::InspectContact(ContactEvidence *out) const {
    if (phase_ != Phase::Contact || out == nullptr) return false;
    // A disrupted roadside patch on the road versus a harmless civilian
    // group on the ridge. Neither is labelled as hostile or safe.
    *out = route_ == Route::MainRoad
        ? ContactEvidence{true, false, true, false}
        : ContactEvidence{false, true, false, false};
    return true;
}
bool Simulation::ResolveContact(Decision decision) {
    if (phase_ != Phase::Contact ||
        (decision != Decision::HaltAndReport && decision != Decision::Avoid
         && decision != Decision::Engage)) return false;
    decision_ = decision;
    if (route_ == Route::MainRoad) {
        if (decision == Decision::HaltAndReport) information_value_ = 3;
        else if (decision == Decision::Avoid) information_value_ = 1;
        else {
            information_value_ = -3;
            vehicle_condition_ -= 25;
            unnecessary_force_ = true;
        }
    } else {
        if (decision == Decision::Avoid) information_value_ = 2;
        else if (decision == Decision::HaltAndReport) information_value_ = -1;
        else {
            information_value_ = -5;
            vehicle_condition_ -= 35;
            unnecessary_force_ = true;
        }
    }
    phase_ = Phase::Resolved;
    return true;
}
bool Simulation::ReturnToBase() {
    if (phase_ != Phase::Resolved) return false;
    fuel_remaining_ -= (route_ == Route::MainRoad ? 5 : 8);
    phase_ = Phase::Returned;
    return true;
}
bool Simulation::FileDebrief(Debrief *out) {
    if (phase_ != Phase::Returned || out == nullptr) return false;
    *out = {information_value_, vehicle_condition_, fuel_remaining_, unnecessary_force_};
    phase_ = Phase::Debriefed;
    return true;
}
}}  // namespace reverie::forward
