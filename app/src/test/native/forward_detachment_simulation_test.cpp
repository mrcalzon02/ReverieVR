#include "forward_detachment_simulation.h"
#include <cassert>
#include <iostream>

using namespace reverie::forward;

// A hazardous patrol spends fuel and may damage the vehicle. None of that
// expenditure is refundable mid-mission or by replaying a completed debrief.
static void TestRiskyPatrolRefitAndDebriefGuards() {
    for (int pass = 0; pass < 2; ++pass) {
        const Route route = pass == 0 ? Route::MainRoad : Route::RidgeBypass;
        const int expected_condition = pass == 0 ? 75 : 65;
        const int expected_fuel = pass == 0 ? 10 : 4;
        const int expected_intel = pass == 0 ? -3 : -5;
        Simulation patrol;
        Debrief report;
        assert(patrol.BeginPatrol(route));
        assert(patrol.phase() == Phase::Travelling);
        assert(!patrol.RefitAtBase() && patrol.fuel_remaining() == 20);
        assert(!patrol.FileDebrief(&report) && !patrol.NextPatrol());

        if (route == Route::RidgeBypass) {
            for (int step = 0; step < Simulation::kRidgeExit; ++step)
                assert(patrol.WalkRidge(1));
        }
        assert(patrol.ReachContact());
        assert(patrol.fuel_remaining() == (pass == 0 ? 15 : 12));
        assert(!patrol.RefitAtBase());
        assert(patrol.ResolveContact(Decision::Engage));
        assert(patrol.vehicle_condition() == expected_condition);
        assert(!patrol.ResolveContact(Decision::Engage));
        assert(!patrol.RefitAtBase() && !patrol.FileDebrief(&report));
        assert(patrol.ReturnToBase());
        assert(patrol.fuel_remaining() == expected_fuel);
        assert(!patrol.ReturnToBase() && !patrol.RefitAtBase());
        assert(!patrol.NextPatrol());
        assert(patrol.vehicle_condition() == expected_condition);

        assert(patrol.FileDebrief(&report));
        assert(report.information_value == expected_intel);
        assert(report.vehicle_condition == expected_condition);
        assert(report.fuel_remaining == expected_fuel && report.unnecessary_force);
        assert(patrol.LastRouteIntel(route) == expected_intel);
        assert(!patrol.RefitAtBase());
        Debrief rejected{777, 777, 777, false};
        assert(!patrol.FileDebrief(&rejected));
        assert(rejected.information_value == 777 &&
               rejected.vehicle_condition == 777 && rejected.fuel_remaining == 777);
        assert(!patrol.ResolveContact(Decision::Engage) && !patrol.ReturnToBase());
        assert(patrol.LastRouteIntel(route) == expected_intel);

        assert(patrol.NextPatrol());
        assert(patrol.phase() == Phase::Briefing);
        assert(!patrol.NextPatrol() && !patrol.FileDebrief(&rejected));
        assert(patrol.fuel_remaining() == expected_fuel);
        assert(patrol.vehicle_condition() == expected_condition);
        assert(patrol.LastRouteIntel(route) == expected_intel);
        assert(!patrol.CanBeginPatrol(Route::RidgeBypass));
        if (pass == 1) assert(!patrol.CanBeginPatrol(Route::MainRoad));
        // Only an explicit refit at the next legitimate briefing restores resources.
        assert(patrol.RefitAtBase());
        assert(patrol.fuel_remaining() == 20 && patrol.vehicle_condition() == 100);
        assert(patrol.LastRouteIntel(route) == expected_intel);
        assert(patrol.CanBeginPatrol(route));
    }
    std::cout << "PASS: Iron Sight no in-patrol refit or duplicate debrief\\n";
}

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
    TestRiskyPatrolRefitAndDebriefGuards();
    std::cout << "Forward Detachment patrol-to-debrief cycle passed\n";
}
