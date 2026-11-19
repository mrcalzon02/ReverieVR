#include "../../main/jni/native/guess_the_door_save.h"
#include <cassert>
#include <iostream>
using namespace reverie::doors;

static Game RoundTrip(const Game& game) {
    MazeSave bytes{};assert(EncodeMazeSave(game,bytes));
    Game restored;assert(DecodeMazeSave(bytes.data(),bytes.size(),restored));
    assert(restored.timer==0.f && restored.side==-1 && restored.pendingSound==NoSound);
    return restored;
}

/*
 * A wrong choice has one durable fall even when saved before the trap animation.
 * The retried route must use the same root puzzle; a successful choice commits
 * the next procedural room exactly once, without old animation/input/audio.
 */
static void TestWrongChoiceThenNextRoom() {
    const uint64_t seed=0x13579bdf2468ace0ULL;
    Game live;live.Reset(seed);
    const Room first=Generate(seed,0u);
    const int wrong=1-static_cast<int>(first.safeSide);
    assert(first.Matches(first.safeSide)&&!first.Matches(1u-first.safeSide));
    assert(live.Pull(wrong));
    assert(!live.Pull(static_cast<int>(first.safeSide))); // No queued correction.
    assert(live.DrainSound()==LeverSound);
    MazeSave interruptedFall{};assert(EncodeMazeSave(live,interruptedFall));
    Game resumed=RoundTrip(live);
    assert(resumed.seed==seed&&resumed.phase==Phase::Ready);
    assert(resumed.room==0u&&resumed.clearCount==0u&&resumed.falls==1u);
    assert(resumed.current.safeSide==first.safeSide);
    assert(resumed.current.clueFeature==first.clueFeature);
    assert(resumed.current.clueValue==first.clueValue);
    assert(resumed.current.Matches(first.safeSide));
    assert(resumed.DrainSound()==NoSound);
    MazeSave afterInterruptedFall{};
    assert(EncodeMazeSave(resumed,afterInterruptedFall));
    assert(afterInterruptedFall==interruptedFall); // Reload is not a second fall.

    for(unsigned tick=0;tick<100u&&live.phase!=Phase::Falling;++tick) {
        assert(!live.Pull(wrong)); // Old lever is not eligible during transition.
        live.Update(.1f);
    }
    assert(live.phase==Phase::Falling);
    assert(!live.Pull(wrong));
    for(unsigned tick=0;tick<100u&&live.phase!=Phase::Ready;++tick)
        live.Update(.1f);
    assert(live.phase==Phase::Ready&&live.room==0u&&live.falls==1u);
    assert(live.clearCount==0u&&live.side==-1);
    assert((live.DrainSound()&PitSound)!=0u);
    MazeSave settledFall{};assert(EncodeMazeSave(live,settledFall));
    assert(settledFall==interruptedFall);

    // Resume the failed room, choose the real clue, and save while the lever
    // animation is still playing. The codec commits room 1, not another fall.
    Game retry=RoundTrip(live);
    assert(retry.Pull(static_cast<int>(first.safeSide)));
    assert(!retry.Pull(wrong));
    assert(retry.DrainSound()==LeverSound);
    MazeSave committedPass{};assert(EncodeMazeSave(retry,committedPass));
    Game next=RoundTrip(retry);
    const Room generated=Generate(seed,1u);
    assert(next.seed==seed&&next.room==1u&&next.clearCount==1u);
    assert(next.falls==1u&&next.phase==Phase::Ready&&next.side==-1);
    assert(next.timer==0.f&&next.DrainSound()==NoSound);
    assert(next.current.safeSide==generated.safeSide);
    assert(next.current.clueFeature==generated.clueFeature);
    assert(next.current.clueValue==generated.clueValue);
    assert(next.current.wallVariant==generated.wallVariant);
    assert(next.current.Matches(generated.safeSide));
    assert(!next.current.Matches(1u-generated.safeSide));
    assert(next.ClueVisible());
    assert(!next.Pull(-1)&&!next.Pull(2)); // Invalid delayed input cannot act.
    for(unsigned tick=0;tick<40u;++tick)next.Update(.1f);
    assert(next.phase==Phase::Ready&&next.room==1u&&next.falls==1u);
    assert(next.DrainSound()==NoSound);
    MazeSave stableNext{};assert(EncodeMazeSave(next,stableNext));
    assert(stableNext==committedPass);
    Game again=RoundTrip(next);
    assert(again.room==1u&&again.falls==1u&&again.clearCount==1u);
    assert(again.current.clueFeature==generated.clueFeature);
    assert(again.current.clueValue==generated.clueValue);

    // The uninterrupted animation and committed recovery reach identical
    // durable next-room state; the next room still accepts a fresh lever.
    for(unsigned tick=0;tick<100u&&retry.phase!=Phase::Ready;++tick) {
        assert(!retry.Pull(static_cast<int>(first.safeSide)));
        retry.Update(.1f);
    }
    assert(retry.phase==Phase::Ready&&retry.room==1u);
    assert(retry.falls==1u&&retry.clearCount==1u);
    MazeSave animatedPass{};assert(EncodeMazeSave(retry,animatedPass));
    assert(animatedPass==committedPass);
    assert(again.Pull(static_cast<int>(generated.safeSide)));
    assert(!again.Pull(1-static_cast<int>(generated.safeSide)));
    assert(again.DrainSound()==LeverSound);
    std::cout<<"Guess door wrong-path fall, next-room clue and save replay PASS\n";
}

int main() {
    Game game;game.Reset(0x123456789abcdef0ULL);
    auto restored=RoundTrip(game);
    assert(restored.seed==game.seed && restored.room==0u && restored.falls==0u);
    game.room=4u;game.clearCount=4u;game.current=Generate(game.seed,game.room);
    assert(game.Pull(static_cast<int>(game.current.safeSide)));
    for(auto phase:{Phase::Pulling,Phase::Opening,Phase::Walking}) {
        game.phase=phase;restored=RoundTrip(game);
        assert(restored.room==5u && restored.clearCount==5u && restored.phase==Phase::Ready);
    }
    game.side=1-static_cast<int>(game.current.safeSide);game.falls=7u;
    for(auto phase:{Phase::Pulling,Phase::Opening,Phase::Falling}) {
        game.phase=phase;restored=RoundTrip(game);
        assert(restored.room==0u && restored.clearCount==0u && restored.falls==8u);
    }
    game.Reset(42u);game.room=kRooms-1u;game.clearCount=game.room;
    game.current=Generate(game.seed,game.room);game.easterSeen=true;
    assert(game.Pull(static_cast<int>(game.current.safeSide)));
    restored=RoundTrip(game);
    assert(restored.room==kRooms && restored.phase==Phase::Victory && restored.easterSeen);
    game=restored;MazeSave good{};assert(EncodeMazeSave(game,good));
    for(size_t index=0u;index<good.size();++index) {
        auto bad=good;bad[index]^=1u;Game unchanged;unchanged.Reset(99u);
        assert(!DecodeMazeSave(bad.data(),bad.size(),unchanged));
        assert(unchanged.seed==99u && unchanged.room==0u);
    }
    for(size_t length=0u;length<good.size();++length)
        assert(!DecodeMazeSave(good.data(),length,restored));
    for(auto field:{size_t(4u),size_t(16u),size_t(20u),size_t(24u),size_t(28u)}) {
        auto bad=good;SaveWord(bad.data(),field,0xffffffffu);
        SaveWord(bad.data(),32u,MazeSaveChecksum(bad.data()));
        assert(!DecodeMazeSave(bad.data(),bad.size(),restored));
    }
    game.Reset(17u);game.room=kRooms;assert(!EncodeMazeSave(game,good));
    game.Reset(17u);game.falls=kMaxSavedFalls;
    assert(game.Pull(1-static_cast<int>(game.current.safeSide)));
    assert(!EncodeMazeSave(game,good));
    TestWrongChoiceThenNextRoom();
    std::cout<<"Guess save codec: safe phases, committed outcomes, victory, bounds and corruption PASS\n";
}
