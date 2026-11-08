#include "red_ledger_save.h"
#include <cassert>
#include <iostream>
using namespace reverie::redledger;
int main(){
    save::State game;
    assert(game.simulation.TakeCleanCup()&&game.simulation.FillHeldCup()
        &&game.simulation.ServeHeldCup()&&game.simulation.CollectPayment());
    save::Checkpoint::Bytes bytes{};
    assert(save::Checkpoint::Encode(game,&bytes));
    save::State restored;
    assert(save::Checkpoint::Decode(&restored,bytes.data(),bytes.size())==save::Checkpoint::Format::Checked);
    assert(restored.simulation.cash_cents()==game.simulation.cash_cents());
    save::Checkpoint::Bytes roundtrip{};
    assert(save::Checkpoint::Encode(restored,&roundtrip)&&roundtrip==bytes);
    for(size_t i=0;i<bytes.size();++i){
        auto broken=bytes;broken[i]^=1u;
        assert(save::Checkpoint::Decode(&restored,broken.data(),broken.size())==save::Checkpoint::Format::Invalid);
        assert(save::Checkpoint::Encode(restored,&roundtrip)&&roundtrip==bytes);
    }
    for(size_t size=0;size<bytes.size();++size){
        assert(save::Checkpoint::Decode(&restored,bytes.data(),size)==save::Checkpoint::Format::Invalid);
    }
    // Preserve exactly the supported previous composite shapes; the old
    // base has no retrospective checksum guarantee, which the record states.
    for(size_t size:{save::Checkpoint::kLegacySize,save::Checkpoint::kPayloadSize}){
        assert(save::Checkpoint::Decode(&restored,bytes.data()+save::Checkpoint::kHeaderSize,size)
            ==save::Checkpoint::Format::Legacy);
        assert(restored.simulation.cash_cents()==game.simulation.cash_cents());
    }
    assert(!save::Checkpoint::Encode(game,nullptr));
    assert(save::Checkpoint::Decode(nullptr,bytes.data(),bytes.size())==save::Checkpoint::Format::Invalid);
    std::cout<<"PASS Red Ledger whole-envelope bit corruption, transactional rejection and supported composite import\n";
}
