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
static void Pad(State *s,float x,float y,bool click=false){
    auto in=audit::Input();in.move_x=x;in.move_y=y;in.primary_down=click;
    Update(s,&in);
    auto release=audit::Input();Update(s,&release);
}
static void ReliefRoute(){
    using Status=game::Frontier::ReliefStatus;
    audit::GL gl;
    auto host=audit::Host();
    audit::saves.clear();
    auto* s=static_cast<State*>(Create(&host));
    audit::Check(s && GlStart(s)==1,"relief GL start");
    Pad(s,0,0,true);Pad(s,0,0,true); // Title > campaign > begin
    Pad(s,1,0);Pad(s,0,0,true);
    audit::Check(s->relief_view,"pad opens Pilgrim Road");
    const auto unopened=audit::writes;
    Pad(s,0,0,true);
    audit::Check(s->session.frontier.relief_status()==Status::AwaitingDispatch &&
        audit::writes==unopened,"viewing remote manifest never ships supplies");
    auto back=audit::Input();back.secondary_down=true;Update(s,&back);
    back.secondary_down=false;Update(s,&back);
    Pad(s,-1,0);Pad(s,0,0,true); // Hearthfall
    Pad(s,1,0);Pad(s,0,0,true); // travel east from Hearthwell to Charter Gate
    audit::Check(s->session.frontier.location()==7,"pad march to S008");
    back.secondary_down=true;
    for(int i=0;i<15;++i)Update(s,&back);
    back.secondary_down=false;Update(s,&back);
    audit::Check(s->menu.screen==game::MenuScreen::Pause,"long minus opens pause");
    Pad(s,0,1);Pad(s,0,0,true); // wrap to World Map
    audit::Check(s->continent_view && s->menu.screen==game::MenuScreen::Playing,"pad returns to continent");
    Tap(s,.48f,-1.012f);
    audit::Check(s->relief_view,"tracked pointer enters route");
    const int rations=s->session.frontier.rations(),parts=s->session.frontier.salvage();
    Tap(s,RoadX(0),RoadY(0)); // equip the separate detachment
    audit::Check(s->session.frontier.relief_status()==Status::OnRoad &&
        s->session.frontier.rations()==rations-4 && s->session.frontier.salvage()==parts-2,
        "dispatch debits actual campaign inventory once");
    auto hit=[&](int i){Tap(s,RoadX(i),RoadY(i));};
    hit(2);
    audit::Check(s->session.frontier.relief_site()==0,"no teleport to clinic");
    hit(1);hit(2);hit(2);hit(2);
    audit::Check(s->session.frontier.clinic_supplied() && !s->session.frontier.camp_supplied(),
        "clinic delivered on location, camp pending");
    const auto checksum=s->session.Checksum();
    const auto frame=audit::Capture(Render,s,"omega-pilgrim-clinic",1.65f);
    audit::Check(PrepareExit(s)==1,"exit after first delivery");
    GlEnd(s);Destroy(s);
    s=static_cast<State*>(Create(&host));
    audit::Check(s && GlStart(s)==1 && s->session.Checksum()==checksum,"actual host reload mid-route");
    Pad(s,0,0,true);Pad(s,0,0,true);
    Tap(s,.48f,-1.012f);
    audit::Check(s->road_focus==2 && frame==audit::Capture(Render,s,"omega-pilgrim-restored",1.65f),
        "restored clinic, cargo and ledger reproduce stereo image");
    // A stationary head ray stays on the clinic. Pad selection still moves the party.
    auto in=audit::Pointer(RoadX(2),RoadY(2),kBoardZ);
    Update(s,&in);in.move_x=-1;Update(s,&in);in.move_x=0;Update(s,&in);
    in.primary_down=1;Update(s,&in);in.primary_down=0;Update(s,&in);
    audit::Check(s->session.frontier.relief_site()==1,"pad selects Ford under idle clinic ray");
    hit(3);hit(3);
    audit::Check(s->session.frontier.camp_supplied(),"camp unload");
    hit(1);hit(0);
    const int trust=s->session.frontier.trust(),day=s->session.frontier.day();
    hit(0);hit(0);
    audit::Check(s->session.frontier.relief_status()==Status::Filed &&
        s->session.frontier.relation(16)==10 && s->session.frontier.trust()==trust+6 &&
        s->session.frontier.location()==7 && day==8 && s->session.frontier.day()==day,
        "return and file once, six marches advance six strategic days");
    audit::Capture(Render,s,"omega-pilgrim-filed",1.65f);
    const auto done=s->session.Checksum();
    GlEnd(s);Destroy(s);
    s=static_cast<State*>(Create(&host));
    audit::Check(s && s->session.Checksum()==done,"completed manifest persists");
    Destroy(s);
    std::puts("PASS actual Pilgrim Road dispatch, partial delivery, restore, return and one-time reward");
}

int main(int argc,char** argv){
    const int prior=OmegaPositiveLifecycle(argc,argv);
    if(prior)return prior;
    const auto original_saves=audit::saves;
    ReliefRoute();
    audit::saves=original_saves;
    audit::GL context;
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
    Tap(state,0.f,.42f); // Title -> Campaign
    Tap(state,0.f,.42f); // First campaign action must only retry the read.
    audit::Check(state->load_failed && !state->has_campaign &&
        state->menu.screen==game::MenuScreen::Campaign &&
        audit::writes==writes && audit::saves[kSaveSlot]==corrupt,
        "ordinary campaign selection must preserve an unreadable slot");
    audit::Check(GlStart(state)==1,"unreadable campaign recovery GL");
    audit::Capture(Render,state,"omega-campaign-read-recovery");
    Tap(state,0.f,game::Menu::ButtonY(1)); // New campaign
    audit::Check(state->menu.screen==game::MenuScreen::ConfirmNew &&
        audit::writes==writes,"new campaign cannot erase until confirmation");
    Tap(state,0.f,game::Menu::ButtonY(1)); // Cancel
    audit::Check(state->menu.screen==game::MenuScreen::Campaign &&
        audit::saves[kSaveSlot]==corrupt,"cancel keeps original bytes");
    Tap(state,0.f,game::Menu::ButtonY(1)); // New campaign
    Tap(state,0.f,game::Menu::ButtonY(0)); // Confirm erase and start
    audit::Check(state->menu.screen==game::MenuScreen::Playing &&
        state->has_campaign && !state->load_failed && audit::writes==writes+1 &&
        audit::saves[kSaveSlot]!=corrupt,"only confirmed replacement starts fresh");
    GlEnd(state);
    Destroy(state);
    audit::saves[kSaveSlot]=good;read_error=true;
    state=static_cast<State*>(Create(&host));
    audit::Check(state&&state->load_failed&&PrepareExit(state)==0,
        "transient read failure blocks ordinary exit");
    Tap(state,0.f,.42f);
    const auto writes_before_retry=audit::writes;
    Tap(state,0.f,.42f);
    audit::Check(state->load_failed && audit::writes==writes_before_retry,
        "transient I/O error cannot turn a retry into a fresh campaign");
    read_error=false;
    Tap(state,0.f,.42f); // Retry with the original bytes now readable.
    audit::Check(state->has_campaign && !state->load_failed &&
        state->menu.screen==game::MenuScreen::Campaign &&
        state->session.Checksum()==checksum && audit::writes==writes_before_retry,
        "menu retry restores the original session without writing the slot");
    Tap(state,0.f,.42f);
    audit::Check(state->menu.screen==game::MenuScreen::Playing &&
        state->session.Checksum()==checksum,"continue resumes the recovered campaign");
    audit::Check(PrepareExit(state)==1&&!state->load_failed&&
        state->has_campaign&&state->session.Checksum()==checksum,
        "recovered read restores exact existing campaign before exit");
    Destroy(state);
    std::puts("PASS actual Omega ordinary-exit retry, corrupt/read-error protection and recovery");
}
