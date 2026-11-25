// Replay a fresh regional campaign entirely through the hosted module's inputs.
#include "omega_front_module.cpp"
#include "audit_host.h"
#include <sstream>

namespace {
void Tap(State *s,float x,float y){
    auto in=audit::Pointer(x,y,kBoardZ);
    Update(s,&in);
    in.primary_down=1;Update(s,&in);
    in.primary_down=0;Update(s,&in);
}
void Sector(State *s,int id){
    Tap(s,((id%4)-1.5f)*.395f,kCenterY+(1.5f-(id/4))*.395f);
}
void Cell(State *s,int x,int y){
    Tap(s,(x+.5f)*kCell-kHalf,kCenterY+kHalf-(y+.5f)*kCell);
}
void Palette(State *s,int row){
    Tap(s,-.88f,1.065f);
    audit::Check(s->menu.screen==game::MenuScreen::FieldOrders ||
                 s->menu.screen==game::MenuScreen::TacticalOrders,"orders palette");
    Tap(s,0.f,game::Menu::ButtonY(row,true));
}
void Field(State *s,game::FieldAction action,int id){
    if(s->session.action!=action)Palette(s,static_cast<int>(action));
    Sector(s,id);
    audit::Check(!s->session.rejected,"field order");
}
void March(State *s,int id){
    Field(s,game::FieldAction::Deploy,id);
    audit::Check(s->session.frontier.location()==id,"arrived at selected sector");
}
void RecordOrders(State *s,std::istream &orders){
    std::string line;
    while(std::getline(orders,line) && (line.empty() || line[0]=='#')){}
    std::istringstream cells(line);
    auto &control=s->session.tactical;
    for(int id=0;id<4;++id){
        std::string command;
        audit::Check(static_cast<bool>(cells>>command),"recorded controller turn");
        const auto *actor=control.battle.unit(id);
        if(!actor->alive())continue;
        if(command=="-" && !control.battle.queued(id))continue;
        if(control.selected!=id)Cell(s,actor->x,actor->y);
        audit::Check(control.selected==id,"select unit through tactical board");
        if(control.battle.queued(id)){
            Palette(s,5);
            audit::Check(control.battle.queued(id)==0,"clear only this unit's queue");
        }
        if(command=="-")continue;
        const auto mode=command=="S"?game::CommandMode::Resupply:
            command[0]=='A'?game::CommandMode::Attack:game::CommandMode::Move;
        if(control.mode!=mode)Palette(s,static_cast<int>(mode));
        if(mode==game::CommandMode::Attack){
            const auto *target=control.battle.unit(std::stoi(command.substr(1)));
            audit::Check(target && target->alive(),"recorded target alive");
            Cell(s,target->x,target->y);
        }else if(mode==game::CommandMode::Resupply)Cell(s,actor->x,actor->y);
        else{
            int x=-1,y=-1;
            audit::Check(std::sscanf(command.c_str(),"%d,%d",&x,&y)==2,"recorded board tile");
            if(x!=actor->x || y!=actor->y)Cell(s,x,y);
        }
    }
    Tap(s,0.f,1.065f);
}
State *Restore(State *s,ReverieNativeHostV1 &host){
    audit::Check(PrepareExit(s)==1,"active campaign saved");
    const auto checksum=s->session.Checksum();
    GlEnd(s);Destroy(s);
    s=static_cast<State*>(Create(&host));
    audit::Check(s && s->has_campaign && s->session.Checksum()==checksum &&
                 GlStart(s)==1,"campaign checkpoint restores exactly");
    Tap(s,0.f,.42f);Tap(s,0.f,.42f);
    audit::Check(s->menu.screen==game::MenuScreen::Playing,"continue existing campaign");
    return s;
}
void Fight(State *&s,std::istream &orders,ReverieNativeHostV1 &host,int turns){
    auto op=s->session.frontier.operation();
    Field(s,game::FieldAction::Deploy,s->session.frontier.location());
    audit::Check(s->session.view==game::FrontMode::Tactical,"authored operation started");
    for(int turn=0;turn<turns;++turn){
        audit::Check(s->session.tactical.battle.result()==game::Result::Active,"live campaign turn");
        RecordOrders(s,orders);
        audit::Check(s->session.tactical.battle.ticks()==static_cast<unsigned>(turn+1),"exactly one turn");
        if(op==1 && turn==19)s=Restore(s,host);
    }
    audit::Check(s->session.tactical.battle.result()==game::Result::Defended,"actual regional victory");
    if(op==1)audit::Capture(Render,s,"omega-white-orchard",1.50f);
    // The visible top-center action becomes REPORT when combat ends.
    // It must settle through the same production Select path as the board.
    if(op==0)audit::Capture(Render,s,"omega-terminal-report",1.50f);
    Tap(s,0.f,1.065f);
    audit::Check(s->session.frontier.operation()==op+1,"top report action applies mission outcome exactly once");
    std::printf("HEARTHFALL mission=%d turns=%d guards=%d day=%d\n",
                op+1,turns,s->session.frontier.guards(),s->session.frontier.day());
}
}
int main(int argc,char **argv){
    audit::Check(argc==2,"capture directory argument");
    audit::output=argv[1];audit::GL gl;auto host=audit::Host();
    std::ifstream orders("scripts/audit/probes/omega_hearthfall_replay.txt");
    audit::Check(orders.good(),"recorded inputs available");
    auto *s=static_cast<State*>(Create(&host));
    audit::Check(s && !s->has_campaign && GlStart(s)==1,"start with empty campaign storage");
    Tap(s,0.f,.42f);Tap(s,0.f,.42f);Tap(s,0.f,1.065f);
    using A=game::FieldAction;
    for(int n=0;n<4;++n)Field(s,A::Forage,6);
    March(s,10);Fight(s,orders,host,4);
    March(s,6);March(s,5);Fight(s,orders,host,63);
    March(s,6);March(s,10);Fight(s,orders,host,29);
    March(s,9);Fight(s,orders,host,11);
    March(s,5);March(s,6);
    Field(s,A::Scout,7);Field(s,A::Parley,7);Field(s,A::Accord,7);
    audit::Check(s->session.frontier.operation()==5 && (s->session.frontier.accords()&(1<<4)),
                 "fifth operation resolved by Meridian accord");
    March(s,5);March(s,4);Fight(s,orders,host,27);
    audit::Check(s->menu.screen==game::MenuScreen::RegionalReport,"campaign opens regional report");
    const auto expected=audit::Capture(Render,s,"omega-hearthfall-report",1.55f);
    const auto checksum=s->session.Checksum();
    Tap(s,0.f,.42f);
    audit::Check(s->menu.screen==game::MenuScreen::Playing &&
                 checksum==s->session.Checksum(),"closing report preserves continuing frontier");
    s=Restore(s,host);
    Tap(s,.88f,1.065f);Tap(s,0.f,game::Menu::ButtonY(1,true));
    audit::Check(s->menu.screen==game::MenuScreen::RegionalReport &&
                 expected==audit::Capture(Render,s,"omega-reopened-report",1.55f),
                 "report reopened from pause matches both eyes");
    std::printf("HEARTHFALL complete day=%d guards=%d trust=%d pump=%d rations=%d pact=%d\n",
        s->session.frontier.day(),s->session.frontier.guards(),s->session.frontier.trust(),
        s->session.frontier.pump_health(),s->session.frontier.rations(),s->session.frontier.obligation_status(4));
    GlEnd(s);Destroy(s);
}
