#include "sitka_fishing_session.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::sitka;
struct Store {
    bool present=false, fail_read=false, fail_write=false;
    int writes=0;
    std::string data;
    static int Read(void* context,std::string& text) {
        auto& s=*static_cast<Store*>(context);
        if(s.fail_read)return -1;
        if(!s.present)return 0;
        text=s.data;return 1;
    }
    static bool Write(void* context,const std::string& text) {
        auto& s=*static_cast<Store*>(context);
        if(s.fail_write)return false;
        s.present=true;s.data=text;++s.writes;return true;
    }
    FishingStore Port(){return {this,Read,Write};}
};
int main(){
    Store s;
    FishingSession f;
    assert(f.Load(s.Port())==FishingLoad::Fresh);
    assert(f.dirty&&f.Flush(s.Port())&&s.writes==1);
    s.writes=0; // following assertions count gameplay writes

    assert(f.Update(.016f,true,false,false,4.f).event==FishingEvent::CastRejected);
    assert(!f.dirty && !f.Update(.016f,true,false,true,4.f).journal_changed);
    f.Update(.016f,false,false,false,4.f); // release to rearm
    assert(f.Update(.016f,true,false,true,NAN).event==FishingEvent::CastRejected);
    f.Update(.016f,false,false,false,4.f);
    assert(f.Update(.016f,true,false,true,2.f).event==FishingEvent::Cast);
    assert(f.dirty && f.Flush(s.Port()) && s.writes==1);
    // Holding primary throughout the bite cannot auto-hook the fish.
    for(int i=0;i<120 && f.fishing.phase!=FishingPhase::Bite;++i)f.Update(.1f,true,false,true,2.f);
    assert(f.fishing.phase==FishingPhase::Bite);
    f.Update(.1f,true,false,true,2.f);
    assert(f.fishing.phase==FishingPhase::Bite);
    f.Update(.1f,false,false,true,2.f);
    assert(f.Update(.1f,true,false,true,2.f).event==FishingEvent::ReelStarted);
    for(int i=0;i<1200 && f.fishing.phase!=FishingPhase::Landed;++i){
        f.Update(.1f,i%8<4,false,true,2.f);
    }
    assert(f.fishing.phase==FishingPhase::Landed);
    assert(f.fishing.journal.total==1 && f.dirty);
    s.fail_write=true;
    assert(!f.Flush(s.Port()) && f.dirty && f.write_failed);
    s.fail_write=false;
    assert(f.Flush(s.Port()) && s.writes==2 && !f.write_failed);
    // Save/load is precise and never restores an active reel.
    FishingSession restored;
    assert(restored.Load(s.Port())==FishingLoad::Restored);
    assert(restored.fishing.phase==FishingPhase::Ready);
    assert(restored.fishing.journal.total==1);
    assert(!restored.UpgradeTackle());
    assert(restored.Discover(4u,BeachFind::Rock));
    assert(restored.dirty&&restored.Flush(s.Port()));
    FishingSession replay;
    assert(replay.Load(s.Port())==FishingLoad::Restored);
    assert(replay.fishing.journal.beach_sites==(uint64_t(1)<<4u));
    assert(replay.fishing.journal.discoveries==FindMask(BeachFind::Rock));
    assert(!replay.Discover(4u,BeachFind::Rock));
    // A valid load into an existing session must discard an unsaved cast,
    // not carry its hooked fish, timer or reel into the restored journal.
    FishingSession reused;
    assert(reused.Load(s.Port())==FishingLoad::Restored);
    const Journal checkpoint=reused.fishing.journal;
    assert(reused.Update(.016f,true,false,true,2.f).event==FishingEvent::Cast);
    assert(reused.fishing.phase==FishingPhase::Waiting);
    assert(reused.fishing.journal.seed!=checkpoint.seed);
    // Corrupt source must leave that live session untouched. A subsequent
    // successful reload has to clear it and recover the exact saved journal.
    const std::string valid_save=s.data;
    s.data="corrupt";
    assert(reused.Load(s.Port())==FishingLoad::Invalid);
    assert(reused.fishing.phase==FishingPhase::Waiting);
    s.data=valid_save;
    assert(reused.Load(s.Port())==FishingLoad::Restored);
    assert(reused.fishing.phase==FishingPhase::Ready);
    assert(reused.fishing.time==0.f && reused.fishing.reel==0.f &&
           reused.fishing.tension==0.f && reused.fishing.depth==0.f);
    assert(reused.fishing.journal.seed==checkpoint.seed);
    assert(reused.fishing.journal.total==checkpoint.total);
    assert(reused.fishing.journal.beach_sites==checkpoint.beach_sites);
    assert(!reused.dirty && !reused.write_failed);
    assert(reused.Update(.016f,true,false,true,2.f).event==FishingEvent::Cast);

    // A press while a landed catch is displayed only acknowledges it.
    f.Update(.1f,false,false,false,0.f);
    assert(f.Update(.1f,true,false,false,0.f).event==FishingEvent::Acknowledged);
    assert(f.fishing.phase==FishingPhase::Ready);
    f.Update(.1f,false,false,false,0.f);
    f.Update(.1f,true,false,true,3.f);
    f.Update(.1f,false,true,false,0.f);
    assert(f.fishing.phase==FishingPhase::Ready);
    // A corrupt save and IO faults cannot silently reset a paid-for journal.
    s.data="corrupt";
    FishingSession bad;
    assert(bad.Load(s.Port())==FishingLoad::Invalid);
    s.fail_read=true;
    assert(bad.Load(s.Port())==FishingLoad::IoError);
    assert(bad.Load({})==FishingLoad::Unavailable);
    std::cout<<"Sitka host fishing session action/save contract PASS\n";
}
