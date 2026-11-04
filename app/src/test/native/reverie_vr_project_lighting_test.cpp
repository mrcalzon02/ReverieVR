#include "../../main/jni/native/reverie_vr_workshop.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace reverie::authoring;
struct Memory {
    std::array<std::string,3> slots{};
    std::array<bool,3> exists{};
    bool fail=false;
    static int Read(void* ctx,size_t id,std::string& value){
        Memory& m=*static_cast<Memory*>(ctx);
        if(m.fail)return -1;
        if(!m.exists.at(id))return 0;
        value=m.slots.at(id);return 1;
    }
    static bool Write(void* ctx,size_t id,const std::string& value){
        Memory& m=*static_cast<Memory*>(ctx);
        if(m.fail)return false;
        m.slots.at(id)=value;m.exists.at(id)=true;return true;
    }
    VrStore store(){return {this,Read,Write};}
};
static bool bad(const std::string& text){
    try{(void)DecodeVrBundle(text);return false;}
    catch(const std::exception&){return true;}
}
static bool near(double value,double target){
    return std::abs(value-target)<.000001;
}
int main(){
    const Scene seed{"white_room",{
        Piece{"lamp-base","box","-",.75,-1.5,-3.,0.,.5},
        Piece{"box-a","box","-",-1.,-1.5,-2.,0.,.5}
    }};
    Memory memory;auto save=memory.store();
    VrWorkshop workshop;
    assert(workshop.Open(seed,save));
    assert(!workshop.light.enabled);
    assert(!workshop.AttachLight()); // editing disabled
    workshop.editing=true;workshop.selected=0;
    assert(workshop.AttachLight());
    assert(workshop.light.enabled&&workshop.light.anchor_id=="lamp-base");
    assert(workshop.TuneLight(0,true)&&near(workshop.light.power,1.25));
    assert(workshop.TuneLight(1,false)&&near(workshop.light.radius,4.5));
    assert(workshop.TuneLight(2,true)&&near(workshop.light.ambient,.40));
    assert(workshop.Undo()&&near(workshop.light.ambient,.35));
    float x=0,y=0,z=0;
    assert(workshop.LightPosition(0.,x,y,z));
    assert(near(x,.75)&&near(y,-.75)&&near(z,-3.));
    assert(workshop.SetMotion(VrMotionKind::Bob));
    workshop.editing=false;
    assert(workshop.LightPosition(1.5707963267948966,x,y,z));
    assert(y>-.31f&&y<-.21f); // linked light follows authored bobbing object
    workshop.editing=true;
    assert(workshop.Save(save));
    assert(memory.slots[0].compare(0,12,"RVPROJECT\t3\n")==0);
    auto bundle=DecodeVrBundle(memory.slots[0]);
    assert(bundle.light.enabled&&bundle.light.anchor_id=="lamp-base");
    assert(near(bundle.light.power,1.25));
    assert(near(bundle.light.radius,4.5));
    VrWorkshop opened;
    assert(opened.Open(seed,save)&&opened.light.enabled);
    opened.editing=true;opened.selected=0;
    assert(opened.Remove());
    assert(!opened.light.enabled);
    assert(opened.Undo()&&opened.light.enabled);
    assert(opened.ClearLight()&&!opened.light.enabled);
    assert(opened.Undo()&&opened.light.enabled);
    // One previous project format round-trips without an authored light.
    const auto v1=EncodeVrBundle(EncodeVrScene(seed),EncodeMotions(seed,{}));
    assert(DecodeVrBundle(v1).light.enabled==false);
    memory.slots[1]=v1;memory.exists[1]=true;
    assert(opened.NextProject(save)&&opened.slot==1&&!opened.light.enabled);
    assert(opened.Save(save));
    assert(memory.slots[1].compare(0,12,"RVPROJECT\t3\n")==0);
    // Beckenburg LightModel.diffusionFade and attenuation numeric parity.
    assert(near(VrLightFade(2.,5.,.5),1.));
    assert(near(VrLightFade(4.,5.,.5),.4));
    assert(near(VrLightFade(5.,5.,.5),0.));
    const double attenuation=1.25*.4*(.45+.55*.4)/(1.+.13*16.);
    assert(near(VrLightContribution(1.25,.4,16.),attenuation));
    assert(near(VrLightContribution(2.,0.,25.),0.));
    assert(!ValidLight(seed,VrLight{true,"missing",1.,5.,.65,.35}));
    assert(!ValidLight(seed,VrLight{true,"lamp-base",3.2,5.,.65,.35}));
    assert(!ValidLight(seed,VrLight{true,"lamp-base",1.,5.,.65,2.}));
    assert(bad(EncodeVrBundle(EncodeVrScene(seed),EncodeMotions(seed,{}),
        "RVLIGHT\t1\nlight\tmissing\t1\t5\t0.65\t0.35\n")));
    assert(bad(EncodeVrBundle(EncodeVrScene(seed),EncodeMotions(seed,{}),
        "RVLIGHT\t1\nlight\tlamp-base\t1\t5\t0.65\t0.35\nlight\tbox-a\t1\t5\t0.65\t0.35\n")));
    assert(bad(EncodeVrBundle(EncodeVrScene(seed),EncodeMotions(seed,{}),
        "RVLIGHT\t1\nlight\tlamp-base\tNaN\t5\t0.65\t0.35\n")));
    assert(bad(memory.slots[0]+"x"));
    // Failed save cannot silently advance project selection or discard light.
    assert(opened.NextProject(save)&&opened.slot==2);
    opened.editing=true;opened.selected=0;
    // slot 2 is blank, so first seed a selectable primitive.
    assert(opened.Add("box")&&opened.AttachLight());
    memory.fail=true;
    assert(!opened.NextProject(save)&&opened.slot==2&&opened.light.enabled);
    memory.fail=false;
    assert(opened.Save(save)&&!opened.dirty);
    std::cout<<"PASS VR light anchor, attenuation parity, motion-follow, undo, V1->V2, atomic save failure\n";
}
