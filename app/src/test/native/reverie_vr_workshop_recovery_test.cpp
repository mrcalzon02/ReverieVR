#include "../../main/jni/native/reverie_vr_workshop.h"
#include <cassert>
#include <iostream>
using namespace reverie::authoring;
namespace {
struct Memory {
    std::array<std::string,3> bytes{};
    std::array<bool,3> present{},unreadable{};
    bool failWrite=false,applyThenFail=false;
    unsigned writes=0u;
    static int Read(void* context,size_t slot,std::string& output) {
        auto& m=*static_cast<Memory*>(context);
        if(m.unreadable[slot])return -1;
        if(!m.present[slot])return 0;
        output=m.bytes[slot];return 1;
    }
    static bool Write(void* context,size_t slot,const std::string& data) {
        auto& m=*static_cast<Memory*>(context);++m.writes;
        if(m.failWrite&&!m.applyThenFail)return false;
        m.bytes[slot]=data;m.present[slot]=true;return !m.failWrite;
    }
    VrStore store(){return {this,Read,Write};}
};
VrRay Aim(double x,double y) {
    const double length=std::sqrt(x*x+y*y+2.62*2.62);
    return {0,0,0,x/length,y/length,-2.62/length,true};
}
}
int main() {
    const Scene seed{"white_room",{}};
    const Scene authored{"white_room",{Piece{"saved-box","box","-",0,-1,-3,0,.5}}};
    Memory m;m.present[0]=m.present[2]=true;m.bytes[0]="CORRUPT ORIGINAL";
    m.bytes[2]=EncodeVrScene(authored);const auto original=m.bytes[0];
    VrWorkshop w;assert(w.Open(seed,m.store()));
    // A present valid project wins over an earlier unused fallback slot.
    assert(w.slot==2u&&!w.recovering&&w.scene.pieces.size()==1u&&m.writes==0u);
    assert(w.blocked_slots[0]&&m.bytes[0]==original&&!w.recovery_notice.empty());
    assert(w.NextProject(m.store())&&w.slot==1u&&m.bytes[0]==original);
    w.editing=true;assert(w.Add("box"));
    // Applied bytes plus an error must retain dirty state and refuse switching.
    m.failWrite=m.applyThenFail=true;
    assert(!w.NextProject(m.store())&&w.slot==1u&&w.dirty);
    assert(DecodeVrBundle(m.bytes[1]).scene.pieces.size()==1u);
    assert(!w.LoadSlot(2,m.store())&&w.slot==1u&&w.dirty);
    m.failWrite=m.applyThenFail=false;assert(w.NextProject(m.store())&&w.slot==2u&&!w.dirty);
    assert(m.bytes[0]==original);

    // All bad slots retain a usable read-only recovery screen, not create failure.
    Memory bad;bad.present={true,true,true};bad.bytes={"bad-zero","bad-one","bad-two"};
    const auto invalid=bad.bytes;VrWorkshop recovery;
    assert(recovery.Open(seed,bad.store())&&recovery.recovering&&!recovery.editing);
    assert(!recovery.Save(bad.store())&&bad.writes==0u&&bad.bytes==invalid);
    assert(recovery.Step(false,true,{},bad.store())==VrEvent::Rejected);
    assert(recovery.recovering&&!recovery.editing&&bad.bytes==invalid);
    recovery.Step(false,false,{},bad.store());bad.bytes[1]=EncodeVrScene(authored);
    assert(recovery.Step(true,false,{},bad.store())==VrEvent::Project);
    assert(!recovery.recovering&&recovery.slot==1u&&recovery.scene.pieces.size()==1u);
    assert(bad.bytes[0]==invalid[0]&&bad.bytes[2]==invalid[2]&&bad.writes==0u);
    assert(!recovery.NextProject(bad.store())&&recovery.slot==1u);

    // I/O failure is never an empty project and never authorizes overwriting.
    Memory io;io.unreadable[0]=true;io.present[0]=true;io.bytes[0]=original;
    VrWorkshop fallback;assert(fallback.Open(seed,io.store())&&fallback.slot==1u);
    assert(fallback.blocked_slots[0]&&io.bytes[0]==original&&io.writes==0u);
    for(int row=0;row<10;++row) {
        const double y=kVrMenuTop-(row+.5)*kVrMenuRowHeight;
        assert(VrMenuHit(Aim((kVrMenuLeft+kVrMenuRight)*.5,y))==row);
        assert(VrMenuHit(Aim(kVrMenuLeft-.05,y))==-1);
        assert(VrMenuHit(Aim(kVrMenuRight+.05,y))==-1);
    }
    std::cout<<"Workshop recovery: valid-first fallback, all-bad retry, original-byte preservation, ambiguous writes and menu bounds PASS\n";
}
