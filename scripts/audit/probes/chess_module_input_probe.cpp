// Executes production chess module Update input paths; no graphics calls.
#include "classic_games_module.cpp"
#include <cassert>
#include <iostream>
static ReverieNativeInputV1 In(){ReverieNativeInputV1 i={};i.struct_size=sizeof(i);i.delta_seconds=.02f;return i;}
static void Click(Module*m,float x,float y,float z){
 auto i=In();i.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
 float length=std::sqrt(x*x+y*y+z*z);
 i.pointer_direction[0]=x/length;i.pointer_direction[1]=y/length;i.pointer_direction[2]=z/length;
 Update(m,&i);i.primary_down=1;Update(m,&i);i.primary_down=0;Update(m,&i);
}
static void Cell(Module*m,int square){Click(m,BoardX(m->board,square%8),BoardY(m->board,square/8),-2.91f);}
static Module* New(){return static_cast<Module*>(Create(nullptr));}
int main(){
 {auto*m=New();Cell(m,59);Cell(m,43);assert(m->board.ply==0&&m->board.cells[59]==5&&m->board.cells[43]==0);Destroy(m);std::cout<<"PASS white queen cannot jump initial pawn via real pointer input\n";}
 {auto*m=New();Cell(m,52);Cell(m,36);assert(m->board.ply==1&&m->board.cells[52]==0&&m->board.cells[36]==1);assert(m->chessTween.Active());Destroy(m);std::cout<<"PASS real pointer e2-e4 commits one legal move and starts tween\n";}
 {auto*m=New();m->board.StartChessGame(-1);assert(m->board.ChessMove(52,36));Cell(m,3);Cell(m,19);assert(m->board.ply==1&&m->board.cells[3]==-5&&m->board.cells[19]==0);Destroy(m);std::cout<<"PASS rotated black queen cannot jump d7 pawn via real pointer input\n";}
 {auto*m=New();m->board.StartChessGame(-1);assert(m->board.ChessMove(52,36));Cell(m,11);Cell(m,27);assert(m->board.ply==2&&m->board.cells[11]==0&&m->board.cells[27]==-1);Destroy(m);std::cout<<"PASS rotated black pointer maps d7-d5 to exact legal squares\n";}
 {auto*m=New();Click(m,2.22f,1.423f,reverie::chesspanel::kPanelZ);assert(m->chessNewGameOpen);Click(m,0.f,-.407f,-2.75f);assert(!m->chessNewGameOpen&&m->board.ply==0);Destroy(m);std::cout<<"PASS New Game opens and Cancel returns with state retained\n";}
 {auto*m=New();Click(m,2.22f,1.423f,reverie::chesspanel::kPanelZ);assert(m->chessNewGameOpen);Click(m,.65f,.095f,-2.75f);assert(!m->chessNewGameOpen&&m->board.chessHumanColor==-1);Destroy(m);std::cout<<"PASS New Game Black button selects human Black\n";}
 {auto*m=New();Click(m,-2.22f,0.f,reverie::chesspanel::kPanelZ);assert(m->chessWikiOpen);Click(m,2.42f,1.49f,-2.75f);assert(!m->chessWikiOpen);Destroy(m);std::cout<<"PASS left reference opens and Close returns to game\n";}
 {auto*m=New();Click(m,2.22f,-.98f,reverie::chesspanel::kPanelZ);assert(m->board.chessDifficulty==2&&!m->chessWikiOpen);Destroy(m);std::cout<<"PASS angled AI control cycles difficulty without opening reference\n";}
 std::cout<<"8 production module input assertions passed; artificial seeded positions used only for Black orientation tests.\n";
}
