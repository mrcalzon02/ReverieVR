// Audit probe: directly executes the unchanged module Create/Update/Pause/Resume/Destroy.
// Graphics functions are linked out, not mocked. This is not a render/device test.
#include "classic_games_module.cpp"
#include <sstream>
#include <iostream>
#include <map>
#include <cstring>
static int auditReads=0,auditWrites=0;
// In-memory host storage persists across module instances; game encoding is untouched.
static std::map<std::string,std::vector<unsigned char>> auditSlots;
static int32_t AuditRead(const char*slot,void*data,uint32_t capacity,uint32_t*out){
 ++auditReads;auto found=auditSlots.find(slot?slot:"");
 if(found==auditSlots.end()){if(out)*out=0;return 0;}
 if(out)*out=static_cast<uint32_t>(found->second.size());
 if(capacity<found->second.size())return 0;
 std::memcpy(data,found->second.data(),found->second.size());return 1;
}
static int32_t AuditWrite(const char*slot,const void*data,uint32_t size){
 ++auditWrites;const auto*bytes=static_cast<const unsigned char*>(data);
 auditSlots[slot?slot:""]=std::vector<unsigned char>(bytes,bytes+size);return 1;
}
static std::string Signature(Module* m){
    std::ostringstream out;
#if CLASSIC_GAME_KIND==6
    auto& s=m->breakout;out<<s.paddle<<':'<<s.ballX<<':'<<s.ballY<<':'<<s.score<<':'<<s.lives<<':'<<s.moving;
#elif CLASSIC_GAME_KIND>=3
    auto& s=m->board;out<<s.ply<<':'<<s.side<<':'<<s.finished;
    for(int8_t c:s.cells)out<<':'<<static_cast<int>(c);
#else
    auto& s=m->game;out<<s.score<<':'<<s.opponent<<':'<<s.ball_x<<':'<<s.ball_y<<':'<<s.moving<<':'<<s.position<<':'<<s.rival<<':'<<s.snake[0];
#endif
    return out.str();
}
static ReverieNativeInputV1 Input(){ReverieNativeInputV1 i={};i.struct_size=sizeof(i);i.delta_seconds=.02f;return i;}
#if CLASSIC_GAME_KIND>=3 && CLASSIC_GAME_KIND!=6
static void Click(Module* m,int cell){
    auto i=Input();i.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    float x=BoardX(m->board,cell%8),y=BoardY(m->board,cell/8);
#if CLASSIC_GAME_KIND==4
    x=BoardX(m->board,cell%19);y=BoardY(m->board,cell/19);
#endif
    float length=std::sqrt(x*x+y*y+2.91f*2.91f);
    i.pointer_direction[0]=x/length;i.pointer_direction[1]=y/length;i.pointer_direction[2]=-2.91f/length;
    Update(m,&i);i.primary_down=1;Update(m,&i);i.primary_down=0;Update(m,&i);
}
#endif
int main(){
    ReverieNativeHostV1 h={};h.struct_size=sizeof(h);h.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;h.read_save=AuditRead;h.write_save=AuditWrite;
    auto* m=static_cast<Module*>(Create(&h));auto initial=Signature(m);
#if CLASSIC_GAME_KIND==3
    Click(m,40);Click(m,33);
#elif CLASSIC_GAME_KIND==4
    Click(m,180);
#elif CLASSIC_GAME_KIND==5
    Click(m,52);Click(m,36);
#elif CLASSIC_GAME_KIND==7
    Click(m,19);
#else
    auto i=Input();i.primary_down=1;Update(m,&i);i.primary_down=0;
    for(int n=0;n<30;n++)Update(m,&i);
#endif
    const auto progressed=Signature(m);Pause(m);Resume(m);const bool inSession=Signature(m)==progressed;
    Destroy(m);m=static_cast<Module*>(Create(&h));const bool restored=Signature(m)==progressed;const bool reset=Signature(m)==initial;Destroy(m);
    std::cout<<kId<<" progressed="<<(progressed!=initial)<<" pause_resume_preserved="<<inSession<<" recreate_restored="<<restored<<" reset_to_initial="<<reset<<" read_save_calls="<<auditReads<<" write_save_calls="<<auditWrites<<" result="<<((progressed!=initial&&restored)?"PASS":"FAIL_SAVE_RESTORE")<<'\n';
    return (progressed!=initial&&restored)?0:1;
}
