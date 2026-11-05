#ifndef REVERIE_GUESS_THE_DOOR_GAME_H
#define REVERIE_GUESS_THE_DOOR_GAME_H
// Native game-private simulation. No Android, GL, system clock, or renderer dependencies.
// Every room is derived independently from the displayed root seed and room index.
#include <cstdint>
#include <cmath>

namespace reverie { namespace doors {
constexpr unsigned kRooms=12u;
constexpr unsigned kSymbols=6u;
constexpr const char* kSymbolNames[kSymbols]={"SUN","MOON","KEY","EYE","STAR","WAVE"};

inline uint64_t Mix(uint64_t x) {
    x+=0x9e3779b97f4a7c15ULL;
    x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
    x=(x^(x>>27))*0x94d049bb133111ebULL;
    return x^(x>>31);
}
// Independent domains prevent new decoration fields changing existing route choices.
inline uint64_t RoomSeed(uint64_t seed,unsigned room,uint64_t domain) {
    return Mix(seed^Mix(uint64_t(room)+0xd1b54a32d192ed03ULL)^domain);
}
enum class Phase: uint8_t { Ready, Pulling, Opening, Walking, Falling, Victory };
enum Sound: unsigned {
    NoSound=0u, LeverSound=1u, DoorSound=2u, StepSound=4u,
    PitSound=8u, VictorySound=16u
};
struct Room {
    unsigned safeSide=0; // 0 = left, 1 = right
    unsigned clue=0,leftSymbol=0,rightSymbol=0;
    unsigned leftDoor=0,rightDoor=0,wallVariant=0;
};
inline Room Generate(uint64_t seed,unsigned room) {
    Room r{};
    r.safeSide=unsigned(RoomSeed(seed,room,0x5a17ULL)&1ULL);
    r.clue=unsigned(RoomSeed(seed,room,0x6a27ULL)%kSymbols);
    const unsigned other=(r.clue+1u+
        unsigned(RoomSeed(seed,room,0x7a37ULL)%(kSymbols-1u)))%kSymbols;
    r.leftSymbol=r.safeSide==0u?r.clue:other;
    r.rightSymbol=r.safeSide==1u?r.clue:other;
    r.leftDoor=unsigned(RoomSeed(seed,room,0x8a47ULL)%16u);
    r.rightDoor=unsigned(RoomSeed(seed,room,0x9a57ULL)%16u);
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
        // Give players a fleeting memory cue, then leave the two symbols visible.
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
