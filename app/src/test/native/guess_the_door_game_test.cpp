#include "../../main/jni/native/guess_the_door_game.h"
#include <cassert>
#include <cstdint>
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
    for(unsigned i=0;i<1000;i++){
        const Room a=Generate(seed,i),b=Generate(seed,i);
        assert(a.safeSide==b.safeSide&&a.clue==b.clue);
        assert(a.leftDoor==b.leftDoor&&a.rightDoor==b.rightDoor);
        assert(a.safeSide<=1u&&a.leftDoor<16u&&a.rightDoor<16u);
        assert(a.leftSymbol<kSymbols&&a.rightSymbol<kSymbols);
        assert(a.leftSymbol!=a.rightSymbol);
        assert((a.safeSide==0u?a.leftSymbol:a.rightSymbol)==a.clue);
        assert((a.safeSide==0u?a.rightSymbol:a.leftSymbol)!=a.clue);
    }
    assert(Generate(seed,4u).safeSide==Generate(seed,4u).safeSide);
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
    a.Pull(1); // The hidden early-right-lever quote is seed-independent.
    assert(a.easterActive&&a.easterSeen);
    assert(!a.Pull(0));
    for(unsigned i=0;i<100;i++)a.Update(.1f);
    assert(a.easterSeen);
    std::puts("guess_the_door_game_test: deterministic route/door symbols, pitfalls, victory and quote PASS");
}
