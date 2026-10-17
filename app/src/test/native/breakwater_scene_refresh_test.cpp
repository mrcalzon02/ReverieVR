#include "breakwater_scene_refresh.h"
#include <cassert>
#include <iostream>
using namespace reverie::breakwater;
int main(){
    using scene::RefreshNeeded;
    for(Phase idle:{Phase::Briefing,Phase::Shop,Phase::Victory,Phase::Defeat}){
        for(int frame=0;frame<5000;++frame)
            assert(!RefreshNeeded(idle,idle,false,false,false));
        assert(RefreshNeeded(idle,idle,true,false,false));  // new card
        assert(RefreshNeeded(idle,idle,false,true,false));  // purchase
        assert(RefreshNeeded(idle,idle,false,false,true));  // pointer lost/regained
        assert(RefreshNeeded(idle,Phase::Combat,false,false,false));
        assert(RefreshNeeded(Phase::Combat,idle,false,false,false));
    }
    for(int frame=0;frame<5000;++frame)
        assert(RefreshNeeded(Phase::Combat,Phase::Combat,false,false,false));
    assert(RefreshNeeded(Phase::Briefing,Phase::Shop,false,false,false));
    assert(RefreshNeeded(Phase::Victory,Phase::Briefing,false,false,false));
    std::cout<<"Breakwater: idle menu redraw suppression and live combat redraw passed\n";
}
