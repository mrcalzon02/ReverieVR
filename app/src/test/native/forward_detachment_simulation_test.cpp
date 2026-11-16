#include "forward_detachment_simulation.h"
#include <cassert>
#include <iostream>

using namespace reverie::forward;

int main() {
    Simulation main;
    ContactEvidence evidence;
    Debrief report;
    assert(!main.InspectContact(&evidence));
    assert(!main.ReturnToBase());
    assert(!main.BeginPatrol(static_cast<Route>(99)));
    assert(main.BeginPatrol(Route::MainRoad));
    assert(!main.FileDebrief(&report));
    assert(main.ReachContact());
    assert(main.InspectContact(&evidence));
    assert(evidence.disturbed_ground && evidence.vehicle_tracks);
    assert(!evidence.weapon_identified);
    assert(main.ResolveContact(Decision::HaltAndReport));
    assert(!main.ResolveContact(Decision::Engage));
    assert(main.ReturnToBase());
    assert(main.FileDebrief(&report));
    assert(report.information_value == 3);
    assert(report.vehicle_condition == 100 && report.fuel_remaining == 10);
    assert(!report.unnecessary_force && !main.FileDebrief(&report));

    Simulation bypass;
    assert(bypass.BeginPatrol(Route::RidgeBypass));
    assert(!bypass.ReachContact() && !bypass.RecordObservation() && !bypass.WalkRidge(-1));
    for(int i=0;i<Simulation::kRidgeExit;++i){
        assert(bypass.WalkRidge(1));
        if(bypass.ObservationHere())assert(bypass.RecordObservation() && !bypass.RecordObservation());
    }
    assert(!bypass.WalkRidge(1) && bypass.ReachContact());
    assert(bypass.InspectContact(&evidence));
    assert(evidence.civilian_silhouettes && !evidence.weapon_identified);
    assert(bypass.ResolveContact(Decision::Avoid));
    assert(bypass.ReturnToBase() && bypass.FileDebrief(&report));
    assert(report.information_value == 6 && report.fuel_remaining == 4);

    Simulation bad;
    assert(bad.BeginPatrol(Route::RidgeBypass));
    for(int i=0;i<Simulation::kRidgeExit;++i)assert(bad.WalkRidge(1));
    assert(bad.ReachContact());
    assert(bad.ResolveContact(Decision::Engage));
    assert(bad.ReturnToBase() && bad.FileDebrief(&report));
    assert(report.unnecessary_force && report.vehicle_condition == 65);
    assert(report.information_value < 0);
    std::cout << "Forward Detachment patrol-to-debrief cycle passed\n";
}
