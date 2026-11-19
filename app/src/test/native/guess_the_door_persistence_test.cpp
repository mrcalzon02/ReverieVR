// Exercise production module input and lifecycle; linked GLES is not invoked.
#include "../../main/jni/native/guess_the_door_module.cpp"
#include <cassert>
#include <iostream>
#include <map>
#include <string>
namespace {
std::map<std::string,std::vector<uint8_t>> disk;
std::string unreadable;
bool failWrite=false,applyThenFail=false;
unsigned writes=0u;
uint32_t feedback=0u;
void Log(int32_t,const char*,const char*) {}
void Feedback(uint32_t value) {feedback=value;}
int32_t Read(const char* name,void* data,uint32_t capacity,uint32_t* size) {
    if(unreadable==name)return REVERIE_NATIVE_SAVE_ERROR;
    auto found=disk.find(name);if(found==disk.end())return REVERIE_NATIVE_SAVE_NOT_FOUND;
    *size=static_cast<uint32_t>(found->second.size());
    if(!data)return REVERIE_NATIVE_SAVE_OK;
    if(capacity<*size)return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    if(*size)std::memcpy(data,found->second.data(),*size);
    return REVERIE_NATIVE_SAVE_OK;
}
int32_t Write(const char* name,const void* data,uint32_t size) {
    ++writes;
    if(failWrite&&!applyThenFail)return REVERIE_NATIVE_SAVE_ERROR;
    auto& bytes=disk[name];bytes.clear();
    if(size)bytes.assign(static_cast<const uint8_t*>(data),static_cast<const uint8_t*>(data)+size);
    return failWrite?REVERIE_NATIVE_SAVE_ERROR:REVERIE_NATIVE_SAVE_OK;
}
Game Stored() {
    Game game;const auto& bytes=disk.at(reverie::doors::kMazeSaveSlot);
    assert(reverie::doors::DecodeMazeSave(bytes.data(),bytes.size(),game));return game;
}
void Click(State* s,float axis=0.f) {
    // Legacy base-input hosts have no pointer prefix; they still support
    // rocker plus Select. An explicit full-prefix POINTER_NONE is different.
    ReverieNativeInputV1 input{};
    input.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;
    input.delta_seconds=.01f;
    Update(s,&input);input.primary_down=1u;input.move_x=axis;Update(s,&input);
}
State* New(const ReverieNativeHostV1& host) {
    auto* s=static_cast<State*>(Create(&host));assert(s);return s;
}
void InForwardView(State* s) {
    Scene(s);assert(!s->font.vertices.empty());
    const float tangent=std::tan(40.f*3.14159265358979323846f/180.f);
    for(size_t index=0;index<s->font.vertices.size();index+=8u) {
        const auto& vertices=s->font.vertices;
        const float extent=-vertices[index+2u]*tangent-.05f;
        assert(std::fabs(vertices[index])+.032f<extent);
        assert(std::fabs(vertices[index+1u])<extent);
    }
}
}
int main() {
    ReverieNativeHostV1 host{};host.struct_size=sizeof(host);host.abi_version=1u;
    host.log=Log;host.read_save=Read;host.write_save=Write;host.request_feedback=Feedback;
    auto* s=New(host);assert(!Recovering(s)&&!s->save_dirty && writes==1u);
    // A tracked ray parallel to or facing away from the maze must miss.
    for(int axis:{0,2}){
        ReverieNativeInputV1 away{};away.struct_size=sizeof(away);
        away.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
        away.pointer_direction[axis]=1.f;away.primary_down=1u;
        Update(s,&away);assert(s->game.phase==Phase::Ready && writes==1u);
        away.primary_down=0u;Update(s,&away);
    }
    const auto seed=s->game.seed;
    s->choice=static_cast<int>(s->game.current.safeSide);Click(s);
    assert(s->game.phase==Phase::Pulling && Stored().room==1u);
    kApi.pause(s);assert(kApi.prepare_exit(s)==1);Destroy(s);
    s=New(host);assert(s->game.seed==seed && s->game.room==1u && s->game.phase==Phase::Ready);
    s->choice=1-static_cast<int>(s->game.current.safeSide);Click(s);
    assert(s->game.phase==Phase::Pulling && Stored().falls==1u && Stored().room==0u);
    Destroy(s);s=New(host);assert(s->game.falls==1u && s->game.room==0u);

    // A confirmed choice remains pending, exit refuses destruction, pause retries.
    failWrite=true;s->choice=static_cast<int>(s->game.current.safeSide);Click(s);
    assert(s->save_dirty && s->save_failed && Stored().room==0u && kApi.prepare_exit(s)==0);
    failWrite=false;kApi.pause(s);assert(!s->save_dirty && Stored().room==1u && kApi.prepare_exit(s)==1);
    Destroy(s);s=New(host);

    // New Seed consumes its edge and preserves reset intent even if applied bytes return ERROR.
    s->game.room=reverie::doors::kRooms;s->game.phase=Phase::Victory;
    ObserveProgress(s);const auto oldSeed=s->game.seed;
    failWrite=applyThenFail=true;Click(s);
    assert(s->game.seed!=oldSeed && s->game.phase==Phase::Ready && s->game.side==-1);
    const auto newSeed=s->game.seed;
    assert(s->save_dirty && Stored().seed==newSeed && Stored().room==0u && kApi.prepare_exit(s)==0);
    failWrite=applyThenFail=false;kApi.pause(s);
    assert(!s->save_dirty && Stored().seed==newSeed && Stored().room==0u);
    // A held button across pause/resume cannot pull a lever before release.
    kApi.resume(s);ReverieNativeInputV1 input{};
    input.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;
    input.primary_down=1u;Update(s,&input);Update(s,&input);assert(s->game.phase==Phase::Ready);
    input.primary_down=0u;Update(s,&input);input.primary_down=1u;Update(s,&input);
    assert(s->game.phase==Phase::Pulling);Destroy(s);

    // Read failures never look like a missing slot and never authorize replacement.
    const auto original=disk.at(reverie::doors::kMazeSaveSlot);
    unreadable=reverie::doors::kMazeSaveSlot;const auto before=writes;s=New(host);
    assert(Recovering(s)&&kApi.prepare_exit(s)==0);Click(s,1.f);Click(s);
    assert(writes==before && disk.at(reverie::doors::kMazeSaveSlot)==original);
    unreadable.clear();Click(s,-1.f);assert(!Recovering(s)&&kApi.prepare_exit(s)==1);Destroy(s);

    // Invalid bytes are copied into an unused recovery slot before any new maze write.
    const std::vector<uint8_t> invalid={1u,2u,3u,4u,5u};
    disk[reverie::doors::kMazeSaveSlot]=invalid;s=New(host);
    assert(s->save_read==MazeRead::Invalid && kApi.prepare_exit(s)==0);
    s->choice=1;input={};input.struct_size=sizeof(input);
    input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    input.pointer_direction[2]=1.f;input.primary_down=1u;
    Update(s,&input);assert(!s->reset_confirm && disk.at(reverie::doors::kMazeSaveSlot)==invalid);
    input.primary_down=0u;Update(s,&input);
    input={};input.struct_size=sizeof(input);input.primary_down=1u;input.move_x=1.f;
    input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;input.pointer_direction[2]=-1.f;
    Update(s,&input); // Active centre ray misses both buttons; rocker still selects New Maze.
    assert(s->reset_confirm && disk.at(reverie::doors::kMazeSaveSlot)==invalid);
    failWrite=true;Click(s);assert(Recovering(s)&&disk.at(reverie::doors::kMazeSaveSlot)==invalid);
    // The backup may have reached disk before its durability acknowledgement failed.
    applyThenFail=true;Click(s);assert(Recovering(s));
    assert(disk.at("maze-recovery-1.bin")==invalid);
    failWrite=applyThenFail=false;Click(s);
    assert(!Recovering(s)&&!s->save_dirty && disk.at("maze-recovery-1.bin")==invalid);
    assert(Stored().room==0u && kApi.prepare_exit(s)==1);Destroy(s);

    // Existing backup bytes are never silently replaced, including a full recovery set.
    disk[reverie::doors::kMazeSaveSlot]={9u,8u,7u};
    disk["maze-recovery-2.bin"]={2u};disk["maze-recovery-3.bin"]={3u};
    const auto allSaved=disk;s=New(host);Click(s,1.f);Click(s);
    assert(Recovering(s)&&disk==allSaved&&kApi.prepare_exit(s)==0);Destroy(s);

    // Rocker selection is actionable even while a valid pointer aims at a door, not a lever.
    disk.erase(reverie::doors::kMazeSaveSlot);s=New(host);
    input={};input.struct_size=sizeof(input);input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    input.pointer_direction[2]=-1.f;input.move_x=1.f;input.primary_down=1u;Update(s,&input);
    assert(s->game.side==1 && s->game.phase==Phase::Pulling);Destroy(s);
    disk.erase(reverie::doors::kMazeSaveSlot);s=New(host);
    // Older hosts with only part of the pointer prefix retain pointerless Select.
    input={};input.struct_size=offsetof(ReverieNativeInputV1,pointer_direction);
    input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    input.primary_down=1u;Update(s,&input);
    assert(s->game.phase==Phase::Pulling && s->game.side==0);
    for(uint64_t seedCase=1u;seedCase<=64u;++seedCase) {
        s->game.Reset(seedCase);InForwardView(s);
        s->doorGaze=0;s->gazeSeconds=1.f;InForwardView(s);
    }
    s->save_read=MazeRead::Invalid;s->save_notice="INVALID SAVE RETAINED";
    InForwardView(s);s->save_read=MazeRead::Fresh;Destroy(s);

    // Full-prefix POINTER_NONE is shell/Quick Menu ownership, not a legacy
    // base-input host. It cannot silently choose a lever or run puzzle time.
    disk.erase(reverie::doors::kMazeSaveSlot);
    s=New(host);
    const unsigned savedWrites=writes;
    ReverieNativeInputV1 withdrawn{};
    withdrawn.struct_size=sizeof(withdrawn);
    withdrawn.delta_seconds=.1f;
    withdrawn.pointer_kind=REVERIE_NATIVE_POINTER_NONE;
    withdrawn.primary_down=1u;
    withdrawn.move_x=1.f;
    for(int frame=0;frame<35;++frame)Update(s,&withdrawn);
    assert(s->game.phase==Phase::Ready&&s->game.room==0u);
    assert(s->game.timer==0.f&&s->game.side==-1);
    assert(s->choice==0&&!s->rockerSelection);
    assert(writes==savedWrites&&!s->save_dirty);

    // Recover a real tracked ray with Select still held: require a release
    // before any fresh lever press may enter the game's simulation.
    ReverieNativeInputV1 lever=withdrawn;
    lever.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    lever.pointer_direction[0]=-2.f;
    lever.pointer_direction[1]=-.1f;
    lever.pointer_direction[2]=-3.43f;
    lever.move_x=0.f;
    Update(s,&lever);
    assert(s->game.phase==Phase::Ready&&s->game.side==-1);
    lever.primary_down=0u;Update(s,&lever);
    assert(!s->require_release);
    lever.primary_down=1u;Update(s,&lever);
    assert(s->game.phase==Phase::Pulling&&s->game.side==0);
    const float animationTime=s->game.timer;
    const unsigned committedRoom=Stored().room;
    withdrawn.primary_down=0u;withdrawn.move_x=0.f;
    for(int frame=0;frame<35;++frame)Update(s,&withdrawn);
    assert(s->game.phase==Phase::Pulling&&s->game.timer==animationTime);
    assert(Stored().room==committedRoom);
    // Invalid tracked rays also fail closed; only valid rays can resume.
    ReverieNativeInputV1 invalid=lever;
    invalid.pointer_direction[0]=invalid.pointer_direction[1]=
        invalid.pointer_direction[2]=0.f;
    Update(s,&invalid);
    assert(s->game.phase==Phase::Pulling&&s->game.timer==animationTime);
    lever.primary_down=0u;Update(s,&lever);
    for(int frame=0;frame<8;++frame)Update(s,&lever);
    assert(s->game.phase==Phase::Opening);
    Destroy(s);
    std::cout<<"Guess module: safe resume, New Seed, save/read faults, backup recovery, lost-pointer freeze and rocker PASS\n";
}
