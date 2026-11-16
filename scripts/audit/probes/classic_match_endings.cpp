// Play the registered module through its ordinary controls and save callbacks.
// Each build selects the same CLASSIC_GAME_KIND used in Android.mk.
#include "audit_host.h"
#include "classic_games_module.cpp"
#include <chrono>
#include <thread>

namespace {
using audit::Check;
void Tap(Module *m,ReverieNativeInputV1 in){
    in.primary_down=0;Update(m,&in);
    in.primary_down=1;Update(m,&in);
    in.primary_down=0;Update(m,&in);
}
void Capture(Module *m,const char *phase){
    const auto eyes=audit::Capture(Draw,m,std::string(kId)+"-"+phase);
    Check(eyes[0]!=eyes[1],"stereo image must differ");
}
Module *Open(const ReverieNativeHostV1 &host){
    auto *m=static_cast<Module*>(Create(&host));
    Check(m&&GlCreate(m),"open real game and create its GL context");
    Resume(m);
    auto neutral=audit::Input();Update(m,&neutral);
    return m;
}
void Close(Module *m){
    Pause(m);Check(PrepareExit(m)==1,"commit game on exit");GlRelease(m);Destroy(m);
}
void Reopen(Module *&m,const ReverieNativeHostV1 &host){
    const auto prior=reverie::classics::save::Fingerprint(CLASSIC_GAME_KIND,m->game,m->breakout,m->board);
    Close(m);m=Open(host);
    Check(prior==reverie::classics::save::Fingerprint(CLASSIC_GAME_KIND,m->game,m->breakout,m->board),
          "restore exact match before resuming gameplay");
    Check(m->restorePrompt,"restored match has a Continue button");
    Tap(m,audit::Pointer(0.f,-.43f,-2.72f));
    Check(!m->restorePrompt,"Continue accepts the restored match");
}
#if CLASSIC_GAME_KIND == 2
void Play(Module *&m,const ReverieNativeHostV1 &host){
    bool chute=false,ladder=false,overshoot=false,playerWin=false,cpuWin=false,shared=false;
    auto neutral=audit::Input();
    int games=0,turns=0;
    auto record=[&](const State::Roll &roll,int tile){
        if(roll.Overshot()&&!overshoot){overshoot=true;Capture(m,"overshoot");}
        else if(tile<roll.Landing()&&!chute){chute=true;Capture(m,"chute");}
        else if(tile>roll.Landing()&&!ladder){ladder=true;Capture(m,"ladder");}
        Check(roll.die>=1&&roll.die<=6&&State::AdvanceChutes(roll.from,roll.die)==tile,"roll explains result");
        shared|=m->game.position>0&&m->game.rival==m->game.position;
    };
    while(games<20&&!(chute&&ladder&&overshoot&&playerWin&&cpuWin&&shared)){
        while(!m->game.finished&&turns<10000){
            ++turns;Tap(m,neutral);
            record(m->game.playerRoll,m->game.position);
            if(turns==1)Reopen(m,host);
            for(int frame=0;frame<30&&!m->game.player_turn&&!m->game.finished;++frame)
                Update(m,&neutral);
            Check(m->game.player_turn||m->game.finished,"CPU resolves pending roll");
            if(m->game.player_turn)record(m->game.cpuRoll,m->game.rival);
        }
        Check(m->game.finished,"reach square 100");
        playerWin|=m->game.position==100;cpuWin|=m->game.rival==100;
        if(!games){Reopen(m,host);Capture(m,"finish");}
        Tap(m,neutral);Check(!m->game.finished&&m->game.die==0,"replay clears both rolls");
        ++games;
    }
    Check(chute&&ladder&&overshoot&&playerWin&&cpuWin&&shared,"cover both winners and all path outcomes");
    std::cout<<"Chutes: "<<games<<" full matches, "<<turns<<" player rolls, both winners, shared tile, overshoot, chute and ladder.\n";
}
#elif CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 4 || CLASSIC_GAME_KIND == 5 || CLASSIC_GAME_KIND == 7
auto Square(Module *m,int x,int y){
    return audit::Pointer(BoardX(m->board,x),BoardY(m->board,y),-2.91f);
}
[[maybe_unused]] void WaitForPlayer(Module *m){
    auto neutral=audit::Input();
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);
    while(m->board.side==-1&&!m->board.finished){
        Check(std::chrono::steady_clock::now()<deadline&&!m->cpuFailed,"CPU finishes a legal reply");
        Update(m,&neutral);std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
#if CLASSIC_GAME_KIND == 3
void Play(Module *&m,const ReverieNativeHostV1 &host){
    int jumps=0;
    for(int turn=0;turn<400&&!m->board.finished;++turn){
        WaitForPlayer(m);
        const auto &b=m->board;
        if(b.finished)break;
        if(b.CheckerDrawClaimable()){
            Tap(m,audit::Pointer(1.94f,-1.25f,-2.91f));continue;
        }
        int from=-1,to=-1;
        const bool capture=b.HasCapture(1,b.forced);
        for(int i=0;i<64&&to<0;++i)for(int j=0;j<64&&to<0;++j){
            if(b.forced>=0&&i!=b.forced)continue;
            if(capture?b.CheckersCapture(i,j):b.CheckersStep(i,j)){from=i;to=j;}
        }
        Check(from>=0,"a selectable checkers move exists");
        const int ply=b.ply;
        if(b.forced<0)Tap(m,Square(m,from%8,from/8));
        Tap(m,Square(m,to%8,to/8));Check(b.ply>ply,"selected checker moves");
        if(capture&&++jumps==1)Reopen(m,host);
    }
    Check(m->board.finished&&jumps>0,"complete checkers with forced captures");
    Reopen(m,host);Capture(m,"finish");
    std::cout<<"Checkers: "<<m->board.ply<<" plies, "<<jumps<<" human captures, winner "<<m->board.winner<<".\n";
    Tap(m,audit::Input());Check(m->board.ply==0&&!m->board.finished,"restart checkers");
}
#elif CLASSIC_GAME_KIND == 5
void Play(Module *&m,const ReverieNativeHostV1 &host){
    const auto panel=[](float y){return audit::Pointer(2.22f,y,reverie::chesspanel::kPanelZ);};
    auto neutral=audit::Input();
    for(int color:{1,-1}){
        Tap(m,panel(1.423f));Tap(m,audit::Pointer(color==1?-.66f:.66f,.08f,-2.75f));
        Check(m->board.chessHumanColor==color&&!m->chessNewGameOpen,"choose human color");
        for(int turn=0;turn<400;++turn){
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
            while(m->chessTween.Active()||(!m->board.ChessHumanTurn()&&!m->board.finished)){
                Check(std::chrono::steady_clock::now()<deadline&&!m->cpuFailed,"finish CPU move and animation");
                Update(m,&neutral);std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            const auto &b=m->board;
            if(b.finished)break;
            if(b.ChessDrawClaimable()){Tap(m,panel(-1.36f));continue;}
            // Deterministic human player selects legal captures, then legal quiet moves.
            int from=-1,to=-1,best=-1;
            for(int i=0;i<64;++i)for(int j=0;j<64;++j){
                if(!b.ChessLegal(i,j))continue;
                const int value=256*std::abs(b.cells[j])+(i*37+j*73+b.ply*13)%251;
                if(value>best){from=i;to=j;best=value;}
            }
            Check(from>=0,"human has a legal chess move");
            const int ply=b.ply;
            Tap(m,Square(m,from%8,from/8));Tap(m,Square(m,to%8,to/8));
            Check(b.ply>ply,"pointer move committed on chosen orientation");
        }
        Check(m->board.finished,"played a complete chess match");
        Reopen(m,host);Capture(m,color==1?"finish-white":"finish-black");
        std::cout<<"Chess: human "<<(color==1?"White":"Black")<<", "<<m->board.ply
                 <<" plies, winner "<<m->board.winner<<".\n";
        Tap(m,panel(-1.35f));Check(!m->chessWikiOpen&&!m->board.finished&&m->board.ply<2,
                                  "visible Restart button starts a new match");
    }
}
#elif CLASSIC_GAME_KIND == 4
void Play(Module *&m,const ReverieNativeHostV1 &host){
    Tap(m,Square(m,9,9));WaitForPlayer(m);
    const int x=m->board.cells[181]==0?10:8;
    Tap(m,Square(m,x,9));WaitForPlayer(m);
    Check(m->board.ply==4,"two black stones and two CPU replies");
    auto pass=audit::Pointer(1.94f,-1.25f,-2.91f);
    Tap(m,pass);WaitForPlayer(m);Check(m->board.goReview,"both players passed");
    const auto initial=m->board.GoArea();
    Tap(m,Square(m,9,9));Check(m->board.goDead[180]&&m->board.goDead[9*19+x],"mark connected group");
    const auto marked=m->board.GoArea();Check(marked!=initial,"area changes with marked group");
    Reopen(m,host);Capture(m,"review");
    Tap(m,Square(m,x,9));Check(m->board.GoArea()==initial,"unmark entire group");
    Tap(m,audit::Pointer(-1.94f,-1.25f,-2.91f));
    Check(!m->board.goReview&&m->board.side==1,"resume play as Black");
    Tap(m,pass);WaitForPlayer(m);Check(m->board.goReview,"return to review");
    Tap(m,pass);Check(m->board.finished&&m->board.goBlackScore==initial.first&&
        m->board.goWhiteScore==initial.second,"accept exactly the reviewed score");
    Reopen(m,host);Capture(m,"finish");Tap(m,audit::Input());
    Check(!m->board.finished&&m->board.ply==0,"replay starts fresh");
    std::cout<<"Go: connected pair marked, restored, unmarked, resumed, scored "<<initial.first<<" to "<<initial.second<<" and replayed.\n";
}
#else
void Play(Module *&m,const ReverieNativeHostV1 &host){
    bool pass=false;
    auto neutral=audit::Input();
    int games=0;
    do{
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(60);
        while(!m->board.finished){
            Check(std::chrono::steady_clock::now()<deadline,"match reaches a result");
            if(m->board.side==1){
                int at=(games*13)%64;
                for(int i=0;i<64&&!m->board.ReversiFlips(m->board.cells,at,1);++i)at=(at+1)%64;
                Check(m->board.ReversiFlips(m->board.cells,at,1)>0,"human has a marked square");
                const int ply=m->board.ply;
                Tap(m,Square(m,at%8,at/8));Check(m->board.ply>ply,"legal marker activated");
            }else{
                Update(m,&neutral);std::this_thread::sleep_for(std::chrono::milliseconds(1));
                Check(!m->cpuFailed,"CPU replies");
            }
            if(m->board.passes==1&&!pass){pass=true;Capture(m,"pass");}
        }
        Check(m->board.passes==2,"neither side can move");
        const int black=m->board.ReversiCount(1),white=m->board.ReversiCount(-1);
        Check(m->board.winner==((black>white)-(black<white)),"winner matches totals");
        std::cout<<"Reversi: "<<m->board.ply<<" moves, Black "<<black<<", White "<<white<<".\n";
        Reopen(m,host);Capture(m,"finish");Tap(m,neutral);
        Check(!m->board.finished&&m->board.ply==0,"replay clears final score");
        ++games;
    }while(!pass&&games<6);
    Check(pass,"full match includes a forced pass");
}
#endif
#elif CLASSIC_GAME_KIND == 6
void Play(Module *&m,const ReverieNativeHostV1 &host){
    auto neutral=audit::Input();
    // Miss a serve deliberately, then restore the inter-serve state.
    Tap(m,neutral);
    for(int i=0;i<2000&&m->breakout.moving;++i){
        auto miss=audit::Pointer(m->breakout.ballX>0.f?-.95f:.95f,0.f,-2.91f);
        Update(m,&miss);
    }
    Check(m->breakout.lives==2&&!m->breakout.moving,"miss costs exactly one life");
    Reopen(m,host);
    Tap(m,neutral);
    bool sawSecond=false;
    for(int frame=0;frame<300000&&!m->breakout.finished;++frame){
        const auto &b=m->breakout;
        const float offset=.064f*std::sin(1.71f*b.paddleHits);
        auto aim=audit::Pointer((b.ballX+b.velocityX*.025f+offset)*1.08f,0.f,-2.91f);
        if(!b.moving)Tap(m,aim);else Update(m,&aim);
        if(b.wall==2&&!sawSecond){sawSecond=true;Capture(m,"second-wall");}
    }
    Check(m->breakout.finished&&m->breakout.won&&m->breakout.score==896,"all 224 bricks cleared");
    Reopen(m,host);Capture(m,"finish");
    std::cout<<"Breakout: 224 bricks, "<<m->breakout.score<<" points, "<<m->breakout.lives<<" lives.\n";
    Tap(m,neutral);Check(m->breakout.wall==1&&m->breakout.lives==3&&!m->breakout.moving,"fresh replay");
}
#elif CLASSIC_GAME_KIND == 0 || CLASSIC_GAME_KIND == 1
void Play(Module *&m,const ReverieNativeHostV1 &host){
    auto neutral=audit::Input();
#if CLASSIC_GAME_KIND == 0
    for(int frame=0;frame<20000&&!m->game.finished;++frame){
        if(!m->game.moving)Tap(m,neutral);
        // Move away from incoming shots so the CPU can close a match.
        auto in=audit::Pointer(0,m->game.ball_y>0.f?-.9f:.9f,-2.91f);Update(m,&in);
    }
    Check(m->game.finished&&(m->game.opponent==11||m->game.score==11),"Pong reaches 11");
    std::cout<<"Pong: "<<m->game.score<<" to "<<m->game.opponent<<".\n";
#else
    const auto &button=kSnakeButtons[0];
    Tap(m,audit::Pointer(button.x,button.y,kSnakeButtonZ));
    for(int i=0;i<100&&!m->game.finished;++i)Update(m,&neutral);
    Check(m->game.finished&&m->game.snake[0]/12==0,"Snake reaches top wall");
#endif
    Reopen(m,host);Capture(m,"finish");Tap(m,neutral);
    Check(!m->game.finished&&!m->game.moving,"restart waits for a fresh serve or start");
}
#endif
}
int main(int argc,char **argv){
    Check(argc==2,"pass capture directory");audit::output=argv[1];audit::GL gl;
    auto host=audit::Host();auto *m=Open(host);Play(m,host);Close(m);
}
