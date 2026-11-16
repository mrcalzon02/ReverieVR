#include "recon_projects_module.cpp"
#include "featured_module_test_host.h"
using Checkpoint=reverie::recon::Checkpoint;
static void Click(State* s){auto in=audit::Input();Update(s,&in);in.primary_down=1;Update(s,&in);in.primary_down=0;Update(s,&in);}
static void Cycle(State* s,int count=1){for(int i=0;i<count;++i){auto in=audit::Input();Update(s,&in);in.secondary_down=1;Update(s,&in);in.secondary_down=0;Update(s,&in);}}
static Checkpoint::Bytes Bytes(State* s){Checkpoint::Bytes bytes{};audit::Check(Checkpoint::Encode(Snapshot(s),&bytes),"actual current-phase checkpoint encodes");return bytes;}
static State* Recreate(State* s,const ReverieNativeHostV1* host,const std::string& name){
    Pause(s);audit::Check(PrepareExit(s)==1,"durable exit permitted");const auto before=Bytes(s);
    const auto frame=audit::Capture(Render,s,name+"-before");
    GlEnd(s);Destroy(s);s=static_cast<State*>(Create(host));
    audit::Check(s&&!s->loadBlocked&&Bytes(s)==before,"exact actual module restored");
    auto held=audit::Input();held.primary_down=1;Update(s,&held);
    audit::Check(Bytes(s)==before,"held input cannot advance a restored phase");
    held.primary_down=0;Update(s,&held);
    audit::Check(GlStart(s)==1,"recreated board actual GL");
    audit::Check(frame==audit::Capture(Render,s,name+"-after"),"both eye frames identical after phase recreation");
    return s;
}
#if RECON_PROJECT_KIND == 0
static void InspectLoupe(State *s,audit::GL &gl){
    const auto checkpoint=Bytes(s);
    const auto writes=audit::writes;
    auto aim=audit::Pointer(.64f,kPhotoY+.64f,kPhotoZ);
    Update(s,&aim);
    audit::Check(s->cropX==8 && s->cropY==0,"tracked aim reaches upper right of film");
    aim.secondary_down=1;Update(s,&aim);
    auto pan=[&](float x,float y){
        aim.move_x=x;aim.move_y=y;Update(s,&aim);Update(s,&aim);
        aim.move_x=aim.move_y=0;Update(s,&aim);
    };
    pan(-1,1);pan(-1,1);pan(0,1);
    aim.secondary_down=0;Update(s,&aim);Update(s,&aim);
    audit::Check(s->cropX==4 && s->cropY==6 && s->choice==3,
        "pad pans under a stationary ray and releasing Minus preserves report");
    constexpr float scale=1.45f;
    const auto frame=audit::Capture(Render,s,"lantern-bridge-loupe",scale);
    // Sample every cell center in the actual right-eye loupe framebuffer.
    // A bright cell must be the identical bit in the captured source film.
    const auto eye=audit::Eye(1,scale);
    for(int y=0;y<8;++y)for(int x=0;x<8;++x){
        const float wx=kLoupeX+(x-3.5f)*2*kPhotoPixel;
        const float wy=kPhotoY+(3.5f-y)*2*kPhotoPixel;
        const int px=static_cast<int>((1+(wx+eye.view[12])*eye.projection[0]/-kPhotoZ)*audit::GL::width*.5f);
        const int py=static_cast<int>((1+wy*eye.projection[5]/-kPhotoZ)*audit::GL::height*.5f);
        uint8_t rgb[4]{};
        glReadPixels(px,py,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgb);
        const bool observed=(s->photo.pixels[6+y] & (1u<<(4+x)))!=0;
        audit::Check((rgb[0]>130)==observed,"2x loupe framebuffer agrees with source pixel");
    }
    audit::Check(glGetError()==GL_NO_ERROR && Bytes(s)==checkpoint && audit::writes==writes,
        "inspection changes no checkpoint, report selection, or storage");
    Pause(s);GlEnd(s);gl.Recreate();audit::Check(GlStart(s)==1,"loupe GL restored");Resume(s);
    audit::Check(frame==audit::Capture(Render,s,"lantern-bridge-loupe-restored",scale),
        "both magnified eye frames survive pause and lost GL context");
    auto neutral=audit::Input();Update(s,&neutral);
    auto old=aim;old.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;Update(s,&old);
    audit::Check(s->cropX==4 && s->cropY==6,"old input prefix preserves inspection");
    std::puts("PASS Lantern loupe: all 64 framebuffer cells matched original evidence; no report or save on pan");
}
#endif
int main(int argc,char** argv){
    std::setvbuf(stdout,nullptr,_IONBF,0);audit::Check(argc==2,"capture directory argument");audit::output=argv[1];
    audit::GL context;auto host=audit::Host();auto* s=static_cast<State*>(Create(&host));
    audit::Check(s&&!s->loadBlocked&&GlStart(s)==1,"actual recon GL");
    audit::Check(api.prepare_exit==PrepareExit,"published guarded-exit callback");
    auto pad=audit::Input();
    Update(s,&pad);
    pad.move_x=-1.f;Update(s,&pad);Update(s,&pad);
    audit::Check(s->choice==2,"left wraps to previous choice once while held");
    pad.move_x=0.f;Update(s,&pad);pad.move_y=1.f;Update(s,&pad);
    audit::Check(s->choice==0,"down returns to next choice");
    pad.move_y=0.f;Update(s,&pad);
#if RECON_PROJECT_KIND == 0
    const std::string prefix="lantern";
    Cycle(s,2);s=Recreate(s,&host,"lantern-briefing");
    audit::Check(s->choice==2,"selected river preserved");
    Click(s);s=Recreate(s,&host,"lantern-in-flight");
    Click(s);Cycle(s,3);s=Recreate(s,&host,"lantern-photo");
    InspectLoupe(s,context);
    const auto photo=s->photo;const auto prior=audit::saves;
    audit::reject_writes=true;Click(s);
    audit::Check(s->simulation.phase()==sim::Phase::Filed&&s->simulation.credibility()==3
        &&s->simulation.operational_effect()==2&&s->saveDirty&&s->saveFailed,"real report retained after failed write");
    audit::Check(PrepareExit(s)==0&&audit::saves==prior,"report exit guarded, prior photo save retained");
    audit::Capture(Render,s,"lantern-save-warning");audit::reject_writes=false;
    auto input=audit::Input();for(int i=0;i<40;++i)Update(s,&input);
    audit::Check(!s->saveDirty&&!s->saveFailed,"idle report retry recovered");
    s=Recreate(s,&host,"lantern-filed");
    audit::Check(s->hasPhoto&&s->photo.pixels==photo.pixels&&s->result.credibility_delta==3,"filed evidence/result preserved");
    Click(s);audit::Check(s->simulation.phase()==sim::Phase::Briefing&&s->simulation.credibility()==3
        &&s->simulation.operational_effect()==2,"new sortie retains cumulative consequence");
    s=Recreate(s,&host,"lantern-next-sortie");
    Cycle(s);Click(s);Click(s);audit::ambiguous_writes=true;Click(s);
    const auto visible=audit::saves;const auto current=Bytes(s);
    audit::Check(s->simulation.credibility()==5&&s->simulation.operational_effect()==3&&PrepareExit(s)==0,"second actual report cumulative and durability guarded");
    audit::ambiguous_writes=false;Pause(s);
    audit::Check(audit::saves==visible&&Bytes(s)==current&&!s->saveDirty,"unacknowledged report retry cannot double-apply effect");
    s=Recreate(s,&host,"lantern-second-report");
    Click(s);audit::Check(s->simulation.credibility()==5&&s->simulation.operational_effect()==3,"recreated filed report not awarded again");
    Cycle(s,2);Click(s);
    const unsigned before_writes=audit::writes;
    Cycle(s);audit::Check(audit::writes==before_writes,"rocker leaves single-action screen and storage alone");
    Click(s);s=Recreate(s,&host,"lantern-river-revisit");
    audit::Check(s->photo.pixels!=photo.pixels,"the revisited river has a different deck");
    Click(s);audit::Check(s->simulation.credibility()==7 && s->result.operational_effect==1,
        "filing no contact on the intact crossing produces a favorable report");
    std::puts("PASS_LANTERN cumulative trust=5 effect=3; all four phases/evidence/choice/results recreated exactly");
#else
    const std::string prefix="forward";
    Click(s);s=Recreate(s,&host,"forward-travelling");
    Click(s);Cycle(s,2);s=Recreate(s,&host,"forward-contact");
    const auto prior=audit::saves;audit::reject_writes=true;Click(s);
    audit::Check(s->simulation.phase()==sim::Phase::Resolved&&s->simulation.vehicle_condition()==75
        &&s->simulation.fuel_remaining()==15&&s->saveDirty&&s->saveFailed,"real engage consequence retained on failed save");
    Pause(s);audit::Check(PrepareExit(s)==0&&audit::saves==prior,"failed Pause keeps live patrol guarded");
    audit::Capture(Render,s,"forward-save-warning");audit::reject_writes=false;Pause(s);
    audit::Check(!s->saveDirty&&!s->saveFailed,"Pause retries recovered patrol");
    s=Recreate(s,&host,"forward-resolved");Click(s);s=Recreate(s,&host,"forward-returned");
    Click(s);s=Recreate(s,&host,"forward-debriefed");
    audit::Check(s->debrief.information_value==-3&&s->debrief.unnecessary_force,"exact actual debrief restored");
    Click(s);audit::Check(s->simulation.vehicle_condition()==75&&s->simulation.fuel_remaining()==10,"next patrol does not refill or repair");
    s=Recreate(s,&host,"forward-next-patrol");Cycle(s);const auto insufficient=Bytes(s);Click(s);
    audit::Check(Bytes(s)==insufficient&&s->simulation.phase()==sim::Phase::Briefing,"cannot depart without complete return fuel");
    audit::Capture(Render,s,"forward-insufficient-fuel");
    Cycle(s);Click(s);audit::Check(s->confirmRefit&&s->choice==0,"base service requires deliberate confirmation, defaults Cancel");
    s=Recreate(s,&host,"forward-service-cancel");Click(s);
    audit::Check(!s->confirmRefit&&s->simulation.vehicle_condition()==75&&s->simulation.fuel_remaining()==10,"Cancel never refills or repairs");
    // Use the remaining road fuel normally; then explicit service restores it.
    Click(s);Click(s);Cycle(s);Click(s);Click(s);Click(s);Click(s);
    audit::Check(s->simulation.phase()==sim::Phase::Briefing&&s->simulation.fuel_remaining()==0
        &&s->simulation.vehicle_condition()==75,"second complete patrol ends at zero, not negative fuel");
    const auto empty=Bytes(s);Click(s);audit::Check(Bytes(s)==empty,"empty patrol cannot launch into a dead end");
    audit::Capture(Render,s,"forward-empty-fuel");Cycle(s,2);Click(s);Cycle(s);
    audit::ambiguous_writes=true;Click(s);const auto visible=audit::saves;const auto current=Bytes(s);
    audit::Check(!s->confirmRefit&&s->simulation.vehicle_condition()==100&&s->simulation.fuel_remaining()==20
        &&s->saveDirty&&PrepareExit(s)==0,"only confirmed base service refits; uncertain durability retains live result");
    audit::Capture(Render,s,"forward-refit-save-warning");audit::ambiguous_writes=false;Resume(s);
    audit::Check(!s->saveDirty&&Bytes(s)==current&&audit::saves==visible,"refit retries identical latest state");
    s=Recreate(s,&host,"forward-confirmed-refit");
    audit::Check(s->simulation.LastRouteIntel(sim::Route::MainRoad)==1 &&
        s->simulation.LastRouteIntel(sim::Route::RidgeBypass)==0,"last filed road intelligence survived refit and recreation");
    Cycle(s);Click(s);
    audit::Check(s->simulation.OnRidgeTrail() && s->simulation.trail_position()==0,"ridge patrol starts on foot");
    const auto start=audit::Capture(Render,s,"forward-ridge-trailhead");
    auto walk=[&](float direction){
        auto input=audit::Input();input.move_x=direction;
        Update(s,&input);Update(s,&input);input.move_x=0;Update(s,&input);
    };
    walk(1);walk(1);Click(s);
    audit::Check(s->simulation.trail_position()==2 && s->simulation.trail_notes()==sim::Simulation::kGaugeNote,
        "actual pad walk reaches gauge, Select records water mark");
    s=Recreate(s,&host,"forward-ridge-gauge");
    walk(1);walk(-1);
    audit::Check(s->simulation.trail_position()==2 && s->simulation.trail_notes()==1,"can retrace route");
    Click(s);Click(s);Click(s);
    audit::Check(s->simulation.trail_position()==5,"Select-only movement reaches shelter");
    const auto shelter=audit::Capture(Render,s,"forward-ridge-shelter");
    audit::Check(start!=shelter,"world changes through walking");
    audit::reject_writes=true;Click(s);
    audit::Check(s->simulation.trail_notes()==3 && PrepareExit(s)==0,"failed shelter note save guarded");
    audit::reject_writes=false;s=Recreate(s,&host,"forward-ridge-notebook");
    walk(1);walk(1);walk(1);Click(s);
    audit::Check(s->simulation.phase()==sim::Phase::Contact && s->evidence.civilian_silhouettes,
        "ridge turnout joins existing civilian contact");
    Cycle(s);Click(s);Click(s);Click(s);
    audit::Check(s->debrief.information_value==6 && s->debrief.fuel_remaining==4 &&
        s->simulation.filed_ridge_notes()==3,"two verified observations affect actual filed report");
    s=Recreate(s,&host,"forward-ridge-filed");Click(s);Cycle(s);
    audit::Check(s->simulation.LastRouteIntel(sim::Route::RidgeBypass)==6 &&
        s->simulation.filed_ridge_notes()==3 && s->simulation.LastRouteIntel(sim::Route::MainRoad)==1,
        "both observations and route intel survive restart and next briefing");
    std::puts("PASS_FORWARD normal continuation retains75/10, safe second patrol75/0, Cancel unchanged, explicit confirmed refit100/20");
#endif
    const auto valid=audit::saves;const auto lastState=Bytes(s);GlEnd(s);Destroy(s);
    audit::saves[kSaveSlot][16]^=1;const auto corrupt=audit::saves;const auto writes=audit::writes;
    s=static_cast<State*>(Create(&host));audit::Check(s&&s->loadBlocked&&PrepareExit(s)==0,"corrupt read guarded");
    Click(s);Pause(s);audit::Check(audit::saves==corrupt&&audit::writes==writes,"corrupt original never silently replaced");
    audit::Check(GlStart(s)==1,"recovery board GL");audit::Capture(Render,s,prefix+"-read-recovery");
    audit::saves=valid;Resume(s);audit::Check(!s->loadBlocked&&Bytes(s)==lastState,"Resume loads externally recovered original");
    GlEnd(s);Destroy(s);audit::read_error_slots.insert(kSaveSlot);s=static_cast<State*>(Create(&host));
    audit::Check(s->loadBlocked&&PrepareExit(s)==0,"I/O read error protected");audit::read_error_slots.clear();Resume(s);
    audit::Check(!s->loadBlocked&&Bytes(s)==lastState,"I/O read recovery exact");Destroy(s);
    auto unavailable=host;unavailable.read_save=nullptr;s=static_cast<State*>(Create(&unavailable));
    audit::Check(s&&s->loadBlocked&&PrepareExit(s)==0,"unavailable persistence cannot start unprotected progress");Destroy(s);
    std::printf("PASS %s actual module fault/retry/current-phase recreation, held-input safety, exact stereo and guarded exit\n",prefix.c_str());
}
