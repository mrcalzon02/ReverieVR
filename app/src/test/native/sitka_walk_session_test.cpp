#include "sitka_walk_session.h"
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
using namespace reverie::sitka;
int main(){
    const std::string text=
      "SITKA-WALK-1\n"
      "P|survey_x|aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa|EPSG32608|reviewed\n"
      "O|480000.125|6300000.625\n"
      "N|start|0|11.5|0\n"
      "N|street|20|11.5|0\n"
      "N|harbor|20|11.5|-20\n"
      "E|lincoln|start|street|sidewalk|2.2\n"
      "E|shore|street|harbor|shore_path|1.5\n"
      "S|public_start|start\n"
      "F|bank|harbor|fish_a|1.4\n";
    auto g=WalkGraph::Parse(text,480000.125,6300000.625);
    WalkSession walk(g);
    const float right[3]={1.f,0.f,0.f};
    assert(walk.pose().x==0.f);
    assert(walk.pose().ground_y==11.5f);
    for(int i=0;i<100;++i)assert(walk.Advance(1.f,0.f,right,.05f));
    assert(walk.pose().x>7.f&&walk.pose().x<8.f);
    assert(std::abs(walk.pose().z)<.01f);
    assert(walk.accepted_steps()==100u&&walk.blocked_steps()==0u);
    // Large frame gap cannot teleport more than one small safe footstep.
    const float before=walk.pose().x;
    assert(walk.Advance(1.f,0.f,right,99.f));
    assert(walk.pose().x-before<.08f);
    // Synthetic terrain outside a mapped strip cannot be flown across.
    for(int i=0;i<100;++i)walk.Advance(0.f,1.f,right,.05f);
    assert(walk.blocked_steps()>0u);
    const auto current=g.Surface(walk.pose().x,walk.pose().z);
    assert(current.valid);
    assert(!walk.Advance(NAN,0.f,right,.05f));
    assert(!walk.Advance(1.f,0.f,nullptr,.05f));
    assert(!walk.Advance(1.f,0.f,right,-.1f));
    ReverieNativeEyeV1 eye{};
    eye.struct_size=sizeof(eye);eye.eye_index=0;
    for(int i=0;i<16;i+=5){eye.view[i]=1.f;eye.projection[i]=1.f;}
    ReverieNativeEyeV1 shifted{};
    assert(walk.ShiftEye(&eye,shifted));
    assert(shifted.eye_index==eye.eye_index);
    assert(std::abs(shifted.view[12]+walk.pose().x)<.0001f);
    assert(std::abs(shifted.view[13]+walk.pose().ground_y)<.0001f);
    assert(std::abs(shifted.view[14]+walk.pose().z)<.0001f);
    assert(eye.view[12]==0.f); // headset matrices must never be modified
    eye.eye_index=1;
    assert(walk.ShiftEye(&eye,shifted));
    assert(shifted.eye_index==1);
    eye.eye_index=7;
    assert(!walk.ShiftEye(&eye,shifted));
    // Visible crossings are NOT pedestrian connections unless survey
    // source records have the SAME endpoint node. Reachability alone
    // does not grant a shortcut across a different corridor.
    const std::string crossing =
      "SITKA-WALK-1\n"
      "P|survey_x|aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa|EPSG32608|reviewed\n"
      "O|480000.125|6300000.625\n"
      "N|start|0|11.5|0\n"
      "N|east|12|11.5|0\n"
      "N|north|12|11.5|12\n"
      "N|upper|6|11.5|12\n"
      "N|south|6|11.5|-5\n"
      "E|horizontal|start|east|sidewalk|0.4\n"
      "E|rise|east|north|sidewalk|0.4\n"
      "E|top|north|upper|sidewalk|0.4\n"
      "E|crosses_without_node|upper|south|sidewalk|0.4\n"
      "S|public_start|start\n";
    auto crossing_graph=WalkGraph::Parse(crossing,480000.125,6300000.625);
    WalkSession crossing_walk(crossing_graph);
    for(int i=0;i<83;++i)
        assert(crossing_walk.Advance(1.f,0.f,right,.05f));
    const float cross_x=crossing_walk.pose().x;
    for(int i=0;i<40;++i)
        crossing_walk.Advance(0.f,-1.f,right,.05f);
    assert(crossing_walk.blocked_steps()>0u);
    assert(crossing_walk.pose().z<.451f);
    assert(std::abs(crossing_walk.pose().x-cross_x)<.01f);
    // Walking onward along the original route remains permitted.
    assert(crossing_walk.Advance(1.f,0.f,right,.05f));

    // Conversely, shared source nodes permit an actual authored right turn.
    WalkSession junction_walk(g);
    for(int i=0;i<276;++i)
        assert(junction_walk.Advance(1.f,0.f,right,.05f));
    for(int i=0;i<70;++i)
        junction_walk.Advance(0.f,1.f,right,.05f);
    assert(junction_walk.pose().z< -3.f);
    assert(std::abs(junction_walk.pose().x-20.f)<.1f);
    return 0;
}
