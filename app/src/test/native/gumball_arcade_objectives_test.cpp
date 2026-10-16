#include "gumball_arcade_objectives.h"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace reverie::gumball;
int main(){
    auto clear=MissionFor(0,2), courier=MissionFor(1,2), salvage=MissionFor(2,2);
    assert(clear.kind==MissionKind::Sweep&&clear.permitted_escapes==2);
    assert(courier.kind==MissionKind::Intercept&&courier.permitted_escapes==0&&courier.time_limit>0.f);
    assert(salvage.kind==MissionKind::Recovery&&salvage.required_salvage==3);
    assert(std::strcmp(MissionName(salvage.kind),"SALVAGE RECOVERY")==0);
    SalvageField field;
    assert(!field.drop(0,0,0)&&!field.drop(NAN,0,5));
    assert(field.drop(.2f,.5f,24)&&field.drop(-.9f,1.1f,12));
    assert(field.update(.025f,0.f,.5f)==24&&field.collected()==1);
    assert(field.update(NAN,0.f,0.f)==0);
    for(int n=0;n<450;++n)field.update(.05f,0.f,-.8f);
    assert(field.expired()==1&&field.empty());
    field.clear();
    assert(field.dropped()==0&&field.collected()==0&&field.expired()==0);
    for(unsigned i=0;i<SalvageField::kCapacity;++i)assert(field.drop(1.0f,1.0f,1));
    assert(!field.drop(1.0f,1.0f,1));
    assert(field.update(.01f,1.0f,1.0f)==24&&field.collected()==24);
    assert(field.empty());
    field.clear();
    assert(field.drop(.40f,-.30f,25)&&field.drop(.60f,-.10f,15));
    assert(field.tractor(0.f,0.f,.45f)==25);
    assert(field.collected()==1);
    assert(field.tractor(0.f,0.f,.45f)==0);
    assert(field.tractor(.60f,-.10f,.30f)==15);
    assert(field.tractor(.60f,-.10f,.30f)==0);
    assert(field.collected()==2&&field.empty());
    assert(field.tractor(NAN,0.f,1.f)==0);
    std::cout<<"arcade objectives + salvage deterministic tests passed\n";
}
