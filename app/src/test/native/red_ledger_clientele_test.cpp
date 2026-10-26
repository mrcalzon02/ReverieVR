#include "red_ledger_clientele.h"
#include <cassert>
#include <array>
#include <cstdint>
#include <iostream>
using namespace reverie::redledger;
int main(){
    using clientele::State;
    State rough;
    assert(rough.cleanliness()==20&&rough.services()==0);
    assert(rough.comfortable(PatronKind::SalihorskHauler));
    assert(!rough.comfortable(PatronKind::NightClerk));
    assert(rough.reputation_premium(PatronKind::NightClerk,3)==0);
    assert(rough.reputation_premium(PatronKind::SalihorskHauler,2)==2);
    // Successful rough-client service makes the bar progressively dirtier
    // while the preferred supplier attracts repeat business.
    rough.served(PatronKind::SalihorskHauler,
        trade::FavoriteDrink(PatronKind::SalihorskHauler),
        trade::FavoriteSnack(PatronKind::SalihorskHauler),0);
    assert(rough.cleanliness()==13);
    assert(rough.services()==1);
    assert(rough.affinity(trade::FactionFor(PatronKind::SalihorskHauler))==2);
    rough.served(PatronKind::SalihorskHauler,0,-1,0);
    assert(rough.cleanliness()==6);
    assert(rough.affinity(trade::FactionFor(PatronKind::SalihorskHauler))==1);
    for(int i=0;i<500;++i)
        rough.served(PatronKind::RailWorker,0,-1,0);
    assert(rough.cleanliness()==0);
    // Paid renovations help but never instantly create a perfect bar.
    rough.next_day(9);
    assert(rough.cleanliness()==9);
    rough.next_day(0);
    assert(rough.cleanliness()==9);
    State upgraded;
    upgraded.served(PatronKind::SalihorskHauler,
        trade::FavoriteDrink(PatronKind::SalihorskHauler),-1,5);
    assert(upgraded.cleanliness()==18); // seven dirt minus five resistance
    for(int day=0;day<5;++day)upgraded.next_day(20);
    assert(upgraded.cleanliness()==100);
    assert(upgraded.comfortable(PatronKind::NightClerk));
    // Preserve all six distinct patron/supplier trajectories and data.
    for(const auto& patron:Simulation::PatronCatalog()) {
        const int faction=trade::FactionFor(patron.kind);
        assert(faction>=0&&faction<trade::kFactions);
        upgraded.served(patron.kind,trade::FavoriteDrink(patron.kind),
                        trade::FavoriteSnack(patron.kind),5);
        assert(upgraded.affinity(faction)==2);
    }
    std::array<uint8_t,State::kSerializedSize> bytes{};
    assert(!upgraded.serialize(nullptr,bytes.size()));
    assert(!upgraded.serialize(bytes.data(),bytes.size()-1));
    assert(upgraded.serialize(bytes.data(),bytes.size()));
    State restored;
    assert(restored.deserialize(bytes.data(),bytes.size()));
    assert(restored.cleanliness()==upgraded.cleanliness());
    assert(restored.services()==upgraded.services());
    for(int i=0;i<trade::kFactions;++i)
        assert(restored.affinity(i)==upgraded.affinity(i));
    assert(!restored.deserialize(bytes.data(),bytes.size()-1));
    auto corrupt=bytes;
    corrupt[9]^=0x4u;
    assert(!restored.deserialize(corrupt.data(),corrupt.size()));
    assert(restored.cleanliness()==upgraded.cleanliness());
    auto invalid=bytes;
    // impossible cleanliness value: recompute legitimate hash to prove
    // semantic validation, not just checksum comparison.
    invalid[4]=255;invalid[5]=invalid[6]=invalid[7]=0;
    uint32_t hash=2166136261u;
    for(size_t i=0;i<36;++i)hash=(hash^invalid[i])*16777619u;
    for(int i=0;i<4;++i)invalid[36+i]=static_cast<uint8_t>(hash>>(i*8));
    assert(!restored.deserialize(invalid.data(),invalid.size()));
    assert(restored.services()==upgraded.services());
    std::cout<<"PASS Red Ledger actual patron preference, venue condition, upgrades, durable checked state\n";
}
