#ifndef REVERIE_CLASSIC_BOARD_GAMES_H
#define REVERIE_CLASSIC_BOARD_GAMES_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>

namespace reverie { namespace boardgames {

enum class Kind : uint8_t { Checkers=3, Go=4, Chess=5 };
struct State {
    // Human plays Black (+1); CPU plays White (-1).
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
    int halfMoves=0,promotionChoice=5;
    Kind kind=Kind::Checkers;
    Board cells{};
    std::unordered_set<Board,GoBoardHash> goHistory;
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
        kind=type;cells.fill(0);goHistory.clear();chessHistory.clear();
        halfMoves=0;promotionChoice=5;side=1;winner=0;
        selected=forced=-1;ply=passes=0;enPassant=-1;castles=15;
        lastFrom=lastTo=-1;clock=0;finished=false;
        if(type==Kind::Checkers){
            for(int y=0;y<3;y++)for(int x=0;x<8;x++)
                if((x+y)%2)cells[Index(x,y)]=-1;
            for(int y=5;y<8;y++)for(int x=0;x<8;x++)
                if((x+y)%2)cells[Index(x,y)]=1;
        } else if(type==Kind::Chess) {
            static const int8_t back[8]={4,2,3,5,6,3,2,4};
            for(int x=0;x<8;x++){
                cells[Index(x,0)]=-back[x];cells[Index(x,1)]=-1;
                cells[Index(x,6)]=1;cells[Index(x,7)]=back[x];
            }
            chessHistory.emplace(CurrentChessPosition(),1);
        } else if(type==Kind::Go) {
            goHistory.insert(cells);
        }
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
        cells[to]=cells[from];cells[from]=0;
        if(capture)cells[(from+to)/2]=0;
        const bool crown=std::abs(old)==1 &&
            (to/8==(side==1?0:7));
        if(crown)cells[to]=static_cast<int8_t>(side*2);
        lastFrom=from;lastTo=to;selected=-1;++ply;
        if(capture&&!crown&&HasCapture(side,to)){forced=to;return true;}
        forced=-1;side=-side;
        if(!HasCheckerMove(side)){finished=true;winner=-side;}
        return true;
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
        if(kind!=Kind::Go||finished)return false;
        Board after{};
        int captured=0;
        if(!SimulateGo(index,after,captured))return false;
        cells=after;goHistory.insert(cells);passes=0;
        selected=-1;lastFrom=-1;lastTo=index;
        side=-side;++ply;return true;
    }
    void ScoreGo(){
        float black=0.f,white=7.5f; // Chinese 19x19 area komi
        std::array<uint8_t,kCapacity> visited{};
        for(int i=0;i<kCapacity;i++){
            if(cells[i]!=0){if(cells[i]>0)black++;else white++;continue;}
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
                    if(cells[other]>0)border|=1;
                    else if(cells[other]<0)border|=2;
                    else if(!visited[other]){visited[other]=1;queue[end++]=other;}
                }
            }
            if(border==1)black+=end;
            if(border==2)white+=end;
        }
        winner=black>white?1:-1;finished=true;
    }
    bool Pass(){
        if(kind!=Kind::Go||finished)return false;
        selected=-1;lastFrom=-1;lastTo=-1;
        ++passes;++ply;side=-side;
        if(passes>=2)ScoreGo();
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
    bool ChessDrawClaimable() const {
        return kind==Kind::Chess&&!finished&&side==1&&
            (halfMoves>=100||ChessRepetitions()>=3);
    }
    bool ClaimChessDraw() {
        if(!ChessDrawClaimable())return false;
        finished=true;winner=0;return true;
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
    bool ChessMove(int from,int to) {
        if(kind!=Kind::Chess||finished||!ChessLegal(from,to))return false;
        const int piece=cells[from],taken=cells[to];
        const bool enPassantCapture=std::abs(piece)==1&&
            to==enPassant&&taken==0&&from%8!=to%8;
        halfMoves=(std::abs(piece)==1||taken!=0||enPassantCapture)
            ?0:halfMoves+1;
        ApplyChess(cells,from,to,enPassant,castles,
                   side==1?promotionChoice:5);
        promotionChoice=5;
        selected=-1;lastFrom=from;lastTo=to;side=-side;++ply;
        const int repeats=++chessHistory[CurrentChessPosition()];
        // Mate takes precedence over the 75-move automatic draw.
        if(!HasChessMove()){
            finished=true;winner=KingInCheck(cells,side)?-side:0;
        }else if(ChessDeadPosition()||halfMoves>=150||repeats>=5){
            finished=true;winner=0;
        }
        return true;
    }

    bool Select(int square) {
        if(finished){Reset(kind);return true;}
        if(side!=1)return false;
        if(kind==Kind::Go)return GoMove(square);
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

    // One bounded deterministic response per CPU turn, evaluated on Update.
    void Step(float delta) {
        if(side!=-1||finished||!std::isfinite(delta)||delta<=0)return;
        clock+=std::min(delta,.1f);
        if(clock<.28f)return;
        clock=0;
        if(kind==Kind::Go){
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
                if(allies==4&&enemies==0&&captured==0)continue;
                const Group group=Flood(result,i);
                if(group.liberties<=1&&captured==0)continue;
                float value=captured*22.f+group.liberties*.16f
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
            const bool capture=HasCapture(side,forced);
            int bestFrom=-1,bestTo=-1,bestScore=-1000000;
            for(int from=0;from<64;from++)if(Sign(cells[from])==side &&
                   (forced<0||forced==from)){
                int x=from%8,y=from/8;
                for(int dx : {-2,-1,1,2})for(int dy : {-2,-1,1,2}){
                    if(std::abs(dx)!=std::abs(dy))continue;
                    int xx=x+dx,yy=y+dy;
                    if(xx<0||yy<0||xx>=8||yy>=8)continue;
                    int to=yy*8+xx;
                    bool jump=CheckersCapture(from,to);
                    if(capture?!jump:!jump&&!CheckersStep(from,to))continue;
                    int score=(jump?200:0)+(std::abs(cells[from])==1?
                        (yy==(side==1?0:7)?85:0):15)
                        -std::abs(xx-3)*2+static_cast<int>(Random()%5u);
                    if(score>bestScore){bestScore=score;bestFrom=from;bestTo=to;}
                }
            }
            if(bestFrom>=0)CheckerMove(bestFrom,bestTo);
            else {finished=true;winner=1;}
        }else if(kind==Kind::Chess){
            int bestFrom=-1,bestTo=-1,bestScore=-10000000;
            for(int from=0;from<64;from++)if(Sign(cells[from])==side)
                for(int to=0;to<64;to++)if(ChessLegal(from,to)){
                    static const int values[]={0,100,310,330,500,900,20000};
                    int score=values[std::abs(cells[to])]*12;
                    const int piece=std::abs(cells[from]);
                    if(piece==1&&to/8==7)score+=9500;
                    Board after=cells;
                    int ep=enPassant,cs=castles;
                    ApplyChess(after,from,to,ep,cs);
                    if(Attacked(after,to,1))score-=values[piece]*6;
                    if(KingInCheck(after,1))score+=60;
                    score+=18-std::abs(to%8-3)*3-std::abs(to/8-3)*3;
                    score+=static_cast<int>(Random()%11u);
                    if(score>bestScore){bestScore=score;bestFrom=from;bestTo=to;}
                }
            if(bestFrom>=0)ChessMove(bestFrom,bestTo);
            else {finished=true;winner=KingInCheck(cells,side)?1:0;}
        }
    }
};
}} // namespace reverie::boardgames
#endif
