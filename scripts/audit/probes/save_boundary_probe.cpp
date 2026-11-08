#include "gumball_armada_simulation.h"
#include "red_ledger_simulation.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <array>
int main() {
    using namespace reverie::gumball;
    Simulation active(441);
    assert(active.cash()==100u);
    assert(active.hire_support(SupportKind::CapitaineEclair));
    assert(active.cash()==55u);
    assert(active.launch(0));
    assert(active.deployed_support()==SupportKind::CapitaineEclair);
    for(int i=0;i<10;i++)active.update(.05f,.2f,0.f,false);
    assert(active.phase()==Phase::Flight);
    const auto bytes=active.snapshot();
    Simulation restored(2);
    assert(restored.restore(bytes));
    std::printf("GUMBALL_INTERRUPTION: saved_phase=%u restored_phase=%u saved_elapsed=%.2f restored_elapsed=%.2f cash=%u pending_escort=%u deployed_escort=%u snapshot_bytes=%zu\n",
        unsigned(active.phase()),unsigned(restored.phase()),active.elapsed(),restored.elapsed(),restored.cash(),unsigned(restored.support().booked()),unsigned(restored.deployed_support()),sizeof(bytes));
    assert(restored.phase()==Phase::Chart && restored.elapsed()==0.f);
    assert(restored.cash()==55u && restored.support().booked()==SupportKind::None && restored.deployed_support()==SupportKind::None);
    using Bar=reverie::redledger::Simulation;
    Bar bar;
    std::array<uint8_t,Bar::kSerializedSize> data{};
    size_t written=0u;
    assert(bar.Serialize(data.data(),data.size(),&written));
    assert(written==data.size());
    Bar baseline;
    assert(baseline.Deserialize(data.data(),data.size(),true));
    data[12]^=1u; // one bit flipped in the serialized cash integer, no other bytes changed
    Bar corrupted;
    const bool accepted=corrupted.Deserialize(data.data(),data.size(),true);
    std::printf("RED_LEDGER_BIT_FLIP: offset=12 bit=0 accepted=%u expected_cash=%d restored_cash=%d bytes=%zu\n",unsigned(accepted),bar.cash_cents(),corrupted.cash_cents(),data.size());
    assert(accepted && corrupted.cash_cents()!=bar.cash_cents());
    return 0;
}
