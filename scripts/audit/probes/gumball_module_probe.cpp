#include "gumball_armada_module.cpp"
#include "audit_host.h"
static void Tap(Module* state,float x,float y){auto input=audit::Pointer(-4.f,0.f,-2.94f);Update(state,&input);input=audit::Pointer(x,y,-2.94f);Update(state,&input);input.primary_down=1u;Update(state,&input);input.primary_down=0u;Update(state,&input);}
static void Back(Module* state){auto input=audit::Input();Update(state,&input);input.secondary_down=1u;Update(state,&input);input.secondary_down=0u;Update(state,&input);}
int main(int argc,char** argv){
 audit::Check(argc==2,"capture dir");audit::output=argv[1];audit::GL context;auto host=audit::Host();
 auto* state=static_cast<Module*>(Create(&host));audit::Check(state&&GlStart(state)==1,"Gumball create GL");
 audit::Capture(Render,state,"gumball-chart");Tap(state,1.55f,-.94f);audit::Check(state->inSupport,"actual support navigation");
 Tap(state,0.f,.88f);audit::Check(state->sim.cash()==55&&state->sim.support().booked()==SupportKind::CapitaineEclair,"actual Eclair contract purchase");
 audit::Capture(Render,state,"gumball-support-hired");Back(state);audit::Check(!state->inSupport,"actual back from support");
 Tap(state,-1.55f,-.25f);audit::Check(state->sim.phase()==Phase::Chart,"first route click arms");
 auto input=audit::Input();for(int i=0;i<12;++i)Update(state,&input);
 Tap(state,-1.55f,-.25f);audit::Check(state->sim.phase()==Phase::Flight,"second deliberate route click launches");
 for(int i=0;i<10;++i)Update(state,&input);
 audit::Check(state->sim.deployed_support()==SupportKind::CapitaineEclair,"escort deployed in live flight");
 const auto before=state->sim.snapshot();const auto writes=audit::writes;
 const auto frame=audit::Capture(Render,state,"gumball-flight");
 const auto after=state->sim.snapshot();audit::Check(std::memcmp(&before,&after,sizeof(before))==0&&writes==audit::writes,"eye renders cannot change campaign or save");
 GlEnd(state);context.Recreate();audit::Check(GlStart(state)==1,"Gumball context recreation");
 audit::Check(frame==audit::Capture(Render,state,"gumball-flight-recreated"),"Gumball identical stereo after context recreation");
 const float elapsed=state->sim.elapsed();Pause(state);GlEnd(state);Destroy(state);state=static_cast<Module*>(Create(&host));
 audit::Check(state&&state->sim.phase()==Phase::Chart&&state->sim.deployed_support()==SupportKind::None&&state->sim.support().booked()==SupportKind::None&&state->sim.cash()==55,"observed paid escort and active sortie lost at recreation");
 std::printf("OBSERVED_GUMBALL_INTERRUPTION elapsed_before=%.2f phase_after=%d elapsed_after=%.2f cash=%u booked=%d deployed=%d\n",elapsed,static_cast<int>(state->sim.phase()),state->sim.elapsed(),state->sim.cash(),static_cast<int>(state->sim.support().booked()),static_cast<int>(state->sim.deployed_support()));Destroy(state);
 audit::saves.clear();state=static_cast<Module*>(Create(&host));audit::Check(state&&GlStart(state)==1,"new Gumball failure case");
 Tap(state,-1.55f,-.94f);audit::Check(state->inLoadout,"actual loadout navigation");
 audit::reject_writes=true;Tap(state,-1.82f,.53f);audit::Check(state->sim.level(static_cast<Upgrade>(0))==1&&state->campaignSaveError,"failed upgrade write shows warning");
 audit::Capture(Render,state,"gumball-save-failure");const auto attempts=audit::writes;const auto livecash=state->sim.cash();
 audit::reject_writes=false;for(int i=0;i<80;++i)Update(state,&input);Pause(state);audit::Check(audit::writes==attempts,"observed no retry after recovery idle and pause");
 GlEnd(state);Destroy(state);state=static_cast<Module*>(Create(&host));
 audit::Check(state->sim.level(static_cast<Upgrade>(0))==0&&state->sim.cash()==100,"observed lost unsaved upgrade");
 std::printf("OBSERVED_GUMBALL_WRITE_FAILURE live_upgrade=1 live_cash=%u restored_upgrade=0 restored_cash=%u no_retry_idle_pause=1\n",livecash,state->sim.cash());Destroy(state);audit::MissingAssets();
 std::puts("PASS actual Gumball module controls/render/failure audit (observed defects remain)");
}
