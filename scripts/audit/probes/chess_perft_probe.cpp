#include "classic_board_games.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <cctype>
using reverie::boardgames::State;
using reverie::boardgames::Kind;
State Fen(const std::string& fen){
    State s;s.Reset(Kind::Chess);s.cells.fill(0);s.chessHistory.clear();
    std::istringstream in(fen);std::string board,side,rights,ep;in>>board>>side>>rights>>ep;
    int i=0;
    for(char c:board){
        if(c=='/')continue;
        if(std::isdigit(static_cast<unsigned char>(c))){i+=c-'0';continue;}
        const std::string order=" pnbrqk";
        int p=static_cast<int>(order.find(static_cast<char>(std::tolower(static_cast<unsigned char>(c)))));
        if(p<1||p>6||i>=64)throw std::runtime_error("invalid FEN");
        s.cells[i++]=static_cast<int8_t>(std::islower(static_cast<unsigned char>(c))?-p:p);
    }
    s.side=side=="w"?1:-1;s.castles=0;
    for(char c:rights)s.castles|=c=='K'?1:c=='Q'?2:c=='k'?4:c=='q'?8:0;
    s.enPassant=ep=="-"?-1:(8-(ep[1]-'0'))*8+(ep[0]-'a');
    return s;
}
uint64_t Perft(const State& s,int depth){
    if(depth==0)return 1;
    const auto moves=s.ChessTurns();
    if(depth==1)return moves.size();
    uint64_t total=0;
    for(const auto& m:moves)total+=Perft(s.ChessAfter(m.from,m.to,m.promotion),depth-1);
    return total;
}
int main(){
    struct Row{const char* name;const char* fen;std::vector<uint64_t> expected;};
    const std::vector<Row> rows={
      {"initial","rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",{20,400,8902,197281}},
      {"kiwipete","r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",{48,2039,97862}},
      {"rook_pawn_endgame","8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",{14,191,2812,43238}},
      {"promotion_castling","r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -",{6,264,9467}},
      {"underpromotion","rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",{44,1486,62379}},
      {"middlegame","r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -",{46,2079,89890}}
    };
    int failures=0;
    for(const auto& row:rows){
        const State s=Fen(row.fen);
        for(size_t d=0;d<row.expected.size();d++){
            auto start=std::chrono::steady_clock::now();const auto count=Perft(s,static_cast<int>(d+1));
            auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
            std::cout<<row.name<<" depth="<<d+1<<" actual="<<count<<" expected="<<row.expected[d]<<" elapsed_ms="<<ms<<" result="<<(count==row.expected[d]?"PASS":"FAIL")<<std::endl;
            failures+=count!=row.expected[d];
        }
    }
    return failures?1:0;
}
