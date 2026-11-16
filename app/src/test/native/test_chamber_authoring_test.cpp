// Production entry points, controller rays and actual authored-project playback.
#include "test_chamber_module.cpp"
#include "featured_module_test_host.h"

namespace {
const auto* game=reverie_native_module_entry_v1();
unsigned spatialEvents=0;
float lastLeft=0.f,lastRight=0.f;
uint32_t lastCue=0;
bool failLastProject=false;
int32_t Write(const char* name,const void* data,uint32_t size){
    if(failLastProject&&std::string(name)==kLastProject)return REVERIE_NATIVE_SAVE_ERROR;
    return audit::Write(name,data,size);
}
void Hear(uint32_t cue,float left,float right){
    ++spatialEvents;lastCue=cue;lastLeft=left;lastRight=right;
}
void Point(ModuleState* state,float x,float y,float z,bool pressed){
    auto input=audit::Pointer(x,y,z);
    input.primary_down=pressed;
    game->update(state,&input);
}
void Tool(ModuleState* state,int row){
    using namespace reverie::authoring;
    const float x=(kVrMenuLeft+kVrMenuRight)*.5f,y=kVrMenuTop-(row+.5f)*kVrMenuRowHeight;
    Point(state,x,y,-2.62f,true);
    Point(state,x,y,-2.62f,false);
}
void Secondary(ModuleState* state){
    auto input=audit::Input();input.secondary_down=1;game->update(state,&input);
    input.secondary_down=0;game->update(state,&input);
}
void Drag(ModuleState* state,float x,float z){
    Point(state,.75f,-1.16f,-3.f,true); // select newly placed object
    Point(state,x,-1.5f,z,true);
    Point(state,x,-1.5f,z,false);
}
void Close(ModuleState* state){
    game->pause(state);audit::Check(game->prepare_exit(state),"flush authored project");
    game->release_gl_context(state);game->destroy(state);
}
}
int main(int argc,char** argv){
    using namespace reverie::authoring;
    audit::Check(argc==2,"capture directory");audit::output=argv[1];
    audit::GL context;auto host=audit::Host();host.request_spatial_sound=Hear;host.write_save=Write;
    auto* state=static_cast<ModuleState*>(game->create(&host));
    audit::Check(state&&game->on_gl_context_created(state),"boot workshop");
    Secondary(state);
    Point(state,kVrProjectButtonX,kVrProjectButtonY,-2.62f,true);
    Point(state,kVrProjectButtonX,kVrProjectButtonY,-2.62f,false);
    audit::Check(state->workshop.editing&&state->workshop.slot==1&&state->workshop.scene.pieces.empty(),
                 "open empty second project from bundled scene");
    Tool(state,0);Drag(state,-.25f,-3.75f);
    Tool(state,1);Drag(state,1.5f,-2.75f);
    audit::Check(state->workshop.scene.pieces.size()==2&&state->workshop.scene.pieces[0].x==-.25
        &&state->workshop.scene.pieces[1].x==1.5,"author and position box and cylinder with controller");
    Tool(state,9);Tool(state,0);Tool(state,4);Tool(state,6); // bob, speed and amplitude
    Tool(state,8);Tool(state,0);Tool(state,2);Tool(state,4); // attach and tune light
    Tool(state,8);Tool(state,0);Tool(state,2);Tool(state,5); // attach and tune emitter
    audit::Capture(game->render_eye,state,"chamber-authored-editor");
    Tool(state,9);Tool(state,9);Tool(state,9);
    failLastProject=true;Tool(state,7);
    audit::Check(state->workshop.dirty&&game->prepare_exit(state)==0
        &&audit::saves.count(kLastProject)==0,"unacknowledged current slot keeps project pending");
    failLastProject=false;Tool(state,7);
    audit::Check(!state->workshop.dirty&&game->prepare_exit(state)==1,"explicit save retries project and current slot");
    const auto& bytes=audit::saves.at("vr-workshop-project-1.rvproj");
    const auto saved=DecodeVrBundle(std::string(bytes.begin(),bytes.end()));
    const auto& cylinder=saved.scene.pieces.at(1);
    audit::Check(cylinder.kind=="cylinder"&&cylinder.z==-2.75&&saved.motions.size()==1
        &&saved.motions[0].id==cylinder.id&&saved.motions[0].speed==1.25&&saved.motions[0].amount==.60
        &&saved.light.anchor_id==cylinder.id&&saved.light.power==1.25&&saved.light.radius==5.5
        &&saved.sound.anchor_id==cylinder.id&&saved.sound.cue==12&&saved.sound.interval==2.5,
        "atomic project stores authored geometry, tuned motion, light and sound");
    Tool(state,8);audit::Check(!state->workshop.editing,"play test via workshop control");
    Close(state);
    state=static_cast<ModuleState*>(game->create(&host));
    audit::Check(state&&state->workshop.slot==1&&state->workshop.scene.pieces.size()==2,
                 "relaunch reopens the saved authored project in play");
    audit::Check(game->on_gl_context_created(state),"reopen GL");
    auto input=audit::Input();
    input.listener_valid=1;input.listener_right[0]=1.f;
    for(int i=0;i<51;++i)game->update(state,&input);
    audit::Check(spatialEvents==1&&lastCue==12&&lastRight>lastLeft&&lastLeft>0.f,
                 "authored periodic cue reaches stereo host with source on right");
    const auto first=audit::Capture(game->render_eye,state,"chamber-authored-play");
    const auto& piece=state->workshop.scene.pieces[1];
    float x,y,z;
    audit::Check(state->workshop.LightPosition(state->clock,x,y,z),"sample rendered emitter");
    for(int i=0;i<9;++i)game->update(state,&input);
    float laterX,laterY,laterZ;
    audit::Check(state->workshop.LightPosition(state->clock,laterX,laterY,laterZ)&&y!=laterY
        &&x==piece.x&&z==piece.z&&x==laterX&&z==laterZ,"authored light follows bobbing cylinder");
    const auto later=audit::Capture(game->render_eye,state,"chamber-authored-motion");
    audit::Check(first[0]!=later[0]&&first[1]!=later[1],"both eyes animate reopened authored scene");
    input.listener_right[0]=-1.f;
    for(int i=0;i<50;++i)game->update(state,&input);
    audit::Check(spatialEvents==2&&lastLeft>lastRight,"turning listener reverses stereo panning");
    Close(state);
    audit::saves[kLastProject]={1};audit::saves["vr-workshop-project-1.rvproj"]={0xff};
    state=static_cast<ModuleState*>(game->create(&host));
    audit::Check(state&&!state->workshop.recovering&&state->workshop.slot==0
        &&state->workshop.blocked_slots[1]&&audit::saves["vr-workshop-project-1.rvproj"]==std::vector<uint8_t>{0xff},
        "unreadable preferred project retained, bundled fallback available");game->destroy(state);
    std::puts("PASS Test Chamber blank slot -> authored two moved primitives -> tuned bob/light/stereo emitter -> saved -> recreated -> played");
}
