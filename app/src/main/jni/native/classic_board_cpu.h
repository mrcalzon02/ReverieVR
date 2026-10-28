#ifndef REVERIE_CLASSIC_BOARD_CPU_H
#define REVERIE_CLASSIC_BOARD_CPU_H

#include "classic_board_games.h"
#include <chrono>
#include <future>
#include <stdexcept>
#include <utility>

namespace reverie { namespace boardgames {
// Verify every off-thread chess state as a real legal one-ply transition,
// independently of the CPU search. A corrupt or stale worker result must
// never teleport a queen, skip a turn, or alter castling/EP rights on screen.
inline bool ValidChessCpuTransition(const State &before,const State &after) {
    if(before.kind!=Kind::Chess||before.side!=-1||before.finished||
       after.kind!=Kind::Chess)return false;
    if(after.ply==before.ply){
        // A valid non-move is only a genuinely claimable CPU draw.
        return before.ChessDrawAvailable(-1)&&after.finished&&
            after.winner==0&&after.side==-1&&
            after.cells==before.cells&&
            after.chessSanHistory==before.chessSanHistory&&
            after.chessHistory==before.chessHistory;
    }
    if(after.ply!=before.ply+1||after.side!=1||
       after.lastFrom<0||after.lastFrom>=64||
       after.lastTo<0||after.lastTo>=64||
       !before.ChessLegal(after.lastFrom,after.lastTo))return false;
    const int fromPiece=before.cells[after.lastFrom];
    const int targetPiece=after.cells[after.lastTo];
    const bool promotion=std::abs(fromPiece)==1 &&
        (after.lastTo/8==0||after.lastTo/8==7);
    const int requestedPromotion=promotion?std::abs(targetPiece):5;
    if(promotion && (requestedPromotion<2||requestedPromotion>5))
        return false;
    State expected=before;
    if(!expected.ChessMove(after.lastFrom,after.lastTo,requestedPromotion))
        return false;
    return expected.cells==after.cells&&expected.side==after.side&&
        expected.castles==after.castles&&
        expected.enPassant==after.enPassant&&
        expected.halfMoves==after.halfMoves&&
        expected.ply==after.ply&&
        expected.winner==after.winner&&
        expected.finished==after.finished&&
        expected.lastFrom==after.lastFrom&&
        expected.lastTo==after.lastTo&&
        expected.chessSanHistory==after.chessSanHistory&&
        expected.chessHistory==after.chessHistory;
}

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
        State produced=pending.get();
        if(board.kind==Kind::Chess &&
           !ValidChessCpuTransition(board,produced))
            throw std::runtime_error(
                "Rejected illegal or inconsistent chess CPU state");
        board=std::move(produced);
        return true;
    }
};
}}
#endif
