#include "../../main/jni/native/guess_the_door_game.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
using namespace reverie::doors;

static void Until(Game& game,Phase target,unsigned limit=100) {
    for(unsigned i=0;i<limit&&game.phase!=target;++i)game.Update(.1f);
    assert(game.phase==target);
}
static void PassDoor(Game& game) {
    const unsigned old=game.room;
    assert(game.Pull(int(game.current.safeSide)));
    assert((game.DrainSound()&LeverSound)!=0u);
    Until(game,Phase::Opening);
    assert((game.DrainSound()&DoorSound)!=0u);
    Until(game,Phase::Walking);
    assert((game.DrainSound()&StepSound)!=0u);
    for(int i=0;i<30&&game.phase==Phase::Walking;i++)game.Update(.1f);
    if(old+1u<kRooms)assert(game.room==old+1u&&game.phase==Phase::Ready);
}
int main() {
    const uint64_t seed=0xfee1dead12345ULL;
    bool observed[kFeatures][2]={{false}};
    for(unsigned seedIndex=0;seedIndex<80u;++seedIndex){
        const uint64_t value=Mix(seed+seedIndex);
        for(unsigned room=0;room<128u;++room){
            const Room a=Generate(value,room),b=Generate(value,room);
            assert(a.safeSide==b.safeSide);
            assert(a.clueFeature==b.clueFeature&&a.clueValue==b.clueValue);
            assert(a.clueFeature<kFeatures&&a.clueValue<2u);
            assert(a.wallVariant==b.wallVariant);
            assert(a.Matches(a.safeSide));
            assert(!a.Matches(1u-a.safeSide));
            assert(std::strcmp(a.Clue(),FeatureClue(a.clueFeature,a.clueValue))==0);
            assert(std::strlen(a.Clue())>10u);
            observed[a.clueFeature][a.clueValue]=true;
            for(unsigned side=0;side<2u;++side){
                const DoorDesign& x=a.doors[side],& y=b.doors[side];
                for(unsigned feature=0;feature<kFeatures;++feature)
                    assert(Value(x,feature)==Value(y,feature)&&Value(x,feature)<2u);
            }
        }
    }
    for(unsigned feature=0;feature<kFeatures;++feature)
        for(unsigned value=0;value<2u;++value)assert(observed[feature][value]);
    Game a,b;
    a.Reset(seed);b.Reset(seed);
    assert(a.current.safeSide==b.current.safeSide);
    assert(a.ClueVisible());
    for(int i=0;i<24;i++)a.AdvanceReady(.1f);
    assert(!a.ClueVisible());
    a.Update(-1.f);a.Update(NAN);a.AdvanceReady(NAN);
    assert(a.phase==Phase::Ready);
    assert(!a.Pull(-1)&&!a.Pull(2));
    assert(a.Pull(1-int(a.current.safeSide)));
    assert(!a.Pull(int(a.current.safeSide))); // No double lever.
    Until(a,Phase::Opening);
    Until(a,Phase::Falling);
    assert((a.DrainSound()&PitSound)!=0u);
    Until(a,Phase::Ready);
    assert(a.room==0u&&a.falls==1u&&a.seed==seed);
    assert(a.current.safeSide==b.current.safeSide);
    for(unsigned i=0;i<kRooms;i++)PassDoor(a);
    assert(a.phase==Phase::Victory);
    assert(a.clearCount==kRooms);
    assert((a.DrainSound()&VictorySound)!=0u);
    assert(!a.Pull(0));
    a.Reset(seed);
    assert(a.falls==0u&&a.room==0u&&a.phase==Phase::Ready);
    PassDoor(a);
    assert(a.room==1u);
    assert(a.Pull(1)); // The early-right-lever quote is seed-independent.
    assert(a.easterActive&&a.easterSeen);
    assert(!a.Pull(0));
    for(unsigned i=0;i<100;i++)a.Update(.1f);
    assert(a.easterSeen);
    std::puts("guess_the_door_game_test: feature-derived visual clues, deterministic doors, pitfalls, victory and quote PASS");
}
