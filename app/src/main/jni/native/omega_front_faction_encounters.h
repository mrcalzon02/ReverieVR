#ifndef REVERIE_OMEGA_FRONT_FACTION_ENCOUNTERS_H
#define REVERIE_OMEGA_FRONT_FACTION_ENCOUNTERS_H
#include "omega_front_tactics.h"

namespace reverie { namespace omega {
// Deterministic combat fixtures for later F01/F02/F04/F06 operation books.
// These are battle-capable encounter builders, not unlocked 70-arc campaigns.
class FactionEncounters {
public:
    static bool Build(Doctrine doctrine, uint32_t seed, Battle *output){
        if(!output || doctrine==Doctrine::Unaligned ||
           !ValidDoctrine(static_cast<uint8_t>(doctrine)))return false;
        Battle b(seed ? seed : 1);
        b.ClearUnits();
        if(!b.ConfigureDoctrine(doctrine) ||
           !b.ConfigureMission(doctrine==Doctrine::Hearthward?
               Mission::CaptureOrchard:Mission::HoldPump))return false;
        // Common player contingent; specialist roles and opposing objectives
        // make each authored encounter mechanically different.
        if(b.AddUnit(Side::Hearthwell,Class::Rifle,3,11)<0 ||
           b.AddUnit(Side::Hearthwell,Class::Engineer,4,12)<0 ||
           b.AddUnit(Side::Hearthwell,Class::Rifle,3,13)<0 ||
           b.AddUnit(Side::Hearthwell,Class::Heavy,6,12)<0)return false;
        switch(doctrine){
        case Doctrine::Hearthward:
            if(b.AddUnit(Side::Raiders,Class::Rifle,15,11)<0 ||
               b.AddUnit(Side::Raiders,Class::Rifle,16,13)<0 ||
               b.AddUnit(Side::Raiders,Class::Heavy,19,12)<0)return false;
            break;
        case Doctrine::Meridian:
            if(b.AddUnit(Side::Raiders,Class::Sentinel,18,10)<0 ||
               b.AddUnit(Side::Raiders,Class::Sentinel,18,14)<0 ||
               b.AddUnit(Side::Raiders,Class::Rifle,20,12)<0)return false;
            break;
        case Doctrine::Scrapline:
            if(b.AddUnit(Side::Raiders,Class::Salvager,10,11)<0 ||
               b.AddUnit(Side::Raiders,Class::Salvager,10,13)<0 ||
               b.AddUnit(Side::Raiders,Class::Rifle,15,12)<0)return false;
            break;
        case Doctrine::LostNinth:
            if(b.AddUnit(Side::Raiders,Class::Legionary,10,10)<0 ||
               b.AddUnit(Side::Raiders,Class::Legionary,10,11)<0 ||
               b.AddUnit(Side::Raiders,Class::Legionary,10,12)<0 ||
               b.AddUnit(Side::Raiders,Class::Legionary,11,12)<0)return false;
            break;
        default: return false;
        }
        *output=b;return true;
    }
};
}} // namespace reverie::omega
#endif
