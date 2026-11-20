#include "gumball_armada_module.cpp"
#include "featured_module_test_host.h"
#include <iterator>
using Checkpoint=reverie::gumball::CampaignSave;
static ReverieNativeInputV1 LegacyInput(){
    auto frame=audit::Input();
    // This older host intentionally exposes only buttons and pad axes.
    // Full-size POINTER_NONE frames belong to the shell/Quick Menu instead.
    frame.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;
    return frame;
}
static void Tap(Module* state,float x,float y){auto in=audit::Pointer(-4,0,-2.94f);Update(state,&in);in=audit::Pointer(x,y,-2.94f);Update(state,&in);in.primary_down=1;Update(state,&in);in.primary_down=0;Update(state,&in);}
static void Back(Module* state){auto in=LegacyInput();Update(state,&in);in.secondary_down=1;Update(state,&in);in.secondary_down=0;Update(state,&in);}
static Checkpoint::Bytes StateBytes(const Module* m){Checkpoint::Bytes bytes{};audit::Check(Checkpoint::Encode(m->sim,m->ship,&bytes),"encode full actual module state");return bytes;}
static void LaunchCrown(Module* state){
    auto idle=LegacyInput();
    Tap(state,0,-.25f);
    for(int i=0;i<12;++i)Update(state,&idle);
    Tap(state,0,-.25f);
    audit::Check(state->sim.phase()==Phase::Rescue,"arm and launch Crown Route through the actual chart");
}
static void RoyalRescueCheckpoint(ReverieNativeHostV1* host){
    std::ifstream old("app/src/test/native/fixtures/gumball-four-fortresses-v4.bin",std::ios::binary);
    audit::saves.clear();
    auto& slot=audit::saves["campaign.v1"];
    slot.assign(std::istreambuf_iterator<char>(old),std::istreambuf_iterator<char>());
    audit::Check(slot.size()==4096&&std::memcmp(slot.data(),"GUMFLY04",8)==0,"prior build's four-fortress save fixture");
    auto* state=static_cast<Module*>(Create(host));
    audit::Check(state&&!state->loadBlocked&&GlStart(state)==1,"reopen four-fortress v4 save in actual module");
    audit::Check(state->sim.generals_defeated()==4&&state->sim.kills()==1214
        &&state->sim.visited()==61&&!state->sim.king_rescued(),"old endgame migrated without inventing rescue");
    const auto cash=state->sim.cash();
    audit::Capture(Render,state,"gumball-crown-chart");
    LaunchCrown(state);
    auto input=LegacyInput();input.primary_down=1;
    for(int i=0;i<270;++i)Update(state,&input);
    audit::Check(state->sim.phase()==Phase::SignalLost&&!state->sim.king_rescued(),"holding off-target times out and stays on result");
    audit::Check(state->sim.cash()==cash&&state->sim.support().booked()==SupportKind::CapitaineEclair,
                 "loss preserves wallet and the combat escort booking");
    input.primary_down=0;Update(state,&input);input.primary_down=1;Update(state,&input);
    audit::Check(state->sim.phase()==Phase::Chart,"failure returns to chart for retry");
    LaunchCrown(state);
    bool recreated=false,blockedSave=false;
    for(int i=0;i<260&&state->sim.phase()==Phase::Rescue;++i){
        const auto target=state->sim.rescue_target();
        const unsigned leg=state->sim.rescue_leg();
        if(leg==0){
            // Untracked fallback uses real pad axes. The next legs use the
            // controller's world-space ray and its independent click.
            input=LegacyInput();
            const auto axes=reverie::gumball::FlightAxesToTarget(state->sim.player_x(),
                state->sim.player_y(),target,.05f,state->sim.flight_speed());
            input.move_x=axes.x;input.move_y=axes.y;
        }else input=audit::Pointer(target.x,target.y,-2.94f);
        input.primary_down=1;
        Update(state,&input);
        if(!recreated&&leg==1&&state->sim.rescue_link()>.25f){
            audit::Capture(Render,state,"gumball-trace-pod");
            Pause(state);const auto saved=StateBytes(state);
            GlEnd(state);Destroy(state);state=static_cast<Module*>(Create(host));
            audit::Check(GlStart(state)==1&&state->resumeFlight&&StateBytes(state)==saved,
                         "active rescue restores drifting pod, partial link and ship exactly");
            for(int frame=0;frame<8;++frame)Update(state,&input);
            audit::Check(state->resumeFlight&&StateBytes(state)==saved,"held primary cannot skip continuation");
            input.primary_down=0;Update(state,&input);input.primary_down=1;Update(state,&input);
            audit::Check(!state->resumeFlight&&StateBytes(state)==saved,"resume edge leaves partial link intact");
            recreated=true;
        }
        if(!blockedSave&&leg==2&&state->sim.rescue_link()>.5f){
            audit::Capture(Render,state,"gumball-convoy-tow");
            audit::reject_writes=true;blockedSave=true;
        }
    }
    audit::Check(recreated&&blockedSave&&state->sim.phase()==Phase::Homecoming&&state->sim.king_rescued(),
                 "three live controller links tow the King home");
    audit::Check(state->sim.cash()==cash&&PrepareExit(state)==0,"failed rescue write keeps the live ending and exit guard");
    audit::reject_writes=false;Pause(state);
    audit::Check(PrepareExit(state)==1&&slot[8]==5,"V5 ending durably retried");
    GlEnd(state);Destroy(state);state=static_cast<Module*>(Create(host));audit::Check(GlStart(state)==1,"ending GL");
    input=LegacyInput();input.primary_down=1;Update(state,&input);
    audit::Check(state->sim.phase()==Phase::Homecoming,"held restart input cannot skip ending");
    audit::Capture(Render,state,"gumball-king-home");
    input.primary_down=0;Update(state,&input);input.primary_down=1;Update(state,&input);Update(state,&input);
    audit::Check(state->sim.phase()==Phase::Credits,"fresh click opens credits and held click stays on credits");
    audit::Capture(Render,state,"gumball-campaign-credits");
    GlEnd(state);Destroy(state);state=static_cast<Module*>(Create(host));
    audit::Check(state->sim.phase()==Phase::Credits,"credits resume after process recreation");
    input.primary_down=0;Update(state,&input);input.primary_down=1;Update(state,&input);
    audit::Check(state->sim.phase()==Phase::Chart&&state->sim.king_rescued(),"credits return to saved free flight");
    audit::Check(GlStart(state)==1,"postgame chart GL");
    audit::Capture(Render,state,"gumball-free-flight");GlEnd(state);
    audit::Check(state->sim.cash()==cash&&state->sim.support().booked()==SupportKind::CapitaineEclair,
                 "homecoming pays no duplicate reward or escort debit");
    Destroy(state);
    std::puts("PASS Gumball v4 four-fortress save to Crown Route, timeout, retry, pad/ray rescue, interrupted tow, homecoming, credits and saved free flight");
}
 static void ShellFocusOwnership(ReverieNativeHostV1* host){
    audit::saves.clear();
    auto* m=static_cast<Module*>(Create(host));
    audit::Check(m&&!m->loadBlocked&&GlStart(m)==1,"focus regression real module creation");
    auto legacy=LegacyInput();
    // Real route menu first click arms the lane but never launches it.
    Tap(m,-1.55f,-.25f);
    audit::Check(m->sim.phase()==Phase::Chart&&m->routeLaunch.armed==0,
                 "actual Gumball chart route is armed");
    auto withdrawn=audit::Input(); // full-sized native pointer prefix
    withdrawn.primary_down=withdrawn.secondary_down=1u;
    withdrawn.move_x=1.f;
    const auto chartBefore=StateBytes(m);
    for(int i=0;i<35;++i)Update(m,&withdrawn);
    audit::Check(m->routeLaunch.armed==-1&&m->sim.phase()==Phase::Chart&&
                 StateBytes(m)==chartBefore,"Quick Menu cancels armed route and preserves chart");
    auto back=audit::Pointer(-1.55f,-.25f,-2.94f);
    back.primary_down=1u;back.move_x=1.f;
    Update(m,&back);
    audit::Check(m->routeLaunch.armed==-1&&StateBytes(m)==chartBefore,
                 "held select cannot rearm route on focus return");
    back.primary_down=0u;back.move_x=0.f;
    Update(m,&back); // consume neutral rearm frame
    Tap(m,-1.55f,-.25f);
    audit::Check(m->routeLaunch.armed==0&&m->sim.phase()==Phase::Chart,
                 "first intentional route click arms, never instant launch");
    for(int i=0;i<12;++i)Update(m,&legacy);
    Tap(m,-1.55f,-.25f);
    audit::Check(m->sim.phase()==Phase::Flight,"live chart click launches real sortie");
    const auto flight=StateBytes(m);
    const auto elapsed=m->sim.elapsed();
    const auto shots=m->sim.shots_fired();
    for(int i=0;i<140;++i)Update(m,&withdrawn);
    audit::Check(m->sim.phase()==Phase::Flight&&m->sim.elapsed()==elapsed
                 &&m->sim.shots_fired()==shots&&StateBytes(m)==flight,
                 "withdrawn pointer freezes real flight, firing, and saved game");
    back=audit::Pointer(.20f,-.10f,-2.94f);
    back.primary_down=back.secondary_down=1u;
    back.move_y=-1.f;
    for(int i=0;i<8;++i)Update(m,&back);
    audit::Check(StateBytes(m)==flight,"held inputs after pointer return cannot fire or steer");
    back.primary_down=back.secondary_down=0u;back.move_y=0.f;
    Update(m,&back);
    audit::Check(StateBytes(m)==flight,"neutral rearm itself is not gameplay tick");
    Update(m,&legacy);
    audit::Check(m->sim.elapsed()>elapsed&&m->sim.shots_fired()==shots,
                 "legacy button-only gamepad path resumes real flight without firing");
    Pause(m);
    const auto paused=StateBytes(m);
    back.primary_down=1u;
    for(int i=0;i<8;++i)Update(m,&back);
    audit::Check(StateBytes(m)==paused,"Pause requires released controller on Resume");
    back.primary_down=0u;Update(m,&back);
    audit::Check(StateBytes(m)==paused,"pause return neutral consumes no time");
    GlEnd(m);Destroy(m);
    std::puts("PASS Gumball actual module shell loss: armed route cancel, live flight freeze, neutral rearm, legacy gamepad and pause");
}

int main(int argc,char** argv){
    std::setvbuf(stdout,nullptr,_IONBF,0);
    audit::Check(argc==2,"capture directory argument");audit::output=argv[1];audit::GL context;auto host=audit::Host();
    RoyalRescueCheckpoint(&host);
    ShellFocusOwnership(&host);
    audit::saves.clear();
    auto* state=static_cast<Module*>(Create(&host));audit::Check(state&&!state->loadBlocked&&GlStart(state)==1,"fresh Gumball actual GL");
    Tap(state,1.55f,-.94f);Tap(state,0,.88f);
    audit::Check(state->sim.cash()==55&&state->sim.support().booked()==SupportKind::CapitaineEclair,"real paid support purchase");
    Back(state);Tap(state,-1.55f,-.25f);auto input=LegacyInput();for(int i=0;i<12;++i)Update(state,&input);
    const auto booking=audit::saves;audit::reject_writes=true;Tap(state,-1.55f,-.25f);
    audit::Check(state->sim.phase()==Phase::Flight&&state->saveDirty&&state->campaignSaveError,"launch checkpoint failure remains dirty");
    audit::Check(PrepareExit(state)==0&&audit::saves==booking,"paid booking protected while live flight cannot be saved");
    audit::reject_writes=false;for(int i=0;i<110;++i)Update(state,&input);
    audit::Check(state->sim.phase()==Phase::Flight&&state->sim.deployed_support()==SupportKind::CapitaineEclair
        &&state->sim.support_pulses()>0,"real live paid escort has acted");
    Pause(state);audit::Check(!state->saveDirty&&!state->campaignSaveError&&PrepareExit(state)==1,"exact flight flush before exit");
    audit::ambiguous_writes=true;Update(state,&input);Pause(state);
    const auto unacknowledged=audit::saves;const auto current=StateBytes(state);
    audit::Check(state->saveDirty&&PrepareExit(state)==0&&StateBytes(state)==current,
        "visible but unacknowledged checkpoint keeps exact live flight and exit guard");
    audit::ambiguous_writes=false;Pause(state);
    audit::Check(!state->saveDirty&&audit::saves==unacknowledged&&StateBytes(state)==current,
        "acknowledged retry writes identical latest full state");
    const auto before=StateBytes(state);const auto frame=audit::Capture(Render,state,"gumball-flight-before-recreate");
    const auto elapsed=state->sim.elapsed();const auto pulses=state->sim.support_pulses();const auto cash=state->sim.cash();
    auto* restored=static_cast<Module*>(Create(&host));
    audit::Check(restored&&!restored->loadBlocked&&restored->resumeFlight&&StateBytes(restored)==before,"full sortie/ship restored, waiting deliberate continuation");
    auto zero=LegacyInput();zero.delta_seconds=0;zero.primary_down=1;Update(restored,&zero);
    audit::Check(restored->resumeFlight,"held initial click cannot resume saved flight");
    zero.primary_down=0;Update(restored,&zero);
    for(int i=0;i<10;++i)Update(restored,&input);
    audit::Check(StateBytes(restored)==before,"saved-sortie continuation screen freezes simulation");
    zero.primary_down=1;Update(restored,&zero);zero.primary_down=0;Update(restored,&zero);
    audit::Check(!restored->resumeFlight&&StateBytes(restored)==before,"resume edge does not shoot or advance saved state");
    audit::Check(GlStart(restored)==1,"restored flight actual GL");
    audit::Check(frame==audit::Capture(Render,restored,"gumball-flight-after-recreate"),"full flight both-eye frames exactly restored");
    for(int i=0;i<120&&state->sim.phase()==Phase::Flight;++i){
        input.move_x=i%31<16?.6f:-.6f;input.primary_down=i%9!=0;
        Update(state,&input);Update(restored,&input);
        audit::Check(StateBytes(state)==StateBytes(restored),"actual module next-state determinism after recreate");
    }
    std::printf("PASS_GUMBALL_ACTIVE_RESTORE elapsed=%.3f support_pulses=%d cash=%u bytes=%zu\n",elapsed,pulses,cash,before.size());
    Pause(state);Pause(restored);GlEnd(state);GlEnd(restored);Destroy(state);Destroy(restored);
    // A real menu purchase followed by a real construction edit must retain
    // both pending changes; the atomic save cannot separate ship and flight.
    audit::saves.clear();state=static_cast<Module*>(Create(&host));audit::Check(GlStart(state)==1,"purchase case GL");
    Tap(state,-1.55f,-.94f);audit::reject_writes=true;Tap(state,-1.82f,.53f);
    audit::Check(state->sim.level(Upgrade::Blaster)==1&&state->sim.cash()==30&&state->saveDirty,"live failed purchase retained");
    audit::Capture(Render,state,"gumball-after-save-warning");
    const auto attempts=audit::writes;input=LegacyInput();for(int i=0;i<40;++i)Update(state,&input);
    audit::Check(audit::writes<=attempts+3&&state->saveDirty&&PrepareExit(state)==0,"bounded retry; failed exit preserves pending purchase");
    audit::reject_writes=false;Resume(state);audit::Check(!state->saveDirty&&!state->campaignSaveError,"Resume flushes recovered purchase");
    Back(state);Tap(state,0,-.94f);audit::Check(state->inYard,"actual shipyard navigation");
    const auto prior=audit::saves;const auto stock=state->ship.stock(Part::Frame);
    audit::reject_writes=true;Tap(state,-1.19f,.355f);
    audit::Check(state->ship.at(4,2,2)==Part::Frame&&state->ship.stock(Part::Frame)==stock-1,"actual construction debit");
    audit::Check(state->shipDirty&&state->shipSaveError&&PrepareExit(state)==0&&audit::saves==prior,"construction save failure preserves previous whole checkpoint");
    audit::Capture(Render,state,"gumball-ship-save-warning");
    const auto construction=state->ship.snapshot();audit::reject_writes=false;Pause(state);
    audit::Check(!state->saveDirty&&!state->shipDirty&&!state->shipSaveError,"Pause flushes construction together with campaign");
    GlEnd(state);Destroy(state);state=static_cast<Module*>(Create(&host));
    const auto rebuilt=state->ship.snapshot();
    audit::Check(std::memcmp(&construction,&rebuilt,sizeof(rebuilt))==0&&state->sim.level(Upgrade::Blaster)==1&&state->sim.cash()==30,"purchased upgrade and constructed ship both restored");
    const auto valid=audit::saves;Destroy(state);
    audit::saves["campaign.v1"][26]^=1u;const auto damaged=audit::saves;const auto writes=audit::writes;
    state=static_cast<Module*>(Create(&host));audit::Check(state->loadBlocked&&PrepareExit(state)==0,"corrupt full currency state blocked");
    for(int i=0;i<30;++i)Update(state,&input);Pause(state);
    audit::Check(audit::saves==damaged&&audit::writes==writes,"corrupt data not silently replaced");
    audit::Check(GlStart(state)==1,"Gumball recovery screen GL");audit::Capture(Render,state,"gumball-load-recovery");
    audit::saves=valid;Resume(state);audit::Check(!state->loadBlocked&&state->sim.cash()==30,"recovered bytes loaded on Resume");
    GlEnd(state);Destroy(state);
    audit::read_error_slots.insert("campaign.v1");state=static_cast<Module*>(Create(&host));
    audit::Check(state->loadBlocked&&PrepareExit(state)==0,"read I/O error held");audit::read_error_slots.clear();Resume(state);
    audit::Check(!state->loadBlocked&&state->sim.level(Upgrade::Blaster)==1,"transient read recovered without overwriting");Destroy(state);
    // Current checked legacy slots import as one candidate, then become one
    // full checkpoint. An unreadable legacy construction cannot be erased.
    audit::saves.clear();Simulation old;auto legacy=old.snapshot();legacy.cash=345;legacy.checksum=reverie::gumball::SnapshotChecksum(legacy);
    audit::saves["campaign.v1"]=std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&legacy),reinterpret_cast<uint8_t*>(&legacy)+sizeof(legacy));
    ShipAssembly oldShip;const auto legacyShip=oldShip.snapshot();
    audit::saves["shipyard.v2"]=std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(&legacyShip),reinterpret_cast<const uint8_t*>(&legacyShip)+sizeof(legacyShip));
    audit::read_error_slots.insert("shipyard.v2");state=static_cast<Module*>(Create(&host));audit::Check(state->loadBlocked&&PrepareExit(state)==0,"legacy yard I/O error cannot create default replacement");
    audit::read_error_slots.clear();Resume(state);audit::Check(!state->loadBlocked&&state->saveDirty&&state->sim.cash()==345,"checked legacy pair imported");
    Pause(state);audit::Check(audit::saves["campaign.v1"].size()==Checkpoint::kBytes&&!state->saveDirty,"legacy pair upgraded atomically");Destroy(state);
    auto unavailable=host;unavailable.read_save=nullptr;state=static_cast<Module*>(Create(&unavailable));
    audit::Check(state->loadBlocked&&PrepareExit(state)==0,"missing save service cannot start unprotected progress");Destroy(state);
    std::puts("PASS actual Gumball flight/save-failure/next-state/shipyard/read-corruption/legacy/prepare_exit and stereo checkpoint regression");
}
