#ifndef REVERIE_CLASSIC_CHESS_MOVE_TWEEN_H
#define REVERIE_CLASSIC_CHESS_MOVE_TWEEN_H

// Game-private visual interpolation only. Board rules, saved positions, CPU
// legality, capture and promotion remain owned by boardgames::State.
#include "classic_board_games.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace reverie { namespace chesstween {
struct Move {
    using Board=boardgames::State::Board;
    Board before{};
    int from=-1,to=-1,rookw=-1,rookTo=-1;
    int moving=0,rookPiece=0,capturedSquare=-1,captured=0;
    float elapsed=0.f,travel=0.f,hold=.22f;
    bool active=false;

    void Clear() {
        active=false;from=to=rookw=rookTo=-1;
        moving=rookPiece=capturedSquare=captured=0;
        elapsed=travel=0.f;hold=.22f;
    }
    bool Begin(const Board &previous,const boardgames::State &after,
               int previousPly) {
        Clear();
        if(after.kind!=boardgames::Kind::Chess ||
           after.ply!=previousPly+1 || after.lastFrom<0 ||
           after.lastFrom>=64 || after.lastTo<0 ||
           after.lastTo>=64)return false;
        const int f=after.lastFrom,t=after.lastTo,piece=previous[f];
        if(!piece || boardgames::State::Sign(piece)==
                          boardgames::State::Sign(previous[t]) ||
           boardgames::State::Sign(piece)!=-after.side ||
           boardgames::State::Sign(after.cells[t])!=
                          boardgames::State::Sign(piece))return false;
        before=previous;from=f;to=t;moving=piece;
        if(previous[t]!=0) {
            capturedSquare=t;captured=previous[t];
        } else if(std::abs(piece)==1 && f%8!=t%8) {
            // En passant removes the pawn beside the moving pawn, not
            // the empty destination; keep it visible until impact.
            const int adjacent=f/8*8+t%8;
            if(previous[adjacent]==-boardgames::State::Sign(piece)) {
                capturedSquare=adjacent;captured=previous[adjacent];
            }
        }
        if(std::abs(piece)==6 && std::abs(f%8-t%8)==2) {
            const int rank=f/8;
            rookw=rank*8+(t%8>f%8?7:0);
            rookTo=rank*8+(t%8>f%8?5:3);
            rookPiece=previous[rookw];
        }
        const float dx=static_cast<float>(f%8-t%8);
        const float dy=static_cast<float>(f/8-t/8);
        const float squares=std::sqrt(dx*dx+dy*dy);
        // Long queen/rook sweeps remain readable; captures take longer.
        travel=std::clamp(.43f+.13f*squares+(captured? .24f:0.f),
                          .64f,1.45f);
        hold=captured?.32f:.20f;
        active=true;
        return true;
    }
    bool Active() const {return active;}
    bool Impacted() const {return active && elapsed>=travel;}
    bool InFlight() const {return active && !Impacted();}
    void Advance(float dt) {
        if(!active||!std::isfinite(dt)||dt<=0.f)return;
        elapsed+=std::min(dt,.1f);
        if(elapsed>=travel+hold)active=false;
    }
    float Fraction() const {
        if(!active || travel<=0.f)return 1.f;
        const float t=std::clamp(elapsed/travel,0.f,1.f);
        // Smoothstep: gentle acceleration/deceleration, not a teleport.
        return t*t*(3.f-2.f*t);
    }
    int PieceAt(const Board &committed,int cell) const {
        if(!InFlight())return committed[cell];
        if(cell==from||cell==rookw)return 0;
        return before[cell];  // Captured pawn/piece persists until arrival.
    }
    float FileAt(int f,int t) const {
        return static_cast<float>(f%8)+
            (static_cast<float>(t%8-f%8))*Fraction();
    }
    float RankAt(int f,int t) const {
        return static_cast<float>(f/8)+
            (static_cast<float>(t/8-f/8))*Fraction();
    }
};
}} // reverie::chesstween
#endif
