// Source-only native terrain proof. Not registered as a playable game until
// licensed Sitka geography, shell content scope and real activities exist.
#include "sitka_terrain_gl.h"
#include "sitka_water_gl.h"
#include "sitka_fishing_session.h"
#include "sitka_fishing_spots.h"
#include <array>
#include <cstdint>
#include <new>
#include <string>
#include <vector>

namespace {
struct State {
    const ReverieNativeHostV1* host = nullptr;
    reverie::sitka::TerrainMesh terrain;
    reverie::sitka::TerrainGl gl;
    std::vector<reverie::sitka::Water> waters;
    reverie::sitka::Weather weather;
    reverie::sitka::WaterGl water_gl;
    std::vector<reverie::sitka::FishingSpot> fishing_spots;
    reverie::sitka::FishingSession fishing;
};
void Log(const ReverieNativeHostV1* host, const char* message) {
    ReverieNativeLogV1(host, REVERIE_NATIVE_LOG_ERROR, "SitkaTerrain", message);
}
std::string ReadText(const ReverieNativeHostV1* host,const char* path,uint32_t limit){
    uint32_t size=0u;
    const int32_t probe=host->read_asset(path,nullptr,0u,&size);
    if((probe!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL &&
        probe!=REVERIE_NATIVE_SAVE_OK)||size==0u||size>limit)
        throw std::invalid_argument(std::string("Missing/oversized Sitka asset: ")+path);
    std::vector<uint8_t> bytes(size);
    uint32_t actual=0u;
    if(host->read_asset(path,bytes.data(),size,&actual)!=REVERIE_NATIVE_SAVE_OK ||
       actual!=size)
        throw std::invalid_argument(std::string("Incomplete Sitka asset: ")+path);
    return std::string(bytes.begin(),bytes.end());
}
// The shell owns isolated module save slots; no raw filesystem access.
int ReadFishingSave(void* user,std::string& content) {
    auto* state=static_cast<State*>(user);
    uint32_t size=0u;
    const int32_t code=state->host->read_save("sitka-fishing-v1.sav",nullptr,0u,&size);
    if(code==REVERIE_NATIVE_SAVE_NOT_FOUND)return 0;
    if(code!=REVERIE_NATIVE_SAVE_OK||size==0u||size>128u)return -1;
    std::array<char,128> bytes{};
    uint32_t actual=0u;
    if(state->host->read_save("sitka-fishing-v1.sav",bytes.data(),
       static_cast<uint32_t>(bytes.size()),&actual)!=REVERIE_NATIVE_SAVE_OK||
       actual!=size)return -1;
    content.assign(bytes.data(),bytes.data()+actual);return 1;
}
bool WriteFishingSave(void* user,const std::string& text) {
    auto* state=static_cast<State*>(user);
    return text.size()<=128u &&
        state->host->write_save("sitka-fishing-v1.sav",text.data(),
          static_cast<uint32_t>(text.size()))==REVERIE_NATIVE_SAVE_OK;
}
reverie::sitka::FishingStore Store(State* state) {
    return {state,ReadFishingSave,WriteFishingSave};
}
void* Create(const ReverieNativeHostV1* host) {
    if (!ReverieNativeHostSupportsLogV1(host) ||
        !ReverieNativeHostSupportsSaveV1(host) ||
        host->struct_size < REVERIE_NATIVE_HOST_V1_ASSET_MIN_SIZE ||
        host->read_asset == nullptr) return nullptr;
    // The host's game-private read_asset service enforces path/size scope.
    // No synthetic geographic fallback is ever used in the installed game.
    const char* path = "sitka/coast/terrain.sitkam";
    uint32_t size = 0u;
    const int32_t probe = host->read_asset(path, nullptr, 0u, &size);
    const size_t max_bytes = reverie::sitka::kTerrainHeaderBytes +
        size_t(reverie::sitka::kMaxTerrainVertices)*reverie::sitka::kTerrainVertexBytes +
        size_t(reverie::sitka::kMaxTerrainTriangles)*12u;
    if ((probe != REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL &&
         probe != REVERIE_NATIVE_SAVE_OK) || size < 32u || size > max_bytes) {
        Log(host, "No bounded, approved Sitka terrain asset is available.");
        return nullptr;
    }
    std::vector<uint8_t> bytes(size);
    uint32_t actual = 0u;
    if (host->read_asset(path, bytes.data(), size, &actual) != REVERIE_NATIVE_SAVE_OK ||
        actual != size) {
        Log(host, "Sitka terrain asset read was incomplete.");
        return nullptr;
    }
    try {
        State* state = new State;
        state->host = host;
        try {
            state->terrain=reverie::sitka::ParseTerrainMesh(bytes.data(),bytes.size());
            state->waters=reverie::sitka::ParseWater(ReadText(
                host,"sitka/water/sitka.water",2000000u));
            state->weather=reverie::sitka::ParseWeather(ReadText(
                host,"sitka/weather/sitka.properties",4096u));
            if(state->waters.empty())throw std::invalid_argument("No authored Sitka water");
            state->fishing_spots=reverie::sitka::ParseFishingSpots(ReadText(
                host,"sitka/fishing/sites.txt",8192u));
            for(const auto& site:state->fishing_spots){
                bool in_water=false;
                for(const auto& water:state->waters)
                    if(water.id==site.water_id &&
                       reverie::sitka::FootprintContains(water,site.east,site.north))
                        in_water=true;
                if(!in_water)throw std::invalid_argument(
                    "Sitka fishing site outside named water footprint");
            }
            const auto loaded=state->fishing.Load(Store(state));
            if(loaded!=reverie::sitka::FishingLoad::Fresh &&
               loaded!=reverie::sitka::FishingLoad::Restored)
                throw std::invalid_argument(
                    "Sitka fishing save unavailable or invalid: preserve existing data");
        }
        catch (...) { delete state; throw; }
        return state;
    } catch (const std::exception& e) {
        Log(host, e.what());
        return nullptr;
    }
}
void Destroy(void* instance) {
    auto* state=static_cast<State*>(instance);
    if(state&&!state->fishing.Flush(Store(state)))
        Log(state->host,"Sitka journal flush failed on exit");
    delete state;
}
int32_t OnGlCreated(void* instance) {
    State* state = static_cast<State*>(instance);
    if (!state) return 0;
    std::string error;
    if(!state->gl.Create(state->terrain,&error)){
        Log(state->host,error.c_str());return 0;
    }
    if(state->water_gl.Create(state->waters,state->weather,&error))return 1;
    state->gl.Release();
    Log(state->host,error.c_str());
    return 0;
}
void ReleaseGl(void* instance) {
    if(instance){
        static_cast<State*>(instance)->water_gl.Release();
        static_cast<State*>(instance)->gl.Release();
    }
}
void Resume(void*) {}
void Pause(void* instance) {
    auto* state=static_cast<State*>(instance);
    if(state&&!state->fishing.Flush(Store(state)))
        Log(state->host,"Sitka journal flush failed on pause");
}
void Update(void* instance,const ReverieNativeInputV1* input){
    auto* state=static_cast<State*>(instance);
    if(!state||!ReverieNativeInputHasBaseV1(input))return;
    state->water_gl.Update(input->delta_seconds);
    reverie::sitka::FishingCastTarget target;
    if(input->primary_down && ReverieNativeInputHasPointerV1(input)){
        ReverieNativeInputV1 normalized=*input;
        if(ReverieNativeSanitizePointerV1(&normalized)){
            const float* p=normalized.pointer_origin;
            const float* d=normalized.pointer_direction;
            target=reverie::sitka::FindFishingTarget(
                reverie::sitka::FishingRay{p[0],p[1],p[2],d[0],d[1],d[2]},
                state->fishing_spots,state->waters);
        }
    }
    const auto event=state->fishing.Update(input->delta_seconds,
        input->primary_down!=0u,input->secondary_down!=0u,
        target.valid,target.depth_metres);
    if(event.journal_changed&&!state->fishing.Flush(Store(state)))
        Log(state->host,"Sitka journal write failed; pending pause/exit retry");
    if(event.event==reverie::sitka::FishingEvent::CastRejected)
        ReverieNativeRequestFeedbackV1(state->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
    else if(event.event==reverie::sitka::FishingEvent::Bite)
        ReverieNativeRequestFeedbackV1(state->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
    else if(event.event==reverie::sitka::FishingEvent::Cast ||
            event.event==reverie::sitka::FishingEvent::Landed)
        ReverieNativeRequestFeedbackV1(state->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
}
int32_t RenderEye(void* instance, const ReverieNativeEyeV1* eye) {
    if(!instance)return 0;
    State* state=static_cast<State*>(instance);
    return state->gl.RenderEye(eye)&&state->water_gl.RenderEye(eye)?1:0;
}
const ReverieNativeModuleApiV1 kApi = {
    sizeof(ReverieNativeModuleApiV1), REVERIE_NATIVE_MODULE_ABI_VERSION,
    {sizeof(ReverieNativeModuleDescriptorV1), REVERIE_NATIVE_MODULE_ABI_VERSION,
     "sitka", "Sitka [TERRAIN PROOF]", 2u, 0u},
    Create, Destroy, OnGlCreated, ReleaseGl, Resume, Pause, Update, RenderEye, nullptr
};
} // namespace
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1* reverie_native_module_entry_v1(void) { return &kApi; }
