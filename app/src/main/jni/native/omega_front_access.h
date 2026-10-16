#ifndef REVERIE_OMEGA_FRONT_ACCESS_H
#define REVERIE_OMEGA_FRONT_ACCESS_H

#include "omega_front_obligations.h"

namespace reverie { namespace omega {

// The Meridian transit charter and the Lost Ninth passage agreement actually
// pay for corridor access. Entering those sectors costs no *travel* ration
// while their agreement remains in force. A broken treaty revokes the benefit.
// All benefit is derived from validated accord/obligation state: no extra save
// flags, clocks, artificial supply creation, or override of terrain/road gates.
class TransitRights {
public:
    static int OperationForSector(int destination) {
        if (destination == Accord::Sector(4)) return 4;
        if (destination == Accord::Sector(5)) return 5;
        return -1; // Scrapline salvage licence is not a travel permit.
    }
    static bool WaivesTravelRation(int destination,
                                   const Accord &accords,
                                   const Obligations &obligations) {
        const int operation = OperationForSector(destination);
        if (operation < 0 || !accords.completed(operation)) return false;
        const auto status = obligations.status(operation);
        return status == Obligations::Due ||
               status == Obligations::Honored ||
               status == Obligations::Legacy;
    }
};

}} // namespace reverie::omega
#endif
