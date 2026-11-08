#include "red_ledger_module.cpp"
#include "audit_host.h"
static void Tap(ModuleState* state,WorkTarget target){
    const TargetBounds* bounds=nullptr;for(const auto& b:kWorkTargets)if(b.target==target)bounds=&b;
    audit::Check(bounds!=nullptr,"target exists");
    auto input=audit::Pointer(bounds->center[0],bounds->center[1],bounds->center[2]);
    Update(state,&input);audit::Check(state->hovered_target==target,"actual ray focuses intended target");
    input.primary_down=1u;Update(state,&input);input.primary_down=0u;Update(state,&input);
}
int main(int argc,char** argv){
    audit::Check(argc==2,"capture directory argument");audit::output=argv[1];audit::GL context;auto host=audit::Host();
    auto* state=static_cast<ModuleState*>(Create(&host));audit::Check(state!=nullptr,"Red Ledger create");
    audit::Check(OnGlContextCreated(state)==1,"Red Ledger GL create");
    audit::Capture(RenderEye,state,"red-ledger-bar",1.2f,-.12f);
    const int cash=state->simulation.cash_cents();
    Tap(state,WorkTarget::CupStack);Tap(state,WorkTarget::Tap);Tap(state,WorkTarget::Patron);
    const int payment=state->simulation.pending_payment_cents();audit::Check(payment>0,"service produced pending payment");
    audit::reject_writes=true;Tap(state,WorkTarget::Payment);
    audit::Check(state->simulation.cash_cents()==cash+payment,"payment collected in live game");
    const unsigned failed_attempts=audit::writes;audit::reject_writes=false;
    auto idle=audit::Input();for(int i=0;i<80;++i)Update(state,&idle);Pause(state);
    audit::Check(audit::writes==failed_attempts,"observed no idle/pause retry");
    std::printf("OBSERVED_RED_LEDGER_FAILED_WRITE attempts=%u successful=%u cash_live=%d expected_saved=%d pause_retry=0\n",audit::writes,audit::successful_writes,state->simulation.cash_cents(),cash);
    audit::Capture(RenderEye,state,"red-ledger-after-save-failure",1.2f,-.12f);
    ReleaseGlContext(state);Destroy(state);
    state=static_cast<ModuleState*>(Create(&host));audit::Check(state!=nullptr,"recreate after failure");
    audit::Check(state->simulation.cash_cents()==cash,"failed last action lost on recreation");
    audit::Check(state->simulation.pending_payment_cents()==payment,"prior committed mid-transaction state retained");
    std::printf("OBSERVED_RED_LEDGER_RECREATE cash=%d pending_payment=%d\n",state->simulation.cash_cents(),state->simulation.pending_payment_cents());
    audit::Check(OnGlContextCreated(state)==1,"recreated Red Ledger GL");Tap(state,WorkTarget::Payment);
    audit::Check(state->simulation.cash_cents()==cash+payment,"successful retry collects exactly once");
    Tap(state,WorkTarget::DoorStock);audit::Check(state->market.room()==trade::Room::Stockroom,"actual room transition");
    const auto good=audit::saves;const unsigned before_render_writes=audit::writes;
    const auto stockFrame=audit::Capture(RenderEye,state,"red-ledger-stockroom",1.2f,-.12f);
    audit::Check(audit::saves==good&&audit::writes==before_render_writes,"both eyes do not save or mutate persisted state");
    ReleaseGlContext(state);context.Recreate();audit::Check(OnGlContextCreated(state)==1,"Red Ledger context recreation");
    audit::Check(stockFrame==audit::Capture(RenderEye,state,"red-ledger-stockroom-recreated",1.2f,-.12f),"context recreation reproduces both images");
    ReleaseGlContext(state);Destroy(state);
    audit::saves["state-v4.bin"][12]^=1u;
    state=static_cast<ModuleState*>(Create(&host));audit::Check(state!=nullptr,"create with bit-flipped composite save");
    audit::Check(state->simulation.cash_cents()!=cash+payment,"observed base segment bitflip accepted by actual module");
    std::printf("OBSERVED_RED_LEDGER_COMPOSITE_BIT_FLIP accepted=1 original_cash=%d restored_cash=%d composite_bytes=%zu\n",cash+payment,state->simulation.cash_cents(),audit::saves["state-v4.bin"].size());
    Destroy(state);audit::MissingAssets();std::puts("PASS audit reproductions and actual Red Ledger GLES lifecycle (defects observed, not fixed)");
}
