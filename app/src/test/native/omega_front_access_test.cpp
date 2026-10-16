#include "omega_front_access.h"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace reverie::omega;

static Accord AccordAt(int op) {
    Accord a;
    assert(a.Restore(static_cast<uint8_t>(1u << op), op, op+1));
    return a;
}
int main() {
    Accord none;
    Obligations noDebt;
    for(int sector=-1;sector<=16;++sector)
        assert(!TransitRights::WaivesTravelRation(sector,none,noDebt));
    assert(TransitRights::OperationForSector(Accord::Sector(3))==-1);
    assert(TransitRights::OperationForSector(Accord::Sector(4))==4);
    assert(TransitRights::OperationForSector(Accord::Sector(5))==5);
    assert(TransitRights::OperationForSector(-1)==-1);
    assert(TransitRights::OperationForSector(16)==-1);

    // A salvage licence buys wreck metal, NOT unlimited free movement.
    auto scrap = AccordAt(3);
    Obligations scrapDue;
    assert(scrapDue.Start(3,4));
    for(int sector=0;sector<16;++sector)
        assert(!TransitRights::WaivesTravelRation(sector,scrap,scrapDue));

    auto meridian=AccordAt(4);
    Obligations due;
    assert(due.Start(4,10));
    assert(TransitRights::WaivesTravelRation(7,meridian,due));
    assert(!TransitRights::WaivesTravelRation(4,meridian,due));
    assert(!TransitRights::WaivesTravelRation(9,meridian,due));

    // Honoring the follow-up installment retains paid transit access.
    int rations=5,salvage=2,trust=55;
    int8_t relationship=0;
    uint16_t wreck=0,food=0;
    uint8_t claims=30;
    assert(due.Settle(4,13,rations,salvage,trust,relationship,wreck,food,claims));
    assert(due.status(4)==Obligations::Honored);
    assert(TransitRights::WaivesTravelRation(7,meridian,due));
    assert(salvage==0 && wreck==2);

    // An unpaid follow-up immediately revokes free transit, irrespective of
    // whether the one-time border inspection has happened yet.
    Obligations broken;
    assert(broken.Start(4,10));
    salvage=0;
    assert(broken.Settle(4,13,rations,salvage,trust,relationship,wreck,food,claims));
    assert(broken.status(4)==Obligations::Broken);
    assert(!TransitRights::WaivesTravelRation(7,meridian,broken));

    // Lost Ninth owns neither the neutral ford nor the commander's stores:
    // the paid right applies only to the crossing's existing physical tile.
    auto ninth=AccordAt(5);
    Obligations ninthDue;
    assert(ninthDue.Start(5,20));
    assert(TransitRights::WaivesTravelRation(4,ninth,ninthDue));
    assert(!TransitRights::WaivesTravelRation(7,ninth,ninthDue));
    assert(!TransitRights::WaivesTravelRation(1,ninth,ninthDue));
    rations=2;
    assert(ninthDue.Settle(5,23,rations,salvage,trust,relationship,wreck,food,claims));
    assert(ninthDue.status(5)==Obligations::Honored);
    assert(TransitRights::WaivesTravelRation(4,ninth,ninthDue));

    // Older schemas record treaties but deliberately don't create new debts.
    Obligations oldAgreement;
    assert(oldAgreement.Migrate(static_cast<uint8_t>((1u<<4)|(1u<<5))));
    assert(oldAgreement.status(4)==Obligations::Legacy);
    assert(oldAgreement.status(5)==Obligations::Legacy);
    assert(TransitRights::WaivesTravelRation(7,meridian,oldAgreement));
    assert(TransitRights::WaivesTravelRation(4,ninth,oldAgreement));

    // Invalid or unfinished agreements, even alongside a separate live debt,
    // must never grant another faction's ration benefit.
    assert(!TransitRights::WaivesTravelRation(7,none,due));
    assert(!TransitRights::WaivesTravelRation(4,meridian,ninthDue));
    std::cout << "Omega Front transit rights regression passed\n";
}
