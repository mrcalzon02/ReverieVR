#include "lantern_desk_simulation.h"
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace reverie::lantern;


static void TestFollowUpPhotoOwnershipAndOneTimeReport() {
    Simulation mission;
    Photograph earlier;
    ReportResult result{77, 88};
    assert(!mission.NextSortie());
    assert(!mission.previous_exposure(&earlier));
    assert(!mission.SubmitReport(Assessment::VehicleColumn, &result));
    assert(result.credibility_delta == 77 && result.operational_effect == 88);

    assert(mission.AssignRecon(Sector::RailApproach));
    assert(!mission.NextSortie()); // A sortie cannot advance before its report.
    assert(mission.CompleteRecon());
    const Photograph rail_first = *mission.photo();
    assert(!mission.previous_exposure(&earlier));
    assert(!mission.SubmitReport(static_cast<Assessment>(255), &result));
    assert(!mission.SubmitReport(Assessment::VehicleColumn, nullptr));
    assert(mission.photo()->pixels == rail_first.pixels);
    assert(mission.credibility() == 0 && mission.operational_effect() == 0);
    assert(mission.SubmitReport(Assessment::VehicleColumn, &result));
    assert(result.credibility_delta == 3 && result.operational_effect == 2);
    assert(!mission.SubmitReport(Assessment::VehicleColumn, &result));
    assert(mission.credibility() == 3 && mission.operational_effect() == 2);
    assert(!mission.previous_exposure(&earlier)); // Not until the next sortie.
    assert(mission.NextSortie());
    assert(!mission.NextSortie());
    assert(!mission.previous_exposure(&earlier));

    // Filing Rail must not unlock Orchard's later exposure or comparison.
    assert(mission.AssignRecon(Sector::Orchard));
    assert(mission.CompleteRecon());
    const Photograph orchard_first = *mission.photo();
    assert(!mission.previous_exposure(&earlier));
    assert(mission.SubmitReport(Assessment::NoContact, &result));
    assert(result.credibility_delta == 2 && result.operational_effect == 1);
    assert(mission.NextSortie());

    assert(mission.AssignRecon(Sector::RailApproach));
    assert(mission.CompleteRecon());
    const Photograph rail_followup = *mission.photo();
    assert(rail_followup.sector == Sector::RailApproach);
    assert(rail_followup.clarity == rail_first.clarity);
    assert(rail_followup.pixels != rail_first.pixels);
    assert(mission.previous_exposure(&earlier));
    assert(earlier.sector == rail_first.sector);
    assert(earlier.clarity == rail_first.clarity);
    assert(earlier.pixels == rail_first.pixels);
    assert(mission.SubmitReport(Assessment::NoContact, &result));
    assert(result.credibility_delta == 2 && result.operational_effect == 1);
    assert(!mission.SubmitReport(Assessment::NoContact, &result));
    assert(mission.credibility() == 7 && mission.operational_effect() == 4);
    assert(mission.last_report() && mission.last_report()->credibility_delta == 2);
    assert(mission.NextSortie());

    // Orchard gets its own follow-up only after its own filed report.
    assert(mission.AssignRecon(Sector::Orchard));
    assert(mission.CompleteRecon());
    const Photograph orchard_followup = *mission.photo();
    assert(orchard_followup.sector == Sector::Orchard);
    assert(orchard_followup.pixels != orchard_first.pixels);
    assert(mission.previous_exposure(&earlier));
    assert(earlier.pixels == orchard_first.pixels);
    assert(mission.SubmitReport(Assessment::VehicleColumn, &result));
    assert(result.credibility_delta == 3 && result.operational_effect == 2);
    assert(!mission.SubmitReport(Assessment::VehicleColumn, &result));
    assert(mission.credibility() == 10 && mission.operational_effect() == 6);
    assert(mission.NextSortie());

    // A later Rail revisit alternates back without borrowing Orchard's state.
    assert(mission.AssignRecon(Sector::RailApproach));
    assert(mission.CompleteRecon());
    assert(mission.photo()->pixels == rail_first.pixels);
    assert(mission.previous_exposure(&earlier));
    assert(earlier.pixels == rail_followup.pixels);
    std::cout << "PASS: Lantern Desk sector-scoped follow-up photos and one-time report accounting\\n";
}

int main() {
    TestFollowUpPhotoOwnershipAndOneTimeReport();
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
