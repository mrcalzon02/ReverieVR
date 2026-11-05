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
    return 0;
}
