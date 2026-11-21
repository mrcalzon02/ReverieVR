#include "recon_projects_module.cpp"
#include "featured_module_test_host.h"
#include <limits>
using Checkpoint=reverie::recon::Checkpoint;
// The historical button-only host had no pointer ABI tail. A full
// POINTER_NONE prefix is shell ownership, not equivalent legacy input.
static ReverieNativeInputV1 LegacyInput(){
    auto input=audit::Input();
    input.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;
    return input;
}
static void Click(State* s){auto in=LegacyInput();Update(s,&in);in.primary_down=1;Update(s,&in);in.primary_down=0;Update(s,&in);}
static void Cycle(State* s,int count=1){for(int i=0;i<count;++i){auto in=LegacyInput();Update(s,&in);in.secondary_down=1;Update(s,&in);in.secondary_down=0;Update(s,&in);}}
static Checkpoint::Bytes Bytes(State* s){Checkpoint::Bytes bytes{};audit::Check(Checkpoint::Encode(Snapshot(s),&bytes),"actual current-phase checkpoint encodes");return bytes;}
static State* Recreate(State* s,const ReverieNativeHostV1* host,const std::string& name){
    Pause(s);audit::Check(PrepareExit(s)==1,"durable exit permitted");const auto before=Bytes(s);
#if RECON_PROJECT_KIND == 0
    audit::Check(PrepareFrame(s)==1,"pre-eye photographic upload");
#endif
    const auto frame=audit::Capture(Render,s,name+"-before");
    GlEnd(s);Destroy(s);s=static_cast<State*>(Create(host));
    audit::Check(s&&!s->loadBlocked&&Bytes(s)==before,"exact actual module restored");
    auto held=LegacyInput();held.primary_down=1;Update(s,&held);
    audit::Check(Bytes(s)==before,"held input cannot advance a restored phase");
    held.primary_down=0;Update(s,&held);
    audit::Check(GlStart(s)==1,"recreated board actual GL");
#if RECON_PROJECT_KIND == 0
    audit::Check(PrepareFrame(s)==1,"recreated photographic source uploaded before eyes");
#endif
    audit::Check(frame==audit::Capture(Render,s,name+"-after"),
                 "both eye frames identical after phase recreation");
    return s;
}
static void ShellFocusOwnership(const ReverieNativeHostV1 &host){
    audit::saves.clear();
    auto *guest=static_cast<State*>(Create(&host));
    audit::Check(guest&&!guest->loadBlocked,"real recon guest starts before shell ownership");
    const auto initial=Bytes(guest);
    const unsigned writes=audit::writes;
    auto shell=audit::Input(); // complete native ABI with POINTER_NONE
    shell.primary_down=shell.secondary_down=1u;shell.move_x=1.f;
    for(int frame=0;frame<100;++frame)Update(guest,&shell);
    audit::Check(!guest->inputReady&&Bytes(guest)==initial&&audit::writes==writes,
        "100 Quick Menu frames cannot choose recon mission, spend patrol fuel or write save");
    auto ray=audit::Pointer(0.f,0.f,-2.f);
    ray.primary_down=ray.secondary_down=1u;ray.move_x=1.f;
    for(int frame=0;frame<12;++frame)Update(guest,&ray);
    audit::Check(!guest->inputReady&&Bytes(guest)==initial&&audit::writes==writes,
        "held Select, Back and pad after pointer recovery never trigger a mission");
    ray.primary_down=ray.secondary_down=0u;ray.move_x=.4f;
    Update(guest,&ray);
    audit::Check(!guest->inputReady&&Bytes(guest)==initial,
        "partially held pad is not a safe neutral release");
    ray.move_x=0.f;
    Update(guest,&ray);
    audit::Check(guest->inputReady&&Bytes(guest)==initial&&audit::writes==writes,
        "neutral release frame itself does not start a mission");
    ray.primary_down=1u;Update(guest,&ray);
#if RECON_PROJECT_KIND == 0
    audit::Check(guest->simulation.phase()==sim::Phase::InFlight,
        "fresh tracked Select explicitly starts Lantern recon");
#else
    audit::Check(guest->simulation.phase()==sim::Phase::Travelling,
        "fresh tracked Select explicitly starts Forward patrol");
#endif
    audit::Check(audit::writes>writes,"deliberate mission is durably checked");
    const auto started=Bytes(guest);
    const auto written=audit::writes;
    auto invalid=ray;
    invalid.pointer_direction[0]=std::numeric_limits<float>::quiet_NaN();
    for(int frame=0;frame<10;++frame)Update(guest,&invalid);
    audit::Check(!guest->inputReady&&Bytes(guest)==started&&audit::writes==written,
        "invalid full native ray cannot advance a sortie or modify checkpoint");
    ray.primary_down=1u;Update(guest,&ray);
    audit::Check(!guest->inputReady&&Bytes(guest)==started,
        "tracked Select held across invalid ray cannot act");
    ray.primary_down=0u;ray.move_y=std::numeric_limits<float>::quiet_NaN();
    Update(guest,&ray);
    audit::Check(!guest->inputReady&&Bytes(guest)==started,
        "NaN pad cannot falsely rearm the controller");
    ray.move_y=0.f;Update(guest,&ray);
    audit::Check(guest->inputReady&&Bytes(guest)==started,
        "neutral release after invalid pointer consumes no mission step");
    Pause(guest);Resume(guest);
    ray.primary_down=1u;Update(guest,&ray);
    audit::Check(!guest->inputReady&&Bytes(guest)==started,
        "lifecycle Resume refuses still-held Select");
    ray.primary_down=0u;Update(guest,&ray);
    audit::Check(guest->inputReady&&Bytes(guest)==started,
        "lifecycle first neutral frame is consumed without a new sortie");
    ray.primary_down=1u;Update(guest,&ray);
    audit::Check(Bytes(guest)!=started&&guest->simulation.phase()!=sim::Phase::Briefing,
        "new intentional post-pause action advances the existing native mission");
    Destroy(guest);
    audit::saves.clear();
    std::puts("PASS_RECON_SHELL_FOCUS full pointer NONE and invalid rays block actions; neutral rearm and legacy controls");
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
    audit::Check(s->photoRenderer.current.Valid()&&
                 s->photoRenderer.rgb[0].size()==512u*512u*3u,
                 "photo-ready Lantern has an actual 512px photographed master");
    audit::Check(PrepareFrame(s)==1,"upload real registered photograph");
    const auto frame=audit::Capture(Render,s,"lantern-bridge-loupe",scale);
    // Sample all 64 right-eye magnifier cell centers. They must match
    // the corresponding actual high-resolution master pixels rather than
    // obsolete bright/dark 16x16 silhouette bits.
    const auto eye=audit::Eye(1,scale);
    for(int y=0;y<8;++y)for(int x=0;x<8;++x){
        const float wx=kLoupeX+(x-3.5f)*2*kPhotoPixel;
        const float wy=kPhotoY+(3.5f-y)*2*kPhotoPixel;
        const int px=static_cast<int>((1+(wx+eye.view[12])*eye.projection[0]/-kPhotoZ)*audit::GL::width*.5f);
        const int py=static_cast<int>((1+wy*eye.projection[5]/-kPhotoZ)*audit::GL::height*.5f);
        uint8_t rgb[4]{};
        glReadPixels(px,py,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgb);
        const int sx=(s->cropX+x)*32+16,sy=(s->cropY+y)*32+16;
        const auto& photo=s->photoRenderer.rgb[0];
        const int expected=photo[(static_cast<size_t>(sy)*512+sx)*3u];
        audit::Check(std::abs(int(rgb[0])-expected)<45,
                     "2x stereo loupe samples the actual 512px source pixel");
    }
    audit::Check(glGetError()==GL_NO_ERROR && Bytes(s)==checkpoint && audit::writes==writes,
        "inspection changes no checkpoint, report selection, or storage");
    Pause(s);GlEnd(s);gl.Recreate();audit::Check(GlStart(s)==1,"loupe GL restored");Resume(s);
    audit::Check(frame==audit::Capture(Render,s,"lantern-bridge-loupe-restored",scale),
        "both magnified eye frames survive pause and lost GL context");
    auto neutral=LegacyInput();Update(s,&neutral);
    auto old=aim;old.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;Update(s,&old);
    audit::Check(s->cropX==4 && s->cropY==6,"old input prefix preserves inspection");
    std::puts("PASS Lantern true-photo loupe: all 64 right-eye framebuffer cells sampled registered 512px master; no report or save on pan");
}
static void CompareExposures(State *s,const sim::Photograph &first) {
    audit::Check(s->hasPrior && s->prior.pixels==first.pixels &&
                 s->prior.sector==s->photo.sector,"the archived film is from the first river sortie");
    auto aim=audit::Pointer(0,kPhotoY-.17f,kPhotoZ);
    const auto writes=audit::writes;
    Update(s,&aim);
    audit::Check(s->cropX==4 && s->cropY==6 && s->viewDirty && audit::writes==writes,
                 "aim moves both loupes to the same region without a per-frame write");
    constexpr float scale=1.45f;
    audit::Check(s->photoRenderer.has_previous&&
                 s->photoRenderer.previous.Valid(),
                 "prior exposure loaded into independently registered photo texture");
    audit::Check(PrepareFrame(s)==1,
                 "both current and earlier negatives staged before both eyes");
    audit::Capture(Render,s,"lantern-river-film-comparison",scale);
    int differences=0;
    for(int side=0;side<2;++side){
        const auto eye=audit::Eye(side,scale);
        audit::Check(Render(s,&eye)==1,"comparison eye rendered");
        for (int y=0;y<8;++y)for (int x=0;x<8;++x){
            const int bit=1<<(s->cropX+x);
            const int row=s->cropY+y;
            const bool previous=(first.pixels[row]&bit)!=0, current=(s->photo.pixels[row]&bit)!=0;
            if(previous!=current)++differences;
            for(int plate=0;plate<2;++plate){
                const float wx=(plate==0?-kLoupeX:kLoupeX)+(x-3.5f)*2*kPhotoPixel;
                const float wy=kPhotoY+(3.5f-y)*2*kPhotoPixel;
                const int px=static_cast<int>((1+(wx+eye.view[12])*eye.projection[0]/-kPhotoZ)*audit::GL::width*.5f);
                const int py=static_cast<int>((1+wy*eye.projection[5]/-kPhotoZ)*audit::GL::height*.5f);
                uint8_t rgba[4]{};
                glReadPixels(px,py,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
                const int sx=(s->cropX+x)*32+16,sy=(s->cropY+y)*32+16;
                const auto& image=s->photoRenderer.rgb[plate==0?1:0];
                const int expected=image[(static_cast<size_t>(sy)*512+sx)*3u];
                audit::Check(std::abs(int(rgba[0])-expected)<45,
                             "rendered paired loupe cell matches its own registered 512px image");
            }
        }
    }
    audit::Check(differences>0,"compared region contains a visible change");
    audit::Check(VisiblePhotoChangeCount(s)==differences/2,
                 "visible change count matches both eyes and original photo bits");
    // Count the different region independently of the current window, and
    // demonstrate that a pin cannot alter the factual photo delta.
    const int originallyVisible=VisiblePhotoChangeCount(s);
    const auto initialPhoto=s->photo.pixels,oldPhoto=s->prior.pixels;
    const auto checkpoint=Bytes(s);
    s->photoPins[8]=uint16_t(1u<<8);
    audit::Check(VisiblePhotoChangeCount(s)==originallyVisible&&
                 s->photo.pixels==initialPhoto&&s->prior.pixels==oldPhoto,
                 "manual annotations cannot inflate the change count or modify either film");
    s->photoPins.fill(0);
    audit::Check(Bytes(s)==checkpoint,
                 "temporary marker inspection does not modify the stored evidence");
    s->cropX=0;s->cropY=0;
    int independent=0;
    for(int y=0;y<8;++y)for(int x=0;x<8;++x)
        independent+=((s->photo.pixels[y]>>x)&1u)!=((s->prior.pixels[y]>>x)&1u);
    audit::Check(VisiblePhotoChangeCount(s)==independent,
                 "window delta correctly follows other legitimate loupe crops");
    s->cropX=4;s->cropY=6;
    audit::Check(VisiblePhotoChangeCount(s)==originallyVisible,
                 "original paired film window recovers its exact comparison");
    std::printf("PASS_LANTERN_PAIR samples=256 changed_cells_per_eye=%d crop=%d,%d\n",
                differences/2,s->cropX,s->cropY);
}
#endif
#if RECON_PROJECT_KIND == 0
static void TestPhotoPinBudget(){
    State fixture{};
    audit::Check(fixture.simulation.AssignRecon(sim::Sector::RiverCrossing),
                 "fixture assigns an actual Lantern sector");
    audit::Check(fixture.simulation.CompleteRecon(),"fixture receives film");
    RefreshEvidence(&fixture);
    audit::Check(VisiblePhotoChangeCount(&fixture)<0,
                 "first-exposure film never shows imaginary prior-photo differences");
    fixture.cropX=9;
    audit::Check(VisiblePhotoChangeCount(&fixture)<0,
                 "invalid crop cannot manufacture photo comparisons");
    fixture.cropX=4;
    for(int y=0;y<3;++y)for(int x=0;x<4;++x){
        fixture.cropX=x;fixture.cropY=y;
        audit::Check(TogglePhotoPin(&fixture),
                     "photo-pin source code admits twelve distinct cells");
    }
    audit::Check(PhotoPinCount(&fixture)==kMaxPhotoPins,
                 "exactly 12 visible pins admitted");
    fixture.cropX=fixture.cropY=4;
    audit::Check(!TogglePhotoPin(&fixture)&&PhotoPinCount(&fixture)==12,
                 "thirteenth pin is refused without mutation");
    fixture.cropX=fixture.cropY=0;
    audit::Check(TogglePhotoPin(&fixture)&&PhotoPinCount(&fixture)==11,
                 "marked pixel can be unpinned when full");
    fixture.cropX=fixture.cropY=4;
    audit::Check(TogglePhotoPin(&fixture)&&PhotoPinCount(&fixture)==12,
                 "released pin capacity is reusable");
    std::puts("PASS Lantern live twelve-pin budget, overflow refusal, remove and reuse");
}
#endif
int main(int argc,char** argv){
    std::setvbuf(stdout,nullptr,_IONBF,0);audit::Check(argc==2,"capture directory argument");audit::output=argv[1];
    audit::GL context;auto host=audit::Host();
    ShellFocusOwnership(host); // both module kinds, real Save and Update
#if RECON_PROJECT_KIND == 0
    TestPhotoPinBudget();
#endif
    auto* s=static_cast<State*>(Create(&host));
    audit::Check(s&&!s->loadBlocked&&GlStart(s)==1,"actual recon GL");
    audit::Check(api.prepare_exit==PrepareExit&&api.prepare_frame==PrepareFrame,
                 "published guarded-exit and before-both-eyes GPU preparation");
    auto pad=LegacyInput();
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
    // Use the real native Minus-then-Select chord to pin the exact current
    // loupe pixel, write through the 128-byte host checkpoint and render
    // the hollow annotation on the real Mesa GLES2 source + zoomed photo.
    const auto beforePin=Bytes(s);
    audit::Check(PrepareFrame(s)==1,"pin baseline GPU texture prepared");
    const auto beforePinFrame=audit::Capture(Render,s,"lantern-before-pin");
    auto pin=audit::Pointer(0.f,kPhotoY,kPhotoZ);
    pin.secondary_down=1u;Update(s,&pin);
    pin.primary_down=1u;Update(s,&pin);
    audit::Check(PhotoPinCount(s)==1 && (s->photoPins[10]&(1u<<8))!=0u,
                 "real Minus+Select pins current source pixel");
    audit::Check(s->choice==3 && s->simulation.phase()==sim::Phase::PhotoReady,
                 "pin cannot file or change a report assessment");
    audit::Check(Bytes(s)!=beforePin && Bytes(s)[8]==4u &&
                 !s->saveDirty && !s->viewDirty,
                 "photo pin committed by the actual 128-byte V4 store");
    audit::Check(PrepareFrame(s)==1,"gold pin composited into derivative print");
    const auto pinnedFrame=audit::Capture(Render,s,"lantern-photo-pin");
    audit::Check(beforePinFrame!=pinnedFrame,
                 "real GLES2 film and loupe visibly show the pin");
    pin.primary_down=0u;Update(s,&pin);
    pin.secondary_down=0u;Update(s,&pin);
    audit::Check(s->choice==3,"Minus release after pin never cycles report");
    // First-time photographs intentionally reopen with a centered loupe
    // (unlike paired follow-up exposures, which save their crop window).
    // The pin is anchored in source-pixel coordinates and MUST survive.
    // Center the transient crop before the generic exact-frame Recreate
    // parity helper; compare real GL before/after without weakening it.
    const auto committedPin=Bytes(s);
    s->cropX=s->cropY=4;
    DrawBoard(s);
    audit::Check(Bytes(s)==committedPin,
                 "recentered first-film lens does not change saved photo pins");
    audit::Check(PrepareFrame(s)==1,"center crop draws same registered photo");
    const auto centeredPinFrame=audit::Capture(Render,s,"lantern-photo-pin-centered");
    audit::Check(centeredPinFrame!=beforePinFrame,
                 "source film continues showing the marked pixel");
    s=Recreate(s,&host,"lantern-photo-pin-reopened");
    audit::Check(s->cropX==4&&s->cropY==4&&Bytes(s)==committedPin,
                 "first-film reload preserves pins and centers transient crop");
    audit::Check(PhotoPinCount(s)==1 && (s->photoPins[10]&(1u<<8))!=0u,
                 "pin survives real Pause/Destroy/Load/GLES2 recreation");
    const auto photo=s->photo;const auto prior=audit::saves;
    audit::reject_writes=true;Click(s);
    audit::Check(s->simulation.phase()==sim::Phase::Filed&&s->simulation.credibility()==3
        &&s->simulation.operational_effect()==2&&s->saveDirty&&s->saveFailed,"real report retained after failed write");
    audit::Check(PrepareExit(s)==0&&audit::saves==prior,"report exit guarded, prior photo save retained");
    audit::Capture(Render,s,"lantern-save-warning");audit::reject_writes=false;
    auto input=LegacyInput();for(int i=0;i<40;++i)Update(s,&input);
    audit::Check(!s->saveDirty&&!s->saveFailed,"idle report retry recovered");
    s=Recreate(s,&host,"lantern-filed");
    audit::Check(s->hasPhoto&&s->photo.pixels==photo.pixels&&s->result.credibility_delta==3,
                 "filed evidence/result preserved");
    audit::Check(PhotoPinCount(s)==1,
                 "filed result retains its exact manually pinned photograph");
    Click(s);audit::Check(s->simulation.phase()==sim::Phase::Briefing&&s->simulation.credibility()==3
        &&s->simulation.operational_effect()==2&&PhotoPinCount(s)==0,
        "new sortie retains cumulative consequence but clears old photo pins");
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
    CompareExposures(s,photo);
    const auto priorCompare=audit::saves;
    audit::reject_writes=true;
    audit::Check(PrepareExit(s)==0 && audit::saves==priorCompare,"failed inspection checkpoint preserves prior file");
    audit::reject_writes=false;
    s=Recreate(s,&host,"lantern-river-registered-pair");
    audit::Check(s->cropX==4 && s->cropY==6 && s->hasPrior && !s->viewDirty,
                 "the inspected region and both plates reopen");
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
        auto input=LegacyInput();input.move_x=direction;
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
