#include "red_ledger_module.cpp"
#include "featured_module_test_host.h"
using Checkpoint=reverie::redledger::save::Checkpoint;
static void Tap(ModuleState* state,WorkTarget target){
    const TargetBounds* bounds=nullptr;for(const auto& b:kWorkTargets)if(b.target==target)bounds=&b;
    audit::Check(bounds!=nullptr,"target exists");
    auto input=audit::Pointer(bounds->center[0],bounds->center[1],bounds->center[2]);
    Update(state,&input);audit::Check(state->hovered_target==target,"actual ray focuses intended target");
    input.primary_down=1u;Update(state,&input);input.primary_down=0u;Update(state,&input);
}
static void Click(ModuleState* state){auto in=audit::Input();Update(state,&in);in.primary_down=1;Update(state,&in);in.primary_down=0;Update(state,&in);}
static void Service(ModuleState* state){Tap(state,WorkTarget::CupStack);Tap(state,WorkTarget::Tap);Tap(state,WorkTarget::Patron);}
int main(int argc,char** argv){
    std::setvbuf(stdout,nullptr,_IONBF,0);
    audit::Check(argc==2,"capture directory argument");audit::output=argv[1];audit::GL context;auto host=audit::Host();
    auto* state=static_cast<ModuleState*>(Create(&host));audit::Check(state&&!state->load_blocked,"fresh game");
    audit::Check(OnGlContextCreated(state)==1,"Red Ledger real GL");
    audit::Capture(RenderEye,state,"red-ledger-after-bar",1.2f,-.12f);
    const int cash=state->simulation.cash_cents();Service(state);
    const int payment=state->simulation.pending_payment_cents();const auto prior=audit::saves;
    audit::reject_writes=true;Tap(state,WorkTarget::Payment);
    audit::Check(state->simulation.cash_cents()==cash+payment&&state->save_failed&&state->save_dirty,"failed payment is explicit and retained");
    audit::Check(audit::saves==prior&&PrepareExit(state)==0,"failed save cannot destroy live instance or overwrite prior bytes");
    audit::Capture(RenderEye,state,"red-ledger-after-save-warning",1.2f,-.12f);
    const auto attempts=audit::writes;audit::reject_writes=false;
    auto input=audit::Input();for(int i=0;i<40;++i)Update(state,&input);
    audit::Check(!state->save_dirty&&!state->save_failed&&audit::writes==attempts+1,"one bounded recovery retry commits current state");
    audit::Check(PrepareExit(state)==1,"clean ordinary exit permitted");
    ReleaseGlContext(state);Destroy(state);state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state&&!state->load_blocked&&state->simulation.cash_cents()==cash+payment
        &&state->simulation.pending_payment_cents()==0,"recreate preserves recovered collection exactly once");
    std::printf("PASS_RED_IDLE_RECOVERY cash=%d pending=%d bytes=%zu\n",state->simulation.cash_cents(),state->simulation.pending_payment_cents(),audit::saves[kSaveSlot].size());
    Service(state);audit::reject_writes=true;Tap(state,WorkTarget::Payment);Pause(state);
    audit::Check(state->save_dirty&&PrepareExit(state)==0,"pause failure retains live changes");
    audit::reject_writes=false;Pause(state);audit::Check(!state->save_dirty&&PrepareExit(state)==1,"Pause retries recovered storage");
    const int latestCash=state->simulation.cash_cents();const auto valid=audit::saves;
    Destroy(state);
    // Corrupt the former unchecked base currency, not only metadata.
    audit::saves[kSaveSlot][Checkpoint::kHeaderSize+12]^=1u;const auto damaged=audit::saves;
    const auto beforeReadFailureWrites=audit::writes;
    state=static_cast<ModuleState*>(Create(&host));audit::Check(state&&state->load_blocked,"corrupt economic checkpoint blocked");
    audit::Check(OnGlContextCreated(state)==1,"recovery screen GL");
    Click(state);for(int i=0;i<40;++i)Update(state,&input);Pause(state);
    audit::Check(PrepareExit(state)==0&&audit::saves==damaged&&audit::writes==beforeReadFailureWrites,"corrupt save never silently overwritten");
    audit::Capture(RenderEye,state,"red-ledger-save-recovery",1.2f,-.12f);
    audit::saves=valid;Click(state);
    audit::Check(!state->load_blocked&&state->simulation.cash_cents()==latestCash,"actual Retry reloads externally recovered original");
    const auto frame=audit::Capture(RenderEye,state,"red-ledger-recovered",1.2f,-.12f);
    ReleaseGlContext(state);context.Recreate();audit::Check(OnGlContextCreated(state)==1,"context recreation");
    audit::Check(frame==audit::Capture(RenderEye,state,"red-ledger-recovered-context",1.2f,-.12f),"both eye frames exact after GL recreation");
    ReleaseGlContext(state);Destroy(state);
    audit::read_error_slots.insert(kSaveSlot);state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state->load_blocked&&PrepareExit(state)==0,"I/O read fault protected");
    audit::read_error_slots.clear();Resume(state);
    audit::Check(!state->load_blocked&&state->simulation.cash_cents()==latestCash,"Resume retries failed read without default overwrite");Destroy(state);
    const auto& checked=valid.at(kSaveSlot);
    audit::saves[kSaveSlot]=std::vector<uint8_t>(checked.begin()+Checkpoint::kHeaderSize,
        checked.begin()+Checkpoint::kHeaderSize+Checkpoint::kPayloadSize);
    state=static_cast<ModuleState*>(Create(&host));
    audit::Check(!state->load_blocked&&state->save_dirty&&state->simulation.cash_cents()==latestCash,"supported old composite imported");
    Pause(state);audit::Check(audit::saves[kSaveSlot].size()==Checkpoint::kBytes&&!state->save_dirty,"legacy upgrade committed only after validation");
    Service(state);audit::ambiguous_writes=true;const int pending=state->simulation.pending_payment_cents();
    const int beforeCash=state->simulation.cash_cents();Tap(state,WorkTarget::Payment);
    const auto unacknowledged=audit::saves;
    audit::Check(state->simulation.cash_cents()==beforeCash+pending&&state->save_dirty&&PrepareExit(state)==0,
        "visible but unacknowledged write retains current live state and exit guard");
    audit::ambiguous_writes=false;Pause(state);
    audit::Check(!state->save_dirty&&audit::saves==unacknowledged&&state->simulation.pending_payment_cents()==0,
        "durability retry writes identical economic state without duplicate payment");Destroy(state);
    auto unavailable=host;unavailable.read_save=nullptr;
    state=static_cast<ModuleState*>(Create(&unavailable));
    audit::Check(state==nullptr,"missing save service cannot create unprotected progress");Destroy(state);
    std::puts("PASS actual Red Ledger payment failure/retry, Pause, prepare_exit, corruption/read protection, legacy import and real stereo recovery");
}
