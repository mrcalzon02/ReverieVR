#include "classic_games.h"
#include "classic_breakout.h"
#include "classic_board_games.h"
#include "classic_board_cpu.h"
#include <thread>
#include "reverie_native_flat_mesh.h"
#include "classic_vector_art.h"
#include "classic_chess_book.h"
#include "classic_chess_reference.h"
#include "classic_chess_sprite_atlas.h"
#include "classic_chess_wood_atlas.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::classics;


static void TestChessWoodAtlas(){
    using namespace reverie::chesswood;
    assert(kWidth==64&&kHeight==32&&kTile==32);
    std::vector<uint16_t> pixels;
    assert(Decode(pixels)&&pixels.size()==static_cast<size_t>(kPixelCount));
    int uniqueLight=0,uniqueDark=0;
    double light=0.0,dark=0.0;
    for(int band=0;band<2;++band){
        for(int y=0;y<32;++y)for(int x=0;x<32;++x){
            const uint16_t pixel=pixels[y*kWidth+x+band*32];
            // Actual source image grain, not a single-color rectangle.
            bool seenEarlier=false;
            for(int prevY=0;prevY<=y && !seenEarlier;++prevY){
                const int lim=prevY==y?x:32;
                for(int prevX=0;prevX<lim;++prevX){
                    if(pixels[prevY*kWidth+prevX+band*32]==pixel){
                        seenEarlier=true;break;
                    }
                }
            }
            if(!seenEarlier)(band?uniqueDark:uniqueLight)++;
            const double luminance=(.2126*((pixel>>11)&31)/31.0+
                                    .7152*((pixel>>5)&63)/63.0+
                                    .0722*(pixel&31)/31.0);
            if(band)dark+=luminance;
            else light+=luminance;
        }
    }
    assert(uniqueLight>=6&&uniqueDark>=6);
    // Both wood sources remain distinguishable under PSX-era quantization.
    assert((light-dark)/1024.0>.25);
}
static void TestChessSpriteAtlas(){
    using namespace reverie::chessart;
    std::vector<uint8_t> image;
    assert(Decode(image));
    assert(image.size()==kPixels*2);
    assert(kWidth==384&&kHeight==128);
    for(int kind=1;kind<=6;kind++){
        assert(TileForPiece(kind)==kind-1);
        assert(TileForPiece(-kind)==kind+5);
    }
    assert(TileForPiece(0)==-1&&TileForPiece(7)==-1);
    // Queen and king must be different actual atlas silhouettes in BOTH
    // colors, not just adjacent tile numbers with duplicate glyphs.
    for(const int base:{0,6}){
        const int queenTile=base+4,kingTile=base+5;
        int differentPixels=0;
        for(int y=0;y<64;++y)for(int x=0;x<64;++x){
            const std::size_t q=static_cast<std::size_t>(
                ((queenTile/6)*64+y)*kWidth+(queenTile%6)*64+x)*2;
            const std::size_t k=static_cast<std::size_t>(
                ((kingTile/6)*64+y)*kWidth+(kingTile%6)*64+x)*2;
            if(image[q]!=image[k]||image[q+1]!=image[k+1])
                ++differentPixels;
        }
        assert(differentPixels>200);
    }
    for(int tile=0;tile<12;++tile){
        int lit=0,black=0,solid=0;
        for(int y=0;y<64;++y)for(int x=0;x<64;++x){
            const int ox=(tile%6)*64+x,oy=(tile/6)*64+y;
            const std::size_t offset=(oy*kWidth+ox)*2;
            const int l=image[offset],a=image[offset+1];
            if(a>0){++lit;if(l<128)++black;}
            if(a==255)++solid;
        }
        // The original SVG shapes must be present, not letter glyphs
        // or an all-transparent texture accidentally shipping in the APK.
        assert(lit>=500&&lit<=2800);
        assert(solid>=150);
        assert(black>=180);
    }
    // Specific repair guard: both knights' main silhouettes are filled,
    // including the SVG paths without an explicit Z close command.
    for(int tile:{1,7}){
        int filled=0;
        for(int y=0;y<64;++y)for(int x=0;x<64;++x){
            const int at=(((tile/6)*64+y)*kWidth+(tile%6)*64+x)*2;
            if(image[at+1]>=200)++filled;
        }
        assert(filled>=700);
    }
}


static void TestChessIdentityHoverAndReference() {
    using reverie::boardgames::Kind;
    using reverie::boardgames::State;
    using namespace reverie::chessref;
    State game;game.Reset(Kind::Chess);
    // The actual playable board is White human at bottom; Black CPU on top.
    assert(game.cells[59]==5&&game.cells[60]==6);
    assert(game.cells[3]==-5&&game.cells[4]==-6);
    assert(PieceName(game.cells[59])=="WHITE QUEEN");
    assert(PieceName(game.cells[60])=="WHITE KING");
    assert(PieceName(game.cells[3])=="BLACK QUEEN");
    assert(PieceName(game.cells[4])=="BLACK KING");
    assert(Square(59)=="d1"&&Square(60)=="e1");
    assert(Square(3)=="d8"&&Square(4)=="e8");
    assert(Square(-1)=="--");
    assert(Destinations(game,59).empty()); // queen is blocked at d1
    assert(Destinations(game,3).empty()); // black queen is blocked at d8
    auto white=Destinations(game,52); // e2 pawn
    assert(white.size()==2&&white[0]==36&&white[1]==44);
    auto black=Destinations(game,12); // e7 pawn during White's turn
    assert(black.size()==2&&black[0]==20&&black[1]==28);
    assert(std::find(white.begin(),white.end(),28)==white.end());
    assert(!game.ChessMove(12,28)); // Black cannot play on White's turn
    assert(!game.ChessMove(59,31)); // blocked queen cannot jump
    assert(game.ChessMove(52,36));
    assert(!game.ChessMove(52,44)); // wrong turn and now empty
    assert(game.ChessMove(12,28)); // legal Black response
    assert(game.ChessLegal(59,31)); // White queen d1-h5 now clear
    const auto queen=Destinations(game,59);
    assert(std::find(queen.begin(),queen.end(),31)!=queen.end());
    assert(kEntries==3865);
    const auto start=Read(0),last=Read(kEntries-1);
    assert(start.eco=="A00"&&start.name=="Amar Opening");
    assert(last.eco=="E99");
    assert(Read(-1).name.empty()&&Read(kEntries).name.empty());
    bool grob=false;
    for(int i=0;i<kEntries;++i) {
        const auto entry=Read(i);
        if(entry.name=="Grob Opening"){
            assert(entry.eco=="A00"&&entry.san=="g4");
            grob=true;break;
        }
    }
    assert(grob);
    assert(!Matching(game).empty());
    const int current=Current(game);
    assert(current>=0&&current<kEntries);
    assert(Read(current).san.find("e4")!=std::string::npos);
    State fresh;fresh.Reset(Kind::Chess);
    assert(Current(fresh)==-1);
    auto wrapped=Wrap("The queen moves diagonally or in straight lines",17);
    assert(wrapped.size()>1);
    std::string recovered;
    for(const auto &line:wrapped){
        if(!recovered.empty())recovered+=" ";
        recovered+=line;
        assert(line.size()<=17);
    }
    assert(recovered=="The queen moves diagonally or in straight lines");
}
static void TestFlatGeometry(){
    using namespace reverie::flatmesh;
    std::vector<float> a,b;
    Rect(a,0.f,0.f,2.f,2.f,-3.f,1.f,.5f,0.f);
    Rect(b,0.f,0.f,2.f,2.f,-3.f,1.f,.5f,0.f);
    assert(a==b&&a.size()==36u);
    assert(a[0]==-1.f&&a[1]==-1.f&&a[2]==-3.f);
    assert(a[6]==1.f&&a[7]==-1.f);
    Segment(a,0.f,0.f,1.f,0.f,.1f,-2.f,1.f,1.f,1.f);
    assert(a.size()==72u);
    Segment(a,0.f,0.f,0.f,0.f,.1f,-2.f,1.f,1.f,1.f);
    assert(a.size()==72u);
    Disc(a,0.f,0.f,.25f,-2.f,1.f,1.f,1.f);
    assert(a.size()==288u);
    for(float f:a)assert(std::isfinite(f));
}
static void TestClassicArtworkAndOpeningIndex(){
    using reverie::classicart::StrokeText;
    using reverie::classicart::ChessPiece;
    using reverie::boardgames::Kind;
    using reverie::boardgames::State;
    // The shared font is line-based, never a 3x5 grid of isolated cubes.
    std::vector<float> textGeometry;
    StrokeText(textGeometry,"CHESS 2026: Nf3",0.f,0.f,.03f,1.f,1.f,1.f);
    assert(!textGeometry.empty()&&textGeometry.size()%18u==0u);
    for(float value:textGeometry)assert(std::isfinite(value));
    for(int piece=1;piece<=6;piece++){
        std::vector<float> light,dark;
        ChessPiece(light,piece,0.f,0.f,.1f,.1f,.1f,.1f);
        ChessPiece(dark,-piece,0.f,0.f,.1f,.9f,.9f,.9f);
        assert(light.size()>=108u&&dark.size()==light.size());
        for(float value:light)assert(std::isfinite(value));
        for(float value:dark)assert(std::isfinite(value));
    }
    // All ECO books A-E are compiled offline, not downloaded in a headset.
    constexpr size_t openings=sizeof(reverie::chessbook::kOpeningLines)/
                              sizeof(reverie::chessbook::kOpeningLines[0]);
    assert(openings==3865u);
    State chess;
    chess.Reset(Kind::Chess);
    auto start=reverie::chessbook::Analyze(chess);
    assert(start.matching&&start.from>=0&&start.to>=0);
    assert(chess.ChessLegal(start.from,start.to));
    assert(chess.ChessMoveNotation(52,36)=="e4");
    assert(chess.ChessMove(52,36)); // 1. e4
    assert(chess.chessSanHistory.size()==1u);
    assert(chess.chessSanHistory[0]=="e4");
    assert(chess.ChessMoveNotation(12,28)=="e5");
    assert(chess.ChessMove(12,28)); // ...e5
    assert(chess.chessSanHistory[1]=="e5");
    auto book=reverie::chessbook::Analyze(chess);
    assert(book.previousCpuMove=="e5");
    assert(book.cpuComparison=="IN BOOK");
    assert(book.matching&&book.candidates>0);
    assert(book.recommendationOrigin=="BOOK CONTINUATION");
    assert(book.from>=0&&chess.ChessLegal(book.from,book.to));
    // Proper knight disambiguation and both players' notation histories are
    // maintained by real legal moves, not by guessing from a board picture.
    assert(chess.ChessMoveNotation(62,45)=="Nf3");
    assert(chess.ChessMove(62,45));
    assert(chess.chessSanHistory.back()=="Nf3");
    chess.Reset(Kind::Chess);
    assert(chess.chessSanHistory.empty());
}

static int ChessPerft(const reverie::boardgames::State &board,int depth){
    if(depth==0)return 1;
    int count=0;
    for(const auto &move:board.ChessTurns())
        count+=ChessPerft(board.ChessAfter(move.from,move.to,move.promotion),depth-1);
    return count;
}
static void TestBoardGames() {
    using Boards = reverie::boardgames::State;
    using BoardKind = reverie::boardgames::Kind;
    Boards checkers;
    checkers.Reset(BoardKind::Checkers);
    int light=0,dark=0;
    for(int i=0;i<64;i++){
        if(checkers.cells[i]>0)++light;
        if(checkers.cells[i]<0)++dark;
    }
    assert(light==12&&dark==12);
    assert(!checkers.CheckerMove(40,24)); // men cannot move two without capture
    assert(checkers.Select(40)); // lower white man
    assert(checkers.Select(33));
    assert(checkers.side==-1&&checkers.cells[33]==1);
    for(int i=0;i<4;i++)checkers.Step(.1f);
    assert(checkers.side==1||checkers.forced>=0); // CPU plays real moves

    // Forced capture and continued capture cannot be evaded by another man.
    Boards jumps;
    jumps.Reset(BoardKind::Checkers);
    jumps.cells.fill(0);jumps.side=1;
    jumps.cells[45]=1;jumps.cells[36]=-1;jumps.cells[18]=-1;
    jumps.cells[41]=1;
    assert(jumps.HasCapture(1));
    assert(!jumps.CheckerMove(41,34));
    assert(jumps.CheckerMove(45,27));
    assert(jumps.forced==27&&jumps.side==1&&jumps.cells[36]==0);
    assert(!jumps.CheckerMove(41,34));
    assert(jumps.CheckerMove(27,9));
    assert(jumps.forced==-1&&jumps.side==-1&&jumps.cells[18]==0);
    Boards crown;
    crown.Reset(BoardKind::Checkers);
    crown.cells.fill(0);crown.cells[9]=1;crown.side=1;
    assert(crown.CheckerMove(9,0));
    assert(crown.cells[0]==2);

    // WCDF English/American checkers: draw claims require either an
    // identical third position or 40 turns per side without capture or
    // movement of an uncrowned man. Forced capture chains cannot claim.
    Boards drawCheckers;
    drawCheckers.Reset(BoardKind::Checkers);
    assert(drawCheckers.checkerHistory.size()==1);
    assert(!drawCheckers.CheckerDrawClaimable());
    auto checkersPosition=drawCheckers.CurrentCheckerPosition();
    drawCheckers.checkerHistory[checkersPosition]=3;
    assert(drawCheckers.CheckerRepetitions()==3);
    assert(drawCheckers.ClaimCheckerDraw());
    assert(drawCheckers.finished&&drawCheckers.winner==0);
    Boards progress;
    progress.Reset(BoardKind::Checkers);
    progress.checkerQuietPlies=79;
    assert(progress.CheckerMove(40,33)); // white man advances
    assert(progress.checkerQuietPlies==0);
    Boards kings;
    kings.Reset(BoardKind::Checkers);
    kings.cells.fill(0);
    kings.cells[41]=2;kings.cells[14]=-2;
    kings.side=1;kings.checkerQuietPlies=79;
    assert(kings.CheckerMove(41,32)); // quiet king move
    assert(kings.checkerQuietPlies==80);
    kings.side=1;kings.forced=-1;
    assert(kings.CheckerDrawClaimable());
    kings.forced=32;
    assert(!kings.CheckerDrawClaimable());

    // WCDF 1.32.1: a player may claim immediately if any *next legal
    // complete turn* would repeat a board/side position for the third time.
    Boards upcoming;
    upcoming.Reset(BoardKind::Checkers);
    upcoming.cells.fill(0);upcoming.cells[41]=2;upcoming.cells[14]=-2;
    upcoming.side=1;upcoming.checkerHistory.clear();
    upcoming.checkerHistory[upcoming.CurrentCheckerPosition()]=1;
    const auto possible=Boards::CheckerTurns(upcoming.cells,1);
    bool recordedFuture=false;
    for(const auto &turn:possible)if(turn.from==41&&turn.firstTo==32){
        auto next=Boards::CheckerPositionAfter(turn.after,-1);
        upcoming.checkerHistory[next]=2;
        recordedFuture=true;break;
    }
    assert(recordedFuture);
    assert(upcoming.CheckerRepetitions()==1);
    assert(upcoming.CheckerProspectiveRepetition());
    assert(upcoming.CheckerDrawClaimable());
    assert(upcoming.ClaimCheckerDraw()&&upcoming.finished);
    assert(upcoming.cells[41]==2&&upcoming.cells[32]==0);
    upcoming.Reset(BoardKind::Checkers);
    assert(!upcoming.CheckerProspectiveRepetition());
    upcoming.forced=40;
    assert(!upcoming.CheckerDrawClaimable());

    // CPU can claim a genuine 40-move quiet draw rather than losing by
    // refusing a claim its human opponent could have made.
    Boards cpuDraw;
    cpuDraw.Reset(BoardKind::Checkers);
    cpuDraw.cells.fill(0);
    cpuDraw.cells[41]=2;cpuDraw.cells[44]=1;cpuDraw.cells[14]=-2;
    cpuDraw.side=-1;cpuDraw.checkerQuietPlies=80;
    assert(Boards::CheckerPositionValue(cpuDraw.cells)<-100);
    assert(cpuDraw.CheckerDrawAvailable(-1));
    assert(cpuDraw.PlayCheckerCpuTurn());
    assert(cpuDraw.finished&&cpuDraw.winner==0);

    // Quiescence must search a mandatory capture beyond the regular
    // depth-zero horizon; static evaluation would miss the takedown.
    Boards::Board forcedBoard{};
    forcedBoard[18]=-1;forcedBoard[27]=1;
    const int staticScore=Boards::CheckerPositionValue(forcedBoard);
    const int withCapture=Boards::SearchCheckers(
        forcedBoard,-1,0,-300000,300000);
    assert(withCapture>staticScore);
    int nodes=0;
    Boards::SearchCheckersBounded(
        forcedBoard,-1,5,-300000,300000,nodes,8,2);
    assert(nodes>0&&nodes<=9);

    // AI search enumerates *complete* mandatory capture sequences, not
    // isolated first jumps. This prevents a greedy move that ignores a
    // vulnerable continuation or illegally ends a forced multi-jump.
    Boards aiJumps;
    aiJumps.Reset(BoardKind::Checkers);
    aiJumps.cells.fill(0);
    aiJumps.cells[18]=-1;aiJumps.cells[27]=1;aiJumps.cells[45]=1;
    const auto sequences=Boards::CheckerTurns(aiJumps.cells,-1);
    assert(!sequences.empty());
    bool foundDouble=false;
    for(const auto &turn:sequences){
        if(turn.from==18&&turn.firstTo==36&&
           turn.after[54]==-1&&turn.after[27]==0&&
           turn.after[45]==0)foundDouble=true;
    }
    assert(foundDouble);
    assert(aiJumps.SearchCheckers(aiJumps.cells,-1,2,
                                  -300000,300000)>-30000);
    aiJumps.side=-1;
    assert(aiJumps.PlayCheckerCpuTurn());
    assert(aiJumps.cells[36]==-1&&aiJumps.forced==36);
    assert(aiJumps.PlayCheckerCpuTurn());
    assert(aiJumps.cells[54]==-1&&aiJumps.cells[45]==0);
    assert(aiJumps.side==1&&aiJumps.forced==-1);
    Boards initialAi;
    initialAi.Reset(BoardKind::Checkers);
    initialAi.side=-1;
    assert(initialAi.PlayCheckerCpuTurn());
    assert(initialAi.side==1&&initialAi.ply==1);
    // Each level is bounded; a move is still legal on all three modes.
    // The budget is shared by all iterative depths, never per candidate.
    for(int level=0;level<3;level++){
        Boards difficulty;difficulty.Reset(BoardKind::Checkers);
        difficulty.checkerDifficulty=level;difficulty.side=-1;
        assert(difficulty.PlayCheckerCpuTurn());
        assert(difficulty.checkerSearchNodes>0);
        assert(difficulty.checkerSearchNodes<=
               (level==0?1500:level==1?6000:12000));
        assert(difficulty.side==1||difficulty.forced>=0);
        assert(difficulty.cells!=Boards::Board{});
        difficulty.Reset(BoardKind::Checkers);
        assert(difficulty.checkerDifficulty==level);
    }
    // An incomplete search must advertise exhaustion, rather than
    // quietly presenting a partial branch score as a completed result.
    Boards::Board cappedBoard{};
    cappedBoard[18]=-1;cappedBoard[27]=1;
    int checked=0;bool exhausted=false;
    Boards::SearchCheckersBounded(cappedBoard,-1,4,-300000,300000,
                                 checked,1,3,&exhausted);
    assert(exhausted&&checked==1);

    // Regulation 19x19 Go board with 361 playable intersections.
    const auto at=[](int x,int y){return y*19+x;};
    Boards go;
    go.Reset(BoardKind::Go);
    assert(go.Size()==19&&Boards::kCapacity==361);
    assert(go.Inside(18,18)&&!go.Inside(19,18));
    assert(go.GoMove(at(18,18)));
    assert(go.cells[at(18,18)]==1);
    assert(go.goHistory.size()==2);
    // Capture against the edge of the full-sized grid.
    Boards edgeGo;
    edgeGo.Reset(BoardKind::Go);
    edgeGo.cells[at(18,18)]=-1;
    edgeGo.cells[at(17,18)]=1;
    edgeGo.side=1;
    assert(edgeGo.GoMove(at(18,17)));
    assert(edgeGo.cells[at(18,18)]==0);
    assert(!edgeGo.GoMove(at(18,17)));
    Boards suicide;
    suicide.Reset(BoardKind::Go);
    suicide.cells[at(9,8)]=-1;
    suicide.cells[at(8,9)]=-1;
    suicide.cells[at(10,9)]=-1;
    suicide.cells[at(9,10)]=-1;
    assert(!suicide.GoMove(at(9,9)));
    // Ko: recapturing the prior whole-board shape is forbidden. The
    // setup board is added to history exactly as a played game would.
    Boards ko;
    ko.Reset(BoardKind::Go);
    ko.cells[at(2,2)]=1;
    ko.cells[at(2,1)]=-1;ko.cells[at(1,2)]=-1;ko.cells[at(3,2)]=-1;
    ko.cells[at(1,3)]=1;ko.cells[at(3,3)]=1;ko.cells[at(2,4)]=1;
    ko.goHistory.insert(ko.cells);
    ko.side=-1;
    assert(ko.GoMove(at(2,3)));
    assert(ko.cells[at(2,2)]==0);
    assert(!ko.GoMove(at(2,2)));
    // Positional superko rejects an earlier shape even if it was not
    // the immediately previous board, without hash-only false matches.
    Boards repeat;
    repeat.Reset(BoardKind::Go);
    Boards::Board forbidden=repeat.cells;
    forbidden[at(9,9)]=1;
    repeat.goHistory.insert(forbidden);
    assert(!repeat.GoMove(at(9,9)));
    assert(repeat.cells[at(9,9)]==0);
    // Both players may pass, and the 7.5 komi decides an empty board.
    Boards emptyGo;
    emptyGo.Reset(BoardKind::Go);
    assert(emptyGo.Pass()&&!emptyGo.finished);
    assert(emptyGo.Pass()&&emptyGo.goReview&&!emptyGo.finished);
    assert(!emptyGo.Pass()&&!emptyGo.GoMove(at(3,3)));
    assert(emptyGo.AcceptGoScore()&&emptyGo.finished&&emptyGo.winner==-1);
    assert(emptyGo.goBlackScore==0.f&&emptyGo.goWhiteScore==7.5f);

    // Two passes invite dead-group review, rather than automatically
    // assigning a result while players may dispute living/dead stones.
    Boards reviewGo;
    reviewGo.Reset(BoardKind::Go);
    reviewGo.cells[at(2,2)]=1;reviewGo.cells[at(2,3)]=1;
    reviewGo.cells[at(5,5)]=-1;
    assert(reviewGo.Pass()&&reviewGo.Pass()&&reviewGo.goReview);
    assert(reviewGo.ToggleGoDead(at(2,2)));
    assert(reviewGo.goDead[at(2,2)]&&reviewGo.goDead[at(2,3)]);
    assert(!reviewGo.goDead[at(5,5)]);
    assert(reviewGo.ToggleGoDead(at(2,3)));
    assert(!reviewGo.goDead[at(2,2)]&&!reviewGo.goDead[at(2,3)]);
    assert(!reviewGo.ToggleGoDead(at(1,1))); // cannot mark empty points
    assert(reviewGo.ToggleGoDead(at(5,5)));
    assert(reviewGo.ResumeGoPlay()&&!reviewGo.goReview);
    assert(reviewGo.cells[at(5,5)]==-1&&!reviewGo.goDead[at(5,5)]);
    assert(reviewGo.passes==0&&reviewGo.side==1);
    assert(reviewGo.GoMove(at(10,10))); // real play resumed

    // White passes first, then Black: the review temporarily uses
    // Black's controls but must not gift Black a third consecutive turn.
    Boards whiteFirstReview;
    whiteFirstReview.Reset(BoardKind::Go);
    assert(whiteFirstReview.GoMove(at(9,9))); // Black, then White to act
    assert(whiteFirstReview.side==-1);
    assert(whiteFirstReview.Pass()&&whiteFirstReview.side==1); // White passes
    assert(whiteFirstReview.Pass()&&whiteFirstReview.goReview); // Black passes
    assert(whiteFirstReview.side==1&&whiteFirstReview.goResumeSide==-1);
    assert(!whiteFirstReview.GoMove(at(8,8))); // no play during review
    assert(whiteFirstReview.ResumeGoPlay());
    assert(whiteFirstReview.side==-1&&whiteFirstReview.passes==0);
    for(int tick=0;tick<3;tick++)whiteFirstReview.Step(.1f);
    assert(whiteFirstReview.side==1&&whiteFirstReview.ply==4);
    whiteFirstReview.Reset(BoardKind::Go);
    assert(!whiteFirstReview.goReview&&whiteFirstReview.goResumeSide==1);
    assert(whiteFirstReview.side==1);

    // Removed dead white stones count as surrounded black territory;
    // preserve original board until scoring is accepted.
    Boards settled;
    settled.Reset(BoardKind::Go);
    settled.cells.fill(1);settled.cells[at(9,9)]=-1;
    assert(settled.Pass()&&settled.Pass());
    assert(settled.ToggleGoDead(at(9,9)));
    assert(settled.AcceptGoScore()&&settled.finished);
    assert(settled.goBlackScore==361.f&&settled.goWhiteScore==7.5f);
    assert(settled.winner==1&&settled.cells[at(9,9)]==-1);
    assert(!settled.ResumeGoPlay());
    // The CPU must finish an already winning game after Black passes,
    // rather than creating filler moves indefinitely on an empty board.
    Boards winningCpu;
    winningCpu.Reset(BoardKind::Go);
    assert(winningCpu.Pass());
    for(int tick=0;tick<3;tick++)winningCpu.Step(.1f);
    assert(winningCpu.goReview&&!winningCpu.finished);
    assert(winningCpu.passes==2&&winningCpu.side==1);
    assert(winningCpu.AcceptGoScore()&&winningCpu.winner==-1);

    // Eye preservation must include the edge and corner, where only
    // two or three orthogonal neighbors exist.
    Boards goEyes;
    goEyes.Reset(BoardKind::Go);
    goEyes.cells.fill(-1);
    goEyes.cells[at(0,0)]=0;goEyes.cells[at(9,9)]=0;
    assert(goEyes.IsGoEye(goEyes.cells,at(0,0),-1));
    assert(goEyes.IsGoEye(goEyes.cells,at(9,9),-1));
    goEyes.cells[at(1,1)]=1;
    assert(!goEyes.IsGoEye(goEyes.cells,at(0,0),-1));

    Boards cpuGo;
    cpuGo.Reset(BoardKind::Go);
    assert(cpuGo.GoMove(at(9,9)));
    for(int i=0;i<3;i++)cpuGo.Step(.1f);
    assert(cpuGo.ply==2&&cpuGo.side==1);
    assert(cpuGo.goHistory.size()==3);
    // Immediate tactical defense must outweigh arbitrary open-board
    // expansion: White in atari can save its threatened center stone.
    Boards saveGo;
    saveGo.Reset(BoardKind::Go);
    saveGo.cells.fill(0);
    saveGo.cells[at(9,9)]=-1;
    saveGo.cells[at(8,9)]=1;
    saveGo.cells[at(10,9)]=1;
    saveGo.cells[at(9,8)]=1;
    saveGo.side=-1;
    assert(saveGo.Flood(saveGo.cells,at(9,9)).liberties==1);
    for(int tick=0;tick<3;tick++)saveGo.Step(.1f);
    assert(saveGo.cells[at(9,10)]==-1);
    assert(saveGo.Flood(saveGo.cells,at(9,9)).liberties>=2);
    assert(saveGo.side==1&&!saveGo.finished);
    // In the mirrored position, White must complete the actual capture.
    Boards captureGo;
    captureGo.Reset(BoardKind::Go);
    captureGo.cells.fill(0);
    captureGo.cells[at(9,9)]=1;
    captureGo.cells[at(8,9)]=-1;
    captureGo.cells[at(10,9)]=-1;
    captureGo.cells[at(9,8)]=-1;
    captureGo.side=-1;
    for(int tick=0;tick<3;tick++)captureGo.Step(.1f);
    assert(captureGo.cells[at(9,9)]==0);
    assert(captureGo.cells[at(9,10)]==-1);
    assert(captureGo.side==1);
    // The CPU should still respond late on a regulation board, instead
    // of auto-passing at the obsolete 155-ply 9x9 cutoff.
    Boards laterGo;
    laterGo.Reset(BoardKind::Go);
    laterGo.ply=160;laterGo.side=-1;
    for(int i=0;i<3;i++)laterGo.Step(.1f);
    assert(laterGo.ply==161&&!laterGo.finished&&laterGo.side==1);
    // AI only passes a board with no useful legal placement.
    Boards surrounded;
    surrounded.Reset(BoardKind::Go);
    surrounded.cells.fill(-1);surrounded.side=-1;
    surrounded.cells[at(9,9)]=0;
    surrounded.goHistory.insert(surrounded.cells);
    for(int i=0;i<3;i++)surrounded.Step(.1f);
    assert(surrounded.passes==1&&surrounded.side==1);

    Boards chess;
    chess.Reset(BoardKind::Chess);
    assert(chess.Size()==8&&chess.cells[60]==6&&chess.cells[4]==-6);
    assert(!chess.ChessLegal(52,28)); // e2-e5 illegal
    assert(chess.ChessMove(52,36));  // e2-e4
    assert(chess.enPassant==44);
    assert(chess.ChessMove(8,16)); // a7-a6
    assert(chess.ChessMove(36,28)); // e4-e5
    assert(chess.ChessMove(11,27)); // d7-d5
    assert(chess.ChessLegal(28,19)); // e5xd6 en passant
    assert(chess.ChessMove(28,19));
    assert(chess.cells[27]==0&&chess.cells[19]==1);

    Boards castle;
    castle.Reset(BoardKind::Chess);
    castle.cells.fill(0);
    castle.cells[60]=6;castle.cells[63]=4;castle.cells[4]=-6;
    castle.castles=1;castle.side=1;
    assert(castle.ChessLegal(60,62));
    assert(castle.ChessMove(60,62));
    assert(castle.cells[62]==6&&castle.cells[61]==4);
    Boards illegalKing;
    illegalKing.Reset(BoardKind::Chess);
    illegalKing.cells.fill(0);
    illegalKing.cells[60]=6;illegalKing.cells[4]=-4;illegalKing.cells[0]=-6;
    assert(!illegalKing.ChessLegal(60,52)); // rook threatens e-file

    Boards mate;
    mate.Reset(BoardKind::Chess);
    assert(mate.ChessMove(53,45)); // f3
    assert(mate.ChessMove(12,28)); // ...e5
    assert(mate.ChessMove(54,38)); // g4
    assert(mate.ChessMove(3,39)); // ...Qh4 mate
    assert(mate.finished&&mate.winner==-1);
    assert(mate.Select(-1)); // restart from a completed game
    assert(!mate.finished&&mate.cells[60]==6);

    // Native chess: underpromotion, dead positions, claimable and
    // automatic FIDE draw rules. Repetition identity retains castling.
    Boards promotion;
    promotion.Reset(BoardKind::Chess);
    promotion.cells.fill(0);
    promotion.cells[60]=6;promotion.cells[4]=-6;promotion.cells[8]=1;
    promotion.castles=0;
    promotion.selected=8;
    promotion.CyclePromotion(); // rook
    promotion.CyclePromotion(); // bishop
    promotion.CyclePromotion(); // knight
    assert(promotion.promotionChoice==2);
    assert(promotion.ChessMove(8,0));
    assert(promotion.cells[0]==2 && promotion.promotionChoice==5);
    assert(promotion.finished&&promotion.winner==0); // K+N vs K

    Boards claim;
    claim.Reset(BoardKind::Chess);
    assert(!claim.ChessDrawClaimable());
    claim.halfMoves=100;
    assert(claim.ChessDrawClaimable());
    assert(claim.ClaimChessDraw()&&claim.finished&&claim.winner==0);
    claim.Reset(BoardKind::Chess);
    auto current=claim.CurrentChessPosition();
    claim.chessHistory[current]=3;
    assert(claim.ChessRepetitions()==3);
    assert(claim.ClaimChessDraw());

    Boards seventyFive;
    seventyFive.Reset(BoardKind::Chess);
    seventyFive.cells.fill(0);
    seventyFive.cells[60]=6;seventyFive.cells[4]=-6;
    seventyFive.cells[56]=4;seventyFive.cells[7]=-4;
    seventyFive.castles=0;seventyFive.halfMoves=149;
    assert(seventyFive.ChessMove(56,48));
    assert(seventyFive.halfMoves==150);
    assert(seventyFive.finished&&seventyFive.winner==0);

    Boards fivefold;
    fivefold.Reset(BoardKind::Chess);
    fivefold.cells.fill(0);
    fivefold.cells[60]=6;fivefold.cells[4]=-6;
    fivefold.cells[56]=4;fivefold.cells[7]=-4;
    fivefold.castles=0;
    auto resultPos=fivefold.CurrentChessPosition();
    resultPos.pieces[56]=0;resultPos.pieces[48]=4;
    resultPos.side=-1;resultPos.castles=0;resultPos.enPassant=-1;
    fivefold.chessHistory[resultPos]=4;
    assert(fivefold.ChessMove(56,48));
    assert(fivefold.finished&&fivefold.winner==0);
    assert(fivefold.ChessRepetitions()==5);

    Boards dead;
    dead.Reset(BoardKind::Chess);
    dead.cells.fill(0);dead.cells[60]=6;dead.cells[4]=-6;
    assert(dead.ChessDeadPosition());
    dead.cells[58]=3;
    assert(dead.ChessDeadPosition()); // lone bishop is insufficient
    dead.cells[56]=4;
    assert(!dead.ChessDeadPosition()); // a rook can still mate

    // The CPU now searches legal human replies before choosing a move.
    // A hanging rook must be captured when safe instead of following
    // the previous greedy centralization heuristic.
    // A horizon leaf must still recognize mate, not reward it
    // as a material advantage for the player who is checkmated.
    Boards matedLeaf;
    matedLeaf.Reset(BoardKind::Chess);
    matedLeaf.cells.fill(0);
    matedLeaf.cells[7]=-6;  // black king h8
    matedLeaf.cells[14]=5;  // white queen g7
    matedLeaf.cells[21]=6;  // white king f6
    matedLeaf.side=-1;
    assert(matedLeaf.KingInCheck(matedLeaf.cells,-1));
    assert(!matedLeaf.HasChessMove());
    assert(matedLeaf.SearchChess(0,-300000,300000)==-50000);

    {
    Boards stale=matedLeaf;
    stale.cells[14]=0;stale.cells[13]=5;
    assert(!stale.KingInCheck(stale.cells,-1)&&!stale.HasChessMove());
    assert(stale.SearchChess(0,-300000,300000)==0);

    // Complete legal knight cycles, without injecting repetition counts.
    Boards prospective;
    prospective.Reset(BoardKind::Chess);
    for(int round=0;round<2;round++){
        assert(prospective.ChessMove(62,45));
        assert(prospective.ChessMove(6,21));
        if(round==0){
            assert(prospective.ChessMove(45,62));
            assert(prospective.ChessMove(21,6));
        }
    }
    assert(!prospective.ChessDrawClaimable());
    assert(prospective.ChessMove(45,62));
    assert(prospective.ChessRepetitions()==2);
    int intendedFrom=-1,intendedTo=-1;
    assert(prospective.ChessDrawAvailable(-1,&intendedFrom,&intendedTo));
    assert(intendedFrom==21&&intendedTo==6);
    const auto beforeClaim=prospective.cells;
    assert(prospective.ClaimChessDraw(-1));
    assert(prospective.finished&&prospective.winner==0);
    assert(prospective.cells==beforeClaim&&prospective.ply==7);

    Boards fifty;
    fifty.Reset(BoardKind::Chess);
    fifty.halfMoves=99;
    assert(fifty.ChessDrawClaimable()); // intended quiet knight move
    assert(fifty.ClaimChessDraw());
    assert(fifty.lastFrom>=0&&fifty.lastTo>=0&&fifty.ply==0);
    fifty.Reset(BoardKind::Chess);
    fifty.halfMoves=98;
    assert(!fifty.ChessDrawClaimable());
    fifty.halfMoves=99;
    assert(fifty.ChessAfter(52,36).halfMoves==0); // pawn resets progress
    assert(fifty.ChessAfter(62,45).halfMoves==100);

    // Both colors generate all four promotions, not only a queen.
    for(int player : {1,-1}){
        Boards promote;
        promote.Reset(BoardKind::Chess);promote.cells.fill(0);
        promote.cells[60]=6;promote.cells[4]=-6;
        promote.side=player;promote.castles=0;
        const int from=player==1?8:48,to=player==1?0:56;
        promote.cells[from]=static_cast<int8_t>(player);
        int variants=0;
        for(const auto &turn:promote.ChessTurns())if(turn.from==from&&turn.to==to){
            ++variants;
            assert(promote.ChessAfter(from,to,turn.promotion).cells[to]
                   ==player*turn.promotion);
        }
        assert(variants==4);
        for(int piece=2;piece<=5;piece++){
            Boards applied=promote;applied.promotionChoice=piece;
            assert(applied.ChessMove(from,to,piece));
            assert(applied.cells[to]==player*piece);
        }
    }
    Boards cpuDraw;
    cpuDraw.Reset(BoardKind::Chess);cpuDraw.cells.fill(0);
    cpuDraw.cells[4]=-6;cpuDraw.cells[60]=6;cpuDraw.cells[56]=5;
    cpuDraw.side=-1;cpuDraw.castles=0;cpuDraw.halfMoves=100;
    assert(cpuDraw.PlayChessCpuTurn());
    assert(cpuDraw.finished&&cpuDraw.winner==0&&cpuDraw.ply==0);
    int nodes=0;bool exhausted=false;
    Boards budgeted;budgeted.Reset(BoardKind::Chess);
    budgeted.SearchChessBounded(4,-300000,300000,nodes,7,exhausted);
    assert(exhausted&&nodes==7);
    for(int level=0;level<3;level++){
        Boards limited;limited.Reset(BoardKind::Chess);
        limited.chessDifficulty=level;limited.side=-1;
        assert(limited.PlayChessCpuTurn());
        assert(limited.chessSearchNodes>0);
        assert(limited.chessSearchNodes<=(level==0?1500:level==1?6000:12000));
        assert(limited.side==1&&!limited.KingInCheck(limited.cells,-1));
    }

    }
    Boards perft;perft.Reset(BoardKind::Chess);
    assert(ChessPerft(perft,1)==20);
    assert(ChessPerft(perft,2)==400);
    assert(ChessPerft(perft,3)==8902);
    Boards pinnedEp; pinnedEp.Reset(BoardKind::Chess);
    pinnedEp.cells.fill(0);pinnedEp.castles=0;
    pinnedEp.cells[28]=1;pinnedEp.cells[27]=-1;
    pinnedEp.cells[60]=6;pinnedEp.cells[4]=-4;pinnedEp.cells[0]=-6;
    pinnedEp.enPassant=19;
    assert(!pinnedEp.ChessLegal(28,19)); // exposes own king to rook
    assert(pinnedEp.CurrentChessPosition().enPassant==-1);

    // Worker results cannot mutate the live board until Update polls them.
    for(auto kind:{BoardKind::Chess,BoardKind::Checkers,BoardKind::Go}){
        Boards live;live.Reset(kind);live.side=-1;
        const auto initial=live.cells;
        reverie::boardgames::CpuWorker worker;
        assert(worker.Start(live));
        assert(!worker.Start(live)); // exactly one in-flight response
        assert(live.cells==initial&&live.ply==0);
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
        while(!worker.Poll(live)){
            assert(std::chrono::steady_clock::now()<deadline);
            std::this_thread::yield();
        }
        assert(!worker.Busy()&&live.ply>0);
        assert(live.side==1||live.finished||live.forced>=0);
        assert(!worker.Poll(live)); // no duplicate turn commit
    }

    Boards tacticalChess;
    tacticalChess.Reset(BoardKind::Chess);
    tacticalChess.cells.fill(0);
    tacticalChess.cells[4]=-6;tacticalChess.cells[60]=6;
    tacticalChess.cells[0]=-4;tacticalChess.cells[8]=4;
    tacticalChess.side=-1;tacticalChess.castles=0;
    auto nextChess=tacticalChess.ChessAfter(0,8);
    assert(nextChess.cells[8]==-4&&nextChess.cells[0]==0);
    assert(nextChess.side==1);
    assert(nextChess.SearchChess(1,-300000,300000)>-10000);
    assert(tacticalChess.PlayChessCpuTurn());
    assert(tacticalChess.cells[8]==-4);
    assert(tacticalChess.side==1);
    Boards openingChess;
    openingChess.Reset(BoardKind::Chess);
    openingChess.side=-1;
    assert(openingChess.PlayChessCpuTurn());
    assert(openingChess.ply==1&&openingChess.side==1);

    Boards cpuChess;
    cpuChess.Reset(BoardKind::Chess);
    assert(cpuChess.ChessMove(52,36));
    for(int i=0;i<3;i++)cpuChess.Step(.1f);
    assert(cpuChess.ply==2&&cpuChess.side==1);
    assert(!cpuChess.KingInCheck(cpuChess.cells,-1));

    std::cout<<"Checkers, Go 19x19 and Chess deterministic rules/CPU passed\\n";
}

static void TestReversi() {
    using Boards=reverie::boardgames::State;
    using Kind=reverie::boardgames::Kind;
    Boards game;game.Reset(Kind::Reversi);
    assert(game.Size()==8&&game.side==1&&!game.finished);
    assert(game.ReversiCount(1)==2&&game.ReversiCount(-1)==2);
    assert(game.cells[3*8+3]==-1&&game.cells[4*8+4]==-1);
    assert(game.cells[3*8+4]==1&&game.cells[4*8+3]==1);
    int moves=0;
    for(int at=0;at<64;at++)
        if(Boards::ReversiFlips(game.cells,at,1)>0)++moves;
    assert(moves==4);
    assert(Boards::ReversiFlips(game.cells,3*8+3,1)==0);
    assert(!game.Select(0)&&game.ply==0);
    assert(game.Select(3*8+2));
    assert(game.cells[3*8+2]==1&&game.cells[3*8+3]==1);
    assert(game.ReversiCount(1)==4&&game.ReversiCount(-1)==1);
    assert(game.side==-1&&game.ply==1);
    assert(!game.Select(2*8+3)); // not the human turn
    // Genuine CPU response, not a placeholder or random illegal tile.
    for(int tick=0;tick<3;tick++)game.Step(.1f);
    assert(game.ply==2&&game.side==1);
    assert(game.ReversiCount(1)+game.ReversiCount(-1)==6);
    assert(game.reversiSearchNodes>0&&game.reversiSearchNodes<=4000);
    Boards workerBoard;workerBoard.Reset(Kind::Reversi);
    assert(workerBoard.Select(3*8+2));
    reverie::boardgames::CpuWorker worker;
    const int before=workerBoard.ply;
    assert(worker.Start(workerBoard));
    assert(workerBoard.ply==before&&!worker.Start(workerBoard));
    const auto deadline=std::chrono::steady_clock::now()+
                        std::chrono::seconds(10);
    while(!worker.Poll(workerBoard)){
        assert(std::chrono::steady_clock::now()<deadline);
        std::this_thread::yield();
    }
    assert(workerBoard.ply==2&&!worker.Busy());
    assert(!worker.Poll(workerBoard));
    // If White has no move, its turn must pass automatically to Black.
    Boards pass;pass.Reset(Kind::Reversi);
    pass.cells.fill(1);pass.cells[3*8+3]=-1;
    pass.cells[3*8+2]=0;pass.side=-1;
    assert(!Boards::HasReversiMove(pass.cells,-1));
    assert(Boards::HasReversiMove(pass.cells,1));
    assert(pass.PlayReversiCpuTurn()&&pass.side==1&&pass.passes==1);
    assert(!pass.finished&&pass.Select(3*8+2));
    assert(pass.finished&&pass.winner==1);
    assert(pass.ReversiCount(1)==64&&pass.ReversiCount(-1)==0);
    assert(pass.Select(0)); // a completed game restarts on Select
    assert(!pass.finished&&pass.ReversiCount(1)==2);
    assert(pass.ReversiCount(-1)==2);
    // Terminal scoring must recognize White victories and ties too.
    Boards white;white.Reset(Kind::Reversi);
    white.cells.fill(-1);white.FinishReversi();
    assert(white.finished&&white.winner==-1);
    Boards tie;tie.Reset(Kind::Reversi);
    tie.cells.fill(0);
    for(int i=0;i<64;i++)tie.cells[i]=i<32?1:-1;
    tie.FinishReversi();
    assert(tie.finished&&tie.winner==0);
    // A corner is more valuable than scattered early disc gains.
    Boards corner;corner.Reset(Kind::Reversi);
    corner.cells.fill(0);corner.side=-1;
    corner.cells[2*8+0]=-1;corner.cells[1*8+0]=1;
    assert(Boards::ReversiFlips(corner.cells,0,-1)==1);
    assert(corner.PlayReversiCpuTurn());
    assert(corner.cells[0]==-1&&corner.cells[8]==-1);
    std::cout<<"Reversi 8x8 flip, pass, scores and CPU passed\\n";
}

static void TestBreakout() {
    using reverie::classics::Breakout;
    Breakout game;
    game.Reset();
    assert(game.lives==3&&game.score==0&&game.remaining==112);
    assert(game.wall==1&&Breakout::kColumns==14&&Breakout::kRows==8);
    assert(Breakout::BrickValue(0)==7&&Breakout::BrickValue(2)==5);
    assert(Breakout::BrickValue(4)==3&&Breakout::BrickValue(7)==1);
    assert(!game.moving&&!game.finished);
    game.SetPaddle(.5f);
    game.Step(.05f,0.f);
    assert(game.ballX==game.paddle&&!game.moving);
    game.Press();
    assert(game.moving);
    Breakout split=game;
    game.Step(.05f,.45f);
    split.Step(.025f,.45f);
    split.Step(.025f,.45f);
    assert(std::fabs(game.ballX-split.ballX)<.0001f);
    assert(std::fabs(game.ballY-split.ballY)<.0001f);
    assert(game.score==split.score&&game.remaining==split.remaining);
    assert(game.paddle>0.f);
    const float oldPaddle=game.paddle;
    game.SetPaddle(INFINITY);
    assert(game.paddle==oldPaddle);
    game.SetPaddle(-999.f);
    assert(game.paddle>=-.77f&&game.paddle<=.77f);

    // One wall bounce must keep the ball inside its court.
    game.ballX=.94f;game.ballY=-.2f;game.velocityX=.5f;
    game.velocityY=.7f;
    game.Tick();
    assert(game.ballX<=.94f&&game.velocityX<0.f);
    // A real brick collision changes exactly one tile and awards points.
    game.Reset();game.Press();
    game.ballX=Breakout::BrickX(0);
    game.ballY=Breakout::BrickY(0)-.080f;
    game.velocityY=.7f;
    game.Tick();
    assert(game.bricks[0]==0&&game.remaining==111&&game.score==7);
    assert(game.velocityY<0.f);
    // Paddle contact reverses descent and permits directional deflection.
    game.ballX=game.paddle+.1f;
    game.ballY=-.75f;game.velocityY=-.7f;
    game.Tick();
    assert(game.velocityY>0.f&&game.lives==3);

    // Falling below the bottom consumes exactly one life, then requires
    // an explicit new serve; a finished match restarts with a press.
    game.ballY=-.986f;game.ballX=.5f;
    game.velocityY=-.7f;
    game.Tick();
    assert(game.lives==2&&!game.moving&&!game.finished);
    game.Press();assert(game.moving);
    game.lives=1;game.ballY=-.986f;game.velocityY=-.7f;
    game.Tick();
    assert(game.lives==0&&game.finished&&!game.won);
    game.Press();assert(game.lives==3&&!game.finished&&!game.moving);

    // Reaching the top wall after a red-row hit halves the paddle.
    game.Press();game.topRowBroken=true;game.ballY=.915f;
    game.velocityY=.7f;game.Tick();
    assert(game.narrowPaddle&&game.PaddleWidth()<.25f);
    // Original two-wall campaign: first clearance installs the second
    // full 112-brick wall; only the second clearance wins.
    game.bricks.fill(0);game.bricks[9]=1;game.remaining=1;
    game.ballX=Breakout::BrickX(9);
    game.ballY=Breakout::BrickY(0)-.080f;
    game.velocityY=.7f;
    game.Tick();
    assert(game.wall==2&&game.remaining==112&&game.score==7);
    assert(!game.finished&&game.moving);
    game.bricks.fill(0);game.bricks[9]=1;game.remaining=1;
    game.ballX=Breakout::BrickX(9);
    game.ballY=Breakout::BrickY(0)-.080f;
    game.velocityY=.7f;game.Tick();
    assert(game.remaining==0&&game.score==14);
    assert(game.finished&&game.won&&!game.moving);
    game.Press();
    assert(game.remaining==112&&game.wall==1&&!game.finished);
    std::cout<<"Classic Breakout physics, scoring and lives passed\\n";
}

int main() {
    TestChessWoodAtlas();
    TestChessSpriteAtlas();
    TestChessIdentityHoverAndReference();
    TestClassicArtworkAndOpeningIndex();
    TestFlatGeometry();
    TestBreakout();
    State a,b;
    a.Reset(Game::Pong);b.Reset(Game::Pong);
    assert(!a.moving && !a.finished);
    a.Press();b.Press();
    for(int i=0;i<120;i++) {
        a.Step(.05f,.4f);
        b.Step(.025f,.4f);
        b.Step(.025f,.4f);
    }
    assert(std::isfinite(a.ball_x)&&a.score+a.opponent>=0);
    assert(a.score==b.score&&a.opponent==b.opponent);
    // Full arcade court: opposing side paddles, ricochet off ceiling,
    // first to eleven instead of the seven-point prototype.
    State pong;
    pong.Reset(Game::Pong);
    pong.Press();pong.paddle=.2f;pong.ball_x=-.87f;
    pong.ball_y=.20f;pong.velocity_x=-.55f;
    pong.velocity_y=0.f;
    pong.Step(.02f,0.f);
    assert(pong.velocity_x>0.f); // human paddle returned the ball
    // Paddle motion inherited from trackpad or head-tracked pointer changes
    // rebound energy and angle; anti-direction strokes slow the ball.
    auto moving_with=pong;
    auto still=pong;
    auto moving_against=pong;
    auto setup_contact=[](State &state,float start) {
        state.Reset(Game::Pong);state.Press();
        state.ball_x=-.87f;state.ball_y=.17f;
        state.velocity_x=-.92f;state.velocity_y=.10f;
        state.previous_paddle=start;
        state.paddle=.0f;
    };
    setup_contact(moving_with,-.05f);
    setup_contact(still,0.f);
    setup_contact(moving_against,.05f);
    moving_with.Step(.025f,0.f);
    still.Step(.025f,0.f);
    moving_against.Step(.025f,0.f);
    const auto speed=[](const State &v) {
        return std::hypot(v.velocity_x,v.velocity_y);
    };
    assert(moving_with.velocity_y>still.velocity_y);
    assert(still.velocity_y>moving_against.velocity_y);
    assert(speed(moving_with)>speed(still));
    assert(speed(still)>speed(moving_against));
    // Head-tracked position changes count as paddle movement as well.
    State pointer_contact;
    setup_contact(pointer_contact,-.05f);
    pointer_contact.Step(.025f,0.f);
    assert(pointer_contact.velocity_x>0.f);
    // Even a still centered paddle creates a non-flat rebound.
    State center_hit;
    center_hit.Reset(Game::Pong);center_hit.Press();
    center_hit.ball_x=-.87f;center_hit.ball_y=0.f;
    center_hit.velocity_x=-.9f;center_hit.velocity_y=0.f;
    center_hit.Step(.025f,0.f);
    assert(center_hit.velocity_x>0.f&&std::abs(center_hit.velocity_y)>.05f);
    // Serve begins substantially faster, and resetting preserves dynamics.
    State fresh;
    fresh.Reset(Game::Pong);
    assert(std::hypot(fresh.velocity_x,fresh.velocity_y)>.9f);
    fresh.Press();fresh.ball_x=1.045f;fresh.ball_y=.60f;fresh.velocity_x=.9f;
    fresh.Step(.02f,0.f);
    assert(fresh.score==1&&!fresh.moving);
    assert(std::hypot(fresh.velocity_x,fresh.velocity_y)>.9f);
    // The CPU's movement is strictly limited even on a large miss.
    State cpu_step;
    cpu_step.Reset(Game::Pong);cpu_step.Press();
    cpu_step.ai=-.70f;cpu_step.ball_y=.70f;cpu_step.velocity_x=.9f;
    cpu_step.Step(.02f,0.f);
    assert(cpu_step.ai > -.70f && cpu_step.ai < -.68f);

    // Once the ball has crossed the paddle plane, a late CPU/player
    // position update cannot retroactively rescue the missed volley.
    State too_late;
    too_late.Reset(Game::Pong);too_late.Press();
    too_late.ball_x=1.045f;too_late.ball_y=0.f;
    too_late.velocity_x=.9f;too_late.velocity_y=0.f;
    too_late.Step(.02f,0.f);
    assert(too_late.score==1&&!too_late.moving);

    pong.Reset(Game::Pong);pong.Press();
    pong.score=10;pong.ball_x=1.045f;pong.ball_y=.60f;
    pong.velocity_x=.55f;pong.velocity_y=0.f;
    pong.Step(.02f,0.f);
    assert(pong.score==11&&pong.finished);
    pong.Press();
    assert(pong.score==0&&!pong.finished&&!pong.moving);
    pong.Press();pong.opponent=10;pong.ball_x=-1.045f;
    pong.ball_y=.60f;pong.velocity_x=-.55f;pong.velocity_y=0.f;
    pong.Step(.02f,0.f);
    assert(pong.opponent==11&&pong.finished);
    State snake;
    snake.Reset(Game::Snake);
    assert(snake.length==3&&snake.food>=0&&snake.food<State::kCells);
    snake.Press();
    snake.Direction(3);
    assert(snake.desired==1); // cannot reverse into the body
    for(int tick=0;tick<3;tick++)snake.Step(.1f,0);
    assert(snake.snake[0]==6*12+6);
    snake.Direction(0);
    for(int tick=0;tick<3;tick++)snake.Step(.1f,0);
    assert(snake.snake[0]==5*12+6);
    snake.Direction(3);
    for(int i=0;i<20&&!snake.finished;i++)snake.Step(.25f,0);
    assert(snake.finished);
    snake.Press();
    assert(!snake.finished&&!snake.moving&&snake.length==3);
    // Near-full boards cannot falsely win due to occupied-cell RNG retries.
    State crowded;
    crowded.Reset(Game::Snake);
    crowded.length=State::kCells-1;
    crowded.snake.fill(-1);
    for(int i=0;i<State::kCells-1;i++)crowded.snake[i]=i;
    for(uint32_t seed=1;seed<=128;seed++) {
        crowded.seed=seed;
        crowded.finished=false;
        crowded.NewFood();
        assert(!crowded.finished&&crowded.food==State::kCells-1);
    }
    crowded.length=State::kCells-2;
    crowded.snake[State::kCells-2]=-1;
    bool found_penultimate=false,found_last=false;
    for(uint32_t seed=1;seed<=128;seed++) {
        crowded.seed=seed;
        crowded.finished=false;
        crowded.NewFood();
        assert(!crowded.finished);
        assert(crowded.food==State::kCells-2 ||
               crowded.food==State::kCells-1);
        found_penultimate|=crowded.food==State::kCells-2;
        found_last|=crowded.food==State::kCells-1;
    }
    assert(found_penultimate&&found_last);
    crowded.length=State::kCells;
    crowded.snake[State::kCells-2]=State::kCells-2;
    crowded.snake[State::kCells-1]=State::kCells-1;
    crowded.finished=false;
    crowded.NewFood();
    assert(crowded.finished&&crowded.food==-1);

    // Complete classic board: nine ladders and ten chutes. Every
    // displayed connection comes from the same table as gameplay.
    assert(State::kBoardLinks.size()==19);
    std::array<bool,101> startUsed{};
    int up=0,down=0;
    for(const auto &link:State::kBoardLinks){
        assert(link.from>=1&&link.from<100);
        assert(link.to>=1&&link.to<=100);
        assert(!startUsed[link.from]);
        startUsed[link.from]=true;
        assert(State::Jump(link.from)==link.to);
        if(link.to>link.from)++up;
        else ++down;
    }
    assert(up==9&&down==10);
    assert(State::Jump(1)==38&&State::Jump(9)==31);
    assert(State::Jump(47)==26&&State::Jump(49)==11);
    assert(State::Jump(80)==100&&State::Jump(98)==78);
    assert(State::Jump(45)==45&&State::Jump(99)==99);
    // No bounce, clamp-to-win or phantom ladder when the spinner passes 100.
    assert(State::AdvanceChutes(0,1)==38);
    assert(State::AdvanceChutes(46,1)==26);
    assert(State::AdvanceChutes(79,1)==100);
    assert(State::AdvanceChutes(97,3)==100);
    assert(State::AdvanceChutes(97,4)==97);
    assert(State::AdvanceChutes(99,2)==99);
    assert(State::AdvanceChutes(99,1)==100);
    // The real human and CPU turn paths must obey the same exact-spin rule.
    uint32_t overshootSeed=0,oneSeed=0;
    for(uint32_t candidate=1;candidate<=64;candidate++){
        State probe;probe.Reset(Game::ChutesLadders);probe.seed=candidate;
        const int roll=static_cast<int>(probe.Random()%6u)+1;
        if(roll>=3 && overshootSeed==0)overshootSeed=candidate;
        if(roll==1 && oneSeed==0)oneSeed=candidate;
    }
    assert(overshootSeed!=0&&oneSeed!=0);
    State overshoot;overshoot.Reset(Game::ChutesLadders);
    overshoot.position=98;overshoot.seed=overshootSeed;
    overshoot.Press();
    assert(overshoot.die>=3&&overshoot.position==98);
    assert(!overshoot.finished&&!overshoot.player_turn);
    State cpuOvershoot;cpuOvershoot.Reset(Game::ChutesLadders);
    cpuOvershoot.rival=98;cpuOvershoot.player_turn=false;
    cpuOvershoot.seed=overshootSeed;cpuOvershoot.clock=.71f;
    cpuOvershoot.Step(.02f,0.f);
    assert(cpuOvershoot.die>=3&&cpuOvershoot.rival==98);
    assert(!cpuOvershoot.finished&&cpuOvershoot.player_turn);
    State winFrom80;winFrom80.Reset(Game::ChutesLadders);
    winFrom80.position=79;winFrom80.seed=oneSeed;winFrom80.Press();
    assert(winFrom80.die==1&&winFrom80.position==100&&winFrom80.finished);
    State exactWin;exactWin.Reset(Game::ChutesLadders);
    exactWin.position=99;exactWin.seed=oneSeed;exactWin.Press();
    assert(exactWin.position==100&&exactWin.finished);
    State ladder;
    ladder.Reset(Game::ChutesLadders);
    assert(ladder.player_turn&&ladder.position==0);
    ladder.Press();
    assert(!ladder.player_turn&&ladder.die>=1&&ladder.die<=6);
    int pos=ladder.position;
    ladder.Press();
    assert(ladder.position==pos); // cannot take the opponent's turn
    for(int tick=0;tick<8;tick++)ladder.Step(.1f,0);
    assert(ladder.player_turn&&ladder.rival>=1);
    for(int i=0;i<300&&!ladder.finished;i++) {
        if(ladder.player_turn)ladder.Press();
        else for(int tick=0;tick<8;tick++)ladder.Step(.1f,0);
    }
    assert(ladder.finished&&((ladder.position==100)||(ladder.rival==100)));
    ladder.Press();
    assert(!ladder.finished&&ladder.position==0&&ladder.rival==0);
    TestBoardGames();
    TestReversi();
    std::cout<<"Classic games deterministic simulation passed\n";
}

