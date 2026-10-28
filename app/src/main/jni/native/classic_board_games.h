#ifndef REVERIE_CLASSIC_BOARD_GAMES_H
#define REVERIE_CLASSIC_BOARD_GAMES_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <limits>
#include <utility>

namespace reverie { namespace boardgames {

enum class Kind : uint8_t { Checkers=3, Go=4, Chess=5, Reversi=7 };
struct State {
    // Chess: White human (+1) at ranks 1-2, Black CPU (-1) at 7-8.
    // Go: Black human (+1), White CPU (-1). The sign is game-specific.
    // Go is full 19x19, chess and checkers retain standard 8x8.
    static constexpr int kGoSize=19,kCapacity=kGoSize*kGoSize;
    using Board=std::array<int8_t,kCapacity>;
    struct GoBoardHash {
        size_t operator()(const Board &b) const noexcept {
            uint64_t hash=1469598103934665603ull;
            for(int8_t cell:b){hash^=static_cast<uint8_t>(cell+1);
                hash*=1099511628211ull;}
            return static_cast<size_t>(hash);
        }
    };
    // Position identity includes side, castling and only effective
    // en-passant rights (FIDE Article 9.2).
    struct ChessPosition {
        std::array<int8_t,64> pieces{};
        int8_t side=1,castles=0,enPassant=-1;
        bool operator==(const ChessPosition &other) const noexcept {
            return pieces==other.pieces&&side==other.side&&
                castles==other.castles&&enPassant==other.enPassant;
        }
    };
    struct ChessHash {
        size_t operator()(const ChessPosition &p) const noexcept {
            uint64_t hash=1469598103934665603ull;
            for(int8_t cell:p.pieces) {
                hash^=static_cast<uint8_t>(cell+8);hash*=1099511628211ull;
            }
            hash^=static_cast<uint8_t>(p.side+1);hash*=1099511628211ull;
            hash^=static_cast<uint8_t>(p.castles);hash*=1099511628211ull;
            hash^=static_cast<uint8_t>(p.enPassant+1);
            return static_cast<size_t>(hash);
        }
    };
    std::unordered_map<ChessPosition,int,ChessHash> chessHistory;
    // True standard algebraic notation for human AND CPU half-moves.
    // Pure position searches create independent States and never append here.
    std::vector<std::string> chessSanHistory;
    // WCDF positions are distinguished by pieces/kings and side to move.
    std::unordered_map<ChessPosition,int,ChessHash> checkerHistory;
    int checkerQuietPlies=0;
    int halfMoves=0,promotionChoice=5;
    int chessDifficulty=1,chessSearchNodes=0;
    int checkerDifficulty=1,checkerSearchNodes=0;
    int reversiSearchNodes=0;
    Kind kind=Kind::Checkers;
    Board cells{};
    std::unordered_set<Board,GoBoardHash> goHistory;
    // Review and mark dead stones before accepting Chinese area scoring.
    std::array<uint8_t,kCapacity> goDead{};
    bool goReview=false;
    // Preserve turn order while Black owns the scoring-review controls.
    int goResumeSide=1;
    int goReviewRevision=0;
    float goBlackScore=0.f,goWhiteScore=0.f;
    int side=1, winner=0, selected=-1, forced=-1, ply=0, passes=0;
    int enPassant=-1, castles=15, lastFrom=-1, lastTo=-1;
    bool finished=false;
    float clock=0.f;
    uint32_t seed=0x7e51bd03u;
    int Size() const { return kind==Kind::Go?kGoSize:8; }
    int Index(int x,int y) const {return y*Size()+x;}
    bool Inside(int x,int y) const {return x>=0 && y>=0 && x<Size() && y<Size();}
    uint32_t Random(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
    static int Sign(int n){return n>0?1:(n<0?-1:0);}
    void Reset(Kind type) {
        kind=type;cells.fill(0);goHistory.clear();goDead.fill(0);
        goReview=false;goResumeSide=1;goReviewRevision=0;
        goBlackScore=goWhiteScore=0.f;chessHistory.clear();chessSanHistory.clear();
        checkerHistory.clear();checkerQuietPlies=0;
        halfMoves=0;promotionChoice=5;side=1;winner=0;
        selected=forced=-1;ply=passes=0;enPassant=-1;castles=15;
        lastFrom=lastTo=-1;clock=0;finished=false;
        if(type==Kind::Checkers){
            for(int y=0;y<3;y++)for(int x=0;x<8;x++)
                if((x+y)%2)cells[Index(x,y)]=-1;
            for(int y=5;y<8;y++)for(int x=0;x<8;x++)
                if((x+y)%2)cells[Index(x,y)]=1;
            checkerHistory.emplace(CurrentCheckerPosition(),1);
        } else if(type==Kind::Chess) {
            static const int8_t back[8]={4,2,3,5,6,3,2,4};
            for(int x=0;x<8;x++){
                cells[Index(x,0)]=-back[x];cells[Index(x,1)]=-1;
                cells[Index(x,6)]=1;cells[Index(x,7)]=back[x];
            }
            chessHistory.emplace(CurrentChessPosition(),1);
        } else if(type==Kind::Go) {
            goHistory.insert(cells);
        } else if(type==Kind::Reversi) {
            cells[Index(3,3)]=-1;cells[Index(4,4)]=-1;
            cells[Index(4,3)]=1;cells[Index(3,4)]=1;
        }
    }

    // Full 8x8 Reversi, capturing every enclosed opposing line.
    // Invalid placements leave both the board and output unchanged.
    static int ReversiFlips(const Board &board,int at,int player,
                            Board *after=nullptr) {
        if(at<0||at>=64||board[at]!=0||(player!=1&&player!=-1))
            return 0;
        constexpr int dx[8]={-1,0,1,-1,1,-1,0,1};
        constexpr int dy[8]={-1,-1,-1,0,0,1,1,1};
        std::array<int,64> flipped{};
        int count=0;
        const int x=at%8,y=at/8;
        for(int d=0;d<8;d++){
            const int start=count;
            int xx=x+dx[d],yy=y+dy[d];
            while(xx>=0&&xx<8&&yy>=0&&yy<8 &&
                  board[yy*8+xx]==-player){
                flipped[count++]=yy*8+xx;
                xx+=dx[d];yy+=dy[d];
            }
            if(xx<0||xx>=8||yy<0||yy>=8||
               board[yy*8+xx]!=player)count=start;
        }
        if(count>0&&after){
            *after=board;
            (*after)[at]=static_cast<int8_t>(player);
            for(int i=0;i<count;i++)
                (*after)[flipped[i]]=static_cast<int8_t>(player);
        }
        return count;
    }
    static bool HasReversiMove(const Board &board,int player){
        for(int i=0;i<64;i++)
            if(ReversiFlips(board,i,player)>0)return true;
        return false;
    }
    int ReversiCount(int player) const {
        int count=0;
        for(int i=0;i<64;i++)if(cells[i]==player)++count;
        return count;
    }
    void FinishReversi(){
        const int black=ReversiCount(1),white=ReversiCount(-1);
        winner=black>white?1:white>black?-1:0;
        finished=true;selected=-1;
    }
    bool ReversiMove(int at) {
        if(kind!=Kind::Reversi||finished)return false;
        Board next{};
        if(ReversiFlips(cells,at,side,&next)==0)return false;
        cells=next;lastFrom=-1;lastTo=at;selected=-1;++ply;
        side=-side;
        if(HasReversiMove(cells,side)){passes=0;return true;}
        // A player with no legal move must pass. If neither side can
        // move, finish by actual disc counts, even with empty squares.
        passes=1;side=-side;
        if(!HasReversiMove(cells,side)){
            passes=2;FinishReversi();
        }
        return true;
    }
    // White-positive CPU evaluation. Corners and mobility outrank
    // an early lead in raw disc count.
    static int ReversiValue(const Board &board){
        static constexpr int weights[64]={
            120,-25, 20,  5,  5, 20,-25,120,
            -25,-45, -5, -5, -5, -5,-45,-25,
             20, -5, 12,  3,  3, 12, -5, 20,
              5, -5,  3,  3,  3,  3, -5,  5,
              5, -5,  3,  3,  3,  3, -5,  5,
             20, -5, 12,  3,  3, 12, -5, 20,
            -25,-45, -5, -5, -5, -5,-45,-25,
            120,-25, 20,  5,  5, 20,-25,120
        };
        int value=0;
        for(int i=0;i<64;i++)
            if(board[i]!=0)value+=-board[i]*(weights[i]+2);
        int whiteMoves=0,blackMoves=0;
        for(int i=0;i<64;i++)if(board[i]==0){
            whiteMoves+=ReversiFlips(board,i,-1)>0;
            blackMoves+=ReversiFlips(board,i,1)>0;
        }
        return value+(whiteMoves-blackMoves)*5;
    }
    static int ReversiSearch(const Board &board,int player,int depth,
                             int alpha,int beta,int &nodes,int budget,
                             bool &exhausted){
        if(nodes>=budget){exhausted=true;return ReversiValue(board);}
        ++nodes;
        const bool hasMove=HasReversiMove(board,player);
        if(!hasMove&&!HasReversiMove(board,-player)){
            int white=0,black=0;
            for(int i=0;i<64;i++){
                white+=board[i]==-1;black+=board[i]==1;
            }
            return (white-black)*10000;
        }
        if(depth<=0)return ReversiValue(board);
        if(!hasMove)return ReversiSearch(board,-player,depth-1,
                                        alpha,beta,nodes,budget,exhausted);
        const bool maximize=player==-1;
        int best=maximize?-3000000:3000000;
        for(int at=0;at<64;at++){
            Board next{};
            if(!ReversiFlips(board,at,player,&next))continue;
            const int score=ReversiSearch(next,-player,depth-1,alpha,beta,
                                          nodes,budget,exhausted);
            if(exhausted)break;
            if(maximize){best=std::max(best,score);alpha=std::max(alpha,best);}
            else {best=std::min(best,score);beta=std::min(beta,best);}
            if(beta<=alpha)break;
        }
        return best;
    }
    bool PlayReversiCpuTurn(){
        if(kind!=Kind::Reversi||side!=-1||finished)return false;
        reversiSearchNodes=0;
        if(!HasReversiMove(cells,-1)){
            side=1;passes=1;
            if(!HasReversiMove(cells,1)){
                passes=2;FinishReversi();
            }
            return true;
        }
        int best=-1;
        for(int i=0;i<64;i++)if(ReversiFlips(cells,i,-1)>0){
            best=i;break;
        }
        const int budget=4000;
        for(int depth=1;depth<=4;depth++){
            int candidate=best,scoreBest=-3000000,alpha=-3000000;
            bool exhausted=false;
            for(int at=0;at<64;at++){
                Board next{};
                if(!ReversiFlips(cells,at,-1,&next))continue;
                const int score=ReversiSearch(next,1,depth-1,alpha,3000000,
                                             reversiSearchNodes,budget,exhausted);
                if(exhausted)break;
                if(score>scoreBest){scoreBest=score;candidate=at;}
                alpha=std::max(alpha,scoreBest);
            }
            if(exhausted)break;
            best=candidate;
        }
        return ReversiMove(best);
    }

    // WCDF / English draughts (8x8) requires correct move progress and
    // repetition for valid 40-move-each / third-occurrence claims.
    static ChessPosition CheckerPositionAfter(const Board &board,
                                               int playerToMove) {
        ChessPosition p;
        std::copy_n(board.begin(),64,p.pieces.begin());
        p.side=static_cast<int8_t>(playerToMove);
        return p;
    }
    ChessPosition CurrentCheckerPosition() const {
        return CheckerPositionAfter(cells,side);
    }
    int CheckerRepetitions() const {
        auto it=checkerHistory.find(CurrentCheckerPosition());
        return it==checkerHistory.end()?0:it->second;
    }
    bool CheckerProspectiveRepetition() const {
        if(kind!=Kind::Checkers||finished||forced>=0)return false;
        // WCDF 1.32.1 allows a claim before a third identical
        // occurrence, if the next legal complete turn will create it.
        // Multiple jumps must be evaluated as a single turn.
        bool repeated=false;
        for(const auto &entry:checkerHistory)if(entry.second>=2){
            repeated=true;break;
        }
        if(!repeated)return false;
        for(const auto &turn:CheckerTurns(cells,side)) {
            const auto next=CheckerPositionAfter(turn.after,-side);
            auto it=checkerHistory.find(next);
            if(it!=checkerHistory.end()&&it->second>=2)return true;
        }
        return false;
    }
    bool CheckerDrawAvailable(int player) const {
        return kind==Kind::Checkers&&!finished&&side==player&&
            forced<0&&(checkerQuietPlies>=80||
                       CheckerRepetitions()>=3||
                       CheckerProspectiveRepetition());
    }
    bool CheckerDrawClaimable() const {
        return CheckerDrawAvailable(1);
    }
    bool ClaimCheckerDraw() {
        if(!CheckerDrawClaimable())return false;
        finished=true;winner=0;return true;
    }

    // American/English checkers: forced captures, continuation, crowning ends turn.
    bool CheckersCapture(int from,int to) const {
        if(from<0||to<0||from>=64||to>=64)return false;
        const int p=cells[from],dx=to%8-from%8,dy=to/8-from/8;
        if(Sign(p)!=side||cells[to]!=0||std::abs(dx)!=2||std::abs(dy)!=2)
            return false;
        if(std::abs(p)==1 && Sign(dy)!=-side)return false;
        const int mid=(from+to)/2;
        return Sign(cells[mid])==-side;
    }
    bool CheckersStep(int from,int to) const {
        if(from<0||to<0||from>=64||to>=64)return false;
        const int p=cells[from],dx=to%8-from%8,dy=to/8-from/8;
        return Sign(p)==side && cells[to]==0 && std::abs(dx)==1
            && std::abs(dy)==1 && (std::abs(p)==2 || Sign(dy)==-side);
    }
    bool HasCapture(int player,int only=-1) const {
        for(int from=0;from<64;from++){
            if(from!=only && only>=0)continue;
            if(Sign(cells[from])!=player)continue;
            const int x=from%8,y=from/8;
            for(int dx : {-2,2})for(int dy : {-2,2}){
                int nx=x+dx,ny=y+dy;
                if(nx<0||ny<0||nx>=8||ny>=8)continue;
                const int t=ny*8+nx;
                if(cells[t]!=0||Sign(cells[(from+t)/2])!=-player)continue;
                if(std::abs(cells[from])==2 || Sign(dy)==-player)return true;
            }
        }
        return false;
    }
    bool HasCheckerMove(int player) const {
        if(HasCapture(player))return true;
        for(int from=0;from<64;from++)if(Sign(cells[from])==player){
            int x=from%8,y=from/8;
            for(int dx : {-1,1})for(int dy : {-1,1}){
                int nx=x+dx,ny=y+dy;
                if(nx>=0&&nx<8&&ny>=0&&ny<8&&cells[ny*8+nx]==0 &&
                   (std::abs(cells[from])==2||Sign(dy)==-player))
                    return true;
            }
        }
        return false;
    }
    bool CheckerMove(int from,int to) {
        if(kind!=Kind::Checkers||finished||from<0||to<0||from>=64||to>=64 ||
           (forced>=0&&from!=forced))return false;
        const bool capture=CheckersCapture(from,to);
        if(!capture && (forced>=0||HasCapture(side)||!CheckersStep(from,to)))
            return false;
        const int old=cells[from];
        // Only completed king-only noncapturing turns contribute to the
        // 40 moves per side with no man advancement or captured piece.
        checkerQuietPlies=(capture||std::abs(old)==1)
            ?0:checkerQuietPlies+1;
        cells[to]=cells[from];cells[from]=0;
        if(capture)cells[(from+to)/2]=0;
        const bool crown=std::abs(old)==1 &&
            (to/8==(side==1?0:7));
        if(crown)cells[to]=static_cast<int8_t>(side*2);
        lastFrom=from;lastTo=to;selected=-1;++ply;
        if(capture&&!crown&&HasCapture(side,to)){forced=to;return true;}
        forced=-1;side=-side;
        ++checkerHistory[CurrentCheckerPosition()];
        if(!HasCheckerMove(side)){finished=true;winner=-side;}
        return true;
    }

    // Complete-turn English draughts move generation for the CPU. A move
    // includes every mandatory jump before the opponent can reply. This is
    // independent of the GUI's one-landing-at-a-time controller interaction.
    struct CheckerTurn {
        Board after{};
        int from=-1,firstTo=-1;
    };
    static void ContinueCheckerJumps(const Board &board,int at,int player,
                                     int rootFrom,int firstTo,
                                     std::vector<CheckerTurn>& out){
        const int piece=board[at],x=at%8,y=at/8;
        bool advanced=false;
        for(int dy : {-2,2})for(int dx : {-2,2}){
            if(std::abs(piece)==1&&Sign(dy)!=-player)continue;
            const int tx=x+dx,ty=y+dy;
            if(tx<0||tx>=8||ty<0||ty>=8)continue;
            const int target=ty*8+tx,mid=(y+dy/2)*8+x+dx/2;
            if(board[target]!=0||Sign(board[mid])!=-player)continue;
            Board after=board;
            after[target]=after[at];after[at]=0;after[mid]=0;
            const bool crowned=std::abs(piece)==1&&
                ty==(player==1?0:7);
            if(crowned)after[target]=static_cast<int8_t>(player*2);
            advanced=true;
            if(crowned)out.push_back({after,rootFrom,firstTo});
            else ContinueCheckerJumps(after,target,player,rootFrom,firstTo,
                                      out);
        }
        if(!advanced)out.push_back({board,rootFrom,firstTo});
    }
    static std::vector<CheckerTurn> CheckerTurns(const Board &board,
                                                 int player,int forced=-1){
        std::vector<CheckerTurn> jumps;
        for(int from=0;from<64;from++){
            if((forced>=0&&forced!=from)||Sign(board[from])!=player)continue;
            const int piece=board[from],x=from%8,y=from/8;
            for(int dy : {-2,2})for(int dx : {-2,2}){
                if(std::abs(piece)==1&&Sign(dy)!=-player)continue;
                const int tx=x+dx,ty=y+dy;
                if(tx<0||tx>=8||ty<0||ty>=8)continue;
                const int to=ty*8+tx,mid=(y+dy/2)*8+x+dx/2;
                if(board[to]!=0||Sign(board[mid])!=-player)continue;
                Board after=board;
                after[to]=after[from];after[from]=0;after[mid]=0;
                const bool crown=std::abs(piece)==1&&
                    ty==(player==1?0:7);
                if(crown){
                    after[to]=static_cast<int8_t>(player*2);
                    jumps.push_back({after,from,to});
                }else ContinueCheckerJumps(after,to,player,from,to,jumps);
            }
        }
        if(!jumps.empty()||forced>=0)return jumps;
        std::vector<CheckerTurn> steps;
        for(int from=0;from<64;from++)if(Sign(board[from])==player){
            const int piece=board[from],x=from%8,y=from/8;
            for(int dy : {-1,1})for(int dx : {-1,1}){
                if(std::abs(piece)==1&&Sign(dy)!=-player)continue;
                const int tx=x+dx,ty=y+dy;
                if(tx<0||tx>=8||ty<0||ty>=8)continue;
                const int to=ty*8+tx;
                if(board[to]!=0)continue;
                Board after=board;
                after[to]=after[from];after[from]=0;
                if(std::abs(piece)==1&&ty==(player==1?0:7))
                    after[to]=static_cast<int8_t>(player*2);
                steps.push_back({after,from,to});
            }
        }
        return steps;
    }
    static int CheckerPositionValue(const Board &board){
        int value=0;
        for(int i=0;i<64;i++){
            const int piece=board[i];
            if(piece==0)continue;
            int score=std::abs(piece)==2?190:100;
            // Kings matter, men advance, center squares support mobility.
            if(std::abs(piece)==1)
                score+=(piece<0?i/8:7-i/8)*5;
            score+=6-std::abs(i%8-3)*2;
            value+=piece<0?score:-score;
        }
        return value;
    }
    // Evaluate a complete adversarial turn at every search node. A normal
    // horizon at depth zero must not stop in the middle of a *forced*
    // capture response: limited capture quiescence extends those leaves.
    // The explicit node cap protects Daydream-era phone frame pacing.
    static int SearchCheckersBounded(const Board &board,int player,int depth,
                                    int alpha,int beta,int &nodes,int budget,
                                    int captureExtensions,
                                    bool *exhausted=nullptr) {
        if(nodes>=budget){
            if(exhausted)*exhausted=true;
            return CheckerPositionValue(board);
        }
        ++nodes;
        const auto turns=CheckerTurns(board,player);
        if(turns.empty())return player==-1?-30000-depth:30000+depth;
        const bool forcedCapture=std::abs(turns.front().firstTo
            -turns.front().from)>=14;
        if(depth<=0&&(!forcedCapture||captureExtensions<=0))
            return CheckerPositionValue(board);
        const int nextDepth=depth>0?depth-1:0;
        const int nextExtensions=depth>0?captureExtensions:
                                  captureExtensions-1;
        if(player==-1){
            int best=-300000;
            for(const auto &turn:turns){
                const int value=SearchCheckersBounded(turn.after,1,nextDepth,
                    alpha,beta,nodes,budget,nextExtensions,exhausted);
                if(exhausted&&*exhausted)break;
                best=std::max(best,value);
                alpha=std::max(alpha,best);
                if(beta<=alpha||nodes>=budget)break;
            }
            return best;
        }
        int best=300000;
        for(const auto &turn:turns){
            const int value=SearchCheckersBounded(turn.after,-1,nextDepth,
                alpha,beta,nodes,budget,nextExtensions,exhausted);
            if(exhausted&&*exhausted)break;
            best=std::min(best,value);
            beta=std::min(beta,best);
            if(beta<=alpha||nodes>=budget)break;
        }
        return best;
    }
    static int SearchCheckers(const Board &board,int player,int depth,
                               int alpha,int beta) {
        int nodes=0;
        return SearchCheckersBounded(board,player,depth,alpha,beta,
                                    nodes,12000,3);
    }
    // Search only completed iterations; accepting a partially evaluated
    // root candidate on node exhaustion biases the move order. Difficulty
    // selects real search effort without weakening compulsory captures.
    bool PlayCheckerCpuTurn(){
        if(kind!=Kind::Checkers||finished||side!=-1)return false;
        checkerSearchNodes=0;
        if(CheckerDrawAvailable(-1)&&CheckerPositionValue(cells)<-100){
            finished=true;winner=0;return true;
        }
        const auto moves=CheckerTurns(cells,-1,forced);
        if(moves.empty()){
            finished=true;winner=1;return false;
        }
        int material=0;
        for(int i=0;i<64;i++)material+=cells[i]!=0;
        const int level=std::clamp(checkerDifficulty,0,2);
        const int budget=level==0?1500:level==1?6000:12000;
        const int baseDepth=material<=5?6:material<=10?5:4;
        const int targetDepth=std::max(1,baseDepth+level-1);
        CheckerTurn best=moves.front();
        for(int depth=1;depth<=targetDepth;depth++){
            CheckerTurn iterationBest=best;
            int bestScore=std::numeric_limits<int>::min();
            int alpha=-300000;
            bool exhausted=false;
            for(const auto &move:moves){
                const int value=SearchCheckersBounded(move.after,1,depth-1,
                    alpha,300000,checkerSearchNodes,budget,3,&exhausted);
                if(exhausted)break;
                if(value>bestScore){
                    bestScore=value;iterationBest=move;
                }
                alpha=std::max(alpha,bestScore);
            }
            if(exhausted)break;
            best=iterationBest;
        }
        return CheckerMove(best.from,best.firstTo);
    }

    // Go: 19x19, captures, suicide prohibition, positional superko,
    // two-pass ending and Chinese stone+territory area scoring.
    struct Group {
        std::array<int,kCapacity> stones{};
        int count=0, liberties=0;
    };
    Group Flood(const Board& board,int start,
                std::array<uint8_t,kCapacity>* optionalSeen=nullptr) const {
        Group g;
        if(start<0||start>=kCapacity||board[start]==0)return g;
        std::array<uint8_t,kCapacity> seen{};
        std::array<uint8_t,kCapacity> libertySeen{};
        g.stones[g.count++]=start;seen[start]=1;
        const int color=board[start];
        for(int index=0;index<g.count;index++){
            int cell=g.stones[index],x=cell%kGoSize,y=cell/kGoSize;
            const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
            for(int i=0;i<4;i++){
                int xx=x+dx[i],yy=y+dy[i];
                if(xx<0||xx>=kGoSize||yy<0||yy>=kGoSize)continue;
                int next=yy*kGoSize+xx;
                if(board[next]==0){
                    if(!libertySeen[next]){libertySeen[next]=1;++g.liberties;}
                }else if(board[next]==color&&!seen[next]){
                    seen[next]=1;g.stones[g.count++]=next;
                }
            }
        }
        if(optionalSeen)for(int i=0;i<g.count;i++)
            (*optionalSeen)[g.stones[i]]=1;
        return g;
    }
    bool SimulateGo(int index,Board& after,int& captures) const {
        if(index<0||index>=kCapacity||cells[index]!=0)return false;
        after=cells;after[index]=static_cast<int8_t>(side);
        captures=0;
        std::array<uint8_t,kCapacity> checked{};
        const int x=index%kGoSize,y=index/kGoSize;
        const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
        for(int i=0;i<4;i++){
            int nx=x+dx[i],ny=y+dy[i];
            if(nx<0||nx>=kGoSize||ny<0||ny>=kGoSize)continue;
            int other=ny*kGoSize+nx;
            if(after[other]!=-side||checked[other])continue;
            Group g=Flood(after,other,&checked);
            if(g.liberties==0) {
                captures+=g.count;
                for(int j=0;j<g.count;j++)after[g.stones[j]]=0;
            }
        }
        if(Flood(after,index).liberties==0)return false;
        // Full board equality, not just immediate ko.
        if(goHistory.find(after)!=goHistory.end())return false;
        return true;
    }
    bool GoMove(int index) {
        if(kind!=Kind::Go||finished||goReview)return false;
        Board after{};
        int captured=0;
        if(!SimulateGo(index,after,captured))return false;
        cells=after;goHistory.insert(cells);passes=0;
        selected=-1;lastFrom=-1;lastTo=index;
        side=-side;++ply;return true;
    }
    bool ToggleGoDead(int index) {
        if(kind!=Kind::Go||!goReview||finished||
           index<0||index>=kCapacity||cells[index]==0)return false;
        const Group group=Flood(cells,index);
        const uint8_t next=goDead[index]?0:1;
        for(int i=0;i<group.count;i++)goDead[group.stones[i]]=next;
        selected=index;++goReviewRevision;
        return true;
    }
    // Count accepted dead stones as vacant points for Chinese area score.
    std::pair<float,float> GoArea() const {
        Board counted=cells;
        for(int i=0;i<kCapacity;i++)if(goDead[i])counted[i]=0;
        float black=0.f,white=7.5f; // Chinese 19x19 area komi
        std::array<uint8_t,kCapacity> visited{};
        for(int i=0;i<kCapacity;i++){
            if(counted[i]!=0){if(counted[i]>0)black++;else white++;continue;}
            if(visited[i])continue;
            std::array<int,kCapacity> queue{};int front=0,end=1,border=0;
            queue[0]=i;visited[i]=1;
            while(front<end){
                int p=queue[front++],x=p%kGoSize,y=p/kGoSize;
                const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
                for(int d=0;d<4;d++){
                    int xx=x+dx[d],yy=y+dy[d];
                    if(xx<0||yy<0||xx>=kGoSize||yy>=kGoSize)continue;
                    int other=yy*kGoSize+xx;
                    if(counted[other]>0)border|=1;
                    else if(counted[other]<0)border|=2;
                    else if(!visited[other]){visited[other]=1;queue[end++]=other;}
                }
            }
            if(border==1)black+=end;
            if(border==2)white+=end;
        }
        return {black,white};
    }
    void ScoreGo(){
        const auto area=GoArea();
        goBlackScore=area.first;goWhiteScore=area.second;
        winner=goBlackScore>goWhiteScore?1:-1;finished=true;
    }
    bool IsGoEye(const Board &board,int square,int player) const {
        if(square<0||square>=kCapacity||board[square]!=0)return false;
        const int x=square%kGoSize,y=square/kGoSize;
        const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
        for(int d=0;d<4;d++){
            const int nx=x+dx[d],ny=y+dy[d];
            if(nx<0||nx>=kGoSize||ny<0||ny>=kGoSize)continue;
            if(board[ny*kGoSize+nx]!=player)return false;
        }
        int hostileDiagonals=0;
        for(int yy=-1;yy<=1;yy+=2)for(int xx=-1;xx<=1;xx+=2){
            const int nx=x+xx,ny=y+yy;
            if(nx<0||nx>=kGoSize||ny<0||ny>=kGoSize)continue;
            if(board[ny*kGoSize+nx]==-player)++hostileDiagonals;
        }
        const bool edge=x==0||y==0||x==kGoSize-1||y==kGoSize-1;
        return hostileDiagonals<=(edge?0:1);
    }
    bool AcceptGoScore() {
        if(kind!=Kind::Go||!goReview||finished)return false;
        ScoreGo();goReview=false;++goReviewRevision;
        return true;
    }
    bool ResumeGoPlay() {
        if(kind!=Kind::Go||!goReview||finished)return false;
        goReview=false;goDead.fill(0);passes=0;
        side=goResumeSide;selected=-1;lastFrom=lastTo=-1;
        ++goReviewRevision;
        return true;
    }
    bool Pass(){
        if(kind!=Kind::Go||finished||goReview)return false;
        selected=-1;lastFrom=-1;lastTo=-1;
        ++passes;++ply;side=-side;
        if(passes>=2){
            goResumeSide=side; // next player after the second pass
            goReview=true;goDead.fill(0);++goReviewRevision;
            side=1; // temporary Black-owned scoring/review controls
        }
        return true;
    }

    // Chess: fixed-position legality, king safety, castling, en passant,
    // queen auto-promotion and no-legal-move checkmate/stalemate.
    bool Attacked(const Board& b,int square,int attacker) const {
        const int tx=square%8,ty=square/8;
        for(int from=0;from<64;from++){
            int piece=b[from];if(Sign(piece)!=attacker)continue;
            const int x=from%8,y=from/8,dx=tx-x,dy=ty-y;
            switch(std::abs(piece)){
                case 1:
                    if(std::abs(dx)==1&&dy==-attacker)return true;
                    break;
                case 2:
                    if((std::abs(dx)==1&&std::abs(dy)==2)||
                       (std::abs(dx)==2&&std::abs(dy)==1))return true;
                    break;
                case 6:
                    if(std::max(std::abs(dx),std::abs(dy))==1)return true;
                    break;
                default: {
                    bool diag=std::abs(dx)==std::abs(dy)&&dx!=0;
                    bool straight=(dx==0)!=(dy==0);
                    int type=std::abs(piece);
                    if((type==3&&!diag)||(type==4&&!straight)||
                       (type==5&&!diag&&!straight))break;
                    int sx=Sign(dx),sy=Sign(dy),xx=x+sx,yy=y+sy;
                    bool clear=true;
                    while(xx!=tx||yy!=ty){
                        if(b[yy*8+xx]){clear=false;break;}
                        xx+=sx;yy+=sy;
                    }
                    if(clear)return true;
                    break;
                }
            }
        }
        return false;
    }
    bool KingInCheck(const Board& b,int player) const {
        int king=-1;
        for(int i=0;i<64;i++)if(b[i]==player*6){king=i;break;}
        return king<0 || Attacked(b,king,-player);
    }
    ChessPosition CurrentChessPosition() const {
        ChessPosition p;
        std::copy_n(cells.begin(),64,p.pieces.begin());
        p.side=static_cast<int8_t>(side);
        p.castles=static_cast<int8_t>(castles);
        // En-passant counts for repetition only if it is a legal option.
        if(enPassant>=0&&enPassant<64)for(int dx : {-1,1}){
            const int from=enPassant+side*8+dx;
            if(from>=0&&from<64&&cells[from]==side &&
               std::abs(from%8-enPassant%8)==1&&ChessLegal(from,enPassant)){
                p.enPassant=static_cast<int8_t>(enPassant);break;
            }
        }
        return p;
    }
    int ChessRepetitions() const {
        auto it=chessHistory.find(CurrentChessPosition());
        return it==chessHistory.end()?0:it->second;
    }
    bool ChessDrawAvailable(int player,int *intendedFrom=nullptr,
                            int *intendedTo=nullptr) const {
        if(kind!=Kind::Chess||finished||side!=player)return false;
        if(halfMoves>=100||ChessRepetitions()>=3)return true;
        bool repeated=false;
        for(const auto &entry:chessHistory)if(entry.second>=2){
            repeated=true;break;
        }
        if(halfMoves<99&&!repeated)return false;
        // FIDE 9.2.1.1 / 9.3.1: identify a specific legal intended
        // move before claiming. Never play it and give away the turn.
        for(int from=0;from<64;from++)if(Sign(cells[from])==side)
        for(int to=0;to<64;to++)if(ChessLegal(from,to)){
            State next=ChessAfter(from,to,promotionChoice);
            const auto it=chessHistory.find(next.CurrentChessPosition());
            if(next.halfMoves>=100||
               (it!=chessHistory.end()&&it->second>=2)){
                if(intendedFrom)*intendedFrom=from;
                if(intendedTo)*intendedTo=to;
                return true;
            }
        }
        return false;
    }
    bool ChessDrawClaimable() const {return ChessDrawAvailable(1);}
    bool ClaimChessDraw(int player=1) {
        int from=-1,to=-1;
        if(!ChessDrawAvailable(player,&from,&to))return false;
        if(from>=0){lastFrom=from;lastTo=to;}
        finished=true;winner=0;selected=-1;return true;
    }
    void CyclePromotion(){
        if(kind!=Kind::Chess||finished||side!=1||
           selected<0||selected>=64||cells[selected]!=1||
           selected/8!=1)return;
        promotionChoice=promotionChoice==5?4:
            promotionChoice==4?3:promotionChoice==3?2:5;
    }
    bool ChessDeadPosition() const {
        // Safe FIDE dead-position cases. Don't declare draws where mating
        // sequences are still possible (e.g., K+2N vs K).
        int minors=0,bishops=0,knights=0,bishopColor=-1;
        for(int i=0;i<64;i++){
            const int p=std::abs(cells[i]);
            if(p==0||p==6)continue;
            if(p==1||p==4||p==5)return false;
            ++minors;
            if(p==2)++knights;
            if(p==3) {
                ++bishops;
                const int color=(i/8+i%8)%2;
                if(bishopColor<0)bishopColor=color;
                else if(bishopColor!=color)bishopColor=2;
            }
        }
        if(minors<=1)return true;
        return knights==0&&bishops==minors&&bishopColor<2;
    }
    // Only invoked on a pseudo-legal move; assumes indexes and ownership.
    void ApplyChess(Board& b,int from,int to,
                    int& newEnPassant,int& newCastles,
                    int promotion=5) const {
        const int piece=b[from],target=b[to],player=Sign(piece);
        const int fx=from%8,tx=to%8;
        if(std::abs(piece)==1&&to==enPassant&&target==0&&fx!=tx)
            b[from/8*8+tx]=0;
        if(std::abs(piece)==6&&std::abs(tx-fx)==2){
            int y=from/8;
            int oldRook=y*8+(tx>fx?7:0),newRook=y*8+(tx>fx?5:3);
            b[newRook]=b[oldRook];b[oldRook]=0;
        }
        b[to]=b[from];b[from]=0;newEnPassant=-1;
        if(std::abs(piece)==1){
            if(std::abs(to/8-from/8)==2)newEnPassant=(from+to)/2;
            if(to/8==(player==1?0:7))b[to]=static_cast<int8_t>(
                player*(promotion==2||promotion==3||promotion==4
                    ?promotion:5));
        }
        if(std::abs(piece)==6)newCastles &= player==1?~3:~12;
        if(from==63||to==63)newCastles&=~1;
        if(from==56||to==56)newCastles&=~2;
        if(from==7||to==7)newCastles&=~4;
        if(from==0||to==0)newCastles&=~8;
    }
    bool ChessLegal(int from,int to) const {
        if(from<0||to<0||from>=64||to>=64||from==to)return false;
        int p=cells[from],t=cells[to],player=Sign(p);
        if(player!=side||Sign(t)==side||std::abs(t)==6)return false;
        const int dx=to%8-from%8,dy=to/8-from/8;
        const int adx=std::abs(dx),ady=std::abs(dy),type=std::abs(p);
        bool legal=false;
        if(type==1){
            const int forward=-side;
            if(dx==0&&t==0&&dy==forward)legal=true;
            if(dx==0&&t==0&&dy==2*forward &&
               from/8==(side==1?6:1)&&cells[from+forward*8]==0)
                legal=true;
            if(adx==1&&dy==forward){
                if(Sign(t)==-side)legal=true;
                if(to==enPassant&&t==0 &&
                    cells[from/8*8+to%8]==-side)legal=true;
            }
        }else if(type==2){
            legal=(adx==1&&ady==2)||(adx==2&&ady==1);
        }else if(type==6){
            legal=std::max(adx,ady)==1;
            if(ady==0&&adx==2&&from==(side==1?60:4)){
                const bool right=dx==2;
                const int bit=side==1?(right?1:2):(right?4:8);
                const int rookFrom=(side==1?56:0)+(right?7:0);
                if((castles&bit)&&cells[rookFrom]==side*4 &&
                   !KingInCheck(cells,side) &&
                   cells[from+(right?1:-1)]==0 &&
                   cells[to]==0 &&
                   (right||cells[from-3]==0) &&
                   !Attacked(cells,from+(right?1:-1),-side))
                    legal=true;
            }
        }else{
            const bool diagonal=adx==ady&&adx!=0;
            const bool straight=(dx==0)!=(dy==0);
            if((type==3&&diagonal)||(type==4&&straight)||
               (type==5&&(diagonal||straight))){
                int sx=Sign(dx),sy=Sign(dy),x=from%8+sx,y=from/8+sy;
                legal=true;
                while(x!=to%8||y!=to/8){
                    if(cells[y*8+x]!=0){legal=false;break;}
                    x+=sx;y+=sy;
                }
            }
        }
        if(!legal)return false;
        Board tmp=cells;
        int ep=enPassant,cs=castles;
        ApplyChess(tmp,from,to,ep,cs);
        return !KingInCheck(tmp,side);
    }
    bool HasChessMove() const {
        for(int from=0;from<64;from++)if(Sign(cells[from])==side)
            for(int to=0;to<64;to++)if(ChessLegal(from,to))return true;
        return false;
    }
    // Standard algebraic notation (SAN), computed from a legal pre-move
    // position. Distinguishes file/rank when same-type pieces can reach a
    // square, including castles, en-passant and all promotions.
    std::string ChessMoveNotation(int from,int to,int promotion=5) const {
        if(!ChessLegal(from,to))return "";
        const int piece=std::abs(cells[from]);
        const int fx=from%8,fy=from/8,tx=to%8,ty=to/8;
        if(piece==6&&std::abs(tx-fx)==2)return tx>fx?"O-O":"O-O-O";
        const bool capture=cells[to]!=0||(piece==1&&to==enPassant&&fx!=tx);
        std::string result;
        constexpr char labels[]={'?',' ','N','B','R','Q','K'};
        if(piece!=1){
            result+=labels[piece];
            bool ambiguous=false,sameFile=false,sameRank=false;
            for(int other=0;other<64;other++){
                if(other==from||cells[other]!=cells[from])continue;
                if(ChessLegal(other,to)){
                    ambiguous=true;
                    sameFile|=other%8==fx;
                    sameRank|=other/8==fy;
                }
            }
            if(ambiguous){
                if(!sameFile)result+=static_cast<char>('a'+fx);
                else if(!sameRank)result+=static_cast<char>('8'-fy);
                else {result+=static_cast<char>('a'+fx);
                      result+=static_cast<char>('8'-fy);}
            }
        }else if(capture){
            result+=static_cast<char>('a'+fx);
        }
        if(capture)result+='x';
        result+=static_cast<char>('a'+tx);
        result+=static_cast<char>('8'-ty);
        if(piece==1&&(ty==0||ty==7)){
            constexpr char promo[]={'?','?','N','B','R','Q'};
            result+='=';
            result+=promo[promotion>=2&&promotion<=5?promotion:5];
        }
        return result;
    }
    bool ChessMove(int from,int to,int cpuPromotion=5) {
        if(kind!=Kind::Chess||finished||!ChessLegal(from,to))return false;
        const int piece=cells[from],taken=cells[to];
        std::string notation=ChessMoveNotation(from,to,
            side==1?promotionChoice:cpuPromotion);
        const bool enPassantCapture=std::abs(piece)==1&&
            to==enPassant&&taken==0&&from%8!=to%8;
        halfMoves=(std::abs(piece)==1||taken!=0||enPassantCapture)
            ?0:halfMoves+1;
        ApplyChess(cells,from,to,enPassant,castles,
                   side==1?promotionChoice:cpuPromotion);
        promotionChoice=5;
        selected=-1;lastFrom=from;lastTo=to;side=-side;++ply;
        const int repeats=++chessHistory[CurrentChessPosition()];
        // Mate takes precedence over the 75-move automatic draw.
        if(!HasChessMove()){
            finished=true;winner=KingInCheck(cells,side)?-side:0;
        }else if(ChessDeadPosition()||halfMoves>=150||repeats>=5){
            finished=true;winner=0;
        }
        if(KingInCheck(cells,side))
            notation+=finished&&winner==-side?'#':'+';
        chessSanHistory.push_back(std::move(notation));
        return true;
    }

    bool Select(int square) {
        if(finished){Reset(kind);return true;}
        if(side!=1)return false;
        if(kind==Kind::Go)
            return goReview?ToggleGoDead(square):GoMove(square);
        if(kind==Kind::Reversi)return ReversiMove(square);
        const int cap=Size()*Size();
        if(square<0||square>=cap)return false;
        if(kind==Kind::Checkers){
            if(forced>=0)selected=forced;
            else if(cells[square]>0){selected=square;return true;}
            if(selected>=0&&CheckerMove(selected,square))return true;
            return false;
        }
        if(cells[square]>0){selected=square;return true;}
        return selected>=0&&ChessMove(selected,square);
    }

    // Light, bounded adversarial chess opponent. Material evaluations are
    // from the CPU (negative pieces) perspective, and both players search
    // *legal king-safe replies*, not only destination-square captures.
    static int ChessBoardValue(const Board &board){
        static const int values[7]={0,100,320,330,500,900,0};
        int score=0;
        for(int at=0;at<64;at++){
            const int piece=board[at];
            if(piece==0)continue;
            int value=values[std::abs(piece)];
            if(std::abs(piece)==1)
                value+=5*(piece<0?at/8:7-at/8);
            value+=3*(3-std::abs(at%8-3));
            score+=piece<0?value:-value;
        }
        return score;
    }
    State ChessAfter(int from,int to,int promotion=5) const {
        State result;
        result.kind=Kind::Chess;
        result.cells=cells;
        result.side=side;
        result.enPassant=enPassant;
        result.castles=castles;
        result.halfMoves=(std::abs(cells[from])==1||cells[to]!=0)
            ?0:halfMoves+1;
        result.ApplyChess(result.cells,from,to,result.enPassant,
                          result.castles,promotion);
        result.side=-side;
        return result;
    }
    struct ChessTurn {int from,to,promotion,order;};
    std::vector<ChessTurn> ChessTurns() const {
        std::vector<ChessTurn> moves;
        for(int from=0;from<64;from++)if(Sign(cells[from])==side)
        for(int to=0;to<64;to++)if(ChessLegal(from,to)){
            const bool promote=std::abs(cells[from])==1&&
                (to/8==0||to/8==7);
            for(int promotion=5;promotion>=(promote?2:5);--promotion){
                const int order=10*std::abs(cells[to])-
                    std::abs(cells[from])+(promote?promotion*10:0);
                moves.push_back({from,to,promotion,order});
            }
        }
        std::stable_sort(moves.begin(),moves.end(),
            [](const ChessTurn &a,const ChessTurn &b){return a.order>b.order;});
        return moves;
    }
    int SearchChessBounded(int depth,int alpha,int beta,int &nodes,
                           int budget,bool &exhausted) const {
        if(nodes>=budget){exhausted=true;return ChessBoardValue(cells);}
        ++nodes;
        const auto moves=ChessTurns();
        // Stalemate is a draw even at the material-evaluation horizon.
        // Mate takes priority over the automatic move-count draw.
        if(moves.empty())return KingInCheck(cells,side)
            ?(side==-1?-50000-depth:50000+depth):0;
        if(ChessDeadPosition()||halfMoves>=150)return 0;
        if(depth<=0)return ChessBoardValue(cells);
        const bool maximizing=side==-1;
        int best=maximizing?-300000:300000;
        for(const auto &move:moves){
            State next=ChessAfter(move.from,move.to,move.promotion);
            const int value=next.SearchChessBounded(depth-1,alpha,beta,
                                                    nodes,budget,exhausted);
            if(exhausted)return value;
            if(maximizing){best=std::max(best,value);alpha=std::max(alpha,best);}
            else {best=std::min(best,value);beta=std::min(beta,best);}
            if(beta<=alpha)break;
        }
        return best;
    }
    int SearchChess(int depth,int alpha,int beta) const {
        int nodes=0;bool exhausted=false;
        return SearchChessBounded(depth,alpha,beta,nodes,12000,exhausted);
    }
    bool PlayChessCpuTurn(){
        if(kind!=Kind::Chess||finished||side!=-1)return false;
        chessSearchNodes=0;
        if(ChessBoardValue(cells)<0&&ClaimChessDraw(-1))return true;
        const auto moves=ChessTurns();
        if(moves.empty()){
            finished=true;winner=KingInCheck(cells,side)?1:0;
            return false;
        }
        const int level=std::clamp(chessDifficulty,0,2);
        const int budget=level==0?1500:level==1?6000:12000;
        const int maxDepth=level+2;
        ChessTurn best=moves.front();
        // Keep only fully completed iterations. A budget cutoff must
        // never favor candidates merely because they were visited first.
        for(int depth=1;depth<=maxDepth;depth++){
            ChessTurn iterationBest=best;
            int bestScore=-300000,alpha=-300000;
            bool exhausted=false;
            for(const auto &move:moves){
                State next=ChessAfter(move.from,move.to,move.promotion);
                const int value=next.SearchChessBounded(depth-1,alpha,300000,
                                          chessSearchNodes,budget,exhausted);
                if(exhausted)break;
                if(value>bestScore){bestScore=value;iterationBest=move;}
                alpha=std::max(alpha,bestScore);
            }
            if(exhausted)break;
            best=iterationBest;
        }
        return ChessMove(best.from,best.to,best.promotion);
    }

    // One bounded deterministic response per CPU turn, evaluated on Update.
    void Step(float delta) {
        if(side!=-1||finished||!std::isfinite(delta)||delta<=0)return;
        clock+=std::min(delta,.1f);
        if(clock<.28f)return;
        clock=0;
        if(kind==Kind::Go){
            // White has the 7.5-point komi. If Black has passed and
            // current Chinese-area scoring already wins for the CPU,
            // agree to end play instead of pointlessly filling eyes.
            if(passes==1){
                const auto area=GoArea();
                if(area.second>area.first){Pass();return;}
            }
            // Cache endangered connected groups once. Without this,
            // random positional bonuses can sacrifice a group in atari
            // even when its last liberty is a legal, safe escape.
            std::array<uint8_t,kCapacity> inspected{};
            std::array<int,kCapacity> atariAnchor{};
            std::array<int,kCapacity> atariSize{};
            int atariCount=0;
            for(int stone=0;stone<kCapacity;stone++)
                if(cells[stone]==-1&&!inspected[stone]){
                    const Group group=Flood(cells,stone,&inspected);
                    if(group.liberties==1){
                        atariAnchor[atariCount]=stone;
                        atariSize[atariCount]=group.count;
                        ++atariCount;
                    }
                }
            int best=-1;float rating=-1e8f;
            const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
            for(int i=0;i<kCapacity;i++)if(cells[i]==0){
                Board result{};int captured=0;
                if(!SimulateGo(i,result,captured))continue;
                const int x=i%kGoSize,y=i/kGoSize;
                const int edge=std::min(x,kGoSize-1-x)
                    +std::min(y,kGoSize-1-y);
                int allies=0,enemies=0,open=0;
                for(int d=0;d<4;d++){
                    const int xx=x+dx[d],yy=y+dy[d];
                    if(xx<0||xx>=kGoSize||yy<0||yy>=kGoSize)continue;
                    const int color=cells[yy*kGoSize+xx];
                    if(color==-1)allies++;
                    else if(color==1)enemies++;
                    else open++;
                }
                // Protect own eyes and avoid immediate self-atari.
                if(captured==0&&IsGoEye(cells,i,-1))continue;
                const Group group=Flood(result,i);
                // Capturing an enemy is no excuse to put an entire
                // connected white group into immediate counter-atari.
                if(group.liberties<=1)continue;
                int rescued=0;
                for(int j=0;j<atariCount;j++)
                    if(Flood(result,atariAnchor[j]).liberties>=2)
                        rescued+=atariSize[j];
                float value=captured*22.f+rescued*45.f
                    +group.liberties*.16f
                    +enemies*.50f+allies*.15f+open*.11f
                    -std::abs(edge-6)*.07f
                    +(Random()%97u)*.0001f;
                if(value>rating){rating=value;best=i;}
            }
            // No arbitrary 9x9 turn limit; pass when no useful legal
            // non-eye / non-self-atari play remains.
            if(best<0){Pass();return;}
            GoMove(best);
        }else if(kind==Kind::Checkers){
            PlayCheckerCpuTurn();
        }else if(kind==Kind::Chess){
            PlayChessCpuTurn();
        }else if(kind==Kind::Reversi){
            PlayReversiCpuTurn();
        }
    }
};
}} // namespace reverie::boardgames
#endif

