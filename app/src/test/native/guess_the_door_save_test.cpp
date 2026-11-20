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
    // Room 1's genuine right-lever Easter voice may accompany the new lever;
    // it is not queued audio leaking from the failed room 0 transition.
    const unsigned expected=LeverSound | (generated.safeSide==1u?LeverVoice:NoSound);
    assert(again.DrainSound()==expected);
    std::cout<<"Guess door wrong-path fall, next-room clue and save replay PASS\n";
}


/*
 * Finishing the real twelve-room route is an award boundary, not a permanent
 * input/audio state. A distinct seed must start clean even if the victory
 * chime and the one-time room-one voice were never drained before Reset.
 * Exercise the checked save codec on both sides of that boundary.
 */
static void TestVictoryToFreshSeedCheckedRestart() {
    uint64_t oldSeed=0x1337beef10203040ULL;
    while(Generate(oldSeed,1u).safeSide!=1u)++oldSeed;
    uint64_t freshSeed=0xa5a5ccdd12345678ULL;
    while(Generate(freshSeed,1u).safeSide!=1u ||
          Generate(freshSeed,0u).clueFeature==Generate(oldSeed,0u).clueFeature)
        ++freshSeed;
    assert(oldSeed!=freshSeed);

    Game finished;finished.Reset(oldSeed);
    for(unsigned index=0u;index<kRooms;++index) {
        assert(finished.phase==Phase::Ready&&finished.room==index);
        const Room expected=Generate(oldSeed,index);
        assert(finished.current.Matches(expected.safeSide));
        assert(finished.Pull(static_cast<int>(expected.safeSide)));
        for(unsigned tick=0u;tick<100u &&
            finished.phase!=Phase::Ready&&finished.phase!=Phase::Victory;++tick)
            finished.Update(.1f);
        assert(finished.room==index+1u&&finished.clearCount==index+1u);
    }
    assert(finished.phase==Phase::Victory&&finished.room==kRooms);
    assert(finished.falls==0u&&finished.easterSeen);
    assert((finished.pendingSound&VictorySound)!=0u);
    assert((finished.pendingSound&LeverVoice)!=0u);

    MazeSave victoryBytes{};assert(EncodeMazeSave(finished,victoryBytes));
    Game checkedVictory=RoundTrip(finished);
    assert(checkedVictory.phase==Phase::Victory&&checkedVictory.room==kRooms);
    assert(checkedVictory.clearCount==kRooms&&checkedVictory.easterSeen);
    assert(checkedVictory.DrainSound()==NoSound);
    Game audible=finished;
    const unsigned oldAudio=audible.DrainSound();
    assert((oldAudio&VictorySound)!=0u&&(oldAudio&LeverVoice)!=0u);
    assert(audible.DrainSound()==NoSound); // Each old cue drains only once.

    // Reset while the real victory/voice bits are still pending.
    finished.Reset(freshSeed);
    assert(finished.seed==freshSeed&&finished.phase==Phase::Ready);
    assert(finished.room==0u&&finished.clearCount==0u&&finished.falls==0u);
    assert(!finished.easterSeen&&!finished.easterActive&&!finished.secondStep);
    assert(finished.timer==0.f&&finished.side==-1);
    assert(finished.DrainSound()==NoSound);
    const Room first=Generate(freshSeed,0u);
    assert(finished.current.safeSide==first.safeSide);
    assert(finished.current.clueFeature==first.clueFeature);
    assert(finished.current.clueValue==first.clueValue);
    assert(finished.current.Matches(first.safeSide)&&finished.ClueVisible());
    MazeSave newBytes{};assert(EncodeMazeSave(finished,newBytes));
    assert(newBytes!=victoryBytes);

    Game resumed=RoundTrip(finished);
    MazeSave checkedNewBytes{};assert(EncodeMazeSave(resumed,checkedNewBytes));
    assert(checkedNewBytes==newBytes);
    assert(resumed.seed==freshSeed&&resumed.phase==Phase::Ready);
    assert(resumed.room==0u&&resumed.clearCount==0u&&resumed.falls==0u);
    assert(!resumed.easterSeen&&!resumed.easterActive&&!resumed.secondStep);
    assert(resumed.DrainSound()==NoSound);
    checkedVictory.Reset(freshSeed);
    MazeSave resetAfterReload{};assert(EncodeMazeSave(checkedVictory,resetAfterReload));
    assert(resetAfterReload==newBytes&&checkedVictory.DrainSound()==NoSound);

    // The new seed accepts a genuine choice and awards only its own rooms.
    assert(resumed.Pull(static_cast<int>(first.safeSide)));
    assert(resumed.DrainSound()==LeverSound); // No stale victory or quote.
    for(unsigned tick=0u;tick<100u&&resumed.phase!=Phase::Ready;++tick)
        resumed.Update(.1f);
    assert(resumed.phase==Phase::Ready&&resumed.room==1u);
    assert(resumed.clearCount==1u&&resumed.falls==0u);
    assert((resumed.DrainSound()&(VictorySound|LeverVoice))==NoSound);
    MazeSave afterOne{};assert(EncodeMazeSave(resumed,afterOne));
    Game second=RoundTrip(resumed);
    assert(second.room==1u&&second.clearCount==1u&&!second.easterSeen);
    assert(second.current.Matches(second.current.safeSide));
    assert(second.current.safeSide==1u);
    assert(second.Pull(1));
    assert(second.DrainSound()==(LeverSound|LeverVoice));
    assert(second.DrainSound()==NoSound);
    for(unsigned tick=0u;tick<100u&&second.phase!=Phase::Ready;++tick)
        second.Update(.1f);
    assert(second.phase==Phase::Ready&&second.room==2u);
    assert(second.clearCount==2u&&second.falls==0u&&second.easterSeen);
    assert((second.DrainSound()&VictorySound)==NoSound);
    std::cout<<"Guess door genuine victory, fresh seed checked reset and new award/audio PASS\n";
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
    TestVictoryToFreshSeedCheckedRestart();
    std::cout<<"Guess save codec: safe phases, committed outcomes, victory, bounds and corruption PASS\n";
}
