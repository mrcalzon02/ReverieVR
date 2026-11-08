#ifndef REVERIE_GUESS_THE_DOOR_SAVE_H
#define REVERIE_GUESS_THE_DOOR_SAVE_H

#include "guess_the_door_game.h"
#include <array>
#include <cstddef>

namespace reverie { namespace doors {
constexpr const char* kMazeSaveSlot="maze-v1.bin";
constexpr size_t kMazeSaveBytes=36u;
constexpr uint32_t kMaxSavedFalls=1000000000u;
using MazeSave=std::array<uint8_t,kMazeSaveBytes>;

inline uint32_t SaveWord(const uint8_t* bytes,size_t offset) {
    uint32_t result=0u;
    for(unsigned i=0;i<4u;++i)result|=uint32_t(bytes[offset+i])<<(8u*i);
    return result;
}
inline void SaveWord(uint8_t* bytes,size_t offset,uint32_t value) {
    for(unsigned i=0;i<4u;++i)bytes[offset+i]=uint8_t(value>>(8u*i));
}
inline uint32_t MazeSaveChecksum(const uint8_t* bytes) {
    uint32_t hash=2166136261u;
    for(size_t i=0;i<kMazeSaveBytes-4u;++i)hash=(hash^bytes[i])*16777619u;
    return hash;
}

// A confirmed lever commits its deterministic outcome. Animation is transient:
// reopening goes to the next safe room, room one after a fall, or victory.
// Saving during each animation phase therefore cannot erase a committed fall
// or require a second lever press to earn a previously confirmed correct choice.
inline bool EncodeMazeSave(const Game& game,MazeSave& out) {
    if(!game.seed || game.falls>kMaxSavedFalls || game.room>kRooms ||
       static_cast<unsigned>(game.phase)>static_cast<unsigned>(Phase::Victory))
        return false;
    unsigned room=game.room,falls=game.falls;
    if(game.phase==Phase::Victory) {
        if(room!=kRooms)return false;
    } else {
        if(room>=kRooms)return false;
        if(game.phase!=Phase::Ready) {
            if(game.side<0 || game.side>1)return false;
            if(unsigned(game.side)==Generate(game.seed,room).safeSide)++room;
            else {
                if(falls==kMaxSavedFalls)return false;
                room=0u;++falls;
            }
        }
    }
    MazeSave bytes{};
    SaveWord(bytes.data(),0u,0x31445447u); // GTD1
    SaveWord(bytes.data(),4u,1u);
    SaveWord(bytes.data(),8u,static_cast<uint32_t>(game.seed));
    SaveWord(bytes.data(),12u,static_cast<uint32_t>(game.seed>>32u));
    SaveWord(bytes.data(),16u,room);SaveWord(bytes.data(),20u,falls);
    SaveWord(bytes.data(),24u,game.easterSeen?1u:0u);
    SaveWord(bytes.data(),32u,MazeSaveChecksum(bytes.data()));
    out=bytes;
    return true;
}

inline bool DecodeMazeSave(const uint8_t* bytes,size_t size,Game& destination) {
    if(!bytes || size!=kMazeSaveBytes || SaveWord(bytes,0u)!=0x31445447u ||
       SaveWord(bytes,4u)!=1u || SaveWord(bytes,28u)!=0u ||
       SaveWord(bytes,32u)!=MazeSaveChecksum(bytes))return false;
    const uint64_t seed=uint64_t(SaveWord(bytes,8u)) |
        (uint64_t(SaveWord(bytes,12u))<<32u);
    const uint32_t room=SaveWord(bytes,16u),falls=SaveWord(bytes,20u),flags=SaveWord(bytes,24u);
    if(!seed || room>kRooms || falls>kMaxSavedFalls || flags>1u)return false;
    Game candidate;candidate.Reset(seed);
    candidate.room=candidate.clearCount=room;candidate.falls=falls;
    candidate.phase=room==kRooms?Phase::Victory:Phase::Ready;
    candidate.current=Generate(seed,room==kRooms?kRooms-1u:room);
    candidate.easterSeen=flags!=0u;
    destination=candidate;
    return true;
}
}}
#endif
