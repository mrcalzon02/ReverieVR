#ifndef REVERIE_OMEGA_FRONT_FRONTIER_H
#define REVERIE_OMEGA_FRONT_FRONTIER_H

#include "omega_front_tactics.h"
#include "omega_front_faction_encounters.h"
#include "omega_front_intelligence.h"
#include "omega_front_accord.h"
#include <array>
#include <cstdint>

namespace reverie { namespace omega {

// First-playable Hearthfall March strategy. These are the sixteen authored
// T01 sectors, NOT a replacement for the full 600-sector campaign design.
constexpr int kFrontierSectors = 16;
constexpr int kMajorFactions = 17;
constexpr int kHearthfallOperations = 6; // Three original operations + three first-contact battles.
constexpr uint8_t kIndependentOwner = 18; // Settlement; NOT faction #18.
enum class Ecology : uint8_t { Eden, Wasteland, Barren };
struct FrontierSector {
    const char* name;
    Ecology ecology;
    Ecology previous;
    uint8_t owner;        // 0 neutral, 1..17 major factions, 18 independent.
    uint8_t anomaly;
    uint8_t stability;    // 0..100, low means morphology still unsettled.
    uint16_t residents;
    uint16_t food;
    uint16_t water;
    uint16_t wreck_mass;
    uint8_t claims;       // Local political pressure, not a new faction.
};
struct FrontierReport {
    int operation = -1;
    Result outcome = Result::Active;
    uint8_t defenders_lost = 0;
    uint8_t enemies_lost = 0;
    uint8_t pump_remaining = 0;
    uint32_t battle_ticks = 0;
};

class Frontier {
public:
    explicit Frontier(uint32_t seed=1) : seed_(seed ? seed : 1) {
        sectors_ = {{
            {"Chalkwatch",Ecology::Barren,Ecology::Barren,0,0,95,0,0,0,13,0},
            {"Ninth Camp",Ecology::Wasteland,Ecology::Wasteland,6,0,73,84,43,30,17,9},
            {"Splinter Ridge",Ecology::Wasteland,Ecology::Wasteland,0,0,60,0,0,0,27,0},
            {"Old Relay",Ecology::Wasteland,Ecology::Wasteland,0,0,50,0,0,0,18,4},
            {"Caravan Ford",Ecology::Eden,Ecology::Wasteland,0,0,65,46,34,32,6,6},
            {"White Orchard",Ecology::Eden,Ecology::Wasteland,1,0,82,120,90,58,0,8},
            {"Hearthwell",Ecology::Eden,Ecology::Wasteland,kIndependentOwner,0,82,31,44,34,2,8},
            {"Charter Gate",Ecology::Eden,Ecology::Wasteland,2,0,70,17,18,12,7,16},
            {"Rust Orchard",Ecology::Wasteland,Ecology::Wasteland,0,0,27,0,0,0,19,0},
            {"Ashrail Yard",Ecology::Wasteland,Ecology::Wasteland,4,0,72,21,12,10,130,11},
            {"Lantern Pump",Ecology::Eden,Ecology::Wasteland,0,0,76,9,3,53,8,8},
            {"Toll Foundry",Ecology::Wasteland,Ecology::Wasteland,2,0,82,33,18,15,92,19},
            {"Spoor Nest",Ecology::Wasteland,Ecology::Wasteland,12,0,33,0,0,0,75,0},
            {"Glass Cut",Ecology::Wasteland,Ecology::Wasteland,0,1,18,0,0,0,6,0},
            {"Broken Viaduct",Ecology::Wasteland,Ecology::Wasteland,0,0,45,0,0,0,25,0},
            {"Dry Basin",Ecology::Barren,Ecology::Barren,0,0,88,0,0,0,45,0}
        }};
        // Diplomacy values are opinions of the independent Hearthwell command.
        // These are established 17 faction IDs, not extra entities.
        relations_[1]=-20; relations_[2]=-30;
        relations_[4]=-10; relations_[6]=-5;
    }
    static bool Valid(int i) { return i>=0 && i<kFrontierSectors; }
    const FrontierSector* sector(int i) const { return Valid(i)?&sectors_[i]:nullptr; }
    int location() const {return location_;}
    int day() const {return day_;}
    int operation() const {return operation_;}
    int population() const {return population_;}
    int guards() const {return guards_;}
    int rations() const {return rations_;}
    int salvage() const {return salvage_;}
    int trust() const {return trust_;}
    int pump_health() const {return pump_health_;}
    int relation(int faction) const {return faction>=1 && faction<=kMajorFactions?relations_[faction]:0;}
    uint8_t intelligence(int sector) const {return intel_.level(sector);}
    bool parleyed(int sector) const {return intel_.parleyed(sector);}
    uint8_t accords() const {return accords_.mask();}
    int last_accord() const {return accords_.last_operation();}
    bool bridge_repaired() const {return bridge_;}
    bool deployed() const {return deployed_;}
    const Battle* current_battle() const {return deployed_?&battle_:nullptr;}
    Battle* current_battle() {return deployed_?&battle_:nullptr;}
    FrontierReport last_report() const {return last_;}
    uint32_t seed() const {return seed_;}
    uint32_t morphology_epoch() const {return morphology_epoch_;}
    static uint8_t FactionForOperation(int op){
        constexpr uint8_t factions[kHearthfallOperations]={0,1,1,4,2,6};
        return op>=0 && op<kHearthfallOperations?factions[op]:0;
    }
    static int TargetForOperation(int op){
        // S011 waterworks, S006 orchards, S011 receivership, then
        // S010 salvage yard, S008 charter checkpoint, S005 ford.
        constexpr int targets[kHearthfallOperations]={10,5,10,9,7,4};
        return op>=0 && op<kHearthfallOperations?targets[op]:-1;
    }
    static Mission MissionForOperation(int op){
        // A contested control point works for Charter Gate and Caravan Ford;
        // the objective isn't a fake win screen: troops must hold the tile.
        return op==1 || op==4 || op==5?Mission::CaptureOrchard:
               op==2?Mission::DisableRelay:Mission::HoldPump;
    }
    static const char *OperationName(int op){
        constexpr const char *names[kHearthfallOperations]={
            "PUMP DEFENSE","WHITE ORCHARD","RECEIVERSHIP",
            "PRICE OF METAL","CHARTER GATE","EAGLES AT FORD"};
        return op>=0 && op<kHearthfallOperations?names[op]:"FRONT SECURED";
    }
    uint8_t operation_faction() const {return FactionForOperation(operation_);}
    int target_sector() const {return TargetForOperation(operation_);}
    bool can_travel(int destination) const {
        if(deployed_||!Valid(destination)||destination==location_)return false;
        const int dx=destination%4-location_%4;
        const int dy=destination/4-location_/4;
        if(dx*dx+dy*dy!=1)return false;
        // Broken S011-S015 viaduct cannot be crossed before engineers work.
        if(((location_==10 && destination==14)||(location_==14 && destination==10))&&!bridge_)return false;
        // Advanced Barren expeditions are outside the initial field kit.
        if(sectors_[destination].ecology==Ecology::Barren)return false;
        return true; // Unsupplied retreat remains reachable, but trust pays the cost.
    }
    bool Travel(int destination) {
        if(!can_travel(destination))return false;
        if(rations_>0)--rations_;
        else trust_=trust_>=2?trust_-2:0;
        location_=destination;
        intel_.ObserveArrival(destination);
        AdvanceDay();return true;
    }
    // Scouts cost two real rations plus a day; no free repeat surveys.
    bool Scout(int target){
        if(deployed_||!intel_.Survey(location_,target,rations_))return false;
        AdvanceDay();return true;
    }
    // One negotiated concession per surveyed neighboring faction sector.
    // Salvage transfers to its physical store; no fake mission victories.
    bool Negotiate(int target){
        if(deployed_||!Valid(target))return false;
        FrontierSector &s=sectors_[target];
        // The Lost Ninth occupies the ford militarily but the neutral
        // crossing's legal owner is deliberately not converted to F06.
        const int faction=(target==4 && operation_==5 && s.owner==0)?6:s.owner;
        if(faction<1||faction>kMajorFactions)return false;
        if(!intel_.Parley(location_,target,static_cast<uint8_t>(faction),
                         rations_,salvage_,trust_,
                         relations_[faction],s.wreck_mass,s.claims))return false;
        AdvanceDay();return true;
    }
    // Noncombat resolutions require a prior parley with the active faction
    // and additional real resources. No troop casualty, pump repair, seized
    // territory or fabricated Battle::Defended result is ever awarded.
    bool ConcludeAccord(int target){
        if(deployed_ || operation_<3 || operation_>=kHearthfallOperations ||
           target!=target_sector() || !Valid(target))return false;
        FrontierSector &s=sectors_[target];
        if(!accords_.Conclude(operation_,location_,target,intel_.parleyed(target),
                             s.owner,relations_[FactionForOperation(operation_)],
                             rations_,salvage_,trust_,s.wreck_mass,s.claims))return false;
        ++operation_;
        AdvanceDay();
        return true;
    }
    // Survey forecast is visible before the next ecology transition.
    int next_morphology_day() const {return morphology_epoch_==0?4:-1;}
    int forecast_sector() const {return morphology_epoch_==0?8:-1;}
    bool AdvanceDay() {
        if(deployed_)return false;
        ++day_;
        // Hearthwell consumes actual food and water per day. No negative
        // resource counts and no manufactured supplies from a time pulse.
        FrontierSector &home=sectors_[6];
        const uint16_t need=static_cast<uint16_t>((population_+9)/10);
        if(home.food>=need)home.food-=need;
        else {home.food=0;trust_=trust_>2?trust_-2:0;}
        if(home.water>=need)home.water-=need;
        else {home.water=0;trust_=trust_>3?trust_-3:0;}
        if(day_==4 && morphology_epoch_==0){
            FrontierSector &orchard=sectors_[8];
            orchard.previous=orchard.ecology;
            orchard.ecology=Ecology::Eden;
            orchard.stability=45;
            // New clean terrain is not magically a stocked farm or town.
            orchard.food=0;orchard.water=0;orchard.residents=0;
            ++morphology_epoch_;
        }
        // Healthy waterworks can transfer existing S011 stored water.
        if(pump_health_>0 && home.water<20 && sectors_[10].water>0){
            const uint16_t transfer=sectors_[10].water>=5?5:sectors_[10].water;
            sectors_[10].water-=transfer;home.water+=transfer;
        }
        return true;
    }
    bool RepairBridge() {
        if(deployed_||bridge_||location_!=14||salvage_<6)return false;
        salvage_-=6;bridge_=true;return true;
    }
    bool RepairPump() {
        if(deployed_||location_!=10||pump_health_>=18||salvage_<3)return false;
        salvage_-=3;pump_health_=pump_health_+6>18?18:pump_health_+6;
        return true;
    }
    bool ForageRations() {
        if(deployed_ || sectors_[location_].ecology!=Ecology::Eden ||
           sectors_[location_].food<6 || rations_>90)return false;
        sectors_[location_].food-=6;rations_+=4;return true;
    }
    bool Recruit() {
        if(deployed_||location_!=6||guards_>=12||trust_<15||
           sectors_[6].food<4)return false;
        sectors_[6].food-=4;trust_-=3;++guards_;
        return true;
    }
    // Authoritative operation encounter builder shared by live campaign and
    // deterministic fixture tests. Does not spend or mutate strategic stocks.
    static bool BuildBattle(int op,uint32_t seed,int pumpHp,Battle *output){
        if(!output || op<0 || op>=kHearthfallOperations || pumpHp<1 || pumpHp>18)return false;
        if(op>=3){
            Doctrine doctrine=op==3?Doctrine::Scrapline:
                op==4?Doctrine::Meridian:Doctrine::LostNinth;
            Battle built(seed);
            if(!FactionEncounters::Build(doctrine,seed,&built) ||
               !built.ConfigurePump(pumpHp) ||
               !built.ConfigureMission(MissionForOperation(op)))return false;
            *output=built;return true;
        }
        Battle battle(seed);
        battle.ClearUnits();
        if(!battle.ConfigureDoctrine(op==0?Doctrine::Unaligned:Doctrine::Hearthward) ||
           !battle.ConfigurePump(pumpHp) ||
           !battle.ConfigureMission(MissionForOperation(op)))return false;
        if(op==1) {
            battle.SetGround(7,10,Ground::Rubble);
            battle.SetGround(7,11,Ground::Rubble);
            battle.SetGround(7,13,Ground::Rubble);
        } else if(op==2) {
            battle.SetGround(7,10,Ground::Wall);
            battle.SetGround(7,14,Ground::Wall);
        }
        // Four deployed members; the remainder continue guarding civilians.
        if(battle.AddUnit(Side::Hearthwell,Class::Rifle,3,11)<0 ||
           battle.AddUnit(Side::Hearthwell,Class::Rifle,3,13)<0 ||
           battle.AddUnit(Side::Hearthwell,Class::Engineer,2,13)<0 ||
           battle.AddUnit(Side::Hearthwell,Class::Heavy,5,12)<0)return false;
        if(op==1){
            // Hearthward watches physically defend the orchard rather than
            // using the pump-assault spawn and pretending it is another war.
            if(battle.AddUnit(Side::Raiders,Class::Rifle,15,10)<0 ||
               battle.AddUnit(Side::Raiders,Class::Rifle,15,14)<0 ||
               battle.AddUnit(Side::Raiders,Class::Heavy,19,12)<0 ||
               battle.AddUnit(Side::Raiders,Class::Rifle,19,9)<0)return false;
        }else{
            if(battle.AddUnit(Side::Raiders,Class::Rifle,8,11)<0 ||
               battle.AddUnit(Side::Raiders,Class::Rifle,8,13)<0 ||
               battle.AddUnit(Side::Raiders,Class::Heavy,10,12)<0)return false;
            if(op==2 && (battle.AddUnit(Side::Raiders,Class::Rifle,19,9)<0 ||
                battle.AddUnit(Side::Raiders,Class::Rifle,20,12)<0 ||
                battle.AddUnit(Side::Raiders,Class::Engineer,19,12)<0))return false;
        }
        *output=battle;return true;
    }
    bool StartOperation() {
        if(deployed_||operation_>=kHearthfallOperations||location_!=target_sector()||
           guards_<4||rations_<2||pump_health_==0)return false;
        const uint32_t encounter_seed=Mix(seed_,static_cast<uint32_t>(operation_+1));
        Battle battle(encounter_seed);
        if(!BuildBattle(operation_,encounter_seed,pump_health_,&battle))return false;
        battle_=battle;deployed_=true;rations_-=2;
        return true;
    }
    // Only the owned live Battle can be finalized; repeated resolution pays
    // no duplicate salvage/food and casualties are permanent.
    bool ResolveOperation() {
        if(!deployed_||battle_.result()==Result::Active)return false;
        last_=FrontierReport{};
        last_.operation=operation_;
        last_.outcome=battle_.result();
        last_.battle_ticks=battle_.ticks();
        last_.pump_remaining=static_cast<uint8_t>(battle_.pump_hp());
        for(int i=0;i<battle_.unit_count();++i){
            const Unit *u=battle_.unit(i);
            if(!u||u->alive())continue;
            if(u->side==Side::Hearthwell)++last_.defenders_lost;
            else ++last_.enemies_lost;
        }
        guards_=guards_>=last_.defenders_lost?guards_-last_.defenders_lost:0;
        pump_health_=battle_.pump_hp();
        // Wreck recovery comes from destroyed actual attackers, not a
        // victory-screen reward duplicated by replaying the same result.
        const int recovered=2*last_.enemies_lost;
        salvage_+=recovered;
        deployed_=false;
        if(last_.outcome==Result::Defended){
            if(operation_==0){
                sectors_[10].owner=kIndependentOwner;
                trust_=trust_+5>100?100:trust_+5;
            } else if(operation_==1){
                // Hearthward orchard supplies are physically transferred.
                const uint16_t transfer=sectors_[5].food>=12?12:sectors_[5].food;
                sectors_[5].food-=transfer;sectors_[6].food+=transfer;
                trust_=trust_+3>100?100:trust_+3;
            } else if(operation_==2) {
                // Hearthward hardliners withdraw their receivership claim.
                sectors_[10].claims=sectors_[10].claims>=8?sectors_[10].claims-8:0;
            } else if(operation_==3) {
                // Scrapline controls the yard; an armed recovery wins only a
                // finite engine-stock salvage allotment. We do not mint scrap.
                FrontierSector &yard=sectors_[9];
                const uint16_t recovered_stock=yard.wreck_mass>=18?18:yard.wreck_mass;
                yard.wreck_mass-=recovered_stock;
                salvage_+=recovered_stock;
                yard.claims=yard.claims>80?100:static_cast<uint8_t>(yard.claims+20);
                AdjustRelation(4,-12);
            } else if(operation_==4) {
                // An actual checkpoint seizure, not a diplomatic concession.
                sectors_[7].owner=kIndependentOwner;
                sectors_[7].claims=sectors_[7].claims>=10?sectors_[7].claims-10:0;
                AdjustRelation(2,-15);
            } else if(operation_==5) {
                // A secured crossing, but the Roman camp remains F06 land.
                sectors_[4].owner=kIndependentOwner;
                sectors_[4].claims=sectors_[4].claims>=7?sectors_[4].claims-7:0;
                AdjustRelation(6,-10);
            }
            ++operation_;
        }else{
            trust_=trust_>=5?trust_-5:0;
            if(operation_>=3)AdjustRelation(operation_faction(),-4);
            const int contested=target_sector();
            if(Valid(contested)) {
                FrontierSector &s=sectors_[contested];
                s.claims=s.claims<=90?static_cast<uint8_t>(s.claims+10):100;
            }
        }
        return true;
    }
    uint64_t Checksum() const {
        uint64_t h=UINT64_C(14695981039346656037);
        auto add=[&h](uint64_t v){for(int i=0;i<8;++i){h^=(v>>(i*8))&255u;h*=UINT64_C(1099511628211);}};
        add(seed_);add(location_);add(day_);add(operation_);add(guards_);
        add(population_);add(rations_);add(salvage_);add(trust_);
        add(pump_health_);add(bridge_);add(morphology_epoch_);add(deployed_);
        for(int faction=1;faction<=kMajorFactions;++faction)
            add(static_cast<uint8_t>(relations_[faction]));
        for(int i=0;i<kFrontierSectors;++i){
            add(intel_.levels()[i]);add(intel_.talks()[i]);
        }
        add(accords_.mask());add(accords_.last_operation()+1);
        add(last_.operation+1);add(static_cast<uint8_t>(last_.outcome));
        add(last_.defenders_lost);add(last_.enemies_lost);
        add(last_.pump_remaining);add(last_.battle_ticks);
        for(const auto &s:sectors_){
            add(static_cast<uint8_t>(s.ecology));add(static_cast<uint8_t>(s.previous));
            add(s.owner);add(s.anomaly);add(s.stability);
            add(s.residents);add(s.food);add(s.water);add(s.wreck_mass);add(s.claims);
        }
        if(deployed_)add(battle_.Checksum());
        return h;
    }
private:
    friend class CampaignSave;
    void AdjustRelation(int faction,int delta){
        if(faction<1||faction>kMajorFactions)return;
        int next=static_cast<int>(relations_[faction])+delta;
        if(next<-100)next=-100;
        if(next>100)next=100;
        relations_[faction]=static_cast<int8_t>(next);
    }
    static uint32_t Mix(uint32_t seed,uint32_t index){
        uint32_t x=seed^(index*0x9e3779b9u);
        x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;
        return x?x:1;
    }
    std::array<FrontierSector,kFrontierSectors> sectors_{};
    std::array<int8_t,kMajorFactions+1> relations_{};
    Intelligence intel_{};
    Accord accords_{};
    Battle battle_{1};
    uint32_t seed_;
    int location_=6,day_=1,operation_=0,population_=31;
    int guards_=12,rations_=24,salvage_=12,trust_=55,pump_health_=18;
    uint32_t morphology_epoch_=0;
    bool bridge_=false,deployed_=false;
    FrontierReport last_{};
};

}} // namespace reverie::omega
#endif
