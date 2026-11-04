// Source-only native terrain proof. Not registered as a playable game until
// licensed Sitka geography, shell content scope and real activities exist.
#include "sitka_terrain_gl.h"
#include "sitka_water_gl.h"
#include "sitka_fishing_session.h"
#include "sitka_fishing_spots.h"
#include "sitka_bathymetry.h"
#include "sitka_bathymetry_stream.h"
#include "sitka_procedural_seabed.h"
#include "sitka_seafloor_mesh.h"
#include "sitka_seafloor_frame_plan.h"
#include "sitka_seafloor_sparse_mesh.h"
#include <array>
#include <cstdint>
#include <new>
#include <memory>
#include <cmath>
#include <string>
#include <vector>

namespace {
struct State {
    const ReverieNativeHostV1* host = nullptr;
    reverie::sitka::TerrainMesh terrain;
    reverie::sitka::Lighting lighting;
    reverie::sitka::TerrainGl gl;
    std::vector<reverie::sitka::Water> waters;
    reverie::sitka::Weather weather;
    reverie::sitka::WaterGl water_gl;
    std::vector<reverie::sitka::FishingSpot> fishing_spots;
    reverie::sitka::FishingSession fishing;
    reverie::sitka::BathyAtlas bathymetry; // V1 source compatibility.
    std::unique_ptr<reverie::sitka::BathyStreamCache> stream;
    std::unique_ptr<reverie::sitka::ProceduralSeabed> generated;
    reverie::sitka::SeafloorFramePlanner seafloor_plan;
    double last_focus_east=0.,last_focus_north=0.;
    bool has_focus=false;
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
            state->lighting=reverie::sitka::ParseLighting(ReadText(
                host,"sitka/lighting/sitka.properties",4096u));
            state->waters=reverie::sitka::ParseWater(ReadText(
                host,"sitka/water/sitka.water",2000000u));
            state->weather=reverie::sitka::ParseWeather(ReadText(
                host,"sitka/weather/sitka.properties",4096u));
            // Source-only adapter: no synthetic seabed or global interpolation.
            // A renderer cannot advertise a depth until vetted native tiles
            // have been staged under this host-private asset prefix.
            const auto index_bytes=ReadText(host,"sitka/bathymetry/index.txt",
                static_cast<uint32_t>(reverie::sitka::kBathyStreamMaxIndexBytes));
            const uint32_t max_bathy_bytes=static_cast<uint32_t>(
                reverie::sitka::kBathyHeaderBytes+
                size_t(reverie::sitka::kBathyMaxAxis)*
                size_t(reverie::sitka::kBathyMaxAxis)*5u);
            if(index_bytes.compare(0,20,"SITKA-BATHY-INDEX-2\n")==0){
                state->stream.reset(new reverie::sitka::BathyStreamCache(
                    reverie::sitka::ParseBathyStreamIndex(index_bytes)));
                // This small guide file is deterministic, derived offline
                // from the approved raster. No runtime RNG and no tile
                // load-order influence on generated bathymetric cells.
                const std::string guide=ReadText(host,
                    "sitka/bathymetry/procedural-guides.txt",32000u);
                state->generated.reset(new reverie::sitka::ProceduralSeabed(
                    reverie::sitka::ParseSeabedGeneration(
                        guide,state->stream->directory().datum_id)));
                // One initial cell at the available terrain origin, if covered;
                // never decode all source tiles at launch. Later updates follow
                // the editor/listener; real boat binding is a separate milestone.
                const double first_east=state->terrain.origin_east;
                const double first_north=state->terrain.origin_north;
                state->stream->Pump(first_east,first_north,0.,0.,
                    [&](const std::string& name,std::string& out){
                        out=ReadText(host,("sitka/bathymetry/"+name).c_str(),
                                     max_bathy_bytes);
                        return true;
                    });
            }else{
                // V1 compatibility: previously authored small, eager atlases.
                // New exported worlds use V2 and bounded lazy loading instead.
                const auto index=reverie::sitka::ParseBathyIndex(index_bytes);
                for(const auto& tile_name:index.filenames){
                    const auto filename="sitka/bathymetry/"+tile_name;
                    const auto payload=ReadText(host,filename.c_str(),max_bathy_bytes);
                    auto tile=reverie::sitka::ParseBathymetry(
                        reinterpret_cast<const uint8_t*>(payload.data()),
                        payload.size(),index.datum_id);
                    if(!state->bathymetry.Add(std::move(tile)))
                        throw std::invalid_argument("Sitka legacy tile overlap");
                }
            }
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
    // Keep legacy source-only V1 terrain compatibility. V2 streaming
    // worlds start with land and water; the optional GL frame callback
    // commits only the active seafloor after CPU cache update.
    state->seafloor_plan.Reset();
    std::vector<reverie::sitka::TerrainMesh> sea_floor;
    if(!state->stream){
        try{
            for(const auto& tile:state->bathymetry.Tiles()){
                auto mesh=reverie::sitka::BuildSeafloorTerrain(
                    tile,state->terrain.origin_east,state->terrain.origin_north);
                if(!mesh.indices.empty())sea_floor.push_back(std::move(mesh));
            }
        }catch(const std::exception& e){
            Log(state->host,e.what());return 0;
        }
    }
    if(!state->gl.CreateWithExtras(state->terrain,sea_floor,
                                   state->lighting,&error)){
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
        static_cast<State*>(instance)->seafloor_plan.Reset();
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
    if(state->stream &&
       input->struct_size>=REVERIE_NATIVE_INPUT_V1_LISTENER_MIN_SIZE &&
       input->listener_valid){
        // This source-only preview follows the headset/editor world focus.
        // Actual boat mode will instead provide SkiffNavigation's world pose.
        // The host-scoped reader is synchronous: cap at ONE tile per update,
        // never touch a GL buffer and never read data from an eye render.
        const double east=state->terrain.origin_east+input->listener_position[0];
        const double north=state->terrain.origin_north+input->listener_position[2];
        double vx=0.,vn=0.;
        const double dt=input->delta_seconds;
        if(state->has_focus&&std::isfinite(dt)&&dt>.001&&dt<=.1){
            vx=(east-state->last_focus_east)/dt;
            vn=(north-state->last_focus_north)/dt;
            if(std::hypot(vx,vn)>8.)vx=vn=0.;
        }
        state->stream->Pump(east,north,vx,vn,
            [&](const std::string& name,std::string& out){
                out=ReadText(state->host,("sitka/bathymetry/"+name).c_str(),
                             static_cast<uint32_t>(
                                reverie::sitka::kBathyHeaderBytes+
                                size_t(reverie::sitka::kBathyMaxAxis)*
                                size_t(reverie::sitka::kBathyMaxAxis)*5u));
                return true;
            });
        state->last_focus_east=east;state->last_focus_north=north;
        state->has_focus=true;
    }
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
bool WaterAt(const State* state,double east,double north){
    if(!state||!std::isfinite(east)||!std::isfinite(north)||
       std::abs(east)>1.e8||std::abs(north)>1.e8)return false;
    for(const auto& water:state->waters){
        if(reverie::sitka::FootprintContains(
               water,static_cast<float>(east),static_cast<float>(north)))
            return true;
    }
    return false;
}

// The host invokes this optional ABI callback ONCE after update and before
// drawing either eye, from the owning GLES context. No synchronous asset
// read occurs here; only resident source tiles or seeded infill are used.
int32_t PrepareFrame(void* instance){
    State* state=static_cast<State*>(instance);
    if(!state||!state->gl.ready())return 0;
    if(!state->stream||!state->generated||!state->has_focus)return 1;
    const double east=state->last_focus_east;
    const double north=state->last_focus_north;
    const bool water=WaterAt(state,east,north);
    const auto bed=state->stream->Sample(east,north);
    const auto fallback=water?state->generated->Sample(east,north):
                         reverie::sitka::BedSample{};
    const auto sample=bed.known?bed:fallback;
    const double reference=state->generated->settings().water_reference_m;
    const double distance=sample.known?
        std::abs(reference-sample.elevation_m):80.;
    if(!state->seafloor_plan.Update(east,north,distance,
                                    state->stream.get(),water))return 1;
    const auto key=state->seafloor_plan.desired();
    std::vector<reverie::sitka::TerrainMesh> seafloor;
    try{
        if(key.valid){
            const auto mask=[&](double x,double z){
                return WaterAt(state,x,z);
            };
            reverie::sitka::TerrainMesh mesh;
            if(key.sourced){
                const auto* source=state->stream->ResidentAt(east,north);
                if(!source)return 1; // Retry when a cache tile is replaced.
                // Build only the selected LOD samples: do not generate
                // ~263k full-resolution pixels to discard 15/16 of them.
                // Check every underlying authored water-mask cell before
                // accepting each coarse quad, including tiny land islands.
                mesh=reverie::sitka::BuildHybridSeafloorLod(
                    *source,*state->generated,
                    state->terrain.origin_east,state->terrain.origin_north,
                    key.stride,mask);
            }else{
                const auto tile=state->generated->GenerateTile(
                    east,north,mask);
                mesh=reverie::sitka::BuildSeafloorTerrain(
                    tile,state->terrain.origin_east,state->terrain.origin_north,
                    key.stride);
            }
            if(!mesh.indices.empty())seafloor.push_back(std::move(mesh));
        }
    }catch(const std::exception& e){
        Log(state->host,e.what());
        state->seafloor_plan.Reject();
        return 1; // Never take down the headset for bad optional geography.
    }
    std::string error;
    if(!state->gl.ReplaceSeafloor(seafloor,&error)){
        Log(state->host,error.c_str());
        state->seafloor_plan.Reject();
        return 1; // Retain old GPU floor, retry with bounded backoff.
    }
    state->seafloor_plan.Commit();
    return 1;
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
    Create, Destroy, OnGlCreated, ReleaseGl, Resume, Pause, Update, RenderEye,
    nullptr, PrepareFrame
};
} // namespace
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1* reverie_native_module_entry_v1(void) { return &kApi; }
