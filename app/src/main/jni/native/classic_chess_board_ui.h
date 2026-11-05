#ifndef REVERIE_CLASSIC_CHESS_BOARD_UI_H
#define REVERIE_CLASSIC_CHESS_BOARD_UI_H
// Native Chess UI-only helpers: color-relative orientation, controlled
// square overlays and exact capture tallies. No side effects on game state.
#include "classic_board_games.h"
#include <string>
#include <utility>
namespace reverie { namespace chessui {
enum ThreatMode { Off=0, Neutral=1, Human=2, Opponent=3 };
inline int ScreenIndex(int screenX,int screenY,int humanColor) {
    if(screenX<0||screenX>=8||screenY<0||screenY>=8)return -1;
    if(humanColor==-1) {
        screenX=7-screenX;screenY=7-screenY;
    }
    return screenY*8+screenX;
}
inline std::pair<int,int> ScreenSquare(int square,int humanColor) {
    if(square<0||square>=64)return {-1,-1};
    const int file=square%8,rank=square/8;
    return humanColor==-1?std::make_pair(7-file,7-rank):
                          std::make_pair(file,rank);
}
inline int OverlayButton(float x,float y) {
    if(y<1.285f||y>1.555f||x< -1.455f||x>1.455f)
        return 0;
    return x<-.485f?Neutral:x<.485f?Human:Opponent;
}
inline bool Highlight(const boardgames::State &state,
                      const boardgames::State::Board &shown,
                      int square,int mode) {
    if(state.kind!=boardgames::Kind::Chess||
       square<0||square>=64)return false;
    if(mode==Human)return state.Attacked(shown,square,state.chessHumanColor);
    if(mode==Opponent)return state.Attacked(shown,square,-state.chessHumanColor);
    if(mode==Neutral)return !state.Attacked(shown,square,1)&&
                            !state.Attacked(shown,square,-1);
    return false;
}
inline std::string CapturedLine(const boardgames::State &state,
                                int capturer) {
    const auto &counts=capturer==1?state.chessCapturedByWhite:
                                     state.chessCapturedByBlack;
    std::string line=capturer==state.chessHumanColor?"YOU TOOK: ":"CPU TOOK: ";
    constexpr char piece[]={'?','P','N','B','R','Q'};
    for(int i=1;i<=5;++i) {
        if(i>1)line+=' ';
        line+=piece[i];
        line+=std::to_string(counts[i]);
    }
    return line;
}
}} // reverie::chessui
#endif
