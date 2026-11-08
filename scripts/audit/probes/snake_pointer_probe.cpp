// Single bounded audit probe: unchanged production Snake module, real Update calls.
// Graphics functions are linked out; this is an input/simulation test, not a render test.
#include "classic_games_module.cpp"
#include <iostream>
#include <iomanip>

struct Case {const char* name;float worldY;float worldZ;int expected;bool visible;};
static const char* DirectionName(int d){return d==0?"UP":d==1?"RIGHT":d==2?"DOWN":"LEFT";}
int main(){
    const Case cases[]={
        {"visible_up_background_center",1.02f,-2.97f,0,true},
        {"visible_down_background_center",-1.02f,-2.97f,2,true},
        {"accepted_up_region_control",1.50f,-2.91f,0,false},
        {"accepted_down_region_control",-1.50f,-2.91f,2,false}
    };
    int failures=0,visibleFailures=0,controlPasses=0;
    for(const auto& c:cases){
        auto*m=static_cast<Module*>(Create(nullptr));
        if(!m){std::cerr<<"Create failed\n";return 2;}
        // No game-state injection: each case uses the module's normal fresh Snake.
        const int initialHead=m->game.snake[0],initialHeading=m->game.heading;
        const bool initialMoving=m->game.moving;
        ReverieNativeInputV1 i={};i.struct_size=sizeof(i);i.delta_seconds=.02f;
        i.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
        const float length=std::sqrt(c.worldY*c.worldY+c.worldZ*c.worldZ);
        i.pointer_direction[0]=0.f;i.pointer_direction[1]=c.worldY/length;i.pointer_direction[2]=c.worldZ/length;
        // Explicit release -> press -> release; no inherited or held primary state.
        i.primary_down=0;Update(m,&i);const bool freshEdge=!m->down;
        i.primary_down=1;Update(m,&i);
        const int desiredAfterPress=m->game.desired;
        const bool movingAfterPress=m->game.moving;
        i.primary_down=0;Update(m,&i);const bool releasedAfterClick=!m->down;
        // Enough actual updates for exactly one default 0.24-second movement tick.
        i.delta_seconds=.1f;for(int tick=0;tick<3;++tick)Update(m,&i);
        const int finalHeading=m->game.heading,finalHead=m->game.snake[0];
        const int expectedHead=initialHead+(c.expected==0?-12:12);
        const bool matched=freshEdge&&releasedAfterClick&&!initialMoving&&movingAfterPress&&
            desiredAfterPress==c.expected&&finalHeading==c.expected&&finalHead==expectedHead;
        if(!matched){++failures;if(c.visible)++visibleFailures;}else if(!c.visible)++controlPasses;
        std::cout<<std::fixed<<std::setprecision(6)
            <<c.name<<" world=(0,"<<c.worldY<<','<<c.worldZ<<") hit_plane_y="<<(-2.91f/c.worldZ)*c.worldY
            <<" primary_sequence=0,1,0 fresh_edge="<<freshEdge<<" released_after_click="<<releasedAfterClick
            <<" initial_direction="<<DirectionName(initialHeading)<<" initial_head="<<initialHead
            <<" expected_direction="<<DirectionName(c.expected)<<" desired_after_press="<<DirectionName(desiredAfterPress)
            <<" moving_after_press="<<movingAfterPress<<" actual_heading_after_tick="<<DirectionName(finalHeading)
            <<" actual_head="<<finalHead<<" expected_head="<<expectedHead<<" result="<<(matched?"PASS":"FAIL_DIRECTION_TARGET")<<'\n';
        Destroy(m);
    }
    std::cout<<"SUMMARY visible_button_failures="<<visibleFailures<<"/2 actual_hit_region_controls_passed="<<controlPasses
             <<"/2 total_failures="<<failures<<"\n";
    return failures?1:0;
}
