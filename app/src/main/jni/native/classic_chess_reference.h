#ifndef REVERIE_CLASSIC_CHESS_REFERENCE_H
#define REVERIE_CLASSIC_CHESS_REFERENCE_H
// Headset-offline reference reader: actual CC0 opening data, not fabricated
// engine predictions or external wiki pages. Shared by renderer and unit tests.
#include "classic_chess_book.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <vector>

namespace reverie { namespace chessref {
constexpr int kEntries=static_cast<int>(
    sizeof(chessbook::kOpeningLines)/sizeof(chessbook::kOpeningLines[0]));

struct Entry {
    std::string eco,name,san;
};
inline Entry Read(int index) {
    if(index<0||index>=kEntries)return {};
    const char *raw=chessbook::kOpeningLines[index];
    const char *first=std::strchr(raw,'\t');
    if(!first)return {};
    const char *second=std::strchr(first+1,'\t');
    if(!second)return {};
    return {std::string(raw,first-raw),
            std::string(first+1,second-first-1),
            std::string(second+1)};
}
inline std::string Square(int square) {
    if(square<0||square>=64)return "--";
    std::string result="a8";
    result[0]=static_cast<char>('a'+square%8);
    result[1]=static_cast<char>('8'-square/8);
    return result;
}
inline std::string PieceName(int piece) {
    const char *kind[]={"EMPTY","PAWN","KNIGHT","BISHOP","ROOK","QUEEN","KING"};
    const int absPiece=piece<0?-piece:piece;
    if(absPiece>6)return "UNKNOWN";
    if(!piece)return "EMPTY";
    return std::string(piece>0?"WHITE ":"BLACK ")+kind[absPiece];
}
// Hover can inspect either side. Enemy destinations are a hypothetical
// next-turn view: never expose an obsolete en-passant right to the wrong side.
inline std::vector<int> Destinations(const boardgames::State &board,int square) {
    std::vector<int> result;
    if(board.kind!=boardgames::Kind::Chess||square<0||square>=64||
       !board.cells[square]||board.finished)return result;
    boardgames::State view=board;
    view.side=boardgames::State::Sign(view.cells[square]);
    if(view.side!=board.side)view.enPassant=-1;
    for(int to=0;to<64;++to)if(view.ChessLegal(square,to))
        result.push_back(to);
    return result;
}
inline std::vector<int> Matching(const boardgames::State &board) {
    std::vector<int> matching;
    if(board.kind!=boardgames::Kind::Chess)return matching;
    const auto &history=board.chessSanHistory;
    for(int i=0;i<kEntries;++i){
        size_t depth=0,nextSize=0;const char *next=nullptr;
        if(chessbook::BookLineMatches(chessbook::kOpeningLines[i],
            history,history.size(),&depth,&next,&nextSize))
            matching.push_back(i);
    }
    return matching;
}
// Find deepest historically matching opening, even after an off-book move.
inline int Current(const boardgames::State &board) {
    if(board.kind!=boardgames::Kind::Chess)return -1;
    const auto &history=board.chessSanHistory;
    for(size_t prefix=history.size();;--prefix){
        int best=-1;size_t bestDepth=0;
        for(int i=0;i<kEntries;++i){
            size_t depth=0,nextSize=0;const char *next=nullptr;
            if(chessbook::BookLineMatches(chessbook::kOpeningLines[i],
                history,prefix,&depth,&next,&nextSize)&&
                depth>=bestDepth){best=i;bestDepth=depth;}
        }
        if(best>=0)return best;
        if(prefix==0)break;
    }
    return -1;
}
inline std::vector<std::string> Wrap(const std::string &text,
                                     size_t width=49) {
    std::vector<std::string> lines;
    if(width<1)return lines;
    size_t pos=0;
    while(pos<text.size()){
        while(pos<text.size()&&text[pos]==' ')++pos;
        if(pos>=text.size())break;
        size_t end=std::min(pos+width,text.size());
        if(end<text.size()){
            size_t cut=text.rfind(' ',end);
            if(cut!=std::string::npos&&cut>pos)end=cut;
        }
        lines.push_back(text.substr(pos,end-pos));
        pos=end;
    }
    return lines;
}
}}
#endif
