// Reuse the actual GLES/menu/travel/battle audit, then add exit fault cases.
#include "omega_front_module.cpp"
#include "audit_host.h"
static void Tap(State* state,float x,float y){auto input=audit::Pointer(x,y,kBoardZ);Update(state,&input);input.primary_down=1u;Update(state,&input);input.primary_down=0u;Update(state,&input);}
static void Cell(State* state,int x,int y){Tap(state,(x+.5f)*kCell-kHalf,kCenterY+kHalf-(y+.5f)*kCell);}
static int OmegaPositiveLifecycle(int argc,char** argv){
 audit::Check(argc==2,"capture dir");audit::output=argv[1];audit::GL context;auto host=audit::Host();
 auto* state=static_cast<State*>(Create(&host));audit::Check(state&&GlStart(state)==1,"Omega create GL");
 audit::Capture(Render,state,"omega-title");Tap(state,0.f,.42f);audit::Check(state->menu.screen==game::MenuScreen::Campaign,"actual title to campaign");
 Tap(state,0.f,.42f);audit::Check(state->menu.screen==game::MenuScreen::Playing&&state->has_campaign,"actual begin campaign");
 audit::Capture(Render,state,"omega-continent");Tap(state,0.f,1.065f);audit::Check(!state->continent_view,"actual region tab");
 Tap(state,.1975f,-.1775f);audit::Check(state->session.frontier.location()==10&&state->session.view==game::FrontMode::SectorMap,"actual region travel");
 audit::Capture(Render,state,"omega-region");Tap(state,.1975f,-.1775f);audit::Check(state->session.view==game::FrontMode::Tactical,"actual deploy");
 Cell(state,3,11);audit::Check(state->session.tactical.selected==0,"actual soldier select");Cell(state,7,11);audit::Check(state->session.tactical.battle.queued(0)==1,"actual movement order queued");
 auto input=audit::Input();for(int i=0;i<80;++i)Update(state,&input);audit::Check(state->session.tactical.battle.ticks()==0,"idle cannot resolve turns");
 audit::reject_writes=true;Tap(state,0.f,1.065f);audit::Check(state->session.tactical.battle.ticks()==1&&state->save_failed,"actual end turn failed checkpoint visible");
 audit::Capture(Render,state,"omega-save-failure");audit::reject_writes=false;const auto attempts=audit::writes;
 Pause(state);audit::Check(!state->save_failed&&audit::writes==attempts+1,"actual pause retries checkpoint and clears error");
 const auto checksum=state->session.Checksum();const auto ticks=state->session.tactical.battle.ticks();const auto writes=audit::writes;
 const auto frame=audit::Capture(Render,state,"omega-tactical");audit::Check(state->session.Checksum()==checksum&&state->session.tactical.battle.ticks()==ticks&&audit::writes==writes,"stereo rendering cannot advance simulation or save");
 GlEnd(state);context.Recreate();audit::Check(GlStart(state)==1,"Omega context recreate");audit::Check(frame==audit::Capture(Render,state,"omega-tactical-recreated"),"Omega stereo exact context recreation");
 GlEnd(state);Destroy(state);state=static_cast<State*>(Create(&host));audit::Check(state&&state->has_campaign&&state->menu.screen==game::MenuScreen::Title,"reload offers safe title and existing campaign");
 audit::Check(state->session.Checksum()==checksum&&state->session.tactical.battle.ticks()==1,"actual checkpoint exact session restore");
 std::printf("OMEGA_FAILURE_RECOVERY pause_retry=1 save_failed_after_retry=0 restored_checksum=%llu ticks=%u\n",static_cast<unsigned long long>(checksum),state->session.tactical.battle.ticks());
 Destroy(state);audit::MissingAssets();std::puts("PASS actual Omega module menu/travel/deploy/order/turn/save-failure/pause/recreate/render audit");
 return 0;
}

namespace {
bool read_error=false;
int32_t FaultRead(const char* slot,void* data,uint32_t capacity,uint32_t* size){
    return read_error?REVERIE_NATIVE_SAVE_ERROR:audit::Read(slot,data,capacity,size);
}
}
int main(int argc,char** argv){
    const int prior=OmegaPositiveLifecycle(argc,argv);
    if(prior)return prior;
    auto host=audit::Host();host.read_save=FaultRead;
    auto* state=static_cast<State*>(Create(&host));
    audit::Check(state&&state->has_campaign,"actual continued campaign");
    const auto checksum=state->session.Checksum();
    audit::reject_writes=true;
    audit::Check(PrepareExit(state)==0&&state->save_failed&&
        state->session.Checksum()==checksum,"failed ordinary exit preserves live campaign");
    audit::reject_writes=false;
    audit::Check(PrepareExit(state)==1&&!state->save_failed,
        "ordinary exit can retry pending checkpoint");
    Destroy(state);
    const auto good=audit::saves[kSaveSlot];
    audit::saves[kSaveSlot][20]^=1u;
    const auto corrupt=audit::saves[kSaveSlot];
    const auto writes=audit::writes;
    state=static_cast<State*>(Create(&host));
    audit::Check(state&&state->load_failed,"corrupt save rejected");
    audit::Check(PrepareExit(state)==0&&audit::writes==writes&&
        audit::saves[kSaveSlot]==corrupt,"exit cannot overwrite unreadable old save");
    Destroy(state);
    audit::saves[kSaveSlot]=good;read_error=true;
    state=static_cast<State*>(Create(&host));
    audit::Check(state&&state->load_failed&&PrepareExit(state)==0,
        "transient read failure blocks ordinary exit");
    read_error=false;
    audit::Check(PrepareExit(state)==1&&!state->load_failed&&
        state->has_campaign&&state->session.Checksum()==checksum,
        "recovered read restores exact existing campaign before exit");
    Destroy(state);
    std::puts("PASS actual Omega ordinary-exit retry, corrupt/read-error protection and recovery");
}
