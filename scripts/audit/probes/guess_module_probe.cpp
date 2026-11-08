// Audit-only real-module probe. No production changes. Desktop input with no pointer
// exercises the module's advertised rocker/primary fallback, via actual Update.
#include "guess_the_door_module.cpp"
#include <iostream>
#include <cassert>
static int reads=0,writes=0;
static void Log(int32_t,const char*,const char*){}
static int32_t Read(const char*,void*,uint32_t,uint32_t*){++reads;return REVERIE_NATIVE_SAVE_NOT_FOUND;}
static int32_t Write(const char*,const void*,uint32_t){++writes;return REVERIE_NATIVE_SAVE_OK;}
static void Step(State* s,bool primary,float x=0.f){
    ReverieNativeInputV1 in{};in.struct_size=sizeof(in);in.delta_seconds=.1f;
    in.primary_down=primary?1u:0u;in.move_x=x;Update(s,&in);
}
static void Room(State* s){
    assert(s->game.phase==Phase::Ready);
    Step(s,false);
    Step(s,true,s->game.current.safeSide?1.f:-1.f);
    assert(s->game.phase==Phase::Pulling);
    for(int i=0;i<60&&s->game.phase!=Phase::Ready&&s->game.phase!=Phase::Victory;++i)Step(s,false);
}
int main(){
    ReverieNativeHostV1 host{};host.struct_size=sizeof(host);host.abi_version=1;
    host.log=Log;host.read_save=Read;host.write_save=Write;
    auto*s=static_cast<State*>(Create(&host));assert(s);
    Room(s);Room(s);Room(s);assert(s->game.room==3);
    const auto originalSeed=s->game.seed;
    kApi.pause(s);kApi.resume(s);
    std::cout<<"before destroy: room="<<s->game.room<<" seed="<<s->game.seed<<" reads="<<reads<<" writes="<<writes<<"\n";
    Destroy(s);s=static_cast<State*>(Create(&host));assert(s);
    bool lost=s->game.room!=3||s->game.seed!=originalSeed;
    std::cout<<"after recreate: room="<<s->game.room<<" seed="<<s->game.seed<<" reads="<<reads<<" writes="<<writes<<" progress_lost="<<lost<<"\n";
    for(unsigned i=0;i<reverie::doors::kRooms;++i)Room(s);
    assert(s->game.phase==Phase::Victory);
    std::cout<<"actual Update reached victory: clears="<<s->game.clearCount<<"\n";
    Step(s,false);Step(s,true);
    bool doubleAction=s->game.phase==Phase::Pulling;
    std::cout<<"new-seed click: phase="<<int(s->game.phase)<<" (Ready=0 Pulling=1) side="<<s->game.side<<" unintended_auto_pull="<<doubleAction<<"\n";
    Destroy(s);
    // Exit failure marks release expectations violated, not a failed harness.
    return lost&&doubleAction?1:2;
}
