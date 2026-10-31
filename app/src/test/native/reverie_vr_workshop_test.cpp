#include "../../main/jni/native/reverie_vr_workshop.h"
#include <cassert>
#include <iostream>
#include <array>
using namespace reverie::authoring;
struct Memory {
    std::array<std::string,3> data;
    std::array<bool,3> present{};
    bool fail=false;
    static int Read(void* u,size_t slot,std::string& out) {
        Memory& m=*static_cast<Memory*>(u);
        if(m.fail)return -1;
        if(!m.present[slot])return 0;
        out=m.data[slot];return 1;
    }
    static bool Write(void* u,size_t slot,const std::string& data) {
        Memory& m=*static_cast<Memory*>(u);
        if(m.fail)return false;
        m.data[slot]=data;m.present[slot]=true;return true;
    }
    VrStore store(){return {this,Read,Write};}
};
static VrRay toward(double x,double y,double z){
    const double d=std::sqrt(x*x+y*y+z*z);
    return {0,0,0,x/d,y/d,z/d,true};
}
int main(){
    Memory m;
    Scene original{"white_room",{}};
    VrWorkshop w;
    assert(w.Open(original,m.store())&&w.scene.pieces.empty());
    assert(!w.editing);
    assert(w.Step(false,true,{},m.store())==VrEvent::Edit);
    w.Step(false,false,{},m.store());
    assert(VrMenuHit(toward(-2.,1.0,-2.62))==0);
    assert(w.Step(true,false,toward(-2.,1.0,-2.62),m.store())==VrEvent::Modified);
    assert(w.scene.pieces.size()==1&&w.dirty);
    assert(!w.Add("obj"));
    assert(w.Undo()&&w.scene.pieces.empty());
    assert(w.Add("box")&&w.Add("cylinder")&&w.scene.pieces.size()==2);
    auto before=w.scene;
    w.tool=VrTool::Move;
    assert(w.BeginDrag());
    assert(w.Drag(toward(1.,-1.5,-3.))&&w.scene.pieces[1].x==1.);
    w.EndDrag();
    assert(w.Undo()&&w.scene.pieces[1].x==before.pieces[1].x);
    assert(!ValidVrRay({0,0,0,0,0,0,true}));
    double rejected_x=0.,rejected_z=0.;
    assert(!PlaneHit(toward(0,1,-1),-1.5,rejected_x,rejected_z));
    assert(w.Save(m.store())&&!w.dirty);
    VrWorkshop restored;
    assert(restored.Open(original,m.store()));
    assert(restored.scene.pieces.size()==2);
    assert(restored.NextProject(m.store())&&restored.scene.pieces.empty()&&restored.slot==1);
    restored.editing=true;
    assert(restored.Add("box")&&restored.Save(m.store()));
    assert(restored.NextProject(m.store())&&restored.slot==2);
    assert(restored.NextProject(m.store())&&restored.slot==0&&restored.scene.pieces.size()==2);
    assert(restored.NextProject(m.store())&&restored.slot==1&&restored.scene.pieces.size()==1);
    m.fail=true;
    assert(restored.Add("box"));
    assert(!restored.NextProject(m.store())&&restored.slot==1&&restored.dirty);
    m.fail=false;
    assert(restored.Save(m.store()));
    m.data[2]="CORRUPT";m.present[2]=true;
    assert(!restored.NextProject(m.store())&&restored.slot==1);
    assert(restored.last_error=="CORRUPT PROJECT");
    assert(restored.scene.pieces.size()==2);
    assert(!ScenePlayable(Scene{"white_room",{Piece{"bad","obj","thing.obj",0,0,-2,0,1}}}));
    assert(ParseScene(EncodeVrScene(restored.scene)).pieces.size()==2);
    std::cout<<"VR workshop editing, save slots, undo, play recovery and corrupt data PASS\n";
}
