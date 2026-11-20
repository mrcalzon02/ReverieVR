#include "lantern_desk_simulation.h"
#include "recon_projects_save.h"
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

// A rejected report is not a filed report: the original photographic
// evidence and its uncredited assessment must survive a checked host reload.
static void TestRejectedReportCheckpointReload() {
    using reverie::recon::Checkpoint;
    using reverie::recon::LanternState;
    LanternState uninterrupted;
    auto& mission = uninterrupted.simulation;
    assert(mission.AssignRecon(Sector::RailApproach));
    assert(mission.CompleteRecon());
    const Photograph original = *mission.photo();
    assert(original.sector == Sector::RailApproach && original.clarity > 0);

    Checkpoint::Bytes banked{};
    assert(Checkpoint::Encode(uninterrupted, &banked));
    ReportResult refused{901, 902};
    assert(!mission.SubmitReport(static_cast<Assessment>(255), &refused));
    assert(!mission.SubmitReport(Assessment::VehicleColumn, nullptr));
    assert(refused.credibility_delta == 901 && refused.operational_effect == 902);
    assert(mission.phase() == Phase::PhotoReady);
    assert(mission.credibility() == 0 && mission.operational_effect() == 0);
    assert(mission.photo() && mission.photo()->sector == original.sector);
    assert(mission.photo()->clarity == original.clarity);
    assert(mission.photo()->pixels == original.pixels);
    Checkpoint::Bytes after_refusal{};
    assert(Checkpoint::Encode(uninterrupted, &after_refusal));
    assert(after_refusal == banked);

    LanternState restored;
    assert(Checkpoint::Decode(&restored, banked.data(), banked.size()));
    Checkpoint::Bytes restored_bytes{};
    assert(Checkpoint::Encode(restored, &restored_bytes));
    assert(restored_bytes == banked);
    const Photograph* pending = restored.simulation.evidence_photo();
    assert(pending && pending->sector == original.sector);
    assert(pending->clarity == original.clarity && pending->pixels == original.pixels);
    assert(restored.simulation.phase() == Phase::PhotoReady);
    assert(restored.simulation.credibility() == 0);
    assert(restored.simulation.operational_effect() == 0);
    assert(!restored.simulation.SubmitReport(static_cast<Assessment>(255), &refused));
    assert(refused.credibility_delta == 901 && refused.operational_effect == 902);
    assert(Checkpoint::Encode(restored, &restored_bytes) && restored_bytes == banked);

    ReportResult uninterrupted_result{}, restored_result{};
    assert(mission.SubmitReport(Assessment::VehicleColumn, &uninterrupted_result));
    assert(restored.simulation.SubmitReport(Assessment::VehicleColumn, &restored_result));
    assert(uninterrupted_result.credibility_delta == 3 &&
           uninterrupted_result.operational_effect == 2);
    assert(restored_result.credibility_delta == uninterrupted_result.credibility_delta &&
           restored_result.operational_effect == uninterrupted_result.operational_effect);
    assert(mission.credibility() == 3 && mission.operational_effect() == 2);
    assert(restored.simulation.credibility() == 3 &&
           restored.simulation.operational_effect() == 2);
    assert(mission.evidence_photo() && mission.evidence_photo()->pixels == original.pixels);
    assert(restored.simulation.evidence_photo() &&
           restored.simulation.evidence_photo()->pixels == original.pixels);
    assert(!restored.simulation.SubmitReport(Assessment::VehicleColumn, &refused));
    assert(refused.credibility_delta == 901 && refused.operational_effect == 902);
    Checkpoint::Bytes filed_original{}, filed_restored{};
    assert(Checkpoint::Encode(uninterrupted, &filed_original));
    assert(Checkpoint::Encode(restored, &filed_restored));
    assert(filed_original == filed_restored);
    assert(mission.NextSortie() && restored.simulation.NextSortie());
    assert(!mission.NextSortie() && !restored.simulation.NextSortie());
    assert(!mission.evidence_photo() && !restored.simulation.evidence_photo());
    assert(Checkpoint::Encode(uninterrupted, &filed_original));
    assert(Checkpoint::Encode(restored, &filed_restored));
    assert(filed_original == filed_restored);
    std::cout << "PASS Lantern Desk rejected report checked reload retains unfiled evidence and one valid credit\n";
}

int main() {
    TestRejectedReportCheckpointReload();
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
