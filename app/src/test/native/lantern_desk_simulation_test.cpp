#include "lantern_desk_simulation.h"
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace reverie::lantern;

int main() {
    Simulation rail;
    assert(rail.phase() == Phase::Briefing && rail.photo() == nullptr);
    assert(!rail.CompleteRecon());
    assert(!rail.AssignRecon(static_cast<Sector>(99)));
    assert(rail.AssignRecon(Sector::RailApproach));
    assert(!rail.AssignRecon(Sector::Orchard));
    assert(rail.photo() == nullptr);
    assert(rail.CompleteRecon());
    const Photograph *image = rail.photo();
    assert(image && image->clarity > 0 && image->sector == Sector::RailApproach);
    int occupied = 0;
    for (uint16_t row : image->pixels) if (row) ++occupied;
    assert(occupied >= 5);
    ReportResult result;
    assert(rail.SubmitReport(Assessment::VehicleColumn, &result));
    assert(result.credibility_delta == 3 && result.operational_effect == 2);
    assert(rail.credibility() == 3 && rail.operational_effect() == 2);
    assert(rail.photo() == nullptr && !rail.SubmitReport(Assessment::NoContact, &result));

    Simulation orchard;
    assert(orchard.AssignRecon(Sector::Orchard));
    assert(orchard.CompleteRecon());
    assert(orchard.photo()->pixels != image->pixels);
    assert(orchard.SubmitReport(Assessment::VehicleColumn, &result));
    assert(result.credibility_delta < 0 && result.operational_effect < 0);

    Simulation benign;
    assert(benign.AssignRecon(Sector::Orchard) && benign.CompleteRecon());
    assert(benign.SubmitReport(Assessment::NoContact, &result));
    assert(result.credibility_delta > 0);

    Simulation bridge;
    assert(bridge.AssignRecon(Sector::RiverCrossing) && bridge.CompleteRecon());
    assert(bridge.SubmitReport(Assessment::BridgeDamage, &result));
    assert(result.operational_effect > 0);
    std::cout << "Lantern Desk evidence-to-report cycle passed\n";
}
