#ifndef REVERIE_RED_LEDGER_SAVE_H
#define REVERIE_RED_LEDGER_SAVE_H
#include "red_ledger_simulation.h"
#include "red_ledger_trade.h"
#include "red_ledger_renovations.h"
#include "red_ledger_clientele.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace reverie { namespace redledger { namespace save {
struct State {
    Simulation simulation;
    trade::Trade market;
    property::Renovations renovations;
    clientele::State clientele;
};
// The host still owns atomic replacement of state-v4.bin. The envelope now
// checks every byte, including the formerly unchecked base cash/inventory.
// This is corruption detection, not a claim of cryptographic authenticity.
class Checkpoint {
public:
    static constexpr size_t kLegacySize=Simulation::kSerializedSize+
        trade::Trade::kSerializedSize+property::Renovations::kSerializedSize;
    static constexpr size_t kPayloadSize=kLegacySize+clientele::State::kSerializedSize;
    static constexpr size_t kHeaderSize=16;
    static constexpr size_t kBytes=kHeaderSize+kPayloadSize+8;
    using Bytes=std::array<uint8_t,kBytes>;
    enum class Format { Invalid, Legacy, Checked };
    static bool Encode(const State& state,Bytes* out){
        if(!out)return false;
        Bytes bytes{};
        std::memcpy(bytes.data(),"RLSTATE5",8);
        Put32(bytes.data()+8,5u);Put32(bytes.data()+12,kPayloadSize);
        uint8_t* payload=bytes.data()+kHeaderSize;
        size_t count=0;
        if(!state.simulation.Serialize(payload,Simulation::kSerializedSize,&count)
            ||count!=Simulation::kSerializedSize
            ||!state.market.Serialize(payload+count,trade::Trade::kSerializedSize)
            ||!state.renovations.serialize(payload+count+trade::Trade::kSerializedSize,
                                          property::Renovations::kSerializedSize)
            ||!state.clientele.serialize(payload+kLegacySize,clientele::State::kSerializedSize))
            return false;
        uint64_t hash=Hash(bytes.data(),kBytes-8);
        for(size_t i=kBytes-8;i<kBytes;++i){bytes[i]=static_cast<uint8_t>(hash);hash>>=8;}
        *out=bytes;return true;
    }
    static Format Decode(State* output,const uint8_t* bytes,size_t count){
        if(!output||!bytes)return Format::Invalid;
        const uint8_t* payload=bytes;
        size_t size=count;
        Format format=Format::Legacy;
        if(count==kBytes){
            if(std::memcmp(bytes,"RLSTATE5",8)!=0||Get32(bytes+8)!=5u
                ||Get32(bytes+12)!=kPayloadSize)return Format::Invalid;
            uint64_t hash=Hash(bytes,kBytes-8);
            for(size_t i=kBytes-8;i<kBytes;++i){
                if(bytes[i]!=static_cast<uint8_t>(hash))return Format::Invalid;
                hash>>=8;
            }
            payload=bytes+kHeaderSize;size=kPayloadSize;format=Format::Checked;
        }else if(count!=kLegacySize&&count!=kPayloadSize)return Format::Invalid;
        // Supported old composites have strict semantic/section validation,
        // but their base segment cannot be authenticated retrospectively.
        // Decode into candidates and upgrade only after all sections agree.
        State next;
        if(!next.simulation.Deserialize(payload,Simulation::kSerializedSize,true)
            ||!next.market.Deserialize(payload+Simulation::kSerializedSize,
                                       trade::Trade::kSerializedSize)
            ||!next.renovations.deserialize(payload+Simulation::kSerializedSize+
                  trade::Trade::kSerializedSize,property::Renovations::kSerializedSize)
            ||(size==kPayloadSize&&!next.clientele.deserialize(payload+kLegacySize,
                                                   clientele::State::kSerializedSize)))
            return Format::Invalid;
        *output=next;return format;
    }
private:
    static void Put32(uint8_t* p,uint32_t value){
        for(unsigned i=0;i<4;++i)p[i]=static_cast<uint8_t>(value>>(i*8));
    }
    static uint32_t Get32(const uint8_t* p){
        uint32_t value=0;for(unsigned i=0;i<4;++i)value|=uint32_t(p[i])<<(i*8);return value;
    }
    static uint64_t Hash(const uint8_t* p,size_t count){
        uint64_t value=14695981039346656037ull;
        for(size_t i=0;i<count;++i)value=(value^p[i])*1099511628211ull;
        return value;
    }
};
}}}
#endif
