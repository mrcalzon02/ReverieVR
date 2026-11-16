#include "audit_host.h"
#include <thread>
#ifdef DOORS
#include "guess_the_door_module.cpp"
int main(int argc,char** argv){
    audit::Check(argc==2,"capture directory argument");
    audit::output=argv[1];audit::GL gl;
    auto host=audit::Host();
    Game seedGame;seedGame.Reset(0x06198D461957185DULL);
    reverie::doors::MazeSave saved{};
    audit::Check(reverie::doors::EncodeMazeSave(seedGame,saved),"encode initial production maze save");
    audit::saves[reverie::doors::kMazeSaveSlot]={saved.begin(),saved.end()};
    auto *s=static_cast<State*>(Create(&host));
    audit::Check(s&&s->game.seed==seedGame.seed&&s->game.room==0,"restore specific seed at first room");
    audit::Check(s&&GlStart(s),"start Guess the Door");Resume(s);
    auto in=audit::Input();Update(s,&in);
    in.move_x=s->game.current.safeSide?1.f:-1.f;in.primary_down=1;Update(s,&in);
    audit::Check(s->game.phase==Phase::Pulling,"actual safe lever edge");
    in=audit::Input();
    for(int i=0;i<150&&s->game.phase!=Phase::Ready;++i)Update(s,&in);
    audit::Check(s->game.room==1,"walk through safe first room");
    in=audit::Pointer(-1.f,0.f,-3.43f);
    for(int i=0;i<12;++i)Update(s,&in);
    audit::Check(s->doorGaze==0&&s->gazeSeconds>.45f,"inspect left door");
    auto eyes=audit::Capture(Render,s,kId,1.20f);
    audit::Check(eyes[0]!=eyes[1],"stereo eyes differ");
    std::cout<<"ROOM 2 inspected left door; feature clue "<<s->game.current.Clue()<<"\n";
    Pause(s);audit::Check(PrepareExit(s)==1,"checkpoint");
    GlEnd(s);Destroy(s);
}
#else
#include "classic_games_module.cpp"
static void Tap(Module *m,ReverieNativeInputV1 in){
    in.primary_down=0;Update(m,&in);in.primary_down=1;Update(m,&in);
    in.primary_down=0;Update(m,&in);
}
int main(int argc,char** argv){
    audit::Check(argc==2,"capture directory argument");
    audit::output=argv[1];audit::GL gl;
    auto host=audit::Host();auto *m=static_cast<Module*>(Create(&host));
    audit::Check(m&&GlCreate(m),"start classic module");Resume(m);
    auto in=audit::Input();Update(m,&in);
#if CLASSIC_GAME_KIND==5
    Tap(m,audit::Pointer(2.22f,1.423f,reverie::chesspanel::kPanelZ));
    Tap(m,audit::Pointer(.65f,.095f,-2.75f));
    audit::Check(m->board.chessHumanColor==-1,"select Black in New Game menu");
    for(int i=0;i<3000&&m->board.ply<1;++i){Update(m,&in);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    for(int i=0;i<50;++i)Update(m,&in);
    audit::Check(m->board.side==-1&&m->board.ply==1&&!m->chessTween.Active(),"White CPU first move complete");
    const float gap=BoardSpacing(m->board);
    in=audit::Pointer(BoardX(m->board,3)-gap,BoardY(m->board,1),-2.91f);
    Update(m,&in);in.move_x=1.f;Update(m,&in);in.move_x=0.f;Tap(m,in);
    audit::Check(m->board.selected==11,"mixed input selects Black d7");
    std::cout<<"Selected Black pawn at d7 by touchpad after White CPU opening.\n";
#elif CLASSIC_GAME_KIND==3 || CLASSIC_GAME_KIND==4 || CLASSIC_GAME_KIND==7
#if CLASSIC_GAME_KIND==3
    Tap(m,audit::Pointer(BoardX(m->board,0),BoardY(m->board,5),-2.91f));
    Tap(m,audit::Pointer(BoardX(m->board,1),BoardY(m->board,4),-2.91f));
#elif CLASSIC_GAME_KIND==4
    Tap(m,audit::Pointer(BoardX(m->board,9),BoardY(m->board,9),-2.91f));
#else
    Tap(m,audit::Pointer(BoardX(m->board,3),BoardY(m->board,2),-2.91f));
#endif
    for(int i=0;i<3000&&m->board.ply<2;++i){Update(m,&in);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    audit::Check(m->board.ply>=2,"human move and asynchronous CPU reply");std::cout<<"ply="<<m->board.ply<<" last from/to="<<m->board.lastFrom<<"/"<<m->board.lastTo<<"\n";
#elif CLASSIC_GAME_KIND==0 || CLASSIC_GAME_KIND==6
    in=audit::Pointer(0.f,0.f,-2.91f);Tap(m,in);
#if CLASSIC_GAME_KIND==6
    in.move_x=1.f;
#else
    in.move_y=1.f;
#endif
    for(int i=0;i<6;++i)Update(m,&in);
    in.move_x=in.move_y=0.f;for(int i=0;i<3;++i)Update(m,&in);
#if CLASSIC_GAME_KIND==6
    audit::Check(m->breakout.moving&&m->breakout.paddle>.4f,"Breakout serve and pad steer");std::cout<<"paddle="<<m->breakout.paddle<<" ball="<<m->breakout.ballX<<","<<m->breakout.ballY<<"\n";
#else
    audit::Check(m->game.moving&&m->game.paddle>.4f,"Pong serve and pad steer");std::cout<<"paddle="<<m->game.paddle<<" ball="<<m->game.ball_x<<","<<m->game.ball_y<<"\n";
#endif
#elif CLASSIC_GAME_KIND==1
    const auto &b=kSnakeButtons[0];Tap(m,audit::Pointer(b.x,b.y,kSnakeButtonZ));
    for(int i=0;i<6;++i)Update(m,&in);
    audit::Check(m->game.heading==0&&m->game.snake[0]==65,"Snake upward movement");
    std::cout<<"head 77 -> "<<m->game.snake[0]<<"\n";
#else
    Tap(m,in);for(int i=0;i<20;++i)Update(m,&in);
    audit::Check(m->game.position>0&&m->game.rival>0&&m->game.player_turn,"full round of rolls");
    std::cout<<"player="<<m->game.position<<" opponent="<<m->game.rival<<" die="<<m->game.die<<"\n";
#endif
    auto eyes=audit::Capture(Draw,m,kId,1.20f);audit::Check(eyes[0]!=eyes[1],"stereo eyes differ");
    Pause(m);audit::Check(PrepareExit(m)==1,"checkpoint");GlRelease(m);Destroy(m);
}
#endif
