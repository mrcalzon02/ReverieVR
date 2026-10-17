#ifndef REVERIE_OMEGA_FRONT_INFRASTRUCTURE_H
#define REVERIE_OMEGA_FRONT_INFRASTRUCTURE_H

namespace reverie { namespace omega {

// The first theater reuses one combat objective model for several distinct
// sites. Only battles physically at Lantern Pump may change Hearthwell's
// real waterworks. Other field stations have their own fresh local structure.
struct InfrastructureScope {
    static constexpr int kFieldStationHealth = 18;
    static constexpr bool UsesHearthwellPump(int operation) {
        return operation == 0 || operation == 2;
    }
    static constexpr int BattleSiteHealth(int operation, int actualPumpHealth) {
        return UsesHearthwellPump(operation) ? actualPumpHealth : kFieldStationHealth;
    }
};

}} // namespace reverie::omega
#endif
