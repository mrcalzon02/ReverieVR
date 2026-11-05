#ifndef REVERIE_GUESS_THE_DOOR_GAME_H
#define REVERIE_GUESS_THE_DOOR_GAME_H
// Native game-private simulation. No Android, GL, system clock, or renderer dependencies.
// Every room is derived independently from the displayed root seed and room index.
#include <cstdint>
#include <cmath>

namespace reverie { namespace doors {
constexpr unsigned kRooms=12u;
constexpr unsigned kFeatures=5u;
enum Feature: unsigned { Material=0u, Bracing=1u, Rivets=2u, Handle=3u, Wear=4u };
struct DoorDesign {
    unsigned material=0,bracing=0,rivets=0,handle=0,wear=0;
};
inline unsigned Value(const DoorDesign& d,unsigned feature) {
    switch(feature){
    case Material:return d.material;
    case Bracing:return d.bracing;
    case Rivets:return d.rivets;
    case Handle:return d.handle;
    default:return d.wear;
    }
}
inline void SetValue(DoorDesign& d,unsigned feature,unsigned value) {
    value=value?1u:0u;
    switch(feature){
    case Material:d.material=value;break;
    case Bracing:d.bracing=value;break;
    case Rivets:d.rivets=value;break;
    case Handle:d.handle=value;break;
    default:d.wear=value;break;
    }
}
inline const char* FeatureClue(unsigned feature,unsigned value) {
    static const char* const clues[kFeatures][2]={
        {"FIND THE OAK PLANKS", "FIND THE IRON PLATING"},
        {"FIND THE PANELED DOOR", "FIND THE CROSS-BRACED DOOR"},
        {"FIND THE DOOR WITHOUT RIVETS", "FIND THE FOUR METAL RIVETS"},
        {"FIND THE RING PULL", "FIND THE LONG BAR HANDLE"},
        {"FIND THE UNDAMAGED DOOR", "FIND THREE DEEP SCRATCHES"}
    };
    return clues[feature<kFeatures?feature:Material][value?1u:0u];
}
inline uint64_t Mix(uint64_t x) {
    x+=0x9e3779b97f4a7c15ULL;
    x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
    x=(x^(x>>27))*0x94d049bb133111ebULL;
    return x^(x>>31);
}
inline uint64_t RoomSeed(uint64_t seed,unsigned room,uint64_t domain) {
    return Mix(seed^Mix(uint64_t(room)+0xd1b54a32d192ed03ULL)^domain);
}
inline DoorDesign MakeDoor(uint64_t seed,unsigned room,unsigned side) {
    DoorDesign d;
    const uint64_t s=side?0x9a9a9a9a9a9a9a9aULL:0x5151515151515151ULL;
    d.material=unsigned(RoomSeed(seed,room,s^0x1091ULL)&1ULL);
    d.bracing=unsigned(RoomSeed(seed,room,s^0x2092ULL)&1ULL);
    d.rivets=unsigned(RoomSeed(seed,room,s^0x3093ULL)&1ULL);
    d.handle=unsigned(RoomSeed(seed,room,s^0x4094ULL)&1ULL);
    d.wear=unsigned(RoomSeed(seed,room,s^0x5095ULL)&1ULL);
    return d;
}
enum class Phase: uint8_t { Ready, Pulling, Opening, Walking, Falling, Victory };
enum Sound: unsigned {
    NoSound=0u, LeverSound=1u, DoorSound=2u, StepSound=4u,
    PitSound=8u, VictorySound=16u
};
struct Room {
    unsigned safeSide=0u;
    unsigned clueFeature=0u,clueValue=0u;
    DoorDesign doors[2];
    unsigned wallVariant=0u;
    const char* Clue() const {return FeatureClue(clueFeature,clueValue);}
    bool Matches(unsigned side) const {
        return side<2u&&Value(doors[side],clueFeature)==clueValue;
    }
};
inline Room Generate(uint64_t seed,unsigned room) {
    Room r{};
    r.safeSide=unsigned(RoomSeed(seed,room,0x5a17ULL)&1ULL);
    r.clueFeature=unsigned(RoomSeed(seed,room,0x6a27ULL)%kFeatures);
    r.doors[0]=MakeDoor(seed,room,0u);
    r.doors[1]=MakeDoor(seed,room,1u);
    // The correct door's actual physical feature determines the clue.
    r.clueValue=Value(r.doors[r.safeSide],r.clueFeature);
    SetValue(r.doors[1u-r.safeSide],r.clueFeature,1u-r.clueValue);
    r.wallVariant=unsigned(RoomSeed(seed,room,0xaa67ULL)%5u);
    return r;
}

inline float Ease(float fraction) {
    const float t=fraction<0.f?0.f:(fraction>1.f?1.f:fraction);
    return t*t*(3.f-2.f*t);
}
struct Game {
    uint64_t seed=0x6a09e667f3bcc909ULL;
    unsigned room=0,falls=0,clearCount=0;
    Phase phase=Phase::Ready;
    float timer=0.f;
    int side=-1;               // Current lever choice
    unsigned pendingSound=NoSound;
    bool easterSeen=false,easterActive=false,secondStep=false;
    Room current=Generate(seed,0);

    void Reset(uint64_t nextSeed) {
        seed=nextSeed?nextSeed:0x6a09e667f3bcc909ULL;
        room=falls=clearCount=0u;
        phase=Phase::Ready;timer=0.f;side=-1;
        pendingSound=NoSound;easterSeen=easterActive=secondStep=false;
        current=Generate(seed,0u);
    }
    bool Pull(int selected) {
        if(phase!=Phase::Ready || (selected!=0 && selected!=1))return false;
        side=selected;timer=0.f;phase=Phase::Pulling;
        pendingSound|=LeverSound;
        easterActive=!easterSeen&&room==1u&&side==1;
        if(easterActive)easterSeen=true;
        return true;
    }
    void Update(float delta) {
        if(!std::isfinite(delta)||delta<=0.f)return;
        // The host supplies bounded deltas. Clamp again for direct tests.
        const float dt=delta>.1f?.1f:delta;
        if(phase==Phase::Ready||phase==Phase::Victory)return;
        timer+=dt;
        if(phase==Phase::Pulling&&timer>=.70f) {
            timer=0.f;phase=Phase::Opening;pendingSound|=DoorSound;
        }else if(phase==Phase::Opening&&timer>=1.50f) {
            timer=0.f;
            if(side==int(current.safeSide)){
                phase=Phase::Walking;pendingSound|=StepSound;secondStep=false;
            }else{
                phase=Phase::Falling;pendingSound|=PitSound;
            }
        }else if(phase==Phase::Walking) {
            if(timer>=.72f&&!secondStep){
                secondStep=true;pendingSound|=StepSound;
            }
            if(timer>=1.60f){
                ++clearCount;
                easterActive=false;timer=0.f;side=-1;
                if(++room>=kRooms){
                    phase=Phase::Victory;pendingSound|=VictorySound;
                }else{
                    current=Generate(seed,room);phase=Phase::Ready;
                }
            }
        }else if(phase==Phase::Falling&&timer>=1.80f){
            ++falls;room=clearCount=0u;timer=0.f;side=-1;
            easterActive=false;phase=Phase::Ready;
            current=Generate(seed,0u); // Exact same seed and safe route on retry.
        }
    }
    unsigned DrainSound(){const unsigned out=pendingSound;pendingSound=NoSound;return out;}
    float Progress() const {
        const float length=phase==Phase::Pulling?.70f:
            phase==Phase::Opening?1.50f:
            phase==Phase::Walking?1.60f:
            phase==Phase::Falling?1.80f:1.f;
        return Ease(timer/length);
    }
    bool ClueVisible() const {
        // Show a fleeting description of actual door features; geometry remains visible.
        return phase==Phase::Ready&&timer<2.30f;
    }
    // Ready clock advances for clue expiry, but never changes the layout.
    void AdvanceReady(float dt) {
        if(phase==Phase::Ready&&std::isfinite(dt)&&dt>0.f)
            timer=timer+(dt>.1f?.1f:dt);
    }
};
}} // namespace reverie::doors
#endif
