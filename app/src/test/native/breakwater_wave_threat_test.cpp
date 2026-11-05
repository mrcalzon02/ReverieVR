#include "breakwater_wave_threat.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <cstring>
using reverie::breakwater::UnitType;
using reverie::breakwater::waves::PressureBand;
using reverie::breakwater::waves::SelectInvasionUnit;
using reverie::breakwater::waves::Operation;
using reverie::breakwater::waves::Plan;
using reverie::breakwater::waves::SpawnBudget;
using reverie::breakwater::waves::SpawnGapSeconds;
int main(){
    // Existing early-day single-passenger rowboats are never overwritten.
    for(unsigned roll=0;roll<256;++roll)
      for(unsigned index=1;index<=32;++index)
        assert(SelectInvasionUnit(2,index,32,roll)==UnitType::Rowboat);
    assert(PressureBand(0,16)==0u);
    assert(PressureBand(1,16)==0u);
    assert(PressureBand(4,16)==0u);
    assert(PressureBand(5,16)==1u);
    assert(PressureBand(9,16)==2u);
    assert(PressureBand(13,16)==3u);
    assert(PressureBand(255,16)==3u);
    assert(PressureBand(15,0)==0u);
    // Fixed-budget deterministic operations, never a new entity class.
    assert(Plan(1,14,20)==Operation::Mixed);
    assert(Plan(3,5,20)==Operation::Mixed);
    assert(Plan(3,10,20)==Operation::LandingPush);
    assert(Plan(5,6,20)==Operation::NavalScreen);
    assert(Plan(8,7,20)==Operation::AirRaid);
    assert(Plan(10,20,20)==Operation::Flagship);
    assert(Plan(9,20,20)==Operation::Mixed);
    assert(Plan(10,1,20)==Operation::Mixed);
    assert(Plan(10,21,20)==Operation::Mixed);
    assert(Plan(10,0,20)==Operation::Mixed);
    assert(Plan(10,5,0)==Operation::Mixed);
    assert(std::strcmp(reverie::breakwater::waves::OperationName(
        Operation::AirRaid),"AIR RAID")==0);
    for(unsigned tier=0;tier<4;++tier)
        assert(SpawnBudget(0,1,false,static_cast<uint8_t>(tier),
                           1,12+2*tier)==6u+3u*tier);
    assert(SpawnBudget(0,1,false,0,12,12)==17u);
    assert(SpawnBudget(2,4,false,3,1,30)==30u);
    assert(SpawnBudget(2,4,true,3,26,26)==42u);
    assert(SpawnBudget(2,4,false,3,30,30)==41u);
    for(unsigned tier=0;tier<4;++tier)
      for(unsigned loc=0;loc<3;++loc)
        for(unsigned day=1;day<=4;++day)
          for(unsigned total=8;total<=32;++total) {
            unsigned previous=0;
            for(unsigned wave=1;wave<=total;++wave) {
                const unsigned budget=SpawnBudget(static_cast<uint8_t>(loc),
                    static_cast<uint8_t>(day),false,
                    static_cast<uint8_t>(tier),wave,total);
                assert(budget>=previous && budget<=42u);
                assert(SpawnGapSeconds(wave,total)>.47f);
                assert(SpawnGapSeconds(wave,total)<=.65f);
                previous=budget;
            }
          }
    // First wave exactly preserves the former class-selection behavior.
    for(unsigned grade=3;grade<=20;++grade)
      for(uint32_t roll=0;roll<2048;++roll){
        const auto expected=grade>=8u&&roll%6u==0u?UnitType::Aircraft:
                            grade>=5u&&roll%4u==0u?UnitType::Gunboat:
                            roll%3u==0u?UnitType::LandingCraft:UnitType::Rowboat;
        assert(SelectInvasionUnit(grade,1,16,roll)==expected);
    }
    // Late-period pressure introduces genuinely heavier attackers.
    assert(SelectInvasionUnit(5,1,16,3)==UnitType::LandingCraft);
    assert(SelectInvasionUnit(5,16,16,3)==UnitType::Aircraft);
    assert(SelectInvasionUnit(3,1,16,4)==UnitType::Rowboat);
    assert(SelectInvasionUnit(3,16,16,4)==UnitType::LandingCraft);
    assert(SelectInvasionUnit(3,1,16,6)==UnitType::LandingCraft);
    assert(SelectInvasionUnit(3,16,16,6)==UnitType::Gunboat);
    // The terminal pressure band used the same divisor for aircraft
    // and gunboats, starving gunboats in high-grade invasions.
    bool aircraft=false,gunboat=false,lander=false;
    for(uint32_t high=0;high<128u;++high)
      for(uint32_t low=0;low<128u;++low) {
        const auto roll=(high<<16u)|low;
        const auto kind=SelectInvasionUnit(13u,16u,16u,roll);
        aircraft|=kind==UnitType::Aircraft;
        gunboat|=kind==UnitType::Gunboat;
        lander|=kind==UnitType::LandingCraft;
      }
    assert(aircraft && gunboat && lander);
    for(unsigned grade=0;grade<20;++grade)
      for(unsigned index=0;index<=32;++index)
        for(uint32_t roll=0;roll<256;++roll){
          const auto kind=SelectInvasionUnit(grade,index,32,roll);
          assert(kind==UnitType::Rowboat||kind==UnitType::LandingCraft||
                 kind==UnitType::Gunboat||kind==UnitType::Aircraft);
          assert(SelectInvasionUnit(grade,index,32,roll)==kind);
        }
    std::cout<<"Breakwater wave escalation, opening fairness and reproducibility passed\n";
}
