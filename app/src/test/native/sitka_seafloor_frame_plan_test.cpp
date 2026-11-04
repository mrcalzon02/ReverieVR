#include "../../main/jni/native/sitka_seafloor_frame_plan.h"
#include "../../main/jni/native/reverie_native_sdk.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace reverie::sitka;
static void U32(std::string& bytes,uint32_t value){
    for(unsigned i=0u;i<4u;++i)
        bytes.push_back(static_cast<char>(value>>(i*8u)));
}
static void F64(std::string& bytes,double v){
    uint64_t raw=0u;std::memcpy(&raw,&v,8u);
    for(unsigned i=0u;i<8u;++i)
        bytes.push_back(static_cast<char>(raw>>(i*8u)));
}
static void F32(std::string& bytes,float v){
    uint32_t raw=0u;std::memcpy(&raw,&v,4u);U32(bytes,raw);
}
static std::string SourceTile(int col){
    std::string bytes="SITKAB01";
    U32(bytes,3u);U32(bytes,3u);
    F64(bytes,col*200.);F64(bytes,1000.);
    F64(bytes,100.);F64(bytes,100.);
    U32(bytes,BathyDatumId("NAVD88"));U32(bytes,0u);
    for(int i=0;i<9;++i){F32(bytes,-12.f);bytes.push_back(char(1));}
    return bytes;
}
static int32_t EmptyPrep(void*){return 1;}
int main(){
    ReverieNativeModuleApiV1 api{};
    api.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;
    api.struct_size=REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE;
    api.prepare_frame=EmptyPrep;
    assert(!ReverieNativeApiSupportsPrepareFrameV1(&api));
    api.struct_size=REVERIE_NATIVE_MODULE_API_V1_PREPARE_FRAME_MIN_SIZE;
    assert(ReverieNativeApiSupportsPrepareFrameV1(&api));
    api.prepare_frame=nullptr;
    assert(!ReverieNativeApiSupportsPrepareFrameV1(&api));

    // No cache read: deterministic procedural selection follows the
    // world-aligned 256-metre grid, independently of player frame count.
    SeafloorFramePlanner plan;
    assert(plan.Update(10.,950.,20.,nullptr,true));
    assert(plan.desired().valid&&!plan.desired().sourced);
    assert(plan.desired().stride==1u);
    plan.Commit();
    assert(!plan.Update(255.,950.,20.,nullptr,true));
    for(int i=0;i<120;++i){
        const double east=i%2?256.1:255.9;
        assert(!plan.Update(east,950.,20.,nullptr,true));
    }
    assert(plan.committed_swaps()==1u);
    assert(plan.Update(265.,950.,20.,nullptr,true));
    assert(plan.desired().grid_east==1);
    plan.Commit();
    assert(!plan.Update(255.9,950.,20.,nullptr,true)); // backward jitter
    assert(plan.Update(246.,950.,20.,nullptr,true)); // crossed hysteresis
    plan.Commit();
    assert(plan.committed_swaps()==3u);
    assert(plan.Update(246.,950.,20.,nullptr,false));
    assert(!plan.desired().valid); // land means no seabed rendering
    plan.Commit();
    assert(!plan.Update(246.,950.,20.,nullptr,false));

    const auto dir=ParseBathyStreamIndex(
        "SITKA-BATHY-INDEX-2\n"
        "datum NAVD88\n"
        "tiles 2\n"
        "bathymetry-r00-c00.sitkab 0 1000 100 100 3 3\n"
        "bathymetry-r00-c01.sitkab 200 1000 100 100 3 3\n");
    BathyStreamCache cache(dir);
    int reads=0;
    auto reader=[&](const std::string& filename,std::string& out){
        ++reads;
        out=SourceTile(filename=="bathymetry-r00-c00.sitkab"?0:1);
        return true;
    };
    plan.Reset();
    assert(plan.Update(100.,950.,20.,&cache,true));
    assert(!plan.desired().sourced);
    plan.Commit();
    assert(cache.Pump(100.,950.,4.,0.,reader));
    assert(plan.Update(100.,950.,20.,&cache,true));
    assert(plan.desired().sourced&&plan.desired().source_region==0);
    plan.Commit();
    assert(!plan.Update(100.,950.,20.,&cache,true));
    assert(cache.Pump(190.,950.,4.,0.,reader)); // prefetch source 1
    assert(!plan.Update(200.1,950.,20.,&cache,true)); // seam jitter
    assert(plan.Update(209.,950.,20.,&cache,true));
    assert(plan.desired().source_region==1);
    plan.Commit();
    for(int i=0;i<120;++i)
        assert(!plan.Update(i%2?199.9:200.1,950.,20.,&cache,true));
    assert(plan.committed_swaps()==3u);
    assert(reads==2);
    assert(plan.Update(191.,950.,20.,&cache,true));
    plan.Commit();

    plan.Reset();
    assert(plan.ChooseLod(20.)==1u);
    assert(plan.ChooseLod(63.)==1u);
    assert(plan.ChooseLod(65.)==2u);
    assert(plan.ChooseLod(45.)==2u);
    assert(plan.ChooseLod(43.)==1u);
    assert(plan.ChooseLod(65.)==2u);
    assert(plan.ChooseLod(181.)==4u);
    assert(plan.ChooseLod(150.)==4u);
    assert(plan.ChooseLod(139.)==2u);
    assert(plan.Update(100.,950.,18.,&cache,true));
    plan.Reject(); // invalid GL data must not retry every frame
    for(int i=0;i<30;++i)
        assert(!plan.Update(100.,950.,18.,&cache,true));
    assert(plan.Update(100.,950.,18.,&cache,true));
    plan.Commit();
    assert(!plan.Update(100.,950.,18.,&cache,true));
    std::cout<<"PASS Sitka GL-frame ABI guard, 256m/200m tile seam hysteresis, source arrival and 1/2/4 LOD cooldown\n";
}
