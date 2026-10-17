#ifndef REVERIE_GUMBALL_SHIPYARD_READINESS_H
#define REVERIE_GUMBALL_SHIPYARD_READINESS_H
#include "gumball_shipyard.h"
namespace reverie { namespace gumball {
// Actual launch gates in the same order as Shipyard::ready(). The yard UI
// must say which build constraint blocks launch, not list every requirement.
inline const char* FlightReadiness(const Shipyard& yard) {
    if(!yard.connected())return "CONNECT ALL SHIP PARTS";
    if(yard.count(Part::Core)!=1)return "MISSING SHIP CORE";
    if(yard.count(Part::Thruster)<1)return "ADD AT LEAST ONE THRUSTER";
    if(yard.count(Part::Reactor)<1)return "ADD A REACTOR FOR POWER";
    if(yard.count(Part::Mount)<1)return "ADD A GUN MOUNT";
    const ShipProfile profile=yard.profile();
    if(profile.power<profile.demand)
        return "POWER DEFICIT / ADD BATTERY OR REACTOR";
    if(profile.mass>18*yard.count(Part::Thruster))
        return "OVERWEIGHT / ADD THRUSTER OR REMOVE PARTS";
    return "READY / CLICK SAME PART TO REMOVE";
}
}}
#endif
