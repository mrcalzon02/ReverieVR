#include "forward_detachment_simulation.h"
#include <algorithm>

namespace reverie { namespace forward {
int32_t Simulation::RoundTripFuel(Route route) {
    return route==Route::MainRoad?10:route==Route::RidgeBypass?16:0;
}
bool Simulation::CanBeginPatrol(Route route) const {
    const int32_t cost=RoundTripFuel(route);
    return phase_==Phase::Briefing&&cost>0&&vehicle_condition_>0&&fuel_remaining_>=cost;
}
bool Simulation::BeginPatrol(Route route) {
    if(!CanBeginPatrol(route))return false;
    route_ = route;
    phase_ = Phase::Travelling;
    return true;
}
bool Simulation::WalkRidge(int direction) {
    if(!OnRidgeTrail() || (direction!=-1 && direction!=1))return false;
    const int next=trail_position_+direction;
    if(next<0 || next>kRidgeExit)return false;
    trail_position_=static_cast<uint8_t>(next);
    return true;
}
uint8_t Simulation::ObservationHere() const {
    if(!OnRidgeTrail())return 0;
    return trail_position_==2?kGaugeNote:trail_position_==5?kShelterNote:0;
}
bool Simulation::RecordObservation(){
    const uint8_t note=ObservationHere();
    if(!note || (trail_notes_&note))return false;
    trail_notes_|=note;
    return true;
}
bool Simulation::ReachContact() {
    if(OnRidgeTrail() && trail_position_!=kRidgeExit)return false;
    if (phase_ != Phase::Travelling || fuel_remaining_<RoundTripFuel(route_)) return false;
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
int Simulation::ReportValue(Route route,Decision decision,uint8_t notes){
    if(route==Route::MainRoad)return decision==Decision::HaltAndReport?3:
        decision==Decision::Avoid?1:-3;
    if(decision==Decision::Engage)return -5;
    const int surveyed=((notes&kGaugeNote)?1:0)+((notes&kShelterNote)?1:0);
    return (decision==Decision::Avoid?2:-1)+2*surveyed;
}
bool Simulation::ResolveContact(Decision decision) {
    if (phase_ != Phase::Contact ||
        (decision != Decision::HaltAndReport && decision != Decision::Avoid
         && decision != Decision::Engage)) return false;
    decision_ = decision;
    information_value_=ReportValue(route_,decision,trail_notes_);
    if(decision==Decision::Engage){
        vehicle_condition_=std::max(0,vehicle_condition_-(route_==Route::MainRoad?25:35));
        unnecessary_force_=true;
    }
    phase_ = Phase::Resolved;
    return true;
}
bool Simulation::ReturnToBase() {
    if (phase_ != Phase::Resolved || fuel_remaining_<RoundTripFuel(route_)/2) return false;
    fuel_remaining_ -= (route_ == Route::MainRoad ? 5 : 8);
    phase_ = Phase::Returned;
    return true;
}
bool Simulation::FileDebrief(Debrief *out) {
    if (phase_ != Phase::Returned || out == nullptr) return false;
    *out = {information_value_, vehicle_condition_, fuel_remaining_, unnecessary_force_};
    route_intel_[static_cast<unsigned>(route_)] = information_value_;
    if(route_==Route::RidgeBypass)filed_ridge_notes_=trail_notes_;
    phase_ = Phase::Debriefed;
    return true;
}
bool Simulation::NextPatrol() {
    if(phase_!=Phase::Debriefed)return false;
    phase_=Phase::Briefing;route_=Route::MainRoad;decision_=Decision::Avoid;
    trail_position_=trail_notes_=0;
    information_value_=0;unnecessary_force_=false;
    return true;
}
bool Simulation::RefitAtBase() {
    if(phase_!=Phase::Briefing)return false;
    vehicle_condition_=100;fuel_remaining_=20;
    return true;
}
int32_t Simulation::LastRouteIntel(Route route) const {
    const auto index = static_cast<unsigned>(route);
    return index < route_intel_.size() ? route_intel_[index] : 0;
}
bool Simulation::LastDebrief(Debrief *out) const {
    if(phase_!=Phase::Debriefed||!out)return false;
    *out={information_value_,vehicle_condition_,fuel_remaining_,unnecessary_force_};
    return true;
}
}}  // namespace reverie::forward
