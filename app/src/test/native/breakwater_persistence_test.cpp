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

bool MountedGunInScene(State* state,WeaponKind kind) {
    // Inspect the CPU triangles that PrepareFrame really submits to GLES,
    // including the live battery position. Do not mistake the miniature rack
    // preview or a changed HUD label for the player's mounted cannon.
    const auto station=defense::At(state->campaign.location(),
                                   state->defense_points.current);
    std::vector<float> model;
    gunmesh::Build([&](float x0,float y0,float z0,
                       float x1,float y1,float z1,
                       float x2,float y2,float z2,
                       float,float,float) {
        const float points[]={
            x0+station.x,y0+station.y,z0+station.z,
            x1+station.x,y1+station.y,z1+station.z,
            x2+station.x,y2+station.y,z2+station.z};
        model.insert(model.end(),points,points+9);
    },kind,state->gun_direction,state->flash);
    constexpr size_t kProbeVertices=60u;
    if(model.size()<kProbeVertices*3u ||
       state->mesh.size()<kProbeVertices*6u)return false;
    for(size_t start=0;start+kProbeVertices*6u<=state->mesh.size();start+=6u) {
        bool matching=true;
        for(size_t v=0;v<kProbeVertices && matching;++v) {
            for(size_t axis=0;axis<3u;++axis) {
                if(std::abs(state->mesh[start+v*6u+axis]-
                            model[v*3u+axis])>.0001f) {
                    matching=false;break;
                }
            }
        }
        if(matching)return true;
    }
    return false;
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

    // NEW RUN is a two-press destructive action. Interrupting the pointer
    // or pausing must cancel its pending arm and preserve this save slot.
    const auto protected_slot=saved.at(CheckpointSlot(0));
    auto reset_input=At(menu::kResetCard);
    reset_input.delta_seconds=.016f;
    Update(state,&reset_input);  // release the post-pause input gate
    reset_input.primary_down=1u;
    Update(state,&reset_input);
    assert(state->reset_armed && saved.at(CheckpointSlot(0))==protected_slot);
    reset_input.pointer_kind=REVERIE_NATIVE_POINTER_NONE;
    Update(state,&reset_input);
    assert(!state->reset_armed && saved.at(CheckpointSlot(0))==protected_slot);
    reset_input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    Update(state,&reset_input);  // still holding Select must not re-arm
    assert(!state->reset_armed && saved.at(CheckpointSlot(0))==protected_slot);
    reset_input.primary_down=0u;
    Update(state,&reset_input);
    reset_input.primary_down=1u;
    Update(state,&reset_input);
    assert(state->reset_armed && saved.at(CheckpointSlot(0))==protected_slot);
    kApi.pause(state);
    assert(!state->reset_armed && saved.at(CheckpointSlot(0))==protected_slot);
    assert(state->campaign.phase()==Phase::Shop);

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

    // True module-level left-rack continuation: purchase on the actual
    // armory card, equip by the physical controller ray, end a wave, reopen.
    // Direct Campaign weapon tests cannot exercise this input/save boundary.
    saved.clear();unreadable.clear();fail_write=false;apply_then_fail=false;
    fail_next_writes=0;writes=0;last_feedback=0;
    state=static_cast<State*>(Create(&host));assert(state);
    SelectSlot(state,0);assert(state->campaign.phase()==Phase::Briefing);
    Interact(state,At(menu::DifficultyCard(0)),true);
    assert(state->campaign.phase()==Phase::Shop && !state->save_dirty);
    const int32_t starting_credits=state->campaign.credits();
    Interact(state,At(menu::ShopAction(9)),true);
    assert(state->armory_page);
    Interact(state,At(menu::ShopCard(1)),true);
    assert(state->campaign.owns_weapon(WeaponKind::HeavyGun));
    assert(state->campaign.selected_weapon()==WeaponKind::HeavyGun);
    assert(state->campaign.credits()==starting_credits-
           WeaponFor(WeaponKind::HeavyGun).purchase_cost);
    assert(Stored(0).selected_weapon()==WeaponKind::HeavyGun);
    const int32_t after_buy=state->campaign.credits();
    Interact(state,At(menu::ShopCard(1)),true); // owned, not another charge
    assert(state->campaign.credits()==after_buy && !state->save_dirty);
    assert(BeginActiveWave(state,false));

    const auto rack_input=[](uint8_t id) {
        ReverieNativeInputV1 aim{};
        aim.struct_size=sizeof(aim);
        aim.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
        const gunmesh::RackSlot shelf=gunmesh::RackLocation(id);
        const float length=std::sqrt(shelf.x*shelf.x+shelf.y*shelf.y+
                                     shelf.z*shelf.z);
        aim.pointer_direction[0]=shelf.x/length;
        aim.pointer_direction[1]=shelf.y/length;
        aim.pointer_direction[2]=shelf.z/length;
        return aim;
    };
    const auto locked=rack_input(2);
    assert(gunmesh::HitRack(locked.pointer_origin,
                            locked.pointer_direction)==2);
    // The test calls Interact directly; normally Update begins and
    // flushes its sound/feedback frame around this action.
    state->audio_frame.Begin();
    last_feedback=0;
    Interact(state,locked,true); // unpurchased Flak must not be armed
    state->audio_frame.Flush(&host);
    assert(last_feedback==REVERIE_NATIVE_FEEDBACK_FAILURE);
    assert(state->campaign.selected_weapon()==WeaponKind::HeavyGun);
    assert(state->campaign.credits()==after_buy);
    state->combat_input.Release();
    const auto machine=rack_input(0);
    assert(gunmesh::HitRack(machine.pointer_origin,
                            machine.pointer_direction)==0);
    Interact(state,machine,true);
    assert(state->campaign.selected_weapon()==WeaponKind::MachineGun);
    state->combat_input.Release();
    const auto heavy=rack_input(1);
    assert(gunmesh::HitRack(heavy.pointer_origin,
                            heavy.pointer_direction)==1);
    Interact(state,heavy,true);
    assert(state->campaign.selected_weapon()==WeaponKind::HeavyGun);
    state->combat_input.Release();
    Interact(state,machine,true); // final combat loadout before earned save
    assert(state->campaign.selected_weapon()==WeaponKind::MachineGun);

    // End-to-end purchased rack acceptance uses real Update -> PrepareFrame,
    // not direct Campaign::SelectWeapon or a model-preview-only assertion.
    // The visible mounted triangles must change on the very selecting frame.
    auto step_rack=[&](uint8_t id) {
        auto sample=rack_input(id);
        sample.delta_seconds=.016f;
        Update(state,&sample); // release previous press and hover over the rack
        sample.primary_down=1u;
        Update(state,&sample); // one actual module input-frame selection
        assert(PrepareFrame(state)==1);
    };
    auto idle_rack=rack_input(0);
    idle_rack.delta_seconds=.016f;
    Update(state,&idle_rack);
    assert(PrepareFrame(state)==1);
    assert(MountedGunInScene(state,WeaponKind::MachineGun));
    assert(!MountedGunInScene(state,WeaponKind::HeavyGun));

    const auto ammo_before_rack=state->campaign.magazine_left();
    step_rack(1);
    assert(state->campaign.selected_weapon()==WeaponKind::HeavyGun);
    assert(MountedGunInScene(state,WeaponKind::HeavyGun));
    assert(!MountedGunInScene(state,WeaponKind::MachineGun));
    assert(state->flash==0.f && state->campaign.magazine_left()>0);

    step_rack(2); // locked Flak consumes Select but cannot swap mount or fire
    assert(state->campaign.selected_weapon()==WeaponKind::HeavyGun);
    assert(!state->campaign.owns_weapon(WeaponKind::Flak));
    assert(MountedGunInScene(state,WeaponKind::HeavyGun));
    assert(!MountedGunInScene(state,WeaponKind::Flak));
    assert(state->flash==0.f);

    auto live_fire=rack_input(0);
    live_fire.pointer_direction[0]=0.f;
    live_fire.pointer_direction[1]=0.f;
    live_fire.pointer_direction[2]=-1.f;
    live_fire.delta_seconds=.016f;
    Update(state,&live_fire); // release the locked rack press
    const auto heavy_ammo=state->campaign.magazine_left();
    live_fire.primary_down=1u;
    Update(state,&live_fire); // same selected mount must actually fire
    assert(state->campaign.selected_weapon()==WeaponKind::HeavyGun);
    assert(state->campaign.magazine_left()+1u==heavy_ammo);
    assert(state->flash>0.f);
    assert(PrepareFrame(state)==1);
    assert(MountedGunInScene(state,WeaponKind::HeavyGun));

    step_rack(0); // return to saved machine-gun loadout without free ammo
    assert(state->campaign.selected_weapon()==WeaponKind::MachineGun);
    assert(state->campaign.magazine_left()==ammo_before_rack);
    assert(MountedGunInScene(state,WeaponKind::MachineGun));
    assert(!MountedGunInScene(state,WeaponKind::HeavyGun));
    FinishWave(state);
    assert(state->campaign.phase()==Phase::Shop && state->round_summary);
    assert(Stored(0).selected_weapon()==WeaponKind::MachineGun);
    assert(Stored(0).owns_weapon(WeaponKind::HeavyGun));
    Destroy(state);
    state=static_cast<State*>(Create(&host));SelectSlot(state,0);
    assert(state->campaign.phase()==Phase::Shop);
    assert(state->campaign.selected_weapon()==WeaponKind::MachineGun);
    assert(state->campaign.owns_weapon(WeaponKind::HeavyGun));
    assert(!state->campaign.owns_weapon(WeaponKind::Flak));
    assert(state->campaign.credits()==Stored(0).credits());
    Interact(state,At(menu::ShopAction(9)),true);
    const int32_t restart_credits=state->campaign.credits();
    Interact(state,At(menu::ShopCard(1)),true);
    assert(state->campaign.selected_weapon()==WeaponKind::HeavyGun);
    assert(state->campaign.credits()==restart_credits);
    assert(Stored(0).selected_weapon()==WeaponKind::HeavyGun);
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
    Interact(state,At(menu::ShopCard(0)),true);
    assert(!state->reset_armed); // A miss cannot arm the destructive action.
    fail_write=true;Interact(state,At(menu::DebriefAction(1)),true);
    assert(state->reset_armed&&state->campaign.phase()==Phase::Victory);
    Interact(state,At(menu::DebriefAction(1)),true);
    assert(state->campaign.phase()==Phase::Victory&&saved.at(CheckpointSlot(0))==victory_bytes);
    assert(state->reset_pending&&state->save_dirty);
    // Rename may be visible even though final directory fsync reports ERROR.
    apply_then_fail=true;
    assert(kApi.prepare_exit(state)==0&&saved.at(CheckpointSlot(0)).empty());
    assert(state->reset_pending&&state->campaign.phase()==Phase::Victory);
    Interact(state,At(menu::DebriefAction(1)),true);
    assert(saved.at(CheckpointSlot(0)).empty()&&state->reset_pending);
    auto reset_retry=At(menu::DifficultyCard(0));Update(state,&reset_retry);
    fail_write=false;state->save_retry=.01f;reset_retry.delta_seconds=.05f;
    reset_retry.primary_down=reset_retry.secondary_down=1u;
    Update(state,&reset_retry); // Auto clear must consume edges, not start a new campaign.
    assert(state->campaign.phase()==Phase::Briefing&&saved.at(CheckpointSlot(0)).empty());
    kApi.pause(state);Destroy(state);assert(saved.at(CheckpointSlot(0)).empty());
    // Real module Update transitions the last daylight wave to the night
    // shop; an interrupted combat attempt must never earn a phantom payout.
    saved.clear();unreadable.clear();fail_write=false;apply_then_fail=false;
    fail_next_writes=0u;writes=0u;
    Campaign boundary(Difficulty::Casual,0xBEEFu);
    assert(boundary.Start());
    std::vector<uint8_t> last_day(Campaign::kSerializedSize);
    assert(boundary.SerializeShop(last_day.data(),last_day.size()));
    const uint8_t final_day_wave=boundary.waves_this_period();
    Word(last_day,7,final_day_wave);
    const int32_t before_boundary_credits=boundary.credits();
    saved[CheckpointSlot(0)]=last_day;
    state=static_cast<State*>(Create(&host));assert(state);
    SelectSlot(state,0);
    assert(state->campaign.phase()==Phase::Shop);
    assert(state->campaign.location()==0u&&state->campaign.day()==1u);
    assert(state->campaign.period()==Period::Day);
    assert(state->campaign.wave_index()==final_day_wave);
    assert(state->campaign.credits()==before_boundary_credits);
    assert(BeginActiveWave(state,false));
    assert(state->campaign.phase()==Phase::Combat);
    assert(Stored(0).wave_index()==final_day_wave);
    const auto durable_before=saved.at(CheckpointSlot(0));
    // The host may still hold a valid legacy 80-byte shop until a real
    // state change is committed; never confuse migration in memory with a write.
    assert(durable_before.size()==Campaign::kSerializedSize ||
           durable_before.size()==Campaign::kCheckpointSize);
    Destroy(state); // Interrupted before earning the wave clear.

    state=static_cast<State*>(Create(&host));assert(state);
    SelectSlot(state,0);
    assert(state->campaign.phase()==Phase::Shop);
    assert(state->campaign.period()==Period::Day);
    assert(state->campaign.wave_index()==final_day_wave);
    assert(state->campaign.credits()==before_boundary_credits);
    assert(saved.at(CheckpointSlot(0))==durable_before);
    assert(BeginActiveWave(state,false));
    fail_write=true;
    FinishWave(state); // Real Update and spawn progression, not direct completion.
    assert(state->campaign.phase()==Phase::Shop);
    assert(state->campaign.location()==0u&&state->campaign.day()==1u);
    assert(state->campaign.period()==Period::Night);
    assert(state->campaign.wave_index()==1u);
    assert(state->campaign.credits()==before_boundary_credits+220);
    assert(state->round_summary&&state->wave_credits_gained==220);
    assert(state->save_dirty&&state->save_failed);
    assert(saved.at(CheckpointSlot(0))==durable_before);
    assert(Stored(0).period()==Period::Day);
    assert(Stored(0).wave_index()==final_day_wave);
    assert(Stored(0).credits()==before_boundary_credits);
    assert(kApi.prepare_exit(state)==0);
    Destroy(state); // Failed write stays failed across forced shutdown.
    assert(saved.at(CheckpointSlot(0))==durable_before);

    fail_write=false;
    state=static_cast<State*>(Create(&host));assert(state);
    SelectSlot(state,0);
    assert(state->campaign.phase()==Phase::Shop);
    assert(state->campaign.period()==Period::Day);
    assert(state->campaign.wave_index()==final_day_wave);
    assert(state->campaign.credits()==before_boundary_credits);
    assert(BeginActiveWave(state,false));
    FinishWave(state);
    assert(!state->save_dirty&&!state->save_failed);
    assert(state->round_summary&&state->wave_credits_gained==220);
    assert(state->campaign.phase()==Phase::Shop);
    assert(state->campaign.period()==Period::Night);
    assert(state->campaign.day()==1u&&state->campaign.location()==0u);
    assert(state->campaign.wave_index()==1u);
    assert(state->campaign.credits()==before_boundary_credits+220);
    const auto durable_after=saved.at(CheckpointSlot(0));
    assert(durable_after!=durable_before);
    assert(Stored(0).phase()==Phase::Shop);
    assert(Stored(0).period()==Period::Night);
    assert(Stored(0).wave_index()==1u);
    assert(Stored(0).credits()==before_boundary_credits+220);
    auto idle=At(menu::Rect{0.f,0.f,1.f,1.f});
    idle.delta_seconds=.1f;
    for(int repeat=0;repeat<10;++repeat)Update(state,&idle);
    assert(state->campaign.credits()==before_boundary_credits+220);
    assert(state->campaign.wave_index()==1u);
    assert(saved.at(CheckpointSlot(0))==durable_after);
    assert(kApi.prepare_exit(state)==1);
    Destroy(state);

    state=static_cast<State*>(Create(&host));assert(state);
    SelectSlot(state,0);
    assert(!state->round_summary);
    assert(state->campaign.phase()==Phase::Shop);
    assert(state->campaign.period()==Period::Night);
    assert(state->campaign.wave_index()==1u);
    assert(state->campaign.credits()==before_boundary_credits+220);
    assert(BeginActiveWave(state,false)); // Next night wave, no skip.
    assert(state->campaign.phase()==Phase::Combat);
    assert(Stored(0).credits()==before_boundary_credits+220);
    Destroy(state);

    // A corrupt envelope must block slot reuse, not overwrite prior progress.
    auto corrupt=durable_after;
    corrupt[32]^=1u;
    saved[CheckpointSlot(0)]=corrupt;
    const auto writes_before_corrupt=writes;
    state=static_cast<State*>(Create(&host));assert(state);
    assert(state->slot_read[0]==SlotRead::Invalid);
    SelectSlot(state,0);
    assert(state->slot_picker&&state->read_blocked);
    assert(kApi.prepare_exit(state)==0);
    assert(writes==writes_before_corrupt);
    assert(saved.at(CheckpointSlot(0))==corrupt);
    Destroy(state);
    saved[CheckpointSlot(0)]=durable_after;
    state=static_cast<State*>(Create(&host));assert(state);
    SelectSlot(state,0);
    assert(state->campaign.period()==Period::Night);
    assert(state->campaign.wave_index()==1u);
    assert(state->campaign.credits()==before_boundary_credits+220);
    Destroy(state);
    std::cout<<"PASS actual Breakwater wave boundary: interrupted combat, failed result, exact-once reward, corrupt-save refusal and night reload\n";
    std::cout<<"PASS actual Breakwater: read safety, dirty retry, earned wave/victory checkpoints, migration, corruption and guarded reset/exit\n";
}
