#include "gumball_shipyard_readiness.h"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace reverie::gumball;
bool Is(const Shipyard& yard,const char* text){
  return std::strcmp(FlightReadiness(yard),text)==0;
}
int main(){
  Shipyard ready;
  assert(ready.ready()&&Is(ready,"READY / CLICK SAME PART TO REMOVE"));
  Shipyard noThruster;
  assert(noThruster.remove(3,1));
  assert(!noThruster.ready()&&Is(noThruster,"ADD AT LEAST ONE THRUSTER"));
  Shipyard noReactor;
  assert(noReactor.remove(4,3));
  assert(!noReactor.ready()&&Is(noReactor,"ADD A REACTOR FOR POWER"));
  Shipyard noGun;
  assert(noGun.remove(2,3));
  assert(!noGun.ready()&&Is(noGun,"ADD A GUN MOUNT"));
  Shipyard lowPower;
  assert(lowPower.place(4,2,Part::Mount));
  assert(lowPower.place(4,4,Part::Cargo));
  assert(lowPower.place(2,2,Part::Cargo));
  assert(lowPower.profile().power<lowPower.profile().demand);
  assert(!lowPower.ready()&&Is(lowPower,"POWER DEFICIT / ADD BATTERY OR REACTOR"));
  Shipyard tooHeavy;
  assert(tooHeavy.place(4,2,Part::Armor));
  assert(tooHeavy.place(4,4,Part::Armor));
  assert(tooHeavy.place(2,2,Part::Armor));
  assert(tooHeavy.profile().power>=tooHeavy.profile().demand);
  assert(tooHeavy.profile().mass>18*tooHeavy.count(Part::Thruster));
  assert(!tooHeavy.ready()&&Is(tooHeavy,"OVERWEIGHT / ADD THRUSTER OR REMOVE PARTS"));
  assert(tooHeavy.remove(2,2)&&tooHeavy.remove(4,4));
  assert(tooHeavy.ready()&&Is(tooHeavy,"READY / CLICK SAME PART TO REMOVE"));
  std::cout<<"Gumball Shipyard actionable flight-readiness gates passed\n";
}
