// Audit-only production module callbacks; no GL context or production edits.
#include "breakwater_module.cpp"
#include <cassert>
#include <iostream>
static int reads=0,writes=0;static bool fail=true;static uint32_t feedback=0;
static std::vector<uint8_t> persisted;
static void Log(int32_t,const char*,const char*){}
static int32_t Read(const char*,void*,uint32_t,uint32_t*){++reads;return REVERIE_NATIVE_SAVE_ERROR;}
static int32_t Write(const char*,const void* bytes,uint32_t size){++writes;if(fail)return REVERIE_NATIVE_SAVE_ERROR;persisted.assign(static_cast<const uint8_t*>(bytes),static_cast<const uint8_t*>(bytes)+size);return REVERIE_NATIVE_SAVE_OK;}
static void FeedbackLog(uint32_t f){feedback=f;}
static ReverieNativeInputV1 Aim(float x,float y){
    ReverieNativeInputV1 in{};in.struct_size=sizeof(in);in.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    const float norm=std::sqrt(x*x+y*y+9.f);in.pointer_direction[0]=x/norm;in.pointer_direction[1]=y/norm;in.pointer_direction[2]=-3.f/norm;return in;
}
int main(){
    ReverieNativeHostV1 host{};host.struct_size=sizeof(host);host.abi_version=1;host.log=Log;host.read_save=Read;host.write_save=Write;host.request_feedback=FeedbackLog;
    State s;s.host=&host;LoadState(&s);
    Interact(&s,Aim(menu::SlotCard(0).x,menu::SlotCard(0).y),true);
    assert(!s.slot_picker&&s.campaign.phase()==Phase::Briefing);
    Interact(&s,Aim(menu::DifficultyCard(0).x,menu::DifficultyCard(0).y),true);
    bool silent=s.campaign.phase()==Phase::Shop&&feedback==REVERIE_NATIVE_FEEDBACK_ACTIVATION&&writes==1&&persisted.empty();
    std::cout<<"write failure: phase_shop="<<(s.campaign.phase()==Phase::Shop)<<" activation_feedback="<<(feedback==REVERIE_NATIVE_FEEDBACK_ACTIVATION)<<" writes="<<writes<<" persisted_bytes="<<persisted.size()<<" silent_failure="<<silent<<"\n";
    Pause(&s);bool noRetry=writes==1;
    std::cout<<"after pause: write_calls="<<writes<<" no_retry="<<noRetry<<"\n";
    fail=false;
    State t;t.host=&host;
    Interact(&t,Aim(menu::SlotCard(0).x,menu::SlotCard(0).y),true);
    const bool treatedNew=t.campaign.phase()==Phase::Briefing&&!t.slot_exists[0];
    Interact(&t,Aim(menu::DifficultyCard(0).x,menu::DifficultyCard(0).y),true);
    bool replacement=treatedNew&&persisted.size()==Campaign::kSerializedSize;
    std::cout<<"read I/O failure: treated_as_new="<<treatedNew<<" then_written_bytes="<<persisted.size()<<" overwrote_unreadable_slot="<<replacement<<"\n";
    return silent&&noRetry&&replacement?1:2;
}
