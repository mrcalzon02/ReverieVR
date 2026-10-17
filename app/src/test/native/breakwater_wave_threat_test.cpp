#include "breakwater_wave_threat.h"
#include <cassert>
#include <cstdint>
#include <iostream>
using reverie::breakwater::UnitType;
using reverie::breakwater::waves::PressureBand;
using reverie::breakwater::waves::SelectInvasionUnit;
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
