#include "classic_games_save.h"
#include <cstdlib>
#include <iostream>
#include <map>

namespace cs=reverie::classics::save;
namespace bg=reverie::boardgames;
using reverie::classics::Game;
static int checks=0;
static void Check(bool ok,const char *what){
    ++checks;if(!ok){std::cerr<<"FAIL: "<<what<<'\n';std::exit(1);}
}
static std::map<std::string,std::vector<uint8_t>> slots;
static int writes=0,failWrite=0;
static bool failReads=false,lateManifestError=false;
static int32_t Read(const char *slot,void *data,uint32_t capacity,uint32_t *size){
    *size=0;if(failReads)return REVERIE_NATIVE_SAVE_ERROR;
    const auto it=slots.find(slot);if(it==slots.end())return REVERIE_NATIVE_SAVE_NOT_FOUND;
    *size=static_cast<uint32_t>(it->second.size());
    if(capacity<*size)return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    std::memcpy(data,it->second.data(),*size);return REVERIE_NATIVE_SAVE_OK;
}
static int32_t Write(const char *slot,const void *data,uint32_t size){
    ++writes;Check(size<=REVERIE_NATIVE_SAVE_MAX_BYTES,"host page bound");
    if(failWrite==writes)return REVERIE_NATIVE_SAVE_ERROR;
    const auto *p=static_cast<const uint8_t *>(data);slots[slot]={p,p+size};
    if(lateManifestError&&std::string(slot)==cs::kManifest){lateManifestError=false;return REVERIE_NATIVE_SAVE_ERROR;}
    return REVERIE_NATIVE_SAVE_OK;
}
static ReverieNativeHostV1 Host(){
    ReverieNativeHostV1 h{};h.struct_size=sizeof(h);h.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;
    h.read_save=Read;h.write_save=Write;return h;
}
static cs::Snapshot Initial(int kind){
    cs::Snapshot s;
    if(kind<3)s.game.Reset(static_cast<Game>(kind));
    else if(kind==6)s.breakout.Reset();
    else s.board.Reset(static_cast<bg::Kind>(kind));
    return s;
}
static std::vector<uint8_t> Encode(int kind,const cs::Snapshot &s){
    std::vector<uint8_t> result;
    Check(cs::Encode(kind,s.game,s.breakout,s.board,result),"encode valid production state");return result;
}
static cs::Snapshot RoundTrip(int kind,const cs::Snapshot &s){
    const auto encoded=Encode(kind,s);cs::Snapshot restored;
    Check(cs::Decode(kind,encoded,restored),"decode valid snapshot");
    Check(Encode(kind,restored)==encoded,"all serialized fields exact after round-trip");return restored;
}
static void RepairChecksum(std::vector<uint8_t> &data){
    const uint64_t hash=cs::Hash(data.data()+cs::kHeaderBytes,data.size()-cs::kHeaderBytes);
    for(int i=0;i<8;++i)data[24+i]=static_cast<uint8_t>(hash>>(8*i));
}
static void CodecCases(){
    for(int kind=0;kind<8;++kind){
        auto s=Initial(kind);RoundTrip(kind,s);
        if(kind<3){
            if(kind==1)s.game.Direction(0);
            s.game.Press();s.game.Step(.1f,.35f);s.game.Step(.1f,.35f);
        }else if(kind==3)Check(s.board.CheckerMove(40,33),"production checker move");
        else if(kind==4)Check(s.board.GoMove(180),"production Go move");
        else if(kind==5){Check(s.board.ChessMove(52,36),"production e4");
            s.board.chessDifficulty=2;s.board.promotionChoice=2;}
        else if(kind==6){s.breakout.Press();s.breakout.Step(.077f,.3f);}
        else Check(s.board.ReversiMove(19),"production Reversi move");
        auto restored=RoundTrip(kind,s);
        // Compare the next real simulation/CPU result, including seeded Go
        // choices and delayed Chutes roll, rather than just final counters.
        for(int i=0;i<8;++i){
            if(kind<3){s.game.Step(.1f,-.2f);restored.game.Step(.1f,-.2f);}
            else if(kind==6){s.breakout.Step(.031f,-.2f);restored.breakout.Step(.031f,-.2f);}
            else{s.board.Step(.1f);restored.board.Step(.1f);}
        }
        Check(Encode(kind,s)==Encode(kind,restored),"restored future simulation and RNG identical");
        const auto valid=Encode(kind,s);const auto sentinel=Initial(kind);
        for(size_t cut=0;cut<valid.size();++cut){
            cs::Snapshot target=sentinel;std::vector<uint8_t> truncated(valid.begin(),valid.begin()+cut);
            Check(!cs::Decode(kind,truncated,target),"every truncated length rejected");
            Check(Encode(kind,target)==Encode(kind,sentinel),"failed decode leaves destination intact");
        }
        auto broken=valid;broken.back()^=0x80;cs::Snapshot target;
        Check(!cs::Decode(kind,broken,target),"payload corruption rejected");
        broken=valid;broken[8]=2;Check(!cs::Decode(kind,broken,target),"unknown version rejected");
        broken=valid;broken.push_back(0);Check(!cs::Decode(kind,broken,target),"trailing data rejected");
        Check(!cs::Decode((kind+1)%8,valid,target),"cross-game save rejected");
        std::cout<<"codec kind="<<kind<<" bytes="<<valid.size()<<" PASS\n";
    }
    auto data=Encode(0,Initial(0)); // IEEE NaN in ball_x, with repaired checksum.
    data[59]=0;data[60]=0;data[61]=0xc0;data[62]=0x7f;RepairChecksum(data);
    cs::Snapshot target;Check(!cs::Decode(0,data,target),"semantic finite-value validation after checksum");
    data=Encode(1,Initial(1));data[44]=2;RepairChecksum(data);
    Check(!cs::Decode(1,data,target),"noncanonical boolean rejected after checksum");
    data=Encode(4,Initial(4));
    // BoardCore is 842 bytes; the following field is an untrusted count.
    for(int i=0;i<4;++i)data[cs::kHeaderBytes+842+i]=0xff;
    RepairChecksum(data);
    Check(!cs::Decode(4,data,target),"untrusted history count bounded before allocation");
}
static void RuleHistoryCases(){
    auto chess=Initial(5);
    for(int n=0;n<2;++n){
        Check(chess.board.ChessMove(62,45),"Nf3");Check(chess.board.ChessMove(6,21),"Nf6");
        Check(chess.board.ChessMove(45,62),"Ng1");Check(chess.board.ChessMove(21,6),"Ng8");
    }
    Check(chess.board.ChessDrawAvailable(1),"threefold claim available before save");
    const auto restored=RoundTrip(5,chess);
    Check(restored.board.ChessDrawAvailable(1)&&restored.board.ChessRepetitions()==3,
          "threefold and complete SAN/history survive");
    auto go=Initial(4);Check(go.board.GoMove(180),"Go review placement");
    Check(go.board.Pass()&&go.board.Pass(),"enter two-pass Go review");
    Check(go.board.ToggleGoDead(180),"mark dead group");
    auto resumed=RoundTrip(4,go);
    Check(resumed.board.goReview&&resumed.board.goDead[180]==1&&resumed.board.goResumeSide==-1,
          "review phase, markings and pending side preserved");
    Check(go.board.AcceptGoScore()&&resumed.board.AcceptGoScore(),"accept persisted Go score");
    Check(Encode(4,go)==Encode(4,resumed),"same accepted area result");
    // Explicit legal continuation fixture: White must finish a double jump.
    auto checker=Initial(3);checker.board.cells.fill(0);
    checker.board.cells[40]=1;checker.board.cells[33]=-1;checker.board.cells[19]=-1;
    checker.board.checkerHistory.clear();checker.board.checkerHistory.emplace(checker.board.CurrentCheckerPosition(),1);
    Check(checker.board.CheckerMove(40,26),"first mandatory jump");
    Check(checker.board.forced==26&&checker.board.side==1,"jump continuation remains White's turn");
    auto jump=RoundTrip(3,checker);
    Check(jump.board.forced==26&&jump.board.CheckerMove(26,12),"restored forced second jump legal");
    Check(checker.board.CheckerMove(26,12),"original forced second jump legal");
    Check(Encode(3,checker)==Encode(3,jump),"same completed capture turn");
    // Explicit checkers repetition-claim fixture, with exact occurrence counts.
    auto draw=Initial(3);draw.board.ply=8;draw.board.checkerQuietPlies=8;
    draw.board.checkerHistory[draw.board.CurrentCheckerPosition()]=3;
    Check(RoundTrip(3,draw).board.CheckerDrawClaimable(),"checkers occurrence count retained");
    std::cout<<"rule history and pending phases PASS\n";
}
static void FinishedCases(){
    auto pong=Initial(0);
    // Explicit near-score fixture, resolved by the production simulation.
    pong.game.score=10;pong.game.ball_x=1.04f;pong.game.moving=true;
    pong.game.velocity_x=.5f;pong.game.Step(.1f,0.f);
    Check(pong.game.finished&&RoundTrip(0,pong).game.score==11,"Pong final score retained");
    auto snake=Initial(1);snake.game.Press();
    for(int i=0;i<80&&!snake.game.finished;++i)snake.game.Step(.1f,0.f);
    Check(snake.game.finished&&RoundTrip(1,snake).game.finished,"Snake collision terminal retained");
    auto chutes=Initial(2);
    for(int i=0;i<10000&&!chutes.game.finished;++i){
        if(chutes.game.player_turn)chutes.game.Press();
        chutes.game.Step(.1f,0.f);
    }
    Check(chutes.game.finished&&RoundTrip(2,chutes).game.finished,"Chutes played final square retained");
    auto chess=Initial(5);
    Check(chess.board.ChessMove(53,45)&&chess.board.ChessMove(12,28)&&
          chess.board.ChessMove(54,38)&&chess.board.ChessMove(3,39),"production Fool's Mate");
    Check(chess.board.finished&&RoundTrip(5,chess).board.winner==-1,"Chess mate and SAN retained");
    auto breakout=Initial(6);
    for(int i=0;i<3;++i){breakout.breakout.Press();breakout.breakout.ballY=-.985f;
        breakout.breakout.velocityY=-.7f;breakout.breakout.Tick();}
    Check(breakout.breakout.finished&&RoundTrip(6,breakout).breakout.lives==0,"Breakout final life retained");
    breakout=Initial(6);auto &wall=breakout.breakout;wall.bricks.fill(0);
    wall.bricks[111]=1;wall.remaining=1;wall.wall=2;wall.score=895;
    wall.topRowBroken=wall.narrowPaddle=wall.moving=true;
    wall.ballX=wall.BrickX(13);wall.ballY=wall.BrickY(7);wall.Tick();
    Check(wall.won&&RoundTrip(6,breakout).breakout.score==896,"Breakout second-wall victory retained");
    auto reversi=Initial(7);
    for(int turn=0;turn<61&&!reversi.board.finished;++turn){
        bool moved=false;for(int at=0;at<64&&!moved;++at)moved=reversi.board.ReversiMove(at);
        Check(moved,"production Reversi continuation exists");
    }
    Check(reversi.board.finished&&RoundTrip(7,reversi).board.passes==2,"Reversi final pass/result retained");
    std::cout<<"terminal match states PASS\n";
}
static cs::Snapshot LargeGo(size_t count){
    auto s=Initial(4);
    // Codec/storage capacity fixture, not a claim that these snapshots form
    // a legal played game. Unique exact boards cross the host slot boundary.
    for(size_t n=1;n<count;++n){bg::State::Board b{};size_t value=n;
        for(int i=0;i<12;++i){b[i]=static_cast<int8_t>(int(value%3)-1);value/=3;}
        s.board.goHistory.insert(b);
    }
    s.board.ply=static_cast<int>(s.board.goHistory.size())-1;return s;
}
static void StoreCases(){
    slots.clear();writes=failWrite=0;failReads=lateManifestError=false;
    const auto host=Host();cs::Store store;store.Bind(&host,4);cs::Snapshot loaded;
    Check(store.Load(loaded)==0,"absent manifest starts fresh");
    auto old=LargeGo(900);const auto oldBytes=Encode(4,old);
    Check(oldBytes.size()>65536,"full superko history exceeds one host slot");
    store.dirty=true;Check(store.Flush(old.game,old.breakout,old.board),"commit multiple pages");
    const auto oldSlots=slots;
    cs::Store readback;readback.Bind(&host,4);
    Check(readback.Load(loaded)==1&&Encode(4,loaded)==oldBytes,"multi-page exact readback");
    auto next=old;next.board.goReviewRevision=1;
    store.dirty=true;failWrite=writes+2;
    Check(!store.Flush(next.game,next.breakout,next.board)&&store.dirty,"second page failure retains dirty state");
    Check(slots[cs::kManifest]==oldSlots.at(cs::kManifest),"failed bank never replaces commit record");
    cs::Store interrupted;interrupted.Bind(&host,4);
    Check(interrupted.Load(loaded)==1&&Encode(4,loaded)==oldBytes,"old complete save survives interrupted pages");
    failWrite=0;Check(store.Flush(next.game,next.breakout,next.board),"failed write retries successfully");
    Check(!store.dirty,"dirty cleared only by manifest commit");
    next.board.goReviewRevision=2;store.dirty=true;lateManifestError=true;
    Check(!store.Flush(next.game,next.breakout,next.board)&&store.dirty,"late commit error remains unresolved");
    const auto lateSlots=slots;failReads=true;
    Check(!store.Flush(next.game,next.breakout,next.board),"ambiguous commit requires readable manifest");
    Check(slots==lateSlots,"ambiguous retry never overwrites possibly active bank");
    failReads=false;next.board.goReviewRevision=3;
    Check(store.Flush(next.game,next.breakout,next.board),"ambiguous commit reconciled and retried");
    cs::Store latest;latest.Bind(&host,4);
    Check(latest.Load(loaded)==1&&Encode(4,loaded)==Encode(4,next),"latest progress survives ambiguous commit");
    const auto committedSlots=slots;slots[cs::kManifest][8]=0xff;
    cs::Store broken;broken.Bind(&host,4);const int beforeWrites=writes;
    Check(broken.Load(loaded)<0&&broken.loadBlocked,"unreadable manifest blocks defaults");
    broken.dirty=true;Check(!broken.Flush(old.game,old.breakout,old.board)&&writes==beforeWrites,
        "unreadable progress cannot be overwritten");
    slots=committedSlots;Check(broken.Load(loaded)==1&&!broken.loadBlocked,"explicit read retry recovers exact save");
    cs::Manifest index;Check(cs::DecodeManifest(4,slots[cs::kManifest].data(),slots[cs::kManifest].size(),index),"manifest decode");
    slots[cs::PageName(index.bank,0)].back()^=1;
    cs::Store corrupt;corrupt.Bind(&host,4);
    Check(corrupt.Load(loaded)<0&&corrupt.loadBlocked,"corrupt page blocked transactionally");
    slots=committedSlots;slots.erase(cs::PageName(index.bank,index.pages-1));
    cs::Store missing;missing.Bind(&host,4);
    Check(missing.Load(loaded)<0&&missing.loadBlocked,"missing committed page cannot become a fresh game");
    slots=committedSlots;const auto huge=LargeGo(47000);std::vector<uint8_t> bounded;
    Check(!cs::Encode(4,huge.game,huge.breakout,huge.board,bounded),"4 MiB bound fails without history truncation");
    store.dirty=true;const auto beforeLimit=slots;
    Check(!store.Flush(huge.game,huge.breakout,huge.board)&&store.dirty&&slots==beforeLimit,
          "oversize keeps dirty progress and prior commit");
    Check(huge.board.goHistory.size()==47000,"oversize source history remains complete");
    std::cout<<"bank interruption, read failures, corruption, retry and limits PASS\n";
}
int main(){CodecCases();RuleHistoryCases();FinishedCases();StoreCases();std::cout<<"classic save checks="<<checks<<" PASS\n";}
