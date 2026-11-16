#include "recon_projects_save.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
using reverie::recon::Checkpoint;
using reverie::recon::LanternState;
using reverie::recon::ForwardState;
namespace lantern=reverie::lantern;
namespace forward=reverie::forward;
template<class T>static Checkpoint::Bytes Bytes(const T& state){
    Checkpoint::Bytes bytes{};assert(Checkpoint::Encode(state,&bytes));return bytes;
}
template<class T>static T Restore(const T& state){
    const auto bytes=Bytes(state);T restored;
    assert(Checkpoint::Decode(&restored,bytes.data(),bytes.size()));assert(Bytes(restored)==bytes);return restored;
}
static void Rehash(Checkpoint::Bytes& bytes){
    uint64_t hash=14695981039346656037ull;
    for(size_t i=0;i<bytes.size()-8;++i)hash=(hash^bytes[i])*1099511628211ull;
    for(size_t i=bytes.size()-8;i<bytes.size();++i){bytes[i]=static_cast<uint8_t>(hash);hash>>=8;}
}
int main(){
    LanternState ministry;
    assert(!ministry.simulation.NextSortie());
    for(unsigned sector=0;sector<3;++sector)for(unsigned assessment=0;assessment<4;++assessment){
        ministry.choice=static_cast<uint8_t>(sector);ministry=Restore(ministry);
        assert(ministry.simulation.AssignRecon(static_cast<lantern::Sector>(sector)));
        ministry.choice=0;ministry=Restore(ministry);
        assert(ministry.simulation.CompleteRecon());ministry.choice=static_cast<uint8_t>(assessment);
        auto resumed=Restore(ministry);assert(resumed.simulation.evidence_photo()->pixels==ministry.simulation.photo()->pixels);
        lantern::ReportResult originalResult,restoredResult;
        assert(ministry.simulation.SubmitReport(static_cast<lantern::Assessment>(assessment),&originalResult));
        assert(resumed.simulation.SubmitReport(static_cast<lantern::Assessment>(assessment),&restoredResult));
        ministry.choice=resumed.choice=0;
        assert(Bytes(resumed)==Bytes(ministry));ministry=Restore(ministry);
        assert(!ministry.simulation.SubmitReport(lantern::Assessment::NoContact,&restoredResult));
        const auto trust=ministry.simulation.credibility(),effect=ministry.simulation.operational_effect();
        assert(ministry.simulation.NextSortie());assert(!ministry.simulation.NextSortie());
        assert(ministry.simulation.credibility()==trust&&ministry.simulation.operational_effect()==effect);
        assert(!ministry.simulation.evidence_photo()&&!ministry.simulation.last_report());
        ministry=Restore(ministry);
    }
    // Visit all sectors once before returning to each. Only completed sorties
    // advance their sector; saving the photo or reopening Filed never advances it.
    LanternState tour;
    const lantern::Assessment first[]={lantern::Assessment::VehicleColumn,lantern::Assessment::NoContact,lantern::Assessment::BridgeDamage};
    const lantern::Assessment second[]={lantern::Assessment::NoContact,lantern::Assessment::VehicleColumn,lantern::Assessment::NoContact};
    std::array<lantern::Photograph,3> first_images;
    for(unsigned visit=0;visit<3;++visit)for(unsigned i=0;i<3;++i){
        assert(tour.simulation.AssignRecon(static_cast<lantern::Sector>(i)));
        tour=Restore(tour);assert(tour.simulation.CompleteRecon());tour=Restore(tour);
        const auto photo=*tour.simulation.photo();
        if(visit==0)first_images[i]=photo;
        else assert((photo.pixels==first_images[i].pixels)==(visit==2));
        lantern::ReportResult grade;
        if(visit==1){
            auto old_answer=tour;
            assert(old_answer.simulation.SubmitReport(first[i],&grade));
            assert(grade.credibility_delta<0);
        }
        assert(tour.simulation.SubmitReport(visit==1?second[i]:first[i],&grade));
        assert(grade.credibility_delta>0);
        tour=Restore(tour);assert(tour.simulation.NextSortie());tour=Restore(tour);
    }
    LanternState good;
    assert(good.simulation.AssignRecon(lantern::Sector::RailApproach)&&good.simulation.CompleteRecon());
    lantern::ReportResult result;
    assert(!good.simulation.SubmitReport(static_cast<lantern::Assessment>(255),&result));
    assert(good.simulation.SubmitReport(lantern::Assessment::VehicleColumn,&result));
    assert(good.simulation.NextSortie());
    assert(good.simulation.AssignRecon(lantern::Sector::RiverCrossing)&&good.simulation.CompleteRecon());
    assert(good.simulation.SubmitReport(lantern::Assessment::BridgeDamage,&result));
    assert(good.simulation.credibility()==6&&good.simulation.operational_effect()==4);
    good=Restore(good);assert(good.simulation.last_report()->credibility_delta==3);
    const auto lanternBytes=Bytes(good);
    // V1 used the same photographed scenes and a zero reserved tail.
    auto oldPhoto=lanternBytes;oldPhoto[8]=1;oldPhoto[65]=0;Rehash(oldPhoto);
    LanternState migrated;
    assert(Checkpoint::Decode(&migrated,oldPhoto.data(),oldPhoto.size()));
    assert(migrated.simulation.credibility()==6 && migrated.simulation.operational_effect()==4);
    assert(migrated.simulation.evidence_photo()->pixels==good.simulation.evidence_photo()->pixels);
    assert(migrated.simulation.NextSortie() && migrated.simulation.AssignRecon(lantern::Sector::RiverCrossing));
    assert(migrated.simulation.CompleteRecon() && migrated.simulation.photo()->pixels!=good.simulation.evidence_photo()->pixels);
    auto badMask=lanternBytes;badMask[65]=8;Rehash(badMask);
    assert(!Checkpoint::Decode(&migrated,badMask.data(),badMask.size()));
    for(unsigned route=0;route<2;++route)for(unsigned decision=0;decision<3;++decision){
        ForwardState patrol;patrol.choice=static_cast<uint8_t>(route);patrol=Restore(patrol);
        assert(patrol.simulation.BeginPatrol(static_cast<forward::Route>(route)));
        assert(!patrol.simulation.RefitAtBase());patrol.choice=0;patrol=Restore(patrol);
        if(patrol.simulation.OnRidgeTrail()){
            // The v2 travel checkpoint already offered immediate contact.
            auto old=Bytes(patrol);old[8]=2;Rehash(old);
            ForwardState migrated;assert(Checkpoint::Decode(&migrated,old.data(),old.size()));
            assert(migrated.simulation.trail_position()==8 && !migrated.simulation.trail_notes() &&
                   migrated.simulation.ReachContact());
            for(int i=0;i<forward::Simulation::kRidgeExit;++i){
                assert(patrol.simulation.WalkRidge(1));
                patrol=Restore(patrol);
            }
        }
        assert(patrol.simulation.ReachContact());patrol.choice=static_cast<uint8_t>(decision);
        patrol=Restore(patrol);forward::ContactEvidence evidence;
        assert(patrol.simulation.InspectContact(&evidence));
        assert(patrol.simulation.ResolveContact(static_cast<forward::Decision>(decision)));
        patrol.choice=0;patrol=Restore(patrol);assert(patrol.simulation.ReturnToBase());patrol=Restore(patrol);
        forward::Debrief debrief;assert(patrol.simulation.FileDebrief(&debrief));patrol=Restore(patrol);
        assert(!patrol.simulation.FileDebrief(&debrief));
        const auto taken=static_cast<forward::Route>(route);
        const auto other=static_cast<forward::Route>(1-route);
        assert(patrol.simulation.LastRouteIntel(taken)==debrief.information_value);
        assert(patrol.simulation.LastRouteIntel(other)==0);
        auto legacy=Bytes(patrol);legacy[8]=1;
        std::fill(legacy.begin()+30,legacy.begin()+41,0);Rehash(legacy);
        ForwardState migratedPatrol;
        assert(Checkpoint::Decode(&migratedPatrol,legacy.data(),legacy.size()));
        assert(Bytes(migratedPatrol)==Bytes(patrol));
        const auto condition=patrol.simulation.vehicle_condition(),fuel=patrol.simulation.fuel_remaining();
        assert(patrol.simulation.NextPatrol());
        assert(patrol.simulation.vehicle_condition()==condition&&patrol.simulation.fuel_remaining()==fuel);
        patrol=Restore(patrol);
        assert(!patrol.simulation.BeginPatrol(forward::Route::RidgeBypass));
        patrol.confirm_refit=true;patrol.choice=1;patrol=Restore(patrol);
        assert(patrol.simulation.RefitAtBase());patrol.confirm_refit=false;patrol.choice=0;
        assert(patrol.simulation.vehicle_condition()==100&&patrol.simulation.fuel_remaining()==20);
        patrol=Restore(patrol);
        assert(patrol.simulation.LastRouteIntel(taken)==debrief.information_value);
        assert(patrol.simulation.BeginPatrol(other));
        if(patrol.simulation.OnRidgeTrail())
            for(int i=0;i<forward::Simulation::kRidgeExit;++i)assert(patrol.simulation.WalkRidge(1));
        assert(patrol.simulation.ReachContact());
        assert(patrol.simulation.ResolveContact(forward::Decision::Avoid) && patrol.simulation.ReturnToBase());
        forward::Debrief new_debrief;
        assert(patrol.simulation.FileDebrief(&new_debrief));
        patrol=Restore(patrol);
        assert(patrol.simulation.LastRouteIntel(taken)==debrief.information_value);
        assert(patrol.simulation.LastRouteIntel(other)==new_debrief.information_value);
    }
    ForwardState empty;
    for(int i=0;i<2;++i){
        assert(empty.simulation.BeginPatrol(forward::Route::MainRoad)&&empty.simulation.ReachContact());
        assert(empty.simulation.ResolveContact(forward::Decision::Engage)&&empty.simulation.ReturnToBase());
        forward::Debrief debrief;assert(empty.simulation.FileDebrief(&debrief));assert(empty.simulation.NextPatrol());
    }
    assert(empty.simulation.vehicle_condition()==50&&empty.simulation.fuel_remaining()==0);
    assert(!empty.simulation.BeginPatrol(forward::Route::MainRoad)&&!empty.simulation.BeginPatrol(forward::Route::RidgeBypass));
    empty=Restore(empty);const auto forwardBytes=Bytes(empty);
    ForwardState badTrail;
    auto badOffset=forwardBytes;badOffset[38]=9;Rehash(badOffset);
    assert(!Checkpoint::Decode(&badTrail,badOffset.data(),badOffset.size()));
    auto badNotes=forwardBytes;badNotes[40]=1;Rehash(badNotes);
    assert(!Checkpoint::Decode(&badTrail,badNotes.data(),badNotes.size()));
    LanternState untouchedLantern;ForwardState untouchedForward;
    const auto originalLantern=Bytes(untouchedLantern),originalForward=Bytes(untouchedForward);
    for(size_t i=0;i<Checkpoint::kBytes;++i){
        auto a=lanternBytes,b=forwardBytes;a[i]^=1;b[i]^=1;
        assert(!Checkpoint::Decode(&untouchedLantern,a.data(),a.size()));
        assert(!Checkpoint::Decode(&untouchedForward,b.data(),b.size()));
        assert(Bytes(untouchedLantern)==originalLantern&&Bytes(untouchedForward)==originalForward);
    }
    assert(!Checkpoint::Decode(&untouchedLantern,forwardBytes.data(),forwardBytes.size()));
    assert(!Checkpoint::Decode(&untouchedForward,lanternBytes.data(),lanternBytes.size()));
    for(size_t count=0;count<Checkpoint::kBytes;++count){
        assert(!Checkpoint::Decode(&untouchedLantern,lanternBytes.data(),count));
        assert(!Checkpoint::Decode(&untouchedForward,forwardBytes.data(),count));
    }
    auto negativeFuel=forwardBytes;
    for(unsigned i=20;i<24;++i)negativeFuel[i]=255;
    Rehash(negativeFuel);assert(!Checkpoint::Decode(&untouchedForward,negativeFuel.data(),negativeFuel.size()));
    ForwardState resolved;
    assert(resolved.simulation.BeginPatrol(forward::Route::MainRoad)&&resolved.simulation.ReachContact());
    assert(resolved.simulation.ResolveContact(forward::Decision::Engage));
    auto impossibleFuel=Bytes(resolved);impossibleFuel[20]=20;Rehash(impossibleFuel);
    assert(!Checkpoint::Decode(&untouchedForward,impossibleFuel.data(),impossibleFuel.size()));
    auto impossibleRepair=Bytes(resolved);impossibleRepair[16]=100;Rehash(impossibleRepair);
    assert(!Checkpoint::Decode(&untouchedForward,impossibleRepair.data(),impossibleRepair.size()));
    auto inventedPhoto=lanternBytes;inventedPhoto[16]^=1;Rehash(inventedPhoto);
    assert(!Checkpoint::Decode(&untouchedLantern,inventedPhoto.data(),inventedPhoto.size()));
    std::puts("PASS recon exact-phase/choice/evidence/results, cumulative sorties, finite patrol fuel, explicit refit, corrupt transactional rejection");
}
