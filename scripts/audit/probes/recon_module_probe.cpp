#include "recon_projects_module.cpp"
#include "audit_host.h"
static void Primary(State* state){auto input=audit::Input();Update(state,&input);input.primary_down=1u;Update(state,&input);input.primary_down=0u;Update(state,&input);}
static void Next(State* state){auto input=audit::Input();Update(state,&input);input.secondary_down=1u;Update(state,&input);input.secondary_down=0u;Update(state,&input);}
int main(int argc,char** argv){
    audit::Check(argc==2,"capture dir");audit::output=argv[1];audit::GL context;auto host=audit::Host();
    auto* state=static_cast<State*>(Create(&host));audit::Check(state!=nullptr,"recon create");audit::Check(GlStart(state)==1,"recon GL");
    audit::Capture(Render,state,std::string(kId)+"-briefing");Primary(state);Primary(state);
#if RECON_PROJECT_KIND == 0
    audit::Check(state->simulation.phase()==sim::Phase::PhotoReady&&state->hasPhoto,"actual recon reaches evidence");
    Next(state);Next(state);audit::Check(state->choice==2,"vehicle-column assessment selected through secondary actions");
    audit::Capture(Render,state,"lantern-photo");Primary(state);
    audit::Check(state->simulation.credibility()==3&&state->simulation.operational_effect()==2,"graded report applied");
    audit::Capture(Render,state,"lantern-filed");Primary(state);
    audit::Check(state->simulation.phase()==sim::Phase::Briefing&&state->simulation.credibility()==0&&state->simulation.operational_effect()==0,"observed next sortie resets consequences");
    std::puts("OBSERVED_LANTERN_NEXT_SORTIE credibility=0 operational_effect=0 after +3/+2 report");
#else
    audit::Check(state->simulation.phase()==sim::Phase::Contact,"actual patrol reaches contact");
    Next(state);Next(state);audit::Check(state->choice==2,"engage selected through secondary actions");
    audit::Capture(Render,state,"forward-contact");Primary(state);Primary(state);Primary(state);
    audit::Check(state->simulation.phase()==sim::Phase::Debriefed,"actual debrief");
    audit::Check(state->debrief.vehicle_condition==75&&state->debrief.fuel_remaining==10,"actual engagement cost");
    audit::Capture(Render,state,"forward-debrief");Primary(state);
    audit::Check(state->simulation.vehicle_condition()==100&&state->simulation.fuel_remaining()==20,"observed next patrol resets resources");
    std::puts("OBSERVED_FORWARD_NEXT_PATROL condition=100 fuel=20 after condition=75 fuel=10 debrief");
#endif
    Pause(state);audit::Check(audit::writes==0&&audit::reads==0,"recon modules never consume save callbacks");
    const auto frame=audit::Capture(Render,state,std::string(kId)+"-before-context-loss");
    GlEnd(state);context.Recreate();audit::Check(GlStart(state)==1,"recon new context");
    audit::Check(frame==audit::Capture(Render,state,std::string(kId)+"-after-context-loss"),"recon logical/image state survives context recreation");
    GlEnd(state);Destroy(state);state=static_cast<State*>(Create(&host));
    audit::Check(state&&state->simulation.phase()==sim::Phase::Briefing,"relaunch starts at briefing");
    std::printf("OBSERVED_RECON_LIFECYCLE module=%s read_save_calls=%u write_save_calls=%u\n",kId,audit::reads,audit::writes);
    Destroy(state);std::puts("PASS actual recon module control/render/lifecycle audit (prototype scope gaps remain)");
}
