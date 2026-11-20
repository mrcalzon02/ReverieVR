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
        broken=valid;broken[8]=255;Check(!cs::Decode(kind,broken,target),"unknown version rejected");
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
static void ChutesRollHistory(){
    auto s=Initial(2);
    s.game.Press();s.game.Step(.1f,0.f);
    auto legacy=Encode(2,s);
    legacy.resize(legacy.size()-4);
    legacy[8]=1;
    const auto size=static_cast<uint32_t>(legacy.size()-cs::kHeaderBytes);
    for(int i=0;i<4;++i)legacy[16+i]=static_cast<uint8_t>(size>>(8*i));
    RepairChecksum(legacy);
    // Exact byte fingerprint of the same mid-CPU-turn save from build 2026.
    Check(cs::Hash(legacy.data(),legacy.size())==0x726dcc428b2aaa91ull,"original v1 checkpoint bytes");
    cs::Snapshot migrated;
    Check(cs::Decode(2,legacy,migrated),"load v1 match");
    Check(migrated.game.playerRoll.die==0&&migrated.game.cpuRoll.die==0,
          "v1 has no fabricated roll history");
    for(int i=0;i<8;++i){s.game.Step(.1f,0.f);migrated.game.Step(.1f,0.f);}
    Check(migrated.game.cpuRoll.die==s.game.cpuRoll.die&&migrated.game.rival==s.game.rival&&
          migrated.game.seed==s.game.seed,"v1 resumes exact pending CPU roll");
    migrated.game.Press();
    auto roundTrip=RoundTrip(2,migrated);
    Check(roundTrip.game.playerRoll.die&&roundTrip.game.cpuRoll.die,"both rolls preserved");
    auto invalid=Encode(2,migrated);invalid.back()=7;RepairChecksum(invalid);
    Check(!cs::Decode(2,invalid,roundTrip),"invalid saved die rejected");
    invalid=Encode(2,migrated);invalid[invalid.size()-2]=99;RepairChecksum(invalid);
    Check(!cs::Decode(2,invalid,roundTrip),"roll inconsistent with token rejected");
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
// Real king moves must cross the WCDF 40-moves-each quiet threshold
// without granting an early claim. The 79-ply fixture has a single
// recorded position, isolating the quiet-clock rule from repetition;
// all subsequent moves, claims and checkpoint bytes use production code.
static void CheckerQuietFortyTurnSaveBoundary(){
    auto at79=Initial(3);
    auto &board=at79.board;
    board.cells.fill(0);
    board.cells[40]=2;board.cells[14]=-2; // White and Black kings
    board.side=-1;board.ply=79;board.checkerQuietPlies=79;
    board.checkerHistory.clear();
    board.checkerHistory.emplace(board.CurrentCheckerPosition(),1);
    Check(board.forced==-1&&!board.finished&&board.CheckerRepetitions()==1,
          "79-ply king fixture has no forced capture or repetition");
    Check(!board.CheckerDrawAvailable(-1)&&!board.CheckerDrawClaimable(),
          "39+40 quiet turns cannot claim before the 80th ply");
    auto resumed=RoundTrip(3,at79);
    Check(resumed.board.ply==79&&resumed.board.side==-1&&
          resumed.board.checkerQuietPlies==79&&
          !resumed.board.CheckerDrawAvailable(-1),
          "checkpoint at ply 79 keeps draw unavailable");
    const auto before=Encode(3,at79);
    Check(board.CheckerMove(14,21)&&resumed.board.CheckerMove(14,21),
          "both histories execute the actual 80th legal quiet king move");
    Check(board.ply==80&&board.side==1&&board.forced==-1&&
          board.checkerQuietPlies==80&&!board.finished,
          "40 quiet moves by each side opens a claim on White's turn");
    Check(board.CheckerDrawClaimable()&&resumed.board.CheckerDrawClaimable(),
          "original and resumed boards both enable the quiet claim");
    Check(Encode(3,at79)!=before&&Encode(3,at79)==Encode(3,resumed),
          "same 80th move produces identical changed checkpoint bytes");
    auto at80=RoundTrip(3,at79);
    Check(at80.board.ply==80&&at80.board.side==1&&
          at80.board.checkerQuietPlies==80&&at80.board.CheckerDrawClaimable(),
          "reloading at the exact threshold preserves the claim");
    Check(board.ClaimCheckerDraw()&&resumed.board.ClaimCheckerDraw()&&
          at80.board.ClaimCheckerDraw(),
          "all three states accept the same single legal draw claim");
    Check(at80.board.finished&&at80.board.winner==0&&
          !at80.board.ClaimCheckerDraw()&&!at80.board.CheckerMove(40,33),
          "draw is terminal, neither duplicate claims nor further moves count");
    Check(Encode(3,at79)==Encode(3,resumed)&&
          Encode(3,at79)==Encode(3,at80),
          "claimed terminal state is byte-identical after reload");

    // A genuine man advancement resets an otherwise claimable clock.
    auto man=Initial(3);
    man.board.cells.fill(0);
    man.board.cells[40]=2;man.board.cells[14]=-2;man.board.cells[1]=-1;
    man.board.side=-1;man.board.ply=79;man.board.checkerQuietPlies=79;
    man.board.checkerHistory.clear();
    man.board.checkerHistory.emplace(man.board.CurrentCheckerPosition(),1);
    auto manLoaded=RoundTrip(3,man);
    Check(man.board.CheckerMove(1,8)&&manLoaded.board.CheckerMove(1,8),
          "legal uncrowned Black man advancement resets quiet clock");
    Check(man.board.checkerQuietPlies==0&&man.board.side==1&&
          !man.board.CheckerDrawClaimable()&&
          manLoaded.board.checkerQuietPlies==0,
          "man move precludes a false 40-turn claim after reload");
    Check(Encode(3,man)==Encode(3,manLoaded),
          "man-reset save and restored state agree");
    auto manAfter=RoundTrip(3,man);
    Check(man.board.CheckerMove(40,33)&&manAfter.board.CheckerMove(40,33),
          "next ordinary king turn remains playable following the reset");
    Check(man.board.checkerQuietPlies==1&&
          Encode(3,man)==Encode(3,manAfter),
          "quiet clock restarts at one, never retains 79 after man move");

    // A mandatory real capture also resets the clock and round-trips.
    auto captured=Initial(3);
    captured.board.cells.fill(0);
    captured.board.cells[40]=2;captured.board.cells[14]=-2;
    captured.board.cells[21]=1; // Black king 14 -> 28 jumps White man
    captured.board.side=-1;captured.board.ply=79;
    captured.board.checkerQuietPlies=79;
    captured.board.checkerHistory.clear();
    captured.board.checkerHistory.emplace(
        captured.board.CurrentCheckerPosition(),1);
    Check(captured.board.CheckersCapture(14,28),
          "real king jump is mandatory in the 79-ply capture fixture");
    auto captureLoaded=RoundTrip(3,captured);
    Check(captured.board.CheckerMove(14,28)&&
          captureLoaded.board.CheckerMove(14,28),
          "original and loaded king both make the actual capture");
    Check(captured.board.checkerQuietPlies==0&&
          captured.board.cells[21]==0&&captured.board.cells[28]==-2&&
          captured.board.side==1&&captured.board.forced==-1&&
          !captured.board.CheckerDrawClaimable(),
          "capture removes victim, resets clock and bars quiet claim");
    Check(Encode(3,captured)==Encode(3,captureLoaded),
          "captured position and reset clock survive save and reload");
    auto captureAfter=RoundTrip(3,captured);
    Check(captureAfter.board.checkerQuietPlies==0&&
          !captureAfter.board.CheckerDrawClaimable(),
          "reloaded capture does not resurrect a forty-turn claim");
    std::cout<<"Checkers 79-to-80 quiet draw, claim and man/capture save resets PASS\\n";
}

// Horizontal king exposure: f5xg6 e.p. removes BOTH f5 and g5 blockers,
// revealing the h5 rook's attack on e5.  The illegal capture must not
// mutate either the live match or its round-tripped host checkpoint.
static void ChessPinnedEnPassantCheckpoint(){
    auto setup=[](bool withRook){
        auto s=Initial(5);
        auto &b=s.board;
        b.cells.fill(0);
        b.cells[0]=-6;   // Black king a8
        b.cells[28]=6;   // White king e5
        b.cells[29]=1;   // White pawn f5, first rook-ray blocker
        b.cells[14]=-1;  // Black pawn g7, about to advance to g5
        if(withRook)b.cells[31]=-4; // Black rook h5
        b.side=-1;b.castles=0;b.enPassant=-1;
        b.chessHistory.clear();
        b.chessHistory.emplace(b.CurrentChessPosition(),1);
        return s;
    };
    auto pinned=setup(true);
    auto &b=pinned.board;
    Check(!b.KingInCheck(b.cells,1)&&!b.KingInCheck(b.cells,-1),
          "both kings safe before black g7-g5");
    Check(b.ChessMove(14,30)&&b.enPassant==22&&b.side==1&&b.ply==1,
          "real black double-step creates g6 en-passant target");
    Check(b.cells[29]==1&&b.cells[30]==-1&&b.cells[31]==-4,
          "two adjacent rank-five blockers still shield white king");
    Check(b.CurrentChessPosition().enPassant==-1,
          "repetition identity excludes the illegal pinned capture");
    const auto before=Encode(5,pinned);
    auto loaded=RoundTrip(5,pinned);
    Check(Encode(5,loaded)==before&&loaded.board.enPassant==22,
          "raw en-passant offer and rights survive the checkpoint");
    Check(!b.ChessLegal(29,22)&&!loaded.board.ChessLegal(29,22),
          "en-passant from f5 to g6 would expose rook check on e5");
    Check(!b.ChessMove(29,22)&&!loaded.board.ChessMove(29,22),
          "both original and restored games reject pinned en-passant");
    Check(Encode(5,pinned)==before&&Encode(5,loaded)==before,
          "rejected capture changes no pawn, king, turn, SAN or save byte");
    Check(b.cells[28]==6&&b.cells[29]==1&&b.cells[30]==-1&&
          b.chessCapturedByWhite[1]==0&&b.chessSanHistory.size()==1,
          "illegal capture has no ghost victim or notation");
    Check(b.ChessLegal(29,21)&&loaded.board.ChessLegal(29,21),
          "f5-f6 remains a legal forward pawn move through the pin");
    Check(b.ChessMove(29,21)&&loaded.board.ChessMove(29,21),
          "both states execute the actual legal alternative");
    Check(b.cells[21]==1&&b.cells[30]==-1&&b.side==-1&&
          Encode(5,pinned)==Encode(5,loaded),
          "saved and uninterrupted legal continuation remain identical");

    // Control: without the h5 rook, the very same black double-step
    // permits a real en-passant capture, proving the rejection was
    // about king exposure rather than a blanket pawn/codec refusal.
    auto clear=setup(false);
    Check(clear.board.ChessMove(14,30)&&
          clear.board.CurrentChessPosition().enPassant==22,
          "without rook g6 is an effective repetition right");
    auto clearLoaded=RoundTrip(5,clear);
    Check(clear.board.ChessLegal(29,22)&&clearLoaded.board.ChessLegal(29,22),
          "same en-passant geometry is legal when rook line is absent");
    Check(clear.board.ChessMove(29,22)&&
          clearLoaded.board.ChessMove(29,22),
          "both states perform lawful en-passant after checkpoint");
    Check(clear.board.cells[22]==1&&clear.board.cells[30]==0&&
          clear.board.chessCapturedByWhite[1]==1&&
          Encode(5,clear)==Encode(5,clearLoaded),
          "real en-passant removes the victim once and saves identically");
    std::cout<<"Chess pinned en-passant save rejection and legal control PASS\\n";
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
// Real first-wall -> second-wall rule transition, checked codec, and a
// failed host checkpoint before manifest commit. Terminal wall-two fixtures
// alone do not exercise this boundary.
static void BreakoutFirstWallSaveBoundary(){
    auto before=Initial(6);
    auto &wall=before.breakout;
    wall.bricks.fill(0);wall.bricks[111]=1;wall.remaining=1;wall.score=447;
    wall.topRowBroken=wall.narrowPaddle=wall.moving=true;
    wall.ballX=wall.BrickX(13);wall.ballY=wall.BrickY(7);
    const auto oldBytes=Encode(6,before);
    const auto old=RoundTrip(6,before);
    Check(old.breakout.wall==1&&old.breakout.remaining==1,
          "last wall-one brick preserved before actual hit");

    wall.Tick(); // production collision scores the last brick and refills.
    Check(wall.wall==2&&wall.remaining==112&&wall.score==448,
          "wall-one final brick starts exactly 112 wall-two bricks");
    Check(wall.lives==3&&wall.moving&&!wall.finished&&!wall.won,
          "transition keeps ball in play without losing a life or winning");
    for(uint8_t brick:wall.bricks)Check(brick==1,"all new wall-two bricks present");
    const auto boundary=RoundTrip(6,before);
    const auto boundaryBytes=Encode(6,boundary);
    Check(boundaryBytes!=oldBytes,"transition changes the real checkpoint");

    slots.clear();writes=failWrite=0;failReads=lateManifestError=false;
    const auto host=Host();cs::Store store;store.Bind(&host,6);
    store.dirty=true;
    Check(store.Flush(old.game,old.breakout,old.board),
          "pretransition state durably saved");
    const auto lastCommit=slots;
    store.dirty=true;failWrite=writes+1;
    Check(!store.Flush(boundary.game,boundary.breakout,boundary.board)&&store.dirty,
          "interrupted wall transition remains uncommitted and dirty");
    Check(slots[cs::kManifest]==lastCommit.at(cs::kManifest),
          "failed wall-two page never switches manifest");
    cs::Snapshot recovered;cs::Store previous;previous.Bind(&host,6);
    Check(previous.Load(recovered)==1&&Encode(6,recovered)==oldBytes,
          "interrupted save restores wall-one checkpoint, not phantom wall two");

    failWrite=0;
    Check(store.Flush(boundary.game,boundary.breakout,boundary.board),
          "wall-two transition is durably committed on retry");
    cs::Store reloaded;reloaded.Bind(&host,6);
    Check(reloaded.Load(recovered)==1&&Encode(6,recovered)==boundaryBytes,
          "committed transition restores exact wall, score and moving ball");
    auto uninterrupted=boundary;uninterrupted.breakout.Tick();
    recovered.breakout.Tick(); // real next collision with a wall-two brick.
    Check(recovered.breakout.wall==2&&recovered.breakout.remaining==111&&
          recovered.breakout.score==449&&!recovered.breakout.finished,
          "first wall-two hit earns only its own point, not another first-wall reward");
    Check(Encode(6,uninterrupted)==Encode(6,recovered),
          "restored and uninterrupted second-wall ball/brick states stay identical");
    std::cout<<"Breakout wall-one boundary, write interruption, wall-two continuation PASS\n";
}
// Cross the *last* life-loss boundary using production ball physics, both
// uninterrupted and from a checked, durably reloaded near-miss checkpoint.
// Prior terminal fixtures never resumed the pending final miss from storage.
static void BreakoutLastLifeSaveBoundary(){
    auto before=Initial(6);
    auto &ball=before.breakout;
    ball.Press();
    ball.ballX=reverie::classics::Breakout::BrickX(0);
    ball.ballY=reverie::classics::Breakout::BrickY(0)-.080f;
    ball.velocityY=.7f;
    ball.Tick(); // genuine brick hit: retain its score over game over/reload.
    Check(ball.score==7&&ball.remaining==111,"real Breakout score before last-life checkpoint");
    for(int miss=0;miss<2;++miss){
        ball.ballX=.5f;ball.ballY=-.985f;ball.velocityY=-.7f;
        ball.Step(.01f,0.f);
        Check(ball.lives==2-miss&&!ball.finished&&!ball.moving,
              "first two real floor misses decrement once and await serve");
        ball.Press();
    }
    Check(ball.lives==1&&ball.moving,"third served ball is live on final life");
    ball.ballX=.5f;ball.ballY=-.985f;ball.velocityY=-.7f;
    const auto nearMissBytes=Encode(6,before);
    const auto checked=RoundTrip(6,before);
    Check(Encode(6,checked)==nearMissBytes,"near-miss checked bytes are stable");

    slots.clear();writes=failWrite=0;failReads=lateManifestError=false;
    const auto host=Host();cs::Store store;store.Bind(&host,6);
    store.dirty=true;
    Check(store.Flush(before.game,before.breakout,before.board),
          "near-miss checkpoint committed to real banked host store");
    cs::Snapshot restored;cs::Store reader;reader.Bind(&host,6);
    Check(reader.Load(restored)==1&&Encode(6,restored)==nearMissBytes,
          "last-life pending floor crossing survives real host reload");

    before.breakout.Step(.01f,0.f);
    restored.breakout.Step(.01f,0.f);
    Check(before.breakout.lives==0&&before.breakout.finished&&
          !before.breakout.moving&&!before.breakout.won&&
          before.breakout.score==7&&before.breakout.remaining==111,
          "actual bottom crossing ends match exactly once without losing points");
    const auto terminalBytes=Encode(6,before);
    Check(Encode(6,restored)==terminalBytes,
          "restored last-life crossing matches uninterrupted simulation byte-for-byte");
    store.dirty=true;
    Check(store.Flush(before.game,before.breakout,before.board),
          "terminal score and game-over durably checkpoint once");
    cs::Snapshot terminal;cs::Store finalReader;finalReader.Bind(&host,6);
    Check(finalReader.Load(terminal)==1&&Encode(6,terminal)==terminalBytes,
          "new module load preserves terminal score with no phantom served ball");
    for(int frame=0;frame<16;++frame){
        before.breakout.Step(.1f,0.f);
        terminal.breakout.Step(.1f,0.f);
    }
    Check(Encode(6,before)==terminalBytes&&Encode(6,terminal)==terminalBytes,
          "repeated idle frames cannot award extra life, decrement twice or serve");
    auto again=RoundTrip(6,terminal);
    Check(Encode(6,again)==terminalBytes,"repeated terminal reload remains stable");
    terminal.breakout.Press();again.breakout.Press();
    Check(terminal.breakout.lives==3&&terminal.breakout.score==0&&
          terminal.breakout.remaining==112&&!terminal.breakout.finished&&
          !terminal.breakout.moving,"explicit restart resets result without auto-serving");
    terminal.breakout.Press();again.breakout.Press();
    terminal.breakout.Step(.03f,.2f);again.breakout.Step(.03f,.2f);
    Check(terminal.breakout.moving&&terminal.breakout.lives==3&&
          Encode(6,terminal)==Encode(6,again),
          "fresh legal serve advances identically after restored game-over");
    std::cout<<"Breakout last-life checked checkpoint, game-over and fresh serve PASS\\n";
}
int main(){BreakoutFirstWallSaveBoundary();BreakoutLastLifeSaveBoundary();CodecCases();ChutesRollHistory();RuleHistoryCases();ChessPinnedEnPassantCheckpoint();CheckerQuietFortyTurnSaveBoundary();FinishedCases();StoreCases();std::cout<<"classic save checks="<<checks<<" PASS\n";}
