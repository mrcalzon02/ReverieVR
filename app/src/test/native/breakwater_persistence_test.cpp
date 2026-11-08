// Actual module interaction/lifecycle tests. GLES is linked but not executed.
#include "../../main/jni/native/breakwater_module.cpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace {
std::map<std::string,std::vector<uint8_t>> saved;
std::string unreadable;
bool fail_write=false;
bool apply_then_fail=false;
unsigned writes=0;
unsigned fail_next_writes=0;
uint32_t last_feedback=0;
void TestLog(int32_t,const char*,const char*) {}
int32_t TestRead(const char* name,void* data,uint32_t capacity,uint32_t* size) {
    if(unreadable==name)return REVERIE_NATIVE_SAVE_ERROR;
    const auto found=saved.find(name);
    if(found==saved.end())return REVERIE_NATIVE_SAVE_NOT_FOUND;
    *size=static_cast<uint32_t>(found->second.size());
    if(!data)return REVERIE_NATIVE_SAVE_OK;
    if(capacity<*size)return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    if(*size)std::memcpy(data,found->second.data(),*size);
    return REVERIE_NATIVE_SAVE_OK;
}
int32_t TestWrite(const char* name,const void* data,uint32_t size) {
    ++writes;
    const bool failed=fail_write || fail_next_writes>0u;
    if(fail_next_writes>0u)--fail_next_writes;
    if(failed && !apply_then_fail)return REVERIE_NATIVE_SAVE_ERROR;
    auto& bytes=saved[name];bytes.clear();
    if(size)bytes.assign(static_cast<const uint8_t*>(data),
                         static_cast<const uint8_t*>(data)+size);
    return failed?REVERIE_NATIVE_SAVE_ERROR:REVERIE_NATIVE_SAVE_OK;
}
void TestFeedback(uint32_t value) {last_feedback=value;}
ReverieNativeInputV1 At(const menu::Rect& rectangle) {
    ReverieNativeInputV1 input{};input.struct_size=sizeof(input);
    input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    const float length=std::sqrt(rectangle.x*rectangle.x+
                                 rectangle.y*rectangle.y+9.f);
    input.pointer_direction[0]=rectangle.x/length;
    input.pointer_direction[1]=rectangle.y/length;
    input.pointer_direction[2]=-3.f/length;
    return input;
}
void SelectSlot(State* state,unsigned slot) {
    Interact(state,At(menu::SlotCard(static_cast<int>(slot))),true);
}
Campaign Stored(unsigned slot) {
    Campaign value;const auto& bytes=saved.at(CheckpointSlot(slot));
    assert(value.DeserializeCheckpoint(bytes.data(),bytes.size()));
    return value;
}
void Word(std::vector<uint8_t>& bytes,unsigned index,uint32_t value) {
    for(unsigned i=0;i<4;++i)bytes[index*4u+i]=uint8_t(value>>(8u*i));
}
void FinishWave(State* state) {
    // Controlled combat fixture removes each spawned hull before landing.
    // Actual module Update must own the completion, checkpoint and summary.
    auto input=At(menu::Rect{0.f,0.f,1.f,1.f});input.delta_seconds=.1f;
    for(unsigned tick=0;tick<20000 && state->campaign.phase()==Phase::Combat;++tick) {
        auto& fleet=const_cast<std::array<Enemy,Campaign::kMaxEnemies>&>(
            state->campaign.enemies());
        for(auto& enemy:fleet)enemy.active=false;
        Update(state,&input);
    }
    assert(state->campaign.phase()==Phase::Shop ||
           state->campaign.phase()==Phase::Victory);
}
}

int main() {
    ReverieNativeHostV1 host{};host.struct_size=sizeof(host);host.abi_version=1;
    host.log=TestLog;host.read_save=TestRead;host.write_save=TestWrite;
    host.request_feedback=TestFeedback;
    // Base-only input is safely rejected because this title needs a pointer.
    auto shortInput=std::unique_ptr<uint8_t[]>(new uint8_t[REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE]);
    ReverieNativeInputV1 base{};base.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;
    base.delta_seconds=.01f;
    std::memcpy(shortInput.get(),&base,REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE);
    auto* prefixState=static_cast<State*>(Create(&host));assert(prefixState);
    Update(prefixState,reinterpret_cast<const ReverieNativeInputV1*>(shortInput.get()));
    // A valid pointer prefix has no later listener tail. This exact allocation
    // reaches the real Update path and lets ASan catch a full-struct overread.
    static_assert(REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE<sizeof(ReverieNativeInputV1),
                  "Pointer-prefix test must omit a real API tail");
    auto pointerInput=std::unique_ptr<uint8_t[]>(new uint8_t[REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE]);
    base.struct_size=REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE;
    base.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;base.pointer_direction[2]=-1.f;
    std::memcpy(pointerInput.get(),&base,REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE);
    Update(prefixState,reinterpret_cast<const ReverieNativeInputV1*>(pointerInput.get()));
    assert(prefixState->slot_picker);Destroy(prefixState);
    Campaign original(Difficulty::Casual,0x1234u);assert(original.Start());
    auto& legacy=saved[CheckpointSlot(0)];legacy.resize(Campaign::kSerializedSize);
    assert(original.SerializeShop(legacy.data(),legacy.size()));
    saved[CheckpointSlot(1)]={1,2,3};unreadable=CheckpointSlot(2);
    const auto before=saved;
    auto* state=static_cast<State*>(Create(&host));assert(state);
    assert(state->slot_read[0]==SlotRead::Valid);
    assert(state->slot_read[1]==SlotRead::Invalid);
    assert(state->slot_read[2]==SlotRead::IoError);
    SelectSlot(state,1);assert(state->slot_picker&&state->read_blocked);
    assert(kApi.prepare_exit(state)==0&&writes==0&&saved==before);
    SelectSlot(state,2);assert(state->slot_picker&&writes==0&&saved==before);
    SelectSlot(state,0);assert(!state->slot_picker&&!state->read_blocked);
    assert(state->campaign.phase()==Phase::Shop&&kApi.prepare_exit(state)==1);
    LoadState(state);assert(state->campaign.credits()==original.credits());
    Destroy(state);

    unreadable.clear();saved.clear();writes=0;
    state=static_cast<State*>(Create(&host));assert(state);
    SelectSlot(state,0);assert(state->campaign.phase()==Phase::Briefing);
    fail_write=true;
    Interact(state,At(menu::DifficultyCard(0)),true);
    state->audio_frame.Flush(&host); // Direct fixture call needs Update's normal audio dispatch boundary.
    assert(state->campaign.phase()==Phase::Shop&&state->save_dirty&&state->save_failed);
    assert(last_feedback==REVERIE_NATIVE_FEEDBACK_FAILURE&&saved.empty());
    assert(!BeginActiveWave(state,true)&&state->campaign.phase()==Phase::Shop);
    assert(kApi.prepare_exit(state)==0);
    fail_write=false;kApi.pause(state);
    assert(!state->save_dirty&&!state->save_failed&&kApi.prepare_exit(state)==1);
    assert(saved.at(CheckpointSlot(0)).size()==Campaign::kCheckpointSize);
    assert(Stored(0).phase()==Phase::Shop);

    // A failed purchase is retained, and the retry click cannot buy twice.
    const auto credits=state->campaign.credits();
    fail_write=true;Interact(state,At(menu::ShopCard(0)),true);
    assert(state->campaign.level(Upgrade::FireRate)==1u);
    const auto after_purchase=state->campaign.credits();
    assert(after_purchase<credits&&state->save_dirty);
    assert(Stored(0).credits()==credits);
    Interact(state,At(menu::ShopCard(0)),true);
    assert(state->campaign.level(Upgrade::FireRate)==1u);
    auto retry=At(menu::ShopCard(0));Update(state,&retry); // Release focus while write still fails.
    fail_write=false;state->save_retry=.01f;retry.delta_seconds=.05f;retry.primary_down=1u;
    retry.secondary_down=1u;
    Update(state,&retry); // Auto retry and fresh select share this exact frame.
    assert(!state->save_dirty&&state->campaign.credits()==after_purchase);
    assert(state->campaign.level(Upgrade::FireRate)==1u);
    assert(state->campaign.phase()==Phase::Shop); // Secondary cannot launch on the retry frame.
    assert(Stored(0).credits()==after_purchase);

    // A purchase can first become dirty during this frame. A simultaneous
    // secondary edge must not retry again and launch after that first failure.
    retry.primary_down=retry.secondary_down=0u;Update(state,&retry);
    fail_next_writes=1u;const auto before_failed_purchase=writes;
    retry.primary_down=retry.secondary_down=1u;Update(state,&retry);
    assert(state->campaign.level(Upgrade::FireRate)==2u&&state->save_dirty);
    assert(state->campaign.phase()==Phase::Shop&&writes==before_failed_purchase+1u);
    assert(kApi.prepare_exit(state)==1&&!state->save_dirty);

    assert(BeginActiveWave(state,false));
    FinishWave(state);
    assert(state->round_summary&&state->campaign.wave_index()==2u);
    assert(Stored(0).wave_index()==2u&&Stored(0).credits()==state->campaign.credits());
    // No summary action occurs before destroy/recreate.
    Destroy(state);state=static_cast<State*>(Create(&host));SelectSlot(state,0);
    assert(state->campaign.wave_index()==2u&&state->campaign.phase()==Phase::Shop);
    Destroy(state);

    // Forge only a valid pre-final shop fixture using the public legacy format.
    std::vector<uint8_t> final_shop(Campaign::kSerializedSize);
    assert(original.SerializeShop(final_shop.data(),final_shop.size()));
    Word(final_shop,4,Campaign::kLocations-1u);
    Word(final_shop,5,Campaign::Locations().back().days);
    Word(final_shop,6,static_cast<uint32_t>(Period::Night));Word(final_shop,7,1u);
    Campaign final_campaign;assert(final_campaign.DeserializeShop(final_shop.data(),final_shop.size()));
    Word(final_shop,7,final_campaign.waves_this_period());
    assert(final_campaign.DeserializeShop(final_shop.data(),final_shop.size()));
    saved[CheckpointSlot(0)]=final_shop;
    state=static_cast<State*>(Create(&host));SelectSlot(state,0);
    assert(BeginActiveWave(state,false));FinishWave(state);
    assert(state->campaign.phase()==Phase::Victory);
    assert(Stored(0).phase()==Phase::Victory&&kApi.prepare_exit(state)==1);
    Destroy(state);state=static_cast<State*>(Create(&host));SelectSlot(state,0);
    assert(state->campaign.phase()==Phase::Victory);
    const auto victory_bytes=saved.at(CheckpointSlot(0));
    auto damaged=victory_bytes;damaged[32]^=1u;
    Campaign preserved=state->campaign;
    assert(!preserved.DeserializeCheckpoint(damaged.data(),damaged.size()));
    assert(preserved.phase()==Phase::Victory);
    assert(!preserved.DeserializeCheckpoint(victory_bytes.data(),victory_bytes.size()-1u));
    fail_write=true;Interact(state,At(menu::ShopCard(0)),true);
    assert(state->reset_armed&&state->campaign.phase()==Phase::Victory);
    Interact(state,At(menu::ShopCard(0)),true);
    assert(state->campaign.phase()==Phase::Victory&&saved.at(CheckpointSlot(0))==victory_bytes);
    assert(state->reset_pending&&state->save_dirty);
    // Rename may be visible even though final directory fsync reports ERROR.
    apply_then_fail=true;
    assert(kApi.prepare_exit(state)==0&&saved.at(CheckpointSlot(0)).empty());
    assert(state->reset_pending&&state->campaign.phase()==Phase::Victory);
    Interact(state,At(menu::ShopCard(0)),true);
    assert(saved.at(CheckpointSlot(0)).empty()&&state->reset_pending);
    auto reset_retry=At(menu::DifficultyCard(0));Update(state,&reset_retry);
    fail_write=false;state->save_retry=.01f;reset_retry.delta_seconds=.05f;
    reset_retry.primary_down=reset_retry.secondary_down=1u;
    Update(state,&reset_retry); // Auto clear must consume edges, not start a new campaign.
    assert(state->campaign.phase()==Phase::Briefing&&saved.at(CheckpointSlot(0)).empty());
    kApi.pause(state);Destroy(state);assert(saved.at(CheckpointSlot(0)).empty());
    std::cout<<"PASS actual Breakwater: read safety, dirty retry, earned wave/victory checkpoints, migration, corruption and guarded reset/exit\n";
}
