// Source-only native terrain proof. Not registered as a playable game until
// licensed Sitka geography, shell content scope and real activities exist.
#include "sitka_terrain_gl.h"
#include "sitka_shore_geometry.h"
#include "sitka_shore_detail.h"
#include "sitka_shore_stream.h"
#include "sitka_walk_graph.h"
#include "sitka_free_roam.h"
#include "sitka_shore_deck_gl.h"
#include "sitka_bridge_layout.h"
#include "sitka_bridge_gl.h"
#include "sitka_bridge_visibility.h"
#include "sitka_water_gl.h"
#include "sitka_fishing_session.h"
#include "sitka_fishing_spots.h"
#include "sitka_beach_discoveries.h"
#include "sitka_city_discoveries.h"
#include "sitka_bathymetry.h"
#include "sitka_bathymetry_stream.h"
#include "sitka_procedural_seabed.h"
#include "sitka_seafloor_mesh.h"
#include "sitka_seafloor_frame_plan.h"
#include "sitka_seafloor_sparse_mesh.h"
#include "sitka_boat_session.h"
#include "sitka_helm.h"
#include "sitka_harbor_shop.h"
#include "sitka_boat_mesh.h"
#include "sitka_boat_panels.h"
#include "reverie_native_lato_text.h"
#include "reverie_native_horizon_gl.h"
#include <array>
#include <cstdint>
#include <new>
#include <memory>
#include <limits>
#include <cmath>
#include <string>
#include <vector>

namespace {
struct State {
    const ReverieNativeHostV1* host = nullptr;
    reverie::sitka::TerrainMesh terrain;
    reverie::sitka::Lighting lighting;
    reverie::sitka::TerrainGl gl;
    reverie::sitka::TerrainGl shore_gl; // changing waterfront overlay, not boat
    std::unique_ptr<reverie::sitka::ShoreStreamPlanner> shore_stream;
    std::unique_ptr<reverie::sitka::WalkGraph> walk_graph;
    std::unique_ptr<reverie::sitka::FreeRoamSession> walk_session;
    std::vector<reverie::sitka::ShoreFeature> shore_colliders;
    reverie::sitka::TerrainMesh walk_mesh;
    reverie::sitka::TerrainGl walk_gl; // static sourced walking corridors
    std::unique_ptr<reverie::sitka::ShoreDeckMesh> shore_deck_mesh;
    reverie::omega_art::Image shore_deck_art;
    reverie::sitka::ShoreDeckGl shore_deck_gl;
    std::unique_ptr<reverie::sitka::BridgePlaneMesh> bridge_mesh;
    std::unique_ptr<reverie::sitka::BridgeVisibility> bridge_view;
    reverie::sitka::BridgeArt bridge_art;
    reverie::sitka::BridgeGl bridge_gl; // 3 surface textures, one upload per GL context
    reverie::sitka::TerrainGl boat_gl; // isolated static boat VBO, shared across both eyes
    reverie::sitka::TerrainGl harbor_gl;
    reverie::sitka::TerrainGl harbor_panel_gl;
    reverie::nativetext::LatoLayer harbor_text;
    std::vector<reverie::sitka::HarborShopSite> harbor_sites;
    reverie::sitka::HarborShopSession harbor_shop;
    std::string harbor_last_text;
    int last_aimed_shop=-1;
    float shop_rx=1.f,shop_rz=0.f,shop_y=1.5f;
    bool boat_dirty=false;
    reverie::nativetext::LatoLayer boat_text; // real licensed glyph atlas
    std::string last_boat_text;
    std::vector<reverie::sitka::Water> waters;
    reverie::sitka::Weather weather;
    reverie::sitka::WaterGl water_gl;
    std::array<reverie::omega_art::Image,reverie::native::horizon::kTheaters> horizon_art{};
    reverie::native::horizon::Renderer horizon_gpu;
    std::vector<reverie::sitka::FishingSpot> fishing_spots;
    reverie::sitka::FishingSession fishing;
    std::vector<reverie::sitka::BeachSite> beach_sites;
    std::vector<reverie::sitka::CitySite> city_sites;
    reverie::sitka::TerrainGl beach_gl;
    bool beach_dirty=false,beach_primary_latched=false;
    uint64_t beach_visible_mask=0u;
    reverie::sitka::BathyAtlas bathymetry; // V1 source compatibility.
    std::unique_ptr<reverie::sitka::BathyStreamCache> stream;
    std::unique_ptr<reverie::sitka::ProceduralSeabed> generated;
    std::unique_ptr<reverie::sitka::SitkaSkiffSession> skiff;
    reverie::sitka::HelmProgress helm;
    reverie::sitka::HelmLayout helm_layout;
    reverie::sitka::HelmRudder helm_rudder;
    reverie::sitka::DeckPassenger passenger;
    reverie::sitka::HelmInstruments instruments;
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
    if(code!=REVERIE_NATIVE_SAVE_OK||size==0u||size>512u)return -1;
    std::array<char,512> bytes{};
    uint32_t actual=0u;
    if(state->host->read_save("sitka-fishing-v1.sav",bytes.data(),
       static_cast<uint32_t>(bytes.size()),&actual)!=REVERIE_NATIVE_SAVE_OK||
       actual!=size)return -1;
    content.assign(bytes.data(),bytes.data()+actual);return 1;
}
bool WriteFishingSave(void* user,const std::string& text) {
    auto* state=static_cast<State*>(user);
    return text.size()<=512u &&
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
        // Optional *sky-only* generated atmosphere. A painted coastline
        // is not Sitka geography; the lower hemisphere is never drawn.
        reverie::native::horizon::Load(host,0u,state->horizon_art[0]);
        try {
            state->terrain=reverie::sitka::ParseTerrainMesh(bytes.data(),bytes.size());
            // Optional, strictly scoped, source-anchored shore massing.
            // Missing means no buildings/docks: never fabricate GIS data.
            const char* shore_path="sitka/shore/waterfront.sitkash";
            uint32_t shore_bytes=0u;
            const int32_t shore_probe=host->read_asset(
                shore_path,nullptr,0u,&shore_bytes);
            if(shore_probe!=REVERIE_NATIVE_SAVE_NOT_FOUND){
                if((shore_probe!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL &&
                    shore_probe!=REVERIE_NATIVE_SAVE_OK) ||
                   shore_bytes<14u ||
                   shore_bytes>reverie::sitka::kMaxShoreTextBytes)
                    throw std::invalid_argument("Invalid Sitka shore asset size");
                const auto shore=reverie::sitka::ParseShoreLayout(
                    ReadText(host,shore_path,
                             static_cast<uint32_t>(
                                reverie::sitka::kMaxShoreTextBytes)));
                // Keep immutable source features separate from the terrain:
                // only nearby P0/P1 shore meshes become resident GPU batches.
                if(!shore.empty()){
                    // Keep exact authored buildings for free-roam collision.
                    state->shore_colliders=shore;
                    state->shore_stream.reset(
                        new reverie::sitka::ShoreStreamPlanner(shore));
                    // Build flat textured float tops once from the SAME
                    // source features. No fake rectangular piers or window art.
                    auto deck=reverie::sitka::BuildShoreDeckPlanes(shore);
                    if(!deck.vertices.empty()){
                        reverie::omega_art::Image art;
                        if(reverie::sitka::LoadShoreDeckArt(host,art)){
                            state->shore_deck_mesh.reset(
                                new reverie::sitka::ShoreDeckMesh(std::move(deck)));
                            state->shore_deck_art=std::move(art);
                        }else{
                            Log(host,"Approved shared dock-plank texture missing; harbor tops remain P0");
                        }
                    }
                }
            }
            // Optional sourced pedestrian graph. Missing means NO verified
            // walk corridors or public shore-fishing stance, not an invented
            // rectangular/citywide navigation fallback.
            const char* walk_path="sitka/walk/paths.sitkaw";
            uint32_t walk_size=0u;
            const int32_t walk_probe=host->read_asset(
                walk_path,nullptr,0u,&walk_size);
            if(walk_probe!=REVERIE_NATIVE_SAVE_NOT_FOUND){
                if((walk_probe!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL &&
                    walk_probe!=REVERIE_NATIVE_SAVE_OK)||
                   walk_size<80u||walk_size>reverie::sitka::kMaxWalkBytes)
                    throw std::invalid_argument("Invalid sourced Sitka walking graph size");
                auto graph=reverie::sitka::WalkGraph::Parse(
                    ReadText(host,walk_path,static_cast<uint32_t>(
                        reverie::sitka::kMaxWalkBytes)),
                    state->terrain.origin_east,state->terrain.origin_north);
                state->walk_mesh=graph.BuildPathMesh();
                state->walk_graph.reset(
                    new reverie::sitka::WalkGraph(std::move(graph)));
            }
            // Optional sourced O'Connell profile; no guessed GIS coordinates.
            // All textures are decoded once during Create, not in RenderEye.
            const char* bridge_path="sitka/structures/oconnell.bridge";
            uint32_t bridge_size=0u;
            const int32_t bridge_probe=host->read_asset(
                bridge_path,nullptr,0u,&bridge_size);
            if(bridge_probe!=REVERIE_NATIVE_SAVE_NOT_FOUND){
                if((bridge_probe!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL &&
                    bridge_probe!=REVERIE_NATIVE_SAVE_OK) ||
                   bridge_size<20u ||
                   bridge_size>reverie::sitka::kMaxBridgeLayoutBytes)
                    throw std::invalid_argument("Invalid Sitka bridge layout size");
                auto geometry=reverie::sitka::ParseBridgeLayout(
                    ReadText(host,bridge_path,static_cast<uint32_t>(
                        reverie::sitka::kMaxBridgeLayoutBytes)),
                    state->terrain.origin_east,state->terrain.origin_north);
                reverie::sitka::BridgeArt art;
                if(reverie::sitka::LoadBridgeArt(host,art)){
                    state->bridge_mesh.reset(
                        new reverie::sitka::BridgePlaneMesh(std::move(geometry)));
                    state->bridge_view.reset(
                        new reverie::sitka::BridgeVisibility(*state->bridge_mesh));
                    state->bridge_art=std::move(art);
                }else{
                    // A missing/bad optional bridge texture must not fabricate
                    // a solid bridge silhouette or crash the rest of Sitka.
                    Log(host,"Approved O'Connell bridge textures missing/invalid; bridge omitted");
                }
            }
            state->lighting=reverie::sitka::ParseLighting(ReadText(
                host,"sitka/lighting/sitka.properties",4096u));
            state->waters=reverie::sitka::ParseWater(ReadText(
                host,"sitka/water/sitka.water",2000000u));
            // Grounded free roam uses the same surveyed land and structures
            // as rendering, even when no authored footpath network exists.
            // A reviewed path spawn is preferred when one is installed.
            const bool has_walk_spawn=bool(state->walk_graph);
            double start_x=0.,start_z=0.;
            if(has_walk_spawn){
                const auto& spawn=state->walk_graph->nodes[
                    state->walk_graph->spawn];
                start_x=spawn.x;start_z=spawn.z;
            }
            state->walk_session.reset(new reverie::sitka::FreeRoamSession(
                state->terrain,state->waters,state->shore_colliders,
                has_walk_spawn,start_x,start_z));
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
            state->helm=reverie::sitka::HelmFromJournal(state->fishing.fishing.journal);
            state->helm_layout=reverie::sitka::AssembleHelm(state->helm);
            if(loaded==reverie::sitka::FishingLoad::Fresh&&
               !state->fishing.Flush(Store(state)))
                Log(host,"Sitka starter inventory save pending retry");
            if(loaded!=reverie::sitka::FishingLoad::Fresh &&
               loaded!=reverie::sitka::FishingLoad::Restored)
                throw std::invalid_argument(
                    "Sitka fishing save unavailable or invalid: preserve existing data");
            // The optional authored beach-site anchors are NOT real-world
            // assertions that a shell/crab/rock exists at that spot.
            // Their coordinates must be reviewed and on dry source terrain.
            const char* beach_path="sitka/beach/discoveries.txt";
            uint32_t beach_size=0u;
            const int32_t beach_probe=host->read_asset(
                beach_path,nullptr,0u,&beach_size);
            if(beach_probe!=REVERIE_NATIVE_SAVE_NOT_FOUND){
                if((beach_probe!=REVERIE_NATIVE_SAVE_OK &&
                    beach_probe!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL)||
                   beach_size<100u||
                   beach_size>reverie::sitka::kMaxBeachAssetBytes)
                    throw std::invalid_argument("Invalid Sitka beach-site asset size");
                state->beach_sites=reverie::sitka::ParseBeachSites(
                    ReadText(host,beach_path,
                        static_cast<uint32_t>(reverie::sitka::kMaxBeachAssetBytes)),
                    state->terrain.origin_east,state->terrain.origin_north);
                for(const auto& site:state->beach_sites)
                    if(!state->walk_session->GroundAt(site.x,site.z).valid)
                        throw std::invalid_argument(
                            "Sitka discovery anchor not on dry source land");
                state->beach_dirty=true;
            }
            // Public city visits are source-authorized; an absent file is
            // valid and never implies a fabricated building or landmark.
            const char* city_path="sitka/city/discoveries.txt";
            uint32_t city_size=0u;
            const int32_t city_probe=host->read_asset(
                city_path,nullptr,0u,&city_size);
            if(city_probe!=REVERIE_NATIVE_SAVE_NOT_FOUND){
                if((city_probe!=REVERIE_NATIVE_SAVE_OK &&
                    city_probe!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL)||
                   city_size<100u||city_size>8192u)
                    throw std::invalid_argument("Sitka city discovery asset size");
                state->city_sites=reverie::sitka::ParseCitySites(
                    ReadText(host,city_path,8192u),
                    state->terrain.origin_east,state->terrain.origin_north);
                for(const auto& site:state->city_sites)
                    if(!state->walk_session->GroundAt(site.x,site.z).valid)
                        throw std::invalid_argument(
                            "Sitka city anchor outside dry source terrain");
            }
            // Only the Thomsen real office is verified; other sites are
            // gameplay desks requiring exact, sourced harbor structures.
            const char* shop_path="sitka/harbor/shops.txt";
            uint32_t shop_bytes=0u;
            const int32_t shop_probe=host->read_asset(
                shop_path,nullptr,0u,&shop_bytes);
            if(shop_probe!=REVERIE_NATIVE_SAVE_NOT_FOUND){
                if((shop_probe!=REVERIE_NATIVE_SAVE_OK &&
                    shop_probe!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL)||
                   shop_bytes<110u||
                   shop_bytes>reverie::sitka::kMaxHarborShopBytes)
                    throw std::invalid_argument("Invalid Sitka harbor shop asset");
                state->harbor_sites=reverie::sitka::ParseHarborShops(
                    ReadText(host,shop_path,
                        static_cast<uint32_t>(reverie::sitka::kMaxHarborShopBytes)),
                    state->terrain.origin_east,state->terrain.origin_north);
                for(const auto& site:state->harbor_sites)
                    if(!reverie::sitka::VerifiedHarborSite(
                        site,*state->walk_session,state->shore_colliders))
                        throw std::invalid_argument(
                            "Sitka shop not at reviewed land/shore feature");
            }
            // A skiff berth must be explicitly authored in projected metres.
            // Its absence is a supported editor/fishing-only preview. A
            // malformed PRESENT berth fails closed rather than inventing a
            // launch point or a named water region.
            if(state->stream && state->generated){
                const char* boat_path="sitka/boat/launch.txt";
                uint32_t boat_bytes=0u;
                const int32_t code=host->read_asset(
                    boat_path,nullptr,0u,&boat_bytes);
                if(code!=REVERIE_NATIVE_SAVE_NOT_FOUND){
                    const auto spec=reverie::sitka::ParseSkiffLaunch(
                        ReadText(host,boat_path,4096u),
                        state->stream->directory().datum_id);
                    if(!reverie::sitka::SitkaNamedWorldWater(
                           state->waters,spec.water_id,
                           state->terrain.origin_east,state->terrain.origin_north,
                           spec.east,spec.north))
                        throw std::invalid_argument(
                            "Sitka skiff berth outside its named water footprint");
                    state->skiff.reset(new reverie::sitka::SitkaSkiffSession(spec));
                    if(!state->skiff->SetHullTier(state->helm.hull))
                        throw std::invalid_argument("Invalid hull tier for boat launch");
                }
            }
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
    if(state->bridge_view)state->bridge_view->Reset();
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
    if(!state->water_gl.Create(state->waters,state->weather,&error)){
        state->gl.Release();
        Log(state->host,error.c_str());
        return 0;
    }
    if(state->shore_stream &&
       !state->shore_gl.CreateOverlay(state->lighting,&error)){
        state->water_gl.Release();
        state->gl.Release();
        Log(state->host,error.c_str());
        return 0;
    }
    // Real-source walking segments remain one small static GLES2 layer;
    // this is a visual top surface, not an invented path/collision shortcut.
    if(!state->beach_sites.empty() &&
       !state->beach_gl.CreateOverlay(state->lighting,&error)){
        state->shore_gl.Release();
        state->water_gl.Release();
        state->gl.Release();
        Log(state->host,error.c_str());
        return 0;
    }
    {
        const auto signs=state->walk_session?
            reverie::sitka::HarborShopSigns(state->harbor_sites,*state->walk_session):
            reverie::sitka::TerrainMesh{};
        if((!state->harbor_sites.empty()&&
            !state->harbor_gl.Create(signs,state->lighting,&error))||
           !state->harbor_panel_gl.Create(
                reverie::sitka::HarborShopPanelMesh(),
                state->lighting,&error)||
           !state->harbor_text.Create()){
            state->harbor_text.Release();
            state->harbor_panel_gl.Release();
            state->harbor_gl.Release();
            state->beach_gl.Release();
            state->shore_gl.Release();
            state->water_gl.Release();
            state->gl.Release();
            Log(state->host,error.empty()?
                "Sitka harbor shop GL/font initialization failed":error.c_str());
            return 0;
        }
    }
    if(state->walk_graph &&
       !state->walk_gl.Create(state->walk_mesh,state->lighting,&error)){
        state->shore_deck_gl.Release();
        state->shore_gl.Release();
        state->water_gl.Release();
        state->gl.Release();
        Log(state->host,error.c_str());
        return 0;
    }
    // A SINGLE retained shared plank texture + VBO, with draw ranges for
    // actual authored float polygons. A missing optional material cannot
    // take down approved land, water or the boat workstream.
    if(state->shore_deck_mesh &&
       !state->shore_deck_gl.Create(*state->shore_deck_mesh,
                                   state->shore_deck_art,
                                   state->lighting.ambient_scale,&error)){
        Log(state->host,error.c_str());
        state->shore_deck_gl.Release();
    }
    // Separate textured geometry: static image atlas and VBOs, shared by eyes.
    if(state->bridge_mesh &&
       !state->bridge_gl.Create(*state->bridge_mesh,state->bridge_art,
                               state->lighting.ambient_scale,&error)){
        Log(state->host,error.c_str());
        state->bridge_gl.Release(); // optional landmark fails closed
    }
    if(state->skiff){
        try{
            if(!state->boat_gl.Create(
                reverie::sitka::BuildBoatMesh(state->helm),
                state->lighting,&error))
                throw std::runtime_error(error);
            if(!state->boat_text.Create())
                throw std::runtime_error("Sitka Lato instrument font unavailable");
        }catch(const std::exception& e){
            state->boat_text.Release();
            state->boat_gl.Release();
            state->bridge_gl.Release();
            state->shore_deck_gl.Release();
            state->walk_gl.Release();
            state->shore_gl.Release();
            state->water_gl.Release();
            state->gl.Release();
            Log(state->host,e.what());
            return 0;
        }
    }
    // Optional background only. Real Sitka water/terrain remains authoritative.
    std::string horizon_error;
    if(!state->horizon_gpu.Create(state->horizon_art,&horizon_error) &&
       !horizon_error.empty())
        Log(state->host,horizon_error.c_str());
    return 1;
}
void ReleaseGl(void* instance) {
    if(instance){
        static_cast<State*>(instance)->horizon_gpu.Release();
        static_cast<State*>(instance)->boat_text.Release();
        static_cast<State*>(instance)->last_boat_text.clear();
        static_cast<State*>(instance)->harbor_text.Release();
        static_cast<State*>(instance)->harbor_last_text.clear();
        static_cast<State*>(instance)->harbor_panel_gl.Release();
        static_cast<State*>(instance)->harbor_gl.Release();
        static_cast<State*>(instance)->boat_gl.Release();
        static_cast<State*>(instance)->bridge_gl.Release();
        static_cast<State*>(instance)->shore_deck_gl.Release();
        static_cast<State*>(instance)->walk_gl.Release();
        static_cast<State*>(instance)->beach_gl.Release();
        static_cast<State*>(instance)->beach_dirty=true;
        if(static_cast<State*>(instance)->bridge_view)
            static_cast<State*>(instance)->bridge_view->Reset();
        static_cast<State*>(instance)->shore_gl.Release();
        if(static_cast<State*>(instance)->shore_stream)
            static_cast<State*>(instance)->shore_stream->Reset();
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
    const bool listener=input->struct_size>=
        REVERIE_NATIVE_INPUT_V1_LISTENER_MIN_SIZE&&input->listener_valid;
    // Home-style analog strength over actual terrain triangles. Footpath
    // lines are for navigation only, NEVER locomotion permissions. Water
    // and source building footprints still block unsafe movement.
    // Suspend stepping while actively angling or controlling the boat.
    if(state->walk_session && listener &&
       !(state->skiff&&state->skiff->Active()) &&
       !state->harbor_shop.open() &&
       state->fishing.fishing.phase==reverie::sitka::FishingPhase::Ready)
        state->walk_session->Advance(
            input->move_x,-input->move_y,input->listener_right,
            input->delta_seconds,
            reverie::sitka::CharacterWalkScale(
                state->fishing.fishing.journal));
    const auto foot=state->walk_session?
        state->walk_session->pose():reverie::sitka::RoamPose{};
    // Compute five-foot reveal transitions ONCE per update. No per-eye
    // visibility mutation, no rebuild for stationary frames.
    if(state->walk_session && !state->beach_sites.empty()){
        const uint64_t visible=reverie::sitka::BeachVisibleMask(
            state->beach_sites,state->fishing.fishing.journal,
            *state->walk_session);
        if(visible!=state->beach_visible_mask){
            state->beach_visible_mask=visible;
            state->beach_dirty=true;
        }
    }
    const double listener_east=listener?
        state->terrain.origin_east+foot.x+input->listener_position[0]:
        std::numeric_limits<double>::quiet_NaN();
    const double listener_north=listener?
        state->terrain.origin_north+foot.z+input->listener_position[2]:
        std::numeric_limits<double>::quiet_NaN();
    // Periodic city exploration happens while free-roaming, not on
    // an assigned pedestrian graph. Once per immutable authored site.
    if(state->walk_session && listener && !state->city_sites.empty() &&
       !(state->skiff&&state->skiff->Active())){
        const auto found=reverie::sitka::FindCityVisit(
            *state->walk_session,state->city_sites,
            state->fishing.fishing.journal.city_sites,
            state->fishing.fishing.journal.observation_level);
        if(found.valid && state->fishing.DiscoverCity(found.site,found.kind)){
            if(!state->fishing.Flush(Store(state)))
                Log(state->host,"Sitka city visit save deferred");
            ReverieNativeRequestFeedbackV1(
                state->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        }
    }
    // Source-vetted office sign interaction, once per native simulation update.
    int aimed_shop=-1;
    if(state->walk_session&&listener&&!state->harbor_sites.empty()&&
       ReverieNativeInputHasPointerV1(input)){
        ReverieNativeInputV1 ray=*input;
        if(ReverieNativeSanitizePointerV1(&ray)){
            const float source[3]={
                ray.pointer_origin[0]+foot.x,
                ray.pointer_origin[1]+foot.ground_y,
                ray.pointer_origin[2]+foot.z};
            aimed_shop=reverie::sitka::HarborShopTarget(
                state->harbor_sites,*state->walk_session,
                source,ray.pointer_direction);
        }
    }
    const auto shop_event=state->harbor_shop.Update(
        input->primary_down!=0u,input->secondary_down!=0u,
        input->move_y,aimed_shop,state->fishing,
        state->skiff&&state->skiff->Active());
    if(shop_event==reverie::sitka::ShopEvent::Purchased){
        const auto old=state->helm;
        state->helm=reverie::sitka::HelmFromJournal(
            state->fishing.fishing.journal);
        state->helm_layout=reverie::sitka::AssembleHelm(state->helm);
        if(old.hull!=state->helm.hull||old.owned!=state->helm.owned)
            state->boat_dirty=true;
        if(state->skiff&&!state->skiff->Active() &&
           !state->skiff->SetHullTier(state->helm.hull))
            Log(state->host,"Sitka purchased vessel dynamics unavailable");
        if(!state->fishing.Flush(Store(state)))
            Log(state->host,"Sitka harbor purchase save pending retry");
    }
    if(shop_event==reverie::sitka::ShopEvent::Sold||
       shop_event==reverie::sitka::ShopEvent::Submitted){
        if(!state->fishing.Flush(Store(state)))
            Log(state->host,"Sitka harbor transaction save pending retry");
    }
    if(aimed_shop>=0&&aimed_shop!=state->last_aimed_shop&&!state->harbor_shop.open())
        ReverieNativeRequestFeedbackV1(state->host,REVERIE_NATIVE_FEEDBACK_FOCUS);
    state->last_aimed_shop=aimed_shop;
    if(shop_event==reverie::sitka::ShopEvent::Denied)
        ReverieNativeRequestFeedbackV1(
            state->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
    else if(shop_event==reverie::sitka::ShopEvent::Purchased||
            shop_event==reverie::sitka::ShopEvent::Sold||
            shop_event==reverie::sitka::ShopEvent::Submitted||
            shop_event==reverie::sitka::ShopEvent::Next||
            shop_event==reverie::sitka::ShopEvent::Previous||
            shop_event==reverie::sitka::ShopEvent::Open||
            shop_event==reverie::sitka::ShopEvent::Close)
        ReverieNativeRequestFeedbackV1(
            state->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
    const bool boat_chord=state->skiff &&
        !state->harbor_shop.open()&&
        !state->harbor_shop.consumes_primary()&&
        input->primary_down!=0u&&input->secondary_down!=0u;
    // Boat simulation advances first. This is the AUTHORITATIVE world pose
    // once launched, not a camera projection or the headset's temporary
    // editor focus. The same bathymetry cache backs movement, sounder and
    // subsequent GL visual tile selection.
    if(state->skiff&&state->stream&&state->generated){
        const auto event=state->skiff->Update(
            input->delta_seconds,input->move_y,
            state->helm_rudder.Advance(input->delta_seconds,input->move_x),
            boat_chord,listener_east,listener_north,
            *state->stream,*state->generated,state->waters,
            state->terrain.origin_east,state->terrain.origin_north);
        if(state->skiff->Active()){
            state->instruments=reverie::sitka::HelmReadouts(
                state->helm,state->skiff->Pose(),state->weather,
                state->skiff->Sounder().reading(),
                state->helm_rudder.degrees,input->move_y,1.);
            // Build real Lato text only when data changes. This executes
            // once in the simulation tick, never from a stereo-eye callback.
            const auto panels=reverie::sitka::BuildBoatPanels(
                state->helm,state->skiff->Pose(),state->weather,
                state->skiff->Sounder().reading(),input->move_y);
            const std::string signature=panels.Signature();
            if(signature!=state->last_boat_text){
                state->boat_text.Clear();
                for(size_t i=0;i<panels.count;++i){
                    const auto& label=panels.labels[i];
                    const size_t first=state->boat_text.vertices.size();
                    state->boat_text.Fit(label.text.c_str(),
                        label.x,label.y,label.width,label.scale,
                        label.red,label.green,label.blue,label.z);
                    reverie::sitka::OrientBoatGlyphsAft(
                        state->boat_text.vertices,first,label.x);
                }
                state->last_boat_text=signature;
            }
        }else if(event==reverie::sitka::SkiffSessionEvent::Docked){
            state->helm_rudder.Reset();
            state->boat_text.Clear();
            state->last_boat_text.clear();
        }
        if(event==reverie::sitka::SkiffSessionEvent::Launched||
           event==reverie::sitka::SkiffSessionEvent::Docked)
            ReverieNativeRequestFeedbackV1(state->host,
                                            REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        else if(event==reverie::sitka::SkiffSessionEvent::Rejected||
                event==reverie::sitka::SkiffSessionEvent::BlockedLand||
                event==reverie::sitka::SkiffSessionEvent::BlockedShallow||
                event==reverie::sitka::SkiffSessionEvent::BlockedNoData)
            ReverieNativeRequestFeedbackV1(state->host,
                                            REVERIE_NATIVE_FEEDBACK_FAILURE);
    }
    if(state->stream){
        double east=listener_east,north=listener_north;
        double vx=0.,vn=0.;
        if(state->skiff&&state->skiff->Active()){
            east=state->skiff->Pose().east;
            north=state->skiff->Pose().north;
            vx=state->skiff->EastVelocity();
            vn=state->skiff->NorthVelocity();
        }else if(listener){
            const double dt=input->delta_seconds;
            if(state->has_focus&&std::isfinite(dt)&&dt>.001&&dt<=.1){
                vx=(east-state->last_focus_east)/dt;
                vn=(north-state->last_focus_north)/dt;
                if(std::hypot(vx,vn)>8.)vx=vn=0.;
            }
        }
        if(std::isfinite(east)&&std::isfinite(north)){
            // One source request per update; the larger bulk-read operation
            // remains synchronous until the host gains a safe worker API.
            state->stream->Pump(east,north,vx,vn,
                [&](const std::string& name,std::string& out){
                    out=ReadText(state->host,
                        ("sitka/bathymetry/"+name).c_str(),
                        static_cast<uint32_t>(
                            reverie::sitka::kBathyHeaderBytes+
                            size_t(reverie::sitka::kBathyMaxAxis)*
                            size_t(reverie::sitka::kBathyMaxAxis)*5u));
                    return true;
                });
            state->last_focus_east=east;
            state->last_focus_north=north;
            state->has_focus=true;
        }
    }
    // The shore can remain visible in an editor preview without an authored
    // bathymetry cache; this never substitutes for the skiff's world pose.
    if(!state->stream && listener && std::isfinite(listener_east) &&
       std::isfinite(listener_north)){
        state->last_focus_east=listener_east;
        state->last_focus_north=listener_north;
        state->has_focus=true;
    }
    if(!input->primary_down)state->beach_primary_latched=false;
    if(state->harbor_shop.consumes_primary())
        state->beach_primary_latched=true;
    // Point at nearby low-poly beach finds to record them. This uses the
    // same sanitized native controller ray as fishing, but a distinct
    // one-press latch prevents one pickup from becoming a held cast.
    if(state->walk_session && listener &&
       !state->beach_primary_latched &&
       !state->harbor_shop.open() &&
       !(state->skiff&&state->skiff->Active()) &&
       state->fishing.fishing.phase==reverie::sitka::FishingPhase::Ready &&
       input->primary_down && !state->beach_sites.empty() &&
       ReverieNativeInputHasPointerV1(input)){
        ReverieNativeInputV1 ray_input=*input;
        if(ReverieNativeSanitizePointerV1(&ray_input)){
            const float source[3]={
                ray_input.pointer_origin[0]+foot.x,
                ray_input.pointer_origin[1]+foot.ground_y,
                ray_input.pointer_origin[2]+foot.z
            };
            const auto found=reverie::sitka::FindBeachTarget(
                source,ray_input.pointer_direction,foot.x,foot.z,
                *state->walk_session,state->beach_sites,
                state->fishing.fishing.journal.beach_sites,
                state->fishing.fishing.journal.shore_kit_level,
                state->fishing.fishing.journal.observation_level);
            if(found.valid &&
               state->fishing.Discover(found.site,found.kind)){
                state->beach_primary_latched=true;
                state->beach_dirty=true;
                if(!state->fishing.Flush(Store(state)))
                    Log(state->host,"Sitka beach discovery save deferred");
                ReverieNativeRequestFeedbackV1(
                    state->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
            }
        }
    }
    reverie::sitka::FishingCastTarget target;
    if(!state->beach_primary_latched && !boat_chord&&!(state->skiff&&state->skiff->Active())&&
       input->primary_down&&ReverieNativeInputHasPointerV1(input)){
        ReverieNativeInputV1 normalized=*input;
        if(ReverieNativeSanitizePointerV1(&normalized)){
            const float* p=normalized.pointer_origin;
            const float* d=normalized.pointer_direction;
            target=reverie::sitka::FindFishingTarget(
                reverie::sitka::FishingRay{
                    p[0]+foot.x,p[1]+foot.ground_y,p[2]+foot.z,
                    d[0],d[1],d[2]},
                state->fishing_spots,state->waters);
        }
    }
    // The player can fish from ANY grounded accessible shore, not just
    // a prescribed pedestrian or fishing-stance node. Cast still requires
    // a valid sourced water target; broader spot coverage is forthcoming.
    if(target.valid && (!state->walk_session || !listener))
        target.valid=false;
    const bool fishing_control=!boat_chord&&
        !state->harbor_shop.open()&&
        !state->harbor_shop.consumes_primary()&&
        !(state->skiff&&state->skiff->Active());
    const auto event=state->fishing.Update(input->delta_seconds,
        fishing_control&&!state->beach_primary_latched&&
            input->primary_down!=0u,
        fishing_control&&input->secondary_down!=0u,
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
    if(listener && !(state->skiff&&state->skiff->Active())){
        state->shop_rx=input->listener_right[0];
        state->shop_rz=input->listener_right[2];
        state->shop_y=foot.ground_y+
            std::clamp(input->listener_position[1],.8f,2.f);
        const auto lines=state->harbor_shop.open()?
            reverie::sitka::HarborShopPanel(state->harbor_shop,state->fishing):
            reverie::sitka::ShoreGameplayPanel(state->fishing,aimed_shop>=0);
        std::string signature;
        for(const auto& line:lines){signature+=line;signature+='\n';}
        if(signature!=state->harbor_last_text){
            state->harbor_text.Clear();
            for(size_t i=0;i<lines.size();++i)
                state->harbor_text.Fit(
                    lines[i].c_str(),-.69f,.37f-float(i)*.135f,
                    1.40f,.019f,
                    i==3u?1.f:.84f,
                    i==4u?.90f:.94f,
                    i==4u?.58f:.91f,.015f);
            state->harbor_last_text=signature;
        }
    }else if(!state->harbor_last_text.empty()){
        state->harbor_text.Clear();state->harbor_last_text.clear();
    }

}
bool WaterAt(const State* state,double east,double north){
    if(!state||!std::isfinite(east)||!std::isfinite(north)||
       std::abs(east)>1.e8||std::abs(north)>1.e8)return false;
    return reverie::sitka::SitkaWorldWater(
        state->waters,state->terrain.origin_east,state->terrain.origin_north,
        east,north)!=nullptr;
}

// The host invokes this optional ABI callback ONCE after update and before
// drawing either eye, from the owning GLES context. No synchronous asset
// read occurs here; only resident source tiles or seeded infill are used.
int32_t PrepareFrame(void* instance){
    State* state=static_cast<State*>(instance);
    if(!state||!state->gl.ready())return 0;
    // Collection changes are rebuilt ONCE between stereo eye frames,
    // never from RenderEye or during input simulation.
    if(state->boat_dirty&&state->skiff&&state->boat_gl.ready()){
        std::string error;
        if(!state->boat_gl.Create(
            reverie::sitka::BuildBoatMesh(state->helm),
            state->lighting,&error)){
            Log(state->host,error.c_str());return 0;
        }
        state->boat_dirty=false;
    }
    if(state->beach_dirty && state->beach_gl.ready() &&
       state->walk_session){
        std::string error;
        const auto marker=reverie::sitka::BeachMarkerMesh(
            state->beach_sites,state->fishing.fishing.journal,
            *state->walk_session,state->beach_visible_mask);
        const std::vector<reverie::sitka::TerrainMesh> extras={
            marker.indices.empty()?reverie::sitka::TerrainMesh{}:marker};
        if(!state->beach_gl.ReplaceSeafloor(extras,&error)){
            Log(state->host,error.c_str());
            return 0;
        }
        state->beach_dirty=false;
    }
    // Source-aligned bridge bounds update once, before BOTH stereo eyes.
    // The 3 existing GPU material batches are never rebuilt for visibility.
    if(state->bridge_view && state->has_focus)
        state->bridge_view->Update(
            state->last_focus_east,state->last_focus_north,
            state->terrain.origin_east,state->terrain.origin_north);
    // The shore and seabed retain separate GPU resources. Shore selection
    // uses bounded source-geometry only, never synchronous asset reads.
    if(state->shore_stream && state->shore_gl.ready() && state->has_focus &&
       state->shore_stream->Update(
           state->last_focus_east,state->last_focus_north,
           state->terrain.origin_east,state->terrain.origin_north)){
        try{
            auto mesh=state->shore_stream->BuildPending();
            std::vector<reverie::sitka::TerrainMesh> replacement;
            if(!mesh.indices.empty())replacement.push_back(std::move(mesh));
            std::string error;
            if(state->shore_gl.ReplaceSeafloor(replacement,&error))
                state->shore_stream->Commit();
            else{
                Log(state->host,error.c_str());
                state->shore_stream->Reject();
            }
        }catch(const std::exception& e){
            Log(state->host,e.what());
            state->shore_stream->Reject();
        }
    }
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
    ReverieNativeEyeV1 shifted{};
    const ReverieNativeEyeV1* world_eye=eye;
    if(state->walk_session){
        if(!state->walk_session->ShiftEye(eye,shifted))return 0;
        world_eye=&shifted;
    }
    // Draw only the atmospheric upper hemisphere. Never paint fictional
    // Shingle Bay coastlines over survey-backed Sitka shoreline/harbor data.
    if(!state->horizon_gpu.RenderEye(world_eye,0u,false,true))return 0;
    if(!state->gl.RenderEye(world_eye)||
       !state->water_gl.RenderEye(world_eye))return 0;
    if(state->shore_stream && state->shore_stream->visible_count()>0u &&
       !state->shore_gl.RenderEye(world_eye))return 0;
    if(state->walk_graph && !state->walk_gl.RenderEye(world_eye))return 0;
    if(state->beach_gl.ready() &&
       !state->beach_gl.RenderEye(world_eye))return 0;
    if(state->harbor_gl.ready() &&
       !state->harbor_gl.RenderEye(world_eye))return 0;
    // Reuse the very same per-feature LOD outcome for colored sides and
    // textured top. The two eye renders never rebuild the source polygon.
    if(state->shore_deck_gl.ready() && state->shore_stream &&
       state->shore_stream->visible_count()>0u &&
       !state->shore_deck_gl.RenderEye(world_eye,[&](size_t index){
           return state->shore_stream->tier(index)!=
               reverie::sitka::ShoreStreamPlanner::Hidden;
       }))return 0;
    if(state->bridge_gl.ready() && state->bridge_view &&
       state->bridge_view->visible() &&
       !state->bridge_gl.RenderEye(world_eye))return 0;
    if(state->walk_session && !(state->skiff&&state->skiff->Active())&&
       state->harbor_panel_gl.ready()&&!state->harbor_last_text.empty()){
        const float length=std::hypot(state->shop_rx,state->shop_rz);
        if(length>.01f){
            const float x=state->shop_rx/length,z=state->shop_rz/length;
            const auto body=state->walk_session->pose();
            const float mat[16]={
                x,0.f,z,0.f, 0.f,1.f,0.f,0.f,
                -z,0.f,x,0.f,
                body.x+z*1.85f+(state->harbor_shop.open()?0.f:x*1.1f),
                state->shop_y-(state->harbor_shop.open()?0.f:.40f),
                body.z-x*1.85f+(state->harbor_shop.open()?0.f:z*1.1f),1.f
            };
            if(!state->harbor_panel_gl.RenderEye(world_eye,mat))return 0;
            float vp[16],mvp[16];
            ReverieNativeMat4Multiply(vp,world_eye->projection,world_eye->view);
            ReverieNativeMat4Multiply(mvp,vp,mat);
            if(!state->harbor_text.Draw(mvp,true))return 0;
        }
    }
    if(!state->skiff||!state->skiff->Active())return 1;
    const auto& pose=state->skiff->Pose();
    const reverie::sitka::Water* water=reverie::sitka::SitkaWorldWater(
        state->waters,state->terrain.origin_east,state->terrain.origin_north,
        pose.east,pose.north);
    if(!water)return 1; // never invent a surface for an unapproved footprint
    try{
        const auto model=reverie::sitka::BoatModelMatrix(
            pose,state->terrain.origin_east,state->terrain.origin_north,
            water->y);
        if(!state->boat_gl.RenderEye(world_eye,model.data()))return 0;
        float vp[16],mvp[16];
        ReverieNativeMat4Multiply(vp,world_eye->projection,world_eye->view);
        ReverieNativeMat4Multiply(mvp,vp,model.data());
        return state->boat_text.Draw(mvp,true)?1:0;
    }catch(const std::exception&){return 0;}
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
