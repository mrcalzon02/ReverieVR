#ifndef REVERIE_CLASSIC_GAMES_SAVE_H
#define REVERIE_CLASSIC_GAMES_SAVE_H

#include "classic_games.h"
#include "classic_breakout.h"
#include "classic_board_games.h"
#include "reverie_native_sdk.h"
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace reverie { namespace classics { namespace save {

// Game-private format. A host slot is at most 64 KiB, but Go superko and
// chess/checkers repetition must never be truncated to fit a single slot.
// Sixty-four pages give a bounded 4 MiB snapshot. Hitting this bound fails
// visibly and retains the previous commit; it never shortens rule history.
constexpr uint32_t kVersion=1, kPageBytes=REVERIE_NATIVE_SAVE_MAX_BYTES;
constexpr uint32_t kPages=64, kMaxBytes=kPageBytes*kPages;
constexpr uint32_t kHeaderBytes=32;
constexpr const char *kManifest="classic-match.v1";
using BoardState=reverie::boardgames::State;
struct Snapshot {
    State game;
    Breakout breakout;
    BoardState board;
};

inline uint64_t Hash(const uint8_t *p,size_t n) {
    uint64_t h=1469598103934665603ull;
    for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}
    return h;
}
struct Writer {
    std::vector<uint8_t> bytes;
    bool ok=true;
    void Byte(uint8_t x){
        if(bytes.size()>=kMaxBytes-kHeaderBytes){ok=false;return;}
        bytes.push_back(x);
    }
    void U32(uint32_t x){for(int i=0;i<4;++i)Byte(static_cast<uint8_t>(x>>(8*i)));}
    void U64(uint64_t x){for(int i=0;i<8;++i)Byte(static_cast<uint8_t>(x>>(8*i)));}
    void Int(int x){U32(static_cast<uint32_t>(x));}
    void Float(float x){uint32_t u=0;std::memcpy(&u,&x,4);U32(u);}
    void Bool(bool x){Byte(x?1:0);}
};
struct Fingerprinter {
    uint64_t value=1469598103934665603ull;
    void Byte(uint8_t x){value^=x;value*=1099511628211ull;}
    void U32(uint32_t x){for(int i=0;i<4;++i)Byte(static_cast<uint8_t>(x>>(8*i)));}
    void Int(int x){U32(static_cast<uint32_t>(x));}
    void Float(float x){uint32_t u=0;std::memcpy(&u,&x,4);U32(u);}
    void Bool(bool x){Byte(x?1:0);}
};
struct Reader {
    const uint8_t *data;size_t size,at=0;bool ok=true;
    uint8_t Byte(){if(at>=size){ok=false;return 0;}return data[at++];}
    uint32_t U32(){uint32_t x=0;for(int i=0;i<4;++i)x|=uint32_t(Byte())<<(8*i);return x;}
    uint64_t U64(){uint64_t x=0;for(int i=0;i<8;++i)x|=uint64_t(Byte())<<(8*i);return x;}
    int Int(){const uint32_t x=U32();int32_t s=0;std::memcpy(&s,&x,4);return s;}
    float Float(){const uint32_t u=U32();float x=0;std::memcpy(&x,&u,4);return x;}
    bool Bool(){const uint8_t x=Byte();if(x>1)ok=false;return x==1;}
    bool Count(uint32_t n,size_t minimum) {
        if(!ok||minimum==0||n>(size-at)/minimum){ok=false;return false;}
        return true;
    }
};
template<class W> inline void ArcadeCore(W &w,const State &s){
    w.U32(s.seed);w.Int(s.score);w.Int(s.opponent);
    w.Bool(s.finished);w.Bool(s.moving);w.Bool(s.player_turn);
    w.Float(s.clock);w.Float(s.paddle);w.Float(s.ai);
    w.Float(s.ball_x);w.Float(s.ball_y);w.Float(s.velocity_x);
    w.Float(s.velocity_y);w.Float(s.previous_paddle);
    w.Int(s.length);w.Int(s.heading);w.Int(s.desired);w.Int(s.food);
    for(int c:s.snake)w.Int(c);
    w.Int(s.position);w.Int(s.rival);w.Int(s.die);
}
inline void ReadArcade(Reader &r,int kind,State &s){
    s.game=static_cast<Game>(kind);s.seed=r.U32();s.score=r.Int();s.opponent=r.Int();
    s.finished=r.Bool();s.moving=r.Bool();s.player_turn=r.Bool();
    s.clock=r.Float();s.paddle=r.Float();s.ai=r.Float();
    s.ball_x=r.Float();s.ball_y=r.Float();s.velocity_x=r.Float();
    s.velocity_y=r.Float();s.previous_paddle=r.Float();
    s.length=r.Int();s.heading=r.Int();s.desired=r.Int();s.food=r.Int();
    for(int &c:s.snake)c=r.Int();
    s.position=r.Int();s.rival=r.Int();s.die=r.Int();
}
template<class W> inline void BreakoutCore(W &w,const Breakout &s){
    for(uint8_t b:s.bricks)w.Byte(b);
    w.Float(s.paddle);w.Float(s.ballX);w.Float(s.ballY);
    w.Float(s.velocityX);w.Float(s.velocityY);w.Float(s.accumulator);
    w.Int(s.lives);w.Int(s.score);w.Int(s.remaining);w.Int(s.wall);w.Int(s.paddleHits);
    w.Bool(s.narrowPaddle);w.Bool(s.topRowBroken);
    w.Bool(s.moving);w.Bool(s.finished);w.Bool(s.won);
}
inline void ReadBreakout(Reader &r,Breakout &s){
    for(uint8_t &b:s.bricks)b=r.Byte();
    s.paddle=r.Float();s.ballX=r.Float();s.ballY=r.Float();
    s.velocityX=r.Float();s.velocityY=r.Float();s.accumulator=r.Float();
    s.lives=r.Int();s.score=r.Int();s.remaining=r.Int();s.wall=r.Int();s.paddleHits=r.Int();
    s.narrowPaddle=r.Bool();s.topRowBroken=r.Bool();
    s.moving=r.Bool();s.finished=r.Bool();s.won=r.Bool();
}
template<class W> inline void BoardCore(W &w,const BoardState &s){
    for(int8_t c:s.cells)w.Byte(static_cast<uint8_t>(c+6));
    for(uint8_t c:s.goDead)w.Byte(c);
    w.Int(s.checkerQuietPlies);w.Int(s.halfMoves);w.Int(s.promotionChoice);
    w.Int(s.chessDifficulty);w.Int(s.chessSearchNodes);w.Int(s.chessHumanColor);
    w.U32(s.chessGameSerial);
    for(uint8_t c:s.chessCapturedByWhite)w.Byte(c);
    for(uint8_t c:s.chessCapturedByBlack)w.Byte(c);
    w.Int(s.checkerDifficulty);w.Int(s.checkerSearchNodes);w.Int(s.reversiSearchNodes);
    w.Bool(s.goReview);w.Int(s.goResumeSide);w.Int(s.goReviewRevision);
    w.Float(s.goBlackScore);w.Float(s.goWhiteScore);
    w.Int(s.side);w.Int(s.winner);w.Int(s.selected);w.Int(s.forced);
    w.Int(s.ply);w.Int(s.passes);w.Int(s.enPassant);w.Int(s.castles);
    w.Int(s.lastFrom);w.Int(s.lastTo);w.Bool(s.finished);w.Float(s.clock);w.U32(s.seed);
}
inline void ReadBoard(Reader &r,int kind,BoardState &s){
    s.kind=static_cast<reverie::boardgames::Kind>(kind);
    for(int8_t &c:s.cells){const int b=r.Byte();if(b>12)r.ok=false;c=static_cast<int8_t>(b-6);}
    for(uint8_t &c:s.goDead)c=r.Byte();
    s.checkerQuietPlies=r.Int();s.halfMoves=r.Int();s.promotionChoice=r.Int();
    s.chessDifficulty=r.Int();s.chessSearchNodes=r.Int();s.chessHumanColor=r.Int();
    s.chessGameSerial=r.U32();
    for(uint8_t &c:s.chessCapturedByWhite)c=r.Byte();
    for(uint8_t &c:s.chessCapturedByBlack)c=r.Byte();
    s.checkerDifficulty=r.Int();s.checkerSearchNodes=r.Int();s.reversiSearchNodes=r.Int();
    s.goReview=r.Bool();s.goResumeSide=r.Int();s.goReviewRevision=r.Int();
    s.goBlackScore=r.Float();s.goWhiteScore=r.Float();
    s.side=r.Int();s.winner=r.Int();s.selected=r.Int();s.forced=r.Int();
    s.ply=r.Int();s.passes=r.Int();s.enPassant=r.Int();s.castles=r.Int();
    s.lastFrom=r.Int();s.lastTo=r.Int();s.finished=r.Bool();s.clock=r.Float();s.seed=r.U32();
}
inline bool Range(int x,int a,int b){return x>=a&&x<=b;}
inline bool Real(float x,float a,float b){return std::isfinite(x)&&x>=a&&x<=b;}
inline bool Sign(int x){return x==1||x==-1;}
inline bool ValidateArcade(int kind,const State &s){
    if(static_cast<int>(s.game)!=kind||s.seed==0||!Range(s.score,0,141)||
       !Range(s.opponent,0,11)||!Real(s.clock,0.f,1.f)||
       !Real(s.paddle,-.781f,.781f)||!Real(s.previous_paddle,-.781f,.781f)||
       !Real(s.ai,-.781f,.781f)||!Real(s.ball_x,-1.051f,1.051f)||
       !Real(s.ball_y,-.931f,.931f)||!Real(s.velocity_x,-8.f,8.f)||
       !Real(s.velocity_y,-8.f,8.f)||!Range(s.length,3,144)||
       !Range(s.heading,0,3)||!Range(s.desired,0,3)||!Range(s.food,-1,143)||
       !Range(s.position,0,100)||!Range(s.rival,0,100)||!Range(s.die,0,6))return false;
    for(int c:s.snake)if(!Range(c,-1,143))return false;
    if(kind==0)return s.score<=11&&s.finished==(s.score==11||s.opponent==11);
    if(kind==2)return s.finished==(s.position==100||s.rival==100);
    if(s.score!=s.length-3||(s.desired+2)%4==s.heading)return false;
    std::array<bool,144> seen{};
    for(int i=0;i<s.length;++i){
        const int c=s.snake[i];if(c<0||seen[c])return false;seen[c]=true;
        if(i>0&&std::abs(c%12-s.snake[i-1]%12)+std::abs(c/12-s.snake[i-1]/12)!=1)return false;
    }
    return s.food==-1?(s.finished&&s.length==144):!seen[s.food];
}
inline bool ValidateBreakout(const Breakout &s){
    int remaining=0,earned=0;
    for(int i=0;i<Breakout::kBricks;++i){if(s.bricks[i]>1)return false;
        remaining+=s.bricks[i];if(!s.bricks[i])earned+=Breakout::BrickValue(i/14);}
    if(!Real(s.paddle,-.771f,.771f)||!Real(s.ballX,-.941f,.941f)||
       !Real(s.ballY,-1.f,.921f)||!Real(s.velocityX,-1.151f,1.151f)||
       !Real(s.velocityY,-1.151f,1.151f)||!Real(s.accumulator,0.f,.011f)||
       !Range(s.lives,0,3)||!Range(s.wall,1,2)||!Range(s.paddleHits,0,100000000)||
       s.remaining!=remaining||s.score!=(s.wall-1)*448+earned)return false;
    if(s.won!=(s.wall==2&&s.remaining==0)||s.finished!=(s.won||s.lives==0))return false;
    return !s.finished||!s.moving;
}
inline bool ValidPosition(const BoardState::ChessPosition &p,bool chess){
    if(!Sign(p.side)||!Range(p.castles,0,15)||!Range(p.enPassant,-1,63))return false;
    int white=0,black=0;
    for(int i=0;i<64;++i){
        const int c=p.pieces[i];if(!Range(c,chess?-6:-2,chess?6:2))return false;
        white+=c==6;black+=c==-6;
        if(!chess&&c!=0&&((i/8+i%8)%2==0))return false;
    }
    if(chess)return white==1&&black==1&&(p.enPassant==-1||p.enPassant/8==2||p.enPassant/8==5);
    return p.castles==0&&p.enPassant==-1;
}
inline bool ValidateBoard(int kind,const BoardState &s){
    const int capacity=kind==4?361:64;
    if(static_cast<int>(s.kind)!=kind||!Sign(s.side)||!Range(s.winner,-1,1)||
       !Range(s.selected,-1,capacity-1)||!Range(s.forced,-1,63)||
       !Range(s.ply,0,100000000)||!Range(s.passes,0,2)||
       !Range(s.enPassant,-1,63)||!Range(s.castles,0,15)||
       !Range(s.lastFrom,-1,capacity-1)||!Range(s.lastTo,-1,capacity-1)||
       !Real(s.clock,0.f,.4f)||s.seed==0||
       !Range(s.checkerQuietPlies,0,s.ply)||!Range(s.halfMoves,0,s.ply)||
       !Range(s.promotionChoice,2,5)||!Range(s.chessDifficulty,0,2)||
       !Range(s.checkerDifficulty,0,2)||!Sign(s.chessHumanColor)||
       !Range(s.chessSearchNodes,0,1000000)||!Range(s.checkerSearchNodes,0,1000000)||
       !Range(s.reversiSearchNodes,0,1000000)||!Sign(s.goResumeSide)||
       !Range(s.goReviewRevision,0,100000000)||
       !Real(s.goBlackScore,0.f,361.f)||!Real(s.goWhiteScore,0.f,368.5f))return false;
    const int pieceMax=kind==5?6:kind==3?2:1;
    for(int i=0;i<361;++i){
        if(!Range(s.cells[i],-pieceMax,pieceMax)||(i>=capacity&&s.cells[i]!=0)||
           s.goDead[i]>1||(s.goDead[i]&&s.cells[i]==0))return false;
        if(kind!=4&&s.goDead[i])return false;
    }
    for(uint8_t n:s.chessCapturedByWhite)if(n>15)return false;
    for(uint8_t n:s.chessCapturedByBlack)if(n>15)return false;
    if(kind==4){
        if(s.goHistory.empty()||s.goHistory.count(s.cells)!=1||
           s.goHistory.size()>static_cast<size_t>(s.ply)+1||
           (s.goReview&&(s.finished||s.passes!=2||s.side!=1)))return false;
        for(const auto &b:s.goHistory)for(int8_t c:b)if(!Range(c,-1,1))return false;
    }else if(s.goReview||!s.goHistory.empty())return false;
    if(kind==5){
        if(!ValidPosition(s.CurrentChessPosition(),true)||s.chessHistory.empty()||
           s.chessHistory.count(s.CurrentChessPosition())!=1||
           s.chessSanHistory.size()!=static_cast<size_t>(s.ply))return false;
        uint64_t count=0;
        for(const auto &p:s.chessHistory){if(!ValidPosition(p.first,true)||p.second<1)return false;count+=p.second;}
        if(count!=static_cast<uint64_t>(s.ply)+1)return false;
        for(const auto &san:s.chessSanHistory){
            if(san.empty()||san.size()>32)return false;
            for(unsigned char c:san)if(c<33||c>126)return false;
        }
    }else if(!s.chessHistory.empty()||!s.chessSanHistory.empty())return false;
    if(kind==3){
        if(!ValidPosition(s.CurrentCheckerPosition(),false)||s.checkerHistory.empty())return false;
        if(s.forced>=0&&(s.cells[s.forced]*s.side<=0||!s.HasCapture(s.side,s.forced)))return false;
        if(s.forced<0&&s.checkerHistory.count(s.CurrentCheckerPosition())!=1)return false;
        uint64_t count=0;
        for(const auto &p:s.checkerHistory){if(!ValidPosition(p.first,false)||p.second<1)return false;count+=p.second;}
        if(count>static_cast<uint64_t>(s.ply)+1)return false;
    }else if(!s.checkerHistory.empty()||s.forced!=-1)return false;
    return true;
}

inline void PackedGo(Writer &w,const BoardState::Board &b){
    for(int i=0;i<361;i+=4){uint8_t byte=0;
        for(int j=0;j<4&&i+j<361;++j)byte|=static_cast<uint8_t>((b[i+j]+1)<<(j*2));
        w.Byte(byte);}
}
inline BoardState::Board ReadGo(Reader &r){
    BoardState::Board b{};
    for(int i=0;i<361;i+=4){const uint8_t byte=r.Byte();
        for(int j=0;j<4&&i+j<361;++j){const int c=(byte>>(2*j))&3;
            if(c==3)r.ok=false;
            b[i+j]=static_cast<int8_t>(c-1);}
        if(i==360&&(byte&0xfcu))r.ok=false;
    }return b;
}
inline void Position(Writer &w,const BoardState::ChessPosition &p){
    for(int i=0;i<64;i+=2)w.Byte(static_cast<uint8_t>((p.pieces[i]+6)|((p.pieces[i+1]+6)<<4)));
    w.Byte(static_cast<uint8_t>(p.side+1));w.Byte(static_cast<uint8_t>(p.castles));
    w.Byte(static_cast<uint8_t>(p.enPassant+1));
}
inline BoardState::ChessPosition ReadPosition(Reader &r){
    BoardState::ChessPosition p;
    for(int i=0;i<64;i+=2){const uint8_t b=r.Byte();if((b&15)>12||(b>>4)>12)r.ok=false;
        p.pieces[i]=static_cast<int8_t>((b&15)-6);p.pieces[i+1]=static_cast<int8_t>((b>>4)-6);}
    p.side=static_cast<int8_t>(int(r.Byte())-1);p.castles=static_cast<int8_t>(r.Byte());
    p.enPassant=static_cast<int8_t>(int(r.Byte())-1);return p;
}
template<class Map> inline void History(Writer &w,const Map &map){
    std::vector<const typename Map::value_type *> order;order.reserve(map.size());
    for(const auto &p:map)order.push_back(&p);
    std::sort(order.begin(),order.end(),[](const auto *a,const auto *b){
        const auto &x=a->first,&y=b->first;
        if(x.pieces!=y.pieces)return x.pieces<y.pieces;
        if(x.side!=y.side)return x.side<y.side;
        if(x.castles!=y.castles)return x.castles<y.castles;
        return x.enPassant<y.enPassant;
    });
    w.U32(static_cast<uint32_t>(order.size()));
    for(const auto *p:order){Position(w,p->first);w.Int(p->second);}
}
template<class Map> inline void ReadHistory(Reader &r,Map &map){
    const uint32_t count=r.U32();if(!r.Count(count,39))return;
    for(uint32_t i=0;i<count&&r.ok;++i){auto p=ReadPosition(r);const int n=r.Int();
        if(n<1||!map.emplace(p,n).second)r.ok=false;}
}
inline bool Encode(int kind,const State &game,const Breakout &breakout,
                   const BoardState &board,std::vector<uint8_t> &output){
    if(kind<0||kind>7)return false;
    if(kind<3?!ValidateArcade(kind,game):kind==6?!ValidateBreakout(breakout):!ValidateBoard(kind,board))return false;
    try{
        Writer body;
        if(kind<3)ArcadeCore(body,game);
        else if(kind==6)BreakoutCore(body,breakout);
        else{
            BoardCore(body,board);
            if(kind==4){
                if(board.goHistory.size()>(kMaxBytes-kHeaderBytes)/91)return false;
                std::vector<const BoardState::Board *> order;order.reserve(board.goHistory.size());
                for(const auto &b:board.goHistory)order.push_back(&b);
                std::sort(order.begin(),order.end(),[](const auto *a,const auto *b){return *a<*b;});
                body.U32(static_cast<uint32_t>(order.size()));for(const auto *b:order)PackedGo(body,*b);
            }else if(kind==5){
                if(board.chessHistory.size()>(kMaxBytes-kHeaderBytes)/39||
                   board.chessSanHistory.size()>kMaxBytes/2)return false;
                History(body,board.chessHistory);body.U32(static_cast<uint32_t>(board.chessSanHistory.size()));
                for(const auto &san:board.chessSanHistory){body.Byte(static_cast<uint8_t>(san.size()));for(char c:san)body.Byte(static_cast<uint8_t>(c));}
            }else if(kind==3){
                if(board.checkerHistory.size()>(kMaxBytes-kHeaderBytes)/39)return false;
                History(body,board.checkerHistory);
            }
        }
        if(!body.ok)return false;
        Writer header;for(char c:std::string("RVCLASS1"))header.Byte(static_cast<uint8_t>(c));
        header.U32(kVersion);header.U32(static_cast<uint32_t>(kind));
        header.U32(static_cast<uint32_t>(body.bytes.size()));header.U32(0);
        header.U64(Hash(body.bytes.data(),body.bytes.size()));
        header.bytes.insert(header.bytes.end(),body.bytes.begin(),body.bytes.end());
        output=std::move(header.bytes);return true;
    }catch(...){return false;}
}
inline bool Decode(int kind,const std::vector<uint8_t> &bytes,Snapshot &output){
    if(kind<0||kind>7||bytes.size()<kHeaderBytes||bytes.size()>kMaxBytes)return false;
    Reader header{bytes.data(),bytes.size()};
    for(char c:std::string("RVCLASS1"))if(header.Byte()!=static_cast<uint8_t>(c))return false;
    if(header.U32()!=kVersion||header.U32()!=static_cast<uint32_t>(kind)||
       header.U32()!=bytes.size()-kHeaderBytes||header.U32()!=0)return false;
    if(header.U64()!=Hash(bytes.data()+kHeaderBytes,bytes.size()-kHeaderBytes))return false;
    try{
        Snapshot candidate;Reader r{bytes.data()+kHeaderBytes,bytes.size()-kHeaderBytes};
        if(kind<3)ReadArcade(r,kind,candidate.game);
        else if(kind==6)ReadBreakout(r,candidate.breakout);
        else{
            ReadBoard(r,kind,candidate.board);
            if(kind==4){const uint32_t count=r.U32();if(!r.Count(count,91))return false;
                for(uint32_t i=0;i<count&&r.ok;++i)if(!candidate.board.goHistory.insert(ReadGo(r)).second)r.ok=false;
            }else if(kind==5){
                ReadHistory(r,candidate.board.chessHistory);const uint32_t count=r.U32();
                if(!r.Count(count,2))return false;
                for(uint32_t i=0;i<count&&r.ok;++i){const uint8_t n=r.Byte();
                    if(n==0||n>32||!r.Count(n,1))return false;
                    std::string san;for(int j=0;j<n;++j)san+=static_cast<char>(r.Byte());
                    candidate.board.chessSanHistory.push_back(std::move(san));}
            }else if(kind==3)ReadHistory(r,candidate.board.checkerHistory);
        }
        if(!r.ok||r.at!=r.size)return false;
        if(kind<3?!ValidateArcade(kind,candidate.game):kind==6?!ValidateBreakout(candidate.breakout):!ValidateBoard(kind,candidate.board))return false;
        output=std::move(candidate);return true;
    }catch(...){return false;}
}
// O(board area), allocation-free, and independent of history length. Public
// rule operations that change history also change ply/review/serial or size.
// Only checkpoints encode/sort the complete snapshot, never an eye draw.
inline uint64_t Fingerprint(int kind,const State &game,const Breakout &breakout,const BoardState &board){
    Fingerprinter f;f.Int(kind);
    if(kind<3)ArcadeCore(f,game);
    else if(kind==6)BreakoutCore(f,breakout);
    else{BoardCore(f,board);f.U32(static_cast<uint32_t>(board.goHistory.size()));
        f.U32(static_cast<uint32_t>(board.chessHistory.size()));f.U32(static_cast<uint32_t>(board.chessSanHistory.size()));
        f.U32(static_cast<uint32_t>(board.checkerHistory.size()));}
    return f.value;
}

struct Manifest {
    uint64_t generation=0,hash=0;
    uint32_t bank=0,bytes=0,pages=0;
};
inline std::vector<uint8_t> EncodeManifest(int kind,const Manifest &m){
    Writer w;for(char c:std::string("RVCLIDX1"))w.Byte(static_cast<uint8_t>(c));
    w.U32(kVersion);w.U32(static_cast<uint32_t>(kind));w.U64(m.generation);
    w.U32(m.bank);w.U32(m.bytes);w.U32(m.pages);w.U32(0);w.U64(m.hash);
    w.U64(Hash(w.bytes.data(),w.bytes.size()));return w.bytes;
}
inline bool DecodeManifest(int kind,const uint8_t *bytes,size_t size,Manifest &m){
    if(size!=56)return false;
    Reader r{bytes,size};for(char c:std::string("RVCLIDX1"))if(r.Byte()!=static_cast<uint8_t>(c))return false;
    if(r.U32()!=kVersion||r.U32()!=static_cast<uint32_t>(kind))return false;
    Manifest next;next.generation=r.U64();next.bank=r.U32();next.bytes=r.U32();next.pages=r.U32();
    if(r.U32()!=0)return false;
    next.hash=r.U64();
    if(r.U64()!=Hash(bytes,48)||next.generation==0||next.bank>1||
       next.bytes<kHeaderBytes||next.bytes>kMaxBytes||next.pages==0||next.pages>kPages||
       next.pages!=(next.bytes+kPageBytes-1)/kPageBytes)return false;
    m=next;return true;
}
inline std::string PageName(uint32_t bank,uint32_t page){
    return std::string(bank?"classic-b-":"classic-a-")+char('0'+page/10)+char('0'+page%10)+".v1";
}
enum class Status { Fresh, Saved, Loaded, Unavailable, ReadError, WriteError, InvalidState };
class Store {
    const ReverieNativeHostV1 *host=nullptr;
    int kind=0;
    Manifest committed{};
    bool hasCommit=false,commitKnown=true;
    int ReadManifest(Manifest &out){
        uint8_t bytes[56]{};uint32_t size=0;
        const int result=host->read_save(kManifest,bytes,sizeof(bytes),&size);
        if(result==REVERIE_NATIVE_SAVE_NOT_FOUND&&size==0)return 0;
        if(result!=REVERIE_NATIVE_SAVE_OK){detail="Save manifest could not be read";return -1;}
        if(!DecodeManifest(kind,bytes,size,out)){detail="Save manifest is corrupt or unsupported";return -1;}
        return 1;
    }
public:
    Status status=Status::Fresh;
    const char *detail="No checkpoint yet";
    bool dirty=false,loadBlocked=false;
    float elapsed=0.f;
    uint64_t observed=0;
    void Bind(const ReverieNativeHostV1 *h,int gameKind){host=h;kind=gameKind;}
    bool Available() const {return ReverieNativeHostSupportsSaveV1(host)!=0;}
    // 1 restored, 0 no save, -1 unavailable/unreadable. An unreadable save is
    // never replaced by defaults. Retry reads again before allowing play.
    int Load(Snapshot &out){
        if(!Available()){loadBlocked=true;status=Status::Unavailable;detail="Host save service is unavailable";return -1;}
        try{
            Manifest next;const int found=ReadManifest(next);
            if(found==0){hasCommit=false;commitKnown=true;loadBlocked=false;status=Status::Fresh;return 0;}
            if(found<0){loadBlocked=true;status=Status::ReadError;return -1;}
            std::vector<uint8_t> bytes(next.bytes);uint32_t offset=0;
            for(uint32_t i=0;i<next.pages;++i){
                const uint32_t want=std::min(kPageBytes,next.bytes-offset);uint32_t got=0;
                const auto name=PageName(next.bank,i);
                if(host->read_save(name.c_str(),bytes.data()+offset,want,&got)!=REVERIE_NATIVE_SAVE_OK||got!=want){
                    detail="Committed checkpoint page is missing, short or unreadable";
                    loadBlocked=true;status=Status::ReadError;return -1;}
                offset+=want;
            }
            Snapshot candidate;
            if(Hash(bytes.data(),bytes.size())!=next.hash){
                detail="Committed checkpoint checksum mismatch";
                loadBlocked=true;status=Status::ReadError;return -1;}
            if(!Decode(kind,bytes,candidate)){
                detail="Committed checkpoint schema or state is invalid";
                loadBlocked=true;status=Status::ReadError;return -1;}
            out=std::move(candidate);committed=next;hasCommit=true;commitKnown=true;
            loadBlocked=false;dirty=false;elapsed=0.f;status=Status::Loaded;detail="Checkpoint restored";return 1;
        }catch(...){loadBlocked=true;status=Status::ReadError;detail="Checkpoint read allocation or callback failed";return -1;}
    }
    void Observe(uint64_t fingerprint,float delta){
        if(loadBlocked)return;
        if(fingerprint!=observed){observed=fingerprint;dirty=true;}
        if(dirty&&std::isfinite(delta)&&delta>0.f)elapsed+=std::min(delta,.1f);
    }
    bool Flush(const State &game,const Breakout &breakout,const BoardState &board){
        if(loadBlocked)return false;
        if(!dirty)return true;
        elapsed=0.f;
        if(!Available()){status=Status::Unavailable;detail="Host save service is unavailable";return false;}
        try{
            // A host can report a late error after an atomic manifest rename.
            // Reconcile that ambiguity before choosing the next inactive bank.
            if(!commitKnown){Manifest actual;const int found=ReadManifest(actual);
                if(found<0){status=Status::WriteError;return false;}
                hasCommit=found==1;if(hasCommit)committed=actual;commitKnown=true;}
            std::vector<uint8_t> bytes;
            if(!Encode(kind,game,breakout,board,bytes)){
                status=Status::InvalidState;detail="Snapshot invalid or above 4 MiB bound; prior checkpoint retained";return false;}
            if(hasCommit&&committed.generation==std::numeric_limits<uint64_t>::max()){
                status=Status::InvalidState;detail="Checkpoint generation exhausted";return false;}
            Manifest next;next.generation=hasCommit?committed.generation+1:1;
            next.bank=hasCommit?1-committed.bank:0;next.bytes=static_cast<uint32_t>(bytes.size());
            next.pages=(next.bytes+kPageBytes-1)/kPageBytes;next.hash=Hash(bytes.data(),bytes.size());
            for(uint32_t i=0,offset=0;i<next.pages;++i){
                const uint32_t count=std::min(kPageBytes,next.bytes-offset);const auto name=PageName(next.bank,i);
                if(host->write_save(name.c_str(),bytes.data()+offset,count)!=REVERIE_NATIVE_SAVE_OK){
                    status=Status::WriteError;detail="Checkpoint page write failed; prior commit retained";return false;}offset+=count;}
            const auto index=EncodeManifest(kind,next);commitKnown=false;
            if(host->write_save(kManifest,index.data(),static_cast<uint32_t>(index.size()))!=REVERIE_NATIVE_SAVE_OK){
                status=Status::WriteError;detail="Checkpoint manifest commit not confirmed; retry required";return false;}
            committed=next;hasCommit=true;commitKnown=true;dirty=false;status=Status::Saved;detail="Checkpoint committed";return true;
        }catch(...){status=Status::WriteError;detail="Checkpoint write allocation or callback failed";return false;}
    }
};

}}} // namespace reverie::classics::save
#endif
