#ifndef REVERIE_LANTERN_WARFRONT_STATE_H
#define REVERIE_LANTERN_WARFRONT_STATE_H
// War history is separate from immutable 1,000-terrain asset bytes.
// The first campaign allocates places exactly once, retains every
// assignment and timestamps orders. Every developed photo may ask the
// same historical snapshot, without secretly rerolling terrain.
#include "lantern_tile_library.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>
namespace reverie { namespace lantern { namespace war {
constexpr unsigned kMaxOrders=512u,kHeaderSize=40u,kOrderSize=32u;
constexpr size_t kMaxSaveBytes=kHeaderSize+tiles::kLibrarySize*2u+
                               kMaxOrders*kOrderSize+4u;
inline void Put16(std::vector<uint8_t>& b,size_t at,uint16_t v){
    b[at]=uint8_t(v);b[at+1]=uint8_t(v>>8);
}
inline void Put32(std::vector<uint8_t>& b,size_t at,uint32_t v){
    for(unsigned n=0;n<4;++n)b[at+n]=uint8_t(v>>(n*8u));
}
inline void Put64(std::vector<uint8_t>& b,size_t at,uint64_t v){
    for(unsigned n=0;n<8;++n)b[at+n]=uint8_t(v>>(n*8u));
}
inline uint64_t Read64(const uint8_t* b){
    uint64_t v=0;
    for(unsigned n=0;n<8;++n)v|=uint64_t(b[n])<<(n*8u);
    return v;
}
struct Campaign {
    bool started=false;
    uint32_t seed=0;
    int64_t started_at=0,current_at=0;
    std::array<uint16_t,tiles::kLibrarySize> tile_at_location{};
    std::vector<tiles::WarOrder> orders{};
    bool Start(uint32_t war_seed,int64_t start_at){
        if(started||start_at<0)return false;
        seed=war_seed;started_at=current_at=start_at;
        tile_at_location=tiles::AssignWarfront(seed);
        orders.clear();started=true;return true;
    }
    bool Advance(int64_t next_time){
        if(!started||next_time<current_at)return false;
        current_at=next_time;return true;
    }
    bool AddOrder(const tiles::WarOrder& order){
        if(!started||orders.size()>=kMaxOrders||
           order.unit_id==0||order.front_location>=tiles::kLibrarySize||
           order.strength==0||order.enter_at<started_at||
           order.enter_at>order.depart_at||
           order.from_anchor>=tiles::kAnchorCount||
           order.to_anchor>=tiles::kAnchorCount||
           static_cast<unsigned>(order.type)>
               static_cast<unsigned>(tiles::UnitType::FieldGun)||
           static_cast<unsigned>(order.activity)>
               static_cast<unsigned>(tiles::Activity::Conceal)||
           static_cast<unsigned>(order.doctrine)>
               static_cast<unsigned>(tiles::Doctrine::Dispersed)||
           (order.observer_from_west!=1&&order.observer_from_west!=-1))
            return false;
        for(const auto& past:orders){
            if(past.unit_id==order.unit_id &&
               order.enter_at<=past.depart_at&&
               past.enter_at<=order.depart_at)return false;
        }
        orders.push_back(order);return true;
    }
    bool AssignedTile(uint16_t location,uint16_t* tile_id)const{
        if(!started||!tile_id||location>=tiles::kLibrarySize)return false;
        *tile_id=tile_at_location[location];return true;
    }
    bool Snapshot(uint16_t location,int64_t captured_at,const tiles::Tile& tile,
                  std::vector<tiles::PlacedUnit>* output)const{
        if(!started||!output||location>=tiles::kLibrarySize||
           captured_at<started_at||captured_at>current_at||
           tile_at_location[location]!=tile.id)return false;
        return tiles::ResolveAt(tile,location,captured_at,orders,output);
    }
};
inline bool Encode(const Campaign& state,std::vector<uint8_t>* output){
    if(!output||!state.started||state.started_at<0||
       state.current_at<state.started_at||state.orders.size()>kMaxOrders||
       state.tile_at_location!=tiles::AssignWarfront(state.seed))return false;
    Campaign validation;
    if(!validation.Start(state.seed,state.started_at)||
       !validation.Advance(state.current_at))return false;
    for(const auto& order:state.orders)
        if(!validation.AddOrder(order))return false;
    const size_t total=kHeaderSize+tiles::kLibrarySize*2u+
                       state.orders.size()*kOrderSize+4u;
    std::vector<uint8_t> bytes(total,0u);
    bytes[0]='L';bytes[1]='W';bytes[2]='A';bytes[3]='R';
    Put16(bytes,4,1);Put16(bytes,6,kHeaderSize);
    Put32(bytes,8,state.seed);
    Put64(bytes,12,static_cast<uint64_t>(state.started_at));
    Put64(bytes,20,static_cast<uint64_t>(state.current_at));
    Put16(bytes,28,tiles::kLibrarySize);
    Put16(bytes,30,static_cast<uint16_t>(state.orders.size()));
    Put16(bytes,32,kOrderSize);
    Put32(bytes,36,static_cast<uint32_t>(total));
    for(size_t i=0;i<state.tile_at_location.size();++i)
        Put16(bytes,40+i*2,state.tile_at_location[i]);
    const size_t offset=kHeaderSize+tiles::kLibrarySize*2u;
    for(size_t i=0;i<state.orders.size();++i){
        const auto& o=state.orders[i];
        const size_t at=offset+i*kOrderSize;
        Put32(bytes,at,o.unit_id);
        Put16(bytes,at+4,o.front_location);
        bytes[at+6]=static_cast<uint8_t>(o.type);
        bytes[at+7]=static_cast<uint8_t>(o.activity);
        bytes[at+8]=static_cast<uint8_t>(o.doctrine);
        bytes[at+9]=o.from_anchor;bytes[at+10]=o.to_anchor;
        bytes[at+11]=o.strength;
        bytes[at+12]=static_cast<uint8_t>(o.observer_from_west);
        Put64(bytes,at+14,static_cast<uint64_t>(o.enter_at));
        Put64(bytes,at+22,static_cast<uint64_t>(o.depart_at));
    }
    Put32(bytes,total-4,tiles::Fnv(bytes.data(),total-4));
    *output=std::move(bytes);return true;
}
inline bool Decode(const uint8_t* bytes,size_t length,Campaign* output){
    if(!bytes||!output||length<kHeaderSize+tiles::kLibrarySize*2u+4u||
       length>kMaxSaveBytes||std::memcmp(bytes,"LWAR",4)!=0||
       tiles::Read16(bytes+4)!=1||tiles::Read16(bytes+6)!=kHeaderSize||
       tiles::Read16(bytes+28)!=tiles::kLibrarySize||
       tiles::Read16(bytes+32)!=kOrderSize||
       tiles::Read16(bytes+34)!=0||
       tiles::Read32(bytes+36)!=length||
       tiles::Fnv(bytes,length-4)!=tiles::Read32(bytes+length-4))
        return false;
    const uint16_t count=tiles::Read16(bytes+30);
    if(count>kMaxOrders||length!=kHeaderSize+tiles::kLibrarySize*2u+
                             size_t(count)*kOrderSize+4u)return false;
    const uint64_t start=Read64(bytes+12),now=Read64(bytes+20);
    if(start>uint64_t(std::numeric_limits<int64_t>::max())||
       now>uint64_t(std::numeric_limits<int64_t>::max())||now<start)
        return false;
    Campaign candidate;
    if(!candidate.Start(tiles::Read32(bytes+8),static_cast<int64_t>(start))||
       !candidate.Advance(static_cast<int64_t>(now)))return false;
    for(size_t i=0;i<candidate.tile_at_location.size();++i)
        if(candidate.tile_at_location[i]!=tiles::Read16(bytes+40+i*2))
            return false; // cannot remap places without starting a new war
    const size_t offset=kHeaderSize+tiles::kLibrarySize*2u;
    for(unsigned i=0;i<count;++i){
        const size_t at=offset+i*kOrderSize;
        if(bytes[at+13]||bytes[at+30]||bytes[at+31]||
           Read64(bytes+at+14)>uint64_t(std::numeric_limits<int64_t>::max())||
           Read64(bytes+at+22)>uint64_t(std::numeric_limits<int64_t>::max()))
            return false;
        tiles::WarOrder order;
        order.unit_id=tiles::Read32(bytes+at);
        order.front_location=tiles::Read16(bytes+at+4);
        order.type=static_cast<tiles::UnitType>(bytes[at+6]);
        order.activity=static_cast<tiles::Activity>(bytes[at+7]);
        order.doctrine=static_cast<tiles::Doctrine>(bytes[at+8]);
        order.from_anchor=bytes[at+9];order.to_anchor=bytes[at+10];
        order.strength=bytes[at+11];
        order.observer_from_west=static_cast<int8_t>(bytes[at+12]);
        order.enter_at=static_cast<int64_t>(Read64(bytes+at+14));
        order.depart_at=static_cast<int64_t>(Read64(bytes+at+22));
        if(!candidate.AddOrder(order))return false;
    }
    *output=std::move(candidate);
    return true;
}
}}} // reverie::lantern::war
#endif
