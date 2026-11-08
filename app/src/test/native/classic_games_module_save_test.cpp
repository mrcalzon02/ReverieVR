// Executes the production module, including Create/Update/Pause/Resume/
// prepare_exit/Destroy and its real codec. GL sections are removed by linker
// garbage collection; this is a host lifecycle test, not a render/device test.
#include "classic_games_module.cpp"
#include <cstdlib>
#include <iostream>
#include <map>
#include <thread>

static int checks=0,reads=0,writes=0;
static bool failReads=false,failWrites=false;
static uint32_t feedback=0;
static std::map<std::string,std::vector<uint8_t>> slots;
static void Check(bool ok,const char *what){
    ++checks;if(!ok){std::cerr<<kId<<" FAIL: "<<what<<'\n';std::exit(1);}
}
static int32_t Read(const char *slot,void *data,uint32_t capacity,uint32_t *size){
    ++reads;*size=0;if(failReads)return REVERIE_NATIVE_SAVE_ERROR;
    const auto found=slots.find(slot);if(found==slots.end())return REVERIE_NATIVE_SAVE_NOT_FOUND;
    *size=static_cast<uint32_t>(found->second.size());
    if(capacity<*size)return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    std::memcpy(data,found->second.data(),*size);return REVERIE_NATIVE_SAVE_OK;
}
static int32_t Write(const char *slot,const void *data,uint32_t size){
    ++writes;if(failWrites)return REVERIE_NATIVE_SAVE_ERROR;
    Check(size<=REVERIE_NATIVE_SAVE_MAX_BYTES,"host slot size bound");
    const auto *bytes=static_cast<const uint8_t *>(data);slots[slot]={bytes,bytes+size};
    return REVERIE_NATIVE_SAVE_OK;
}
static void Feedback(uint32_t flags){feedback|=flags;}
static ReverieNativeHostV1 Host(){
    ReverieNativeHostV1 h{};h.struct_size=sizeof(h);h.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;
    h.read_save=Read;h.write_save=Write;h.request_feedback=Feedback;return h;
}
static ReverieNativeInputV1 Input(float seconds=.02f){
    ReverieNativeInputV1 input{};input.struct_size=sizeof(input);input.delta_seconds=seconds;return input;
}
static std::vector<uint8_t> Bytes(Module *m){
    std::vector<uint8_t> data;
    Check(save::Encode(CLASSIC_GAME_KIND,m->game,m->breakout,m->board,data),"production state encodes");
    return data;
}
static void Ray(ReverieNativeInputV1 &input,float x,float y,float z){
    input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    const float length=std::sqrt(x*x+y*y+z*z);
    input.pointer_direction[0]=x/length;input.pointer_direction[1]=y/length;input.pointer_direction[2]=z/length;
}
#if (CLASSIC_GAME_KIND>=3 && CLASSIC_GAME_KIND<=5) || CLASSIC_GAME_KIND==7
static void Click(Module *m,int cell){
    auto input=Input(0.f);const int size=m->board.Size();
    Ray(input,BoardX(m->board,cell%size),BoardY(m->board,cell/size),-2.91f);
    Update(m,&input);input.primary_down=1;Update(m,&input);input.primary_down=0;Update(m,&input);
}
#endif
static void Play(Module *m){
#if CLASSIC_GAME_KIND==3
    Click(m,40);Click(m,33);
#elif CLASSIC_GAME_KIND==4
    Click(m,180);
#elif CLASSIC_GAME_KIND==5
    Click(m,52);Click(m,36);
#elif CLASSIC_GAME_KIND==7
    Click(m,19);
#else
    auto input=Input(.04f);input.primary_down=1;Update(m,&input);
    input.primary_down=0;for(int i=0;i<4;++i)Update(m,&input);
#endif
}
static void AdvanceFixture(Module *m){
    // Explicit setup for lifecycle fault injection. Advance the production
    // simulation while paused, then ask the real callbacks to persist it.
    // Drop/join any worker copy before changing the live fixture.
#if CLASSIC_GAME_KIND==6
    m->breakout.Step(.057f,.6f);
#elif CLASSIC_GAME_KIND>=3
    m->cpu=reverie::boardgames::CpuWorker{};
    for(int i=0;i<3;++i)m->board.Step(.1f);
    // The move can already have completed during Play; preferences are also
    // persistent game state and guarantee a second distinct checkpoint.
    m->board.chessDifficulty=(m->board.chessDifficulty+1)%3;
#else
    m->game.Step(.071f,.6f);
#endif
}
static void Lifecycle(){
    const auto host=Host();Module *m=static_cast<Module *>(Create(&host));
    Check(m!=nullptr&&!m->saves.loadBlocked,"create fresh module");
    const auto initial=Bytes(m);Play(m);Pause(m);const auto saved=Bytes(m);
    Check(saved!=initial,"actual input advances game");Check(!m->saves.dirty&&writes>0,"pause commits exact latest state");
    auto held=Input(.1f);held.primary_down=held.secondary_down=1;held.move_x=.9f;held.move_y=.9f;
    for(int i=0;i<8;++i)Update(m,&held);
    Check(Bytes(m)==saved,"paused updates cannot advance simulation");
    Resume(m);for(int i=0;i<3;++i)Update(m,&held);
    Check(Bytes(m)==saved,"resume held buttons and axes remain disarmed");
    auto neutral=Input(0.f);Update(m,&neutral);
    Check(Bytes(m)==saved&&!m->releaseToArm,"neutral frame rearms without advancing");
    Check(PrepareExit(m)==1,"durable ordinary exit accepted");Destroy(m);
    m=static_cast<Module *>(Create(&host));
    Check(Bytes(m)==saved&&m->restorePrompt,"recreated exact match awaits Continue");
    for(int i=0;i<8;++i)Update(m,&held);
    Check(Bytes(m)==saved,"restored match cannot consume stale input");
    Update(m,&neutral);auto press=Input(0.f);press.primary_down=1;Update(m,&press);
    Check(!m->restorePrompt&&m->releaseToArm&&Bytes(m)==saved,"Continue consumes fresh edge without a game action");
    Update(m,&press);Update(m,&neutral);Check(Bytes(m)==saved,"Continue release cannot leak into game");
    Pause(m);AdvanceFixture(m);const auto unsaved=Bytes(m);
    Check(unsaved!=saved,"fault fixture has later real state");
    const auto committed=slots;failWrites=true;
    Check(PrepareExit(m)==0&&m->saves.dirty,"failed checkpoint refuses ordinary destruction");
    Check(Bytes(m)==unsaved&&slots==committed,"failed exit retains live state and previous save");
    Check(PrepareExit(m)==0&&Bytes(m)==unsaved,"repeated failed exit stays recoverable");
    Resume(m);Update(m,&neutral);for(int i=0;i<4;++i)Update(m,&neutral);
    Check(Bytes(m)==unsaved,"save recovery freezes simulation");
    failWrites=false;
    // Ordinary updates retry a failed checkpoint on a bounded two-second
    // cadence, even without the user pressing Retry or leaving the game.
    Resume(m);for(int i=0;i<24;++i)Update(m,&held);
    Check(!m->saves.dirty&&m->restorePrompt,"automatic retry commits and awaits conscious resume");
    Check(m->releaseToArm&&Bytes(m)==unsaved,"held input cannot prevent retry or advance retained state");
    Check(PrepareExit(m)==1,"recovered exit accepted");Destroy(m);
    m=static_cast<Module *>(Create(&host));Check(Bytes(m)==unsaved,"recovered progress restored exactly");Destroy(m);
    const auto good=slots;const int beforeWrites=writes;failReads=true;
    m=static_cast<Module *>(Create(&host));Check(m->saves.loadBlocked,"read interruption blocks new game");
    auto zero=Input(0.f);Update(m,&zero);Play(m);
    Check(PrepareExit(m)==0,"unresolved read blocks exit gate");Destroy(m);
    Check(writes==beforeWrites&&slots==good,"read failure never overwrites saved match");
    failReads=false;slots[save::kManifest].back()^=0x01;
    const auto corrupt=slots;m=static_cast<Module *>(Create(&host));
    Check(m->saves.loadBlocked&&PrepareExit(m)==0,"corrupt save refused transactionally");
    Destroy(m);Check(slots==corrupt&&writes==beforeWrites,"corrupt save preserved for recovery");
    slots=good;m=static_cast<Module *>(Create(&host));Check(Bytes(m)==unsaved,"restored storage recovers without reset");Destroy(m);
    std::cout<<kId<<" lifecycle, freeze/rearm, exact recreation, I/O retry, corrupt preservation PASS\n";
}
#if (CLASSIC_GAME_KIND>=3 && CLASSIC_GAME_KIND<=5) || CLASSIC_GAME_KIND==7
static void PendingCpuRebuild(){
    slots.clear();const auto host=Host();Module *m=static_cast<Module *>(Create(&host));
    // Use one production legal move to establish an exact pending CPU turn,
    // then create a real worker before the host suspends/destroys the module.
#if CLASSIC_GAME_KIND==3
    Check(m->board.CheckerMove(40,33),"pending checker turn setup");
#elif CLASSIC_GAME_KIND==4
    Check(m->board.GoMove(180),"pending Go turn setup");
#elif CLASSIC_GAME_KIND==5
    Check(m->board.ChessMove(52,36),"pending chess turn setup");
#else
    Check(m->board.ReversiMove(19),"pending Reversi turn setup");
#endif
    const auto before=m->board;Check(m->cpu.Start(m->board),"real CPU worker started");
    Pause(m);const auto checkpoint=Bytes(m);Destroy(m);
    m=static_cast<Module *>(Create(&host));
    Check(Bytes(m)==checkpoint&&!m->cpu.Busy(),"worker future replaced by exact pending board");
    auto input=Input(0.f);Update(m,&input);input.primary_down=1;Update(m,&input);
    input.primary_down=0;Update(m,&input);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(8);
    while(m->board.ply==before.ply&&!m->board.finished&&!m->cpuFailed&&
          std::chrono::steady_clock::now()<deadline){
        Update(m,&input);std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Check(!m->cpuFailed&&m->board.ply>before.ply,"restored real worker completes a legal opponent turn");
#if CLASSIC_GAME_KIND==5
    Check(reverie::boardgames::ValidChessCpuTransition(before,m->board),"restored chess CPU transition verified by production guard");
#else
    auto expected=before;for(int i=0;i<3;++i)expected.Step(.1f);
    std::vector<uint8_t> expectedBytes;
    Check(save::Encode(CLASSIC_GAME_KIND,m->game,m->breakout,expected,expectedBytes),"expected CPU result encodes");
    Check(Bytes(m)==expectedBytes,"restored worker matches deterministic production result including RNG");
#endif
    Pause(m);Check(PrepareExit(m)==1,"completed CPU move durably checkpoints");Destroy(m);
    std::cout<<kId<<" pending CPU destruction/rebuild PASS\n";
}
#endif
#if CLASSIC_GAME_KIND==1
static void SnakeVisibleButtons(){
    const auto host=Host();
    for(int direction=0;direction<4;++direction){
        slots.clear();Module *m=static_cast<Module *>(Create(&host));
        if(direction==3){
            // A fresh snake faces right, so first make a real upward move.
            // Left is then a legal turn instead of a forbidden reversal.
            m->game.Direction(0);m->game.Press();
            for(int i=0;i<3;++i)m->game.Step(.1f,0.f);
        }
        const int head=m->game.snake[0];const auto &button=kSnakeButtons[direction];
        auto input=Input(0.f);Ray(input,button.x,button.y,kSnakeButtonZ);
        Update(m,&input);feedback=0;input.primary_down=1;Update(m,&input);
        Check(m->game.desired==direction,"visible button center queues its named direction");
        Check((feedback&REVERIE_NATIVE_FEEDBACK_ACTIVATION)!=0,"visible direction activation feedback");
        input.primary_down=0;Update(m,&input);input.delta_seconds=.1f;
        for(int i=0;i<3;++i)Update(m,&input);
        const int delta[]={-12,1,12,-1};
        Check(m->game.heading==direction&&m->game.snake[0]==head+delta[direction],
              "fresh visible button click moves one cell in named direction");
        std::cout<<"snake visible "<<button.label<<" desired="<<m->game.desired
                 <<" head="<<head<<"->"<<m->game.snake[0]<<" PASS\n";Destroy(m);
    }
    slots.clear();Module *m=static_cast<Module *>(Create(&host));
    auto input=Input(0.f);Ray(input,0.f,1.50f,-2.91f);Update(m,&input);
    input.primary_down=1;Update(m,&input);
    Check(m->game.desired==1,"old invisible UP region no longer changes direction");Destroy(m);
}
#endif
int main(){Lifecycle();
#if (CLASSIC_GAME_KIND>=3 && CLASSIC_GAME_KIND<=5) || CLASSIC_GAME_KIND==7
    PendingCpuRebuild();
#endif
#if CLASSIC_GAME_KIND==1
    SnakeVisibleButtons();
#endif
    std::cout<<kId<<" checks="<<checks<<" reads="<<reads<<" writes="<<writes<<" PASS\n";
    return 0;
}
