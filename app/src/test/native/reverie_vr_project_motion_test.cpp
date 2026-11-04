#include "../../main/jni/native/reverie_vr_workshop.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace reverie::authoring;

struct Saved {
    std::array<std::string,3> data{};
    std::array<bool,3> present{};
    bool reject_reads=false,reject_writes=false;
    static int read(void* ctx,size_t slot,std::string& out){
        auto& s=*static_cast<Saved*>(ctx);
        if(s.reject_reads)return -1;
        if(!s.present.at(slot))return 0;
        out=s.data.at(slot);return 1;
    }
    static bool write(void* ctx,size_t slot,const std::string& data){
        auto& s=*static_cast<Saved*>(ctx);
        if(s.reject_writes)return false;
        s.data.at(slot)=data;s.present.at(slot)=true;return true;
    }
    VrStore service(){return {this,read,write};}
};
static bool invalid(const std::string& bytes){
    try{(void)DecodeVrBundle(bytes);return false;}
    catch(const std::exception&){return true;}
}
int main(){
    const Scene seed{"white_room",{
        Piece{"crate01","box","-",0,-1.5,-3.,0.,.4},
        Piece{"pillar02","cylinder","-",1.,-1.5,-3.,0.,.25}
    }};
    Saved s;
    auto store=s.service();
    VrWorkshop w;
    assert(w.Open(seed,store));
    assert(w.scene.pieces.size()==2&&w.motions.empty()&&!w.editing);
    w.editing=true;
    w.selected=0;
    assert(w.SetMotion(VrMotionKind::Spin));
    assert(w.motions.size()==1&&w.dirty);
    assert(w.SelectedMotion()!=nullptr);
    assert(w.SampleFor(w.scene.pieces[0],3.).spin==0.f); // edit is static
    w.editing=false;
    const auto spin=w.SampleFor(w.scene.pieces[0],1.);
    assert(spin.spin>.99f&&spin.spin<1.01f);
    w.editing=true;
    assert(w.SetMotion(VrMotionKind::Bob));
    assert(w.motions[0].kind==VrMotionKind::Bob);
    assert(w.TuneMotion(true,true));
    assert(w.TuneMotion(false,true));
    w.editing=false;
    auto bounce=w.SampleFor(w.scene.pieces[0],1.);
    assert(bounce.offset_y>0.f&&bounce.spin==0.f);
    w.editing=true;
    assert(w.SetMotion(VrMotionKind::Pulse));
    w.editing=false;
    assert(w.SampleFor(w.scene.pieces[0],1.).emissive>0.f);
    w.editing=true;
    assert(w.Undo()&&w.motions[0].kind==VrMotionKind::Bob);
    assert(w.Save(store)&&!w.dirty);
    assert(s.data[0].compare(0,12,"RVPROJECT\t2\n")==0);
    auto bundle=DecodeVrBundle(s.data[0]);
    assert(!bundle.legacy);
    assert(bundle.scene.pieces.size()==2&&bundle.motions.size()==1);
    assert(bundle.motions[0].kind==VrMotionKind::Bob);
    VrWorkshop restore;
    assert(restore.Open(seed,store)&&restore.motions.size()==1);
    assert(restore.motions[0].id=="crate01");
    restore.editing=true;restore.selected=0;
    assert(restore.Remove()&&restore.motions.empty());
    assert(restore.Undo()&&restore.motions.size()==1);
    assert(restore.Remove()&&restore.Save(store));
    auto clean=DecodeVrBundle(s.data[0]);
    assert(clean.scene.pieces.size()==1&&clean.motions.empty());
    // Legacy primitive scene is accepted intact and migrated on next save.
    s.data[1]=EncodeVrScene(seed);s.present[1]=true;
    assert(restore.NextProject(store)&&restore.slot==1);
    assert(restore.scene.pieces.size()==2&&restore.motions.empty());
    assert(restore.Save(store)&&!DecodeVrBundle(s.data[1]).legacy);
    // A failed atomic write retains both in-memory scene and animation edits.
    restore.selected=1;
    assert(restore.SetMotion(VrMotionKind::Spin));
    s.reject_writes=true;
    assert(!restore.NextProject(store)&&restore.slot==1&&restore.dirty);
    assert(restore.motions.size()==1);
    s.reject_writes=false;
    assert(restore.Save(store));
    // Corruption, duplicate ID, illegal values, and dangling asset bindings
    // must not partially replace the current project.
    s.present[2]=true;s.data[2]="RVPROJECT\t1\nscene\t999999\n";
    assert(!restore.NextProject(store)&&restore.slot==1);
    s.data[2]=EncodeVrBundle(EncodeVrScene(seed),
        "RVMOTION\t1\nmotion\tmissing\tSPIN\t1\t1\n");
    assert(!restore.NextProject(store)&&restore.slot==1);
    const std::string legal=EncodeVrBundle(EncodeVrScene(seed),
        EncodeMotions(seed,{VrMotion{"crate01",VrMotionKind::Bob,1.,.5}}));
    assert(!invalid(legal));
    assert(invalid(legal+"x"));
    assert(invalid("RVPROJECT\t1\nscene\t18446744073709551616\n"));
    assert(invalid(EncodeVrBundle(EncodeVrScene(seed),
        "RVMOTION\t1\nmotion\tcrate01\tFIRE\t1\t1\n")));
    assert(invalid(EncodeVrBundle(EncodeVrScene(seed),
        "RVMOTION\t1\nmotion\tcrate01\tSPIN\t1\t1\nmotion\tcrate01\tSPIN\t2\t1\n")));
    assert(!ValidateMotions(seed,{VrMotion{"crate01",VrMotionKind::Spin,0.,1.}}));
    assert(!ValidateMotions(seed,{VrMotion{"crate01",VrMotionKind::Spin,1.,1e5}}));
    assert(!ValidateMotions(seed,{VrMotion{"crate01",static_cast<VrMotionKind>(99),1.,1.}}));
    assert(invalid("RVPROJECT\t1\nscene\t0\nmotions\t0\n"));
    assert(SampleMotion({ "crate01",VrMotionKind::Spin,1.,.5},-1.).spin==0.f);
    std::cout<<"PASS: VR authorable spin/bob/pulse, atomic scene+motion storage, legacy upgrade, undo and rejection\n";
}
