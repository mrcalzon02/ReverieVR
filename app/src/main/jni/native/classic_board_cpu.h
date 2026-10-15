#ifndef REVERIE_CLASSIC_BOARD_CPU_H
#define REVERIE_CLASSIC_BOARD_CPU_H

#include "classic_board_games.h"
#include <chrono>
#include <future>

namespace reverie { namespace boardgames {
// Game-private worker: owns its snapshot and never touches GL, host services,
// or the live board. The module commits a ready result only during Update.
// Future destruction joins before module destruction, so no detached work
// can outlive an unloaded native library.
class CpuWorker {
    std::future<State> pending;
public:
    bool Busy() const {return pending.valid();}
    bool Start(const State &board) {
        if(Busy()||board.side!=-1||board.finished)return false;
        pending=std::async(std::launch::async,[snapshot=board]() mutable {
            snapshot.clock=0.f;
            for(int i=0;i<3;i++)snapshot.Step(.1f);
            return snapshot;
        });
        return true;
    }
    bool Poll(State &board) {
        if(!Busy()||pending.wait_for(std::chrono::seconds(0))!=
                    std::future_status::ready)return false;
        board=pending.get();
        return true;
    }
};
}}
#endif
