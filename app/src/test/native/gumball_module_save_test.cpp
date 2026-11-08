#include "gumball_armada_module.cpp"
#include "featured_module_test_host.h"
using Checkpoint=reverie::gumball::CampaignSave;
static void Tap(Module* state,float x,float y){auto in=audit::Pointer(-4,0,-2.94f);Update(state,&in);in=audit::Pointer(x,y,-2.94f);Update(state,&in);in.primary_down=1;Update(state,&in);in.primary_down=0;Update(state,&in);}
static void Back(Module* state){auto in=audit::Input();Update(state,&in);in.secondary_down=1;Update(state,&in);in.secondary_down=0;Update(state,&in);}
static Checkpoint::Bytes StateBytes(const Module* m){Checkpoint::Bytes bytes{};audit::Check(Checkpoint::Encode(m->sim,m->ship,&bytes),"encode full actual module state");return bytes;}
int main(int argc,char** argv){
    std::setvbuf(stdout,nullptr,_IONBF,0);
    audit::Check(argc==2,"capture directory argument");audit::output=argv[1];audit::GL context;auto host=audit::Host();
    auto* state=static_cast<Module*>(Create(&host));audit::Check(state&&!state->loadBlocked&&GlStart(state)==1,"fresh Gumball actual GL");
    Tap(state,1.55f,-.94f);Tap(state,0,.88f);
    audit::Check(state->sim.cash()==55&&state->sim.support().booked()==SupportKind::CapitaineEclair,"real paid support purchase");
    Back(state);Tap(state,-1.55f,-.25f);auto input=audit::Input();for(int i=0;i<12;++i)Update(state,&input);
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
    auto zero=audit::Input();zero.delta_seconds=0;zero.primary_down=1;Update(restored,&zero);
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
    const auto attempts=audit::writes;input=audit::Input();for(int i=0;i<40;++i)Update(state,&input);
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
