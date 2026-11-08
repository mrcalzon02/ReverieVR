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
    std::cout<<"Guess save codec: safe phases, committed outcomes, victory, bounds and corruption PASS\n";
}
