// Actual module Update route with a synthetic walkable harbor fixture.
// This does not certify real Sitka geography, packaged startup or headset input.
#include "../../main/jni/native/sitka_terrain_module.cpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace reverie::sitka;
namespace {
std::string saved;
bool fail_write=false;
int32_t Read(const char*,void* buffer,uint32_t capacity,uint32_t* size){
    if(saved.empty())return REVERIE_NATIVE_SAVE_NOT_FOUND;
    *size=static_cast<uint32_t>(saved.size());
    if(!buffer)return REVERIE_NATIVE_SAVE_OK;
    if(capacity<saved.size())return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    std::memcpy(buffer,saved.data(),saved.size());return REVERIE_NATIVE_SAVE_OK;
}
int32_t Write(const char*,const void* data,uint32_t size){
    if(fail_write)return -1;
    saved.assign(static_cast<const char*>(data),size);return REVERIE_NATIVE_SAVE_OK;
}
}
int main(){
    ReverieNativeHostV1 host{};
    host.struct_size=sizeof(host);host.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;
    host.read_save=Read;host.write_save=Write;
    State world;world.host=&host;
    world.terrain.vertices={
        {-20,0,-20,0,0,0},{20,0,-20,0,0,0},
        {20,0,20,0,0,0},{-20,0,20,0,0,0}};
    world.terrain.indices={0,1,2,0,2,3};
    world.walk_session.reset(new FreeRoamSession(
        world.terrain,world.waters,world.shore_colliders,true,0.,0.));
    world.harbor_sites.push_back({"crescent","fixture_desk",HarborLocationKind::ServiceDesk,6,0});
    assert(world.fishing.Load(Store(&world))==FishingLoad::Fresh);
    auto& j=world.fishing.fishing.journal;
    assert(j.hull_tier==0u&&j.gear==0u&&j.reel_level==0u&&j.line_level==0u);
    assert(j.lure_level==0u&&j.shore_kit_level==0u);
    assert(j.fish_inventory[1]==1u&&j.shellfish_inventory==1u&&j.starter_tokens==1u);
    assert(kStarterGearInfiniteDurability&&kBasicBaitInfinite);
    assert(world.fishing.Flush(Store(&world)));
    ReverieNativeInputV1 in{};
    in.struct_size=sizeof(in);in.delta_seconds=.05f;
    in.listener_valid=1u;in.listener_position[1]=1.4f;in.listener_right[0]=1.f;
    in.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;in.pointer_origin[1]=1.25f;in.pointer_direction[0]=1.f;
    // Out of reach cannot open the office or consume the stock fish.
    in.primary_down=1u;Update(&world,&in);assert(!world.harbor_shop.open());
    in.primary_down=0u;in.move_x=1.f;
    for(int n=0;n<30;++n)Update(&world,&in);
    in.move_x=0.f;
    assert(world.walk_session->pose().x>4.f);
    in.primary_down=1u;Update(&world,&in);
    assert(world.harbor_shop.open());
    assert(world.harbor_shop.selected()==kPurchaseItems);
    const auto pose=world.walk_session->pose();
    // Opening press and holding it cannot sell or cast.
    in.move_x=1.f;Update(&world,&in);in.move_x=0.f;
    assert(world.walk_session->pose().x==pose.x&&j.fish_inventory[1]==1u);
    assert(world.fishing.fishing.phase==FishingPhase::Ready);
    auto release=[&]{in.primary_down=0u;in.move_y=0.f;Update(&world,&in);};
    auto select=[&]{release();in.primary_down=1u;Update(&world,&in);};
    auto next=[&]{release();in.move_y=-1.f;Update(&world,&in);in.move_y=0.f;};
    select();assert(j.credits==100u&&j.fish_inventory[1]==0u);
    select();assert(j.credits==100u); // empty sale rejected
    next();select();assert(j.credits==125u&&j.shellfish_inventory==0u);
    next();select();assert(j.starter_tokens==0u&&RegisteredFindCount(j)==1u);
    select();assert(RegisteredFindCount(j)==1u&&j.credits==125u);
    // First rod level is row 17. Scroll the same live UI, do not call purchase directly.
    while(world.harbor_shop.selected()!=17u)next();
    assert(HarborOffer(j,17u).unlocked);
    select();assert(j.gear==1u&&j.credits==90u);
    assert(!NextEquipment(j,Equipment::Rod).unlocked);
    const auto panel=HarborShopPanel(world.harbor_shop,world.fishing);
    assert(panel[2].find("INFINITE")!=std::string::npos);
    Journal check;assert(LoadJournal(saved,check)&&check.gear==1u&&check.credits==90u);
    // New finds are pending until registration. Live animals/cultural sites remain records.
    assert(world.fishing.Discover(7u,BeachFind::Shell));
    assert(!NextEquipment(j,Equipment::Rod).unlocked);
    while(world.harbor_shop.selected()!=kPurchaseItems+2u)next();
    fail_write=true;select();assert(j.submitted_beach_sites==(uint64_t(1)<<7u));
    assert(world.fishing.dirty&&world.fishing.write_failed);
    assert(HarborShopPanel(world.harbor_shop,world.fishing)[4].find("SAVE FAILED")!=std::string::npos);
    fail_write=false;Pause(&world);assert(!world.fishing.dirty);
    assert(LoadJournal(saved,check)&&NextEquipment(check,Equipment::Rod).unlocked);
    // Reopen from the same host slot without replenishing sold supplies.
    FishingSession restored;assert(restored.Load(Store(&world))==FishingLoad::Restored);
    assert(restored.fishing.journal.fish_inventory[1]==0u);
    assert(restored.fishing.journal.shellfish_inventory==0u);
    assert(restored.fishing.journal.starter_tokens==0u);
    // Starter supplies stay available after repeated casts and cancellations.
    Fishing fallback;fallback.journal=StarterJournal();
    for(unsigned n=0;n<2000u;++n){
        assert(fallback.Cast(true,2.f));fallback.Step(.016f,false,true);
    }
    assert(fallback.journal.gear==0u&&fallback.journal.lure_level==0u);
    assert(fallback.journal.fish_inventory[1]==1u&&fallback.journal.starter_tokens==1u);
    auto corrupt=SaveJournal(check);corrupt+="1\n";
    const auto unchanged=SaveJournal(check);
    assert(!LoadJournal(corrupt,check)&&SaveJournal(check)==unchanged);
    std::cout<<"PASS actual Sitka Update: walk/office/sale/register/upgrade/save/retry\n";
}
