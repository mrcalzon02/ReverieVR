// Integrated native Sitka gameplay. An explicitly labeled NON-GEOGRAPHIC
// test range is used only when approved geographic assets are absent.
#include "sitka_terrain_gl.h"
#include "sitka_chart_survey.h"
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
#include <stdexcept>
#include <algorithm>

namespace {
struct State {
    const ReverieNativeHostV1* host = nullptr;
    bool integration_range = false;
    // User-selected REAL geography inspection, not approved coastal gameplay.
    // The test range and all its player state remain intact while previewing.
    std::unique_ptr<reverie::sitka::LandTileStream> geo_tiles;
    reverie::sitka::TerrainGl geo_gl;
    reverie::nativetext::LatoLayer geo_text;
    reverie::sitka::TerrainGl chart_pin;
    bool chart_route=false,chart_primary_held=false;
    std::string chart_caption;
    bool geo_view=false,geo_chord_held=false;
    double geo_east=0.,geo_north=0.;
    float geo_altitude=210.f;
    unsigned geo_last_errors=0u;
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
// Updated once per input frame; head-locked text stays stable between eyes.
void UpdateChartCaption(State& state){
    using namespace reverie::sitka;
    const unsigned visited=state.fishing.fishing.journal.chart_stations;
    std::string title="SITKA / NOAA ENC + USGS 3DEP";
    std::string target="ELEVATION: LOW GREEN TO HIGH TAN";
    std::string cue="TOUCH PAD TO MOVE";
    if(state.chart_route){
        title="CHART SURVEY   "+std::to_string(visited)+" / "+std::to_string(kChartStations);
        if(visited<kChartStations){
            const auto& site=kSurveyRoute[visited];
            target=site.name;
            const double dx=site.east-state.geo_east,dn=site.north-state.geo_north;
            const double distance=std::hypot(dx,dn);
            if(distance<=kChartSurveyRadius){
                cue=state.geo_tiles->OnDisplayedTile(site.row,site.column)?
                    "SELECT: FILE SOURCE SAMPLE (+25 CR)":"LOADING SOURCE TILE";
            }else{
                const auto meters=int(std::ceil(distance/5.))*5;
                cue="PAD";
                if(std::abs(dn)>kChartSurveyRadius*.5)cue+=dn>0.?" UP":" DOWN";
                if(std::abs(dx)>kChartSurveyRadius*.5)cue+=dx>0.?" RIGHT":" LEFT";
                cue+="   "+std::to_string(meters)+" M";
            }
        }else{
            target="SURVEY COMPLETE / THREE STATIONS FILED";
            cue="EXPLORE THE CHART OR RETURN TO THE HARBOR";
        }
    }
    const std::string record=state.fishing.write_failed?"SAVE PENDING / RETRY ON EXIT":
        state.chart_route&&visited?
        std::string("FILED: ")+kSurveyRoute[visited-1u].name+" / "+
            std::to_string(int(kSurveyRoute[visited-1u].elevation))+"."+
            std::to_string(int(std::lround(kSurveyRoute[visited-1u].elevation*10))%10)+" M 3DEP":
        "NOAA ENC LAND / USGS 3DEP";
    const std::string caption=title+"\n"+target+"\n"+cue+"\n"+record;
    if(caption==state.chart_caption)return;
    state.chart_caption=caption;
    auto& text=state.geo_text;text.Clear();
    text.Fit(title.c_str(),0.f,1.38f,2.90f,.055f,.98f,.94f,.68f);
    text.Fit(target.c_str(),0.f,1.14f,2.90f,.049f,.98f,.97f,.93f);
    text.Fit(cue.c_str(),0.f,.89f,2.90f,.041f,.99f,.82f,.47f);
    text.Fit("+",0.f,0.f,.16f,.05f,.98f,.96f,.72f);
    text.Fit(record.c_str(),0.f,-.92f,2.90f,.038f,.9f,.96f,.94f);
    text.Fit("VOL UP + RIGHT: EXIT / + UP OR DOWN: ALT",0.f,-1.13f,3.15f,.037f,.9f,.96f,.94f);
    text.Fit("CHART LAND / NOT FOR NAVIGATION",0.f,-1.35f,2.90f,.037f,.96f,.78f,.58f);
}
#include "sitka_integration_range.h"
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
#if defined(REVERIE_SITKA_INTEGRATION_RANGE)
    if(probe==REVERIE_NATIVE_SAVE_NOT_FOUND){
        State* state=static_cast<State*>(CreateIntegrationRange(host));
        if(!state)return nullptr;
        // The real NOAA/USGS archive is an OPTIONAL source-backed map view.
        // No unknown marine water or false harbor spawn is inserted into play.
        uint32_t index_size=0u;
        const int32_t index_probe=host->read_asset(
            "sitka/coast/index.txt",nullptr,0u,&index_size);
        if((index_probe==REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL ||
            index_probe==REVERIE_NATIVE_SAVE_OK) &&
           index_size>0u && index_size<=16384u){
            try{
                state->geo_tiles.reset(new reverie::sitka::LandTileStream(
                    ReadText(host,"sitka/coast/index.txt",16384u)));
                state->chart_route=reverie::sitka::HasSurveyRoute(*state->geo_tiles);
                state->geo_east=state->geo_tiles->origin_east()+
                    512.*state->geo_tiles->columns()*.5;
                // Start over the waterfront portion of the archived chart.
                // This frames the land mask for inspection, not a player spawn.
                state->geo_north=state->geo_tiles->origin_north()-
                    512.*state->geo_tiles->rows()*.625;
                state->geo_tiles->SetFocus(state->geo_east,state->geo_north);
                ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_INFO,
                    "SitkaTerrain",
                    "NOAA chart land / USGS 3DEP geography available via VOL UP + PAD RIGHT. "
                    "Source quality only; vertical datum/unverified water.");
            }catch(const std::exception& e){
                Log(host,e.what());state->geo_tiles.reset();
            }
        }else if(index_probe!=REVERIE_NATIVE_SAVE_NOT_FOUND){
            Log(host,"Sitka geographic index invalid; gameplay range kept");
        }
        return state;
    }
#endif
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
    if(state->geo_tiles){
        if(!state->geo_gl.CreateOverlay(state->lighting,&error)||
           (state->chart_route&&!state->chart_pin.Create(reverie::sitka::ChartStationPin(),
                                                         state->lighting,&error))||
           !state->geo_text.Create()){
            Log(state->host,"Real Sitka geographic preview GLES unavailable; "
                "existing fishing test range remains operational");
            state->chart_pin.Release();
            state->geo_text.Release();
            state->geo_gl.Release();
            state->geo_tiles.reset();
            state->geo_view=false;
        }else{
            state->geo_tiles->AfterGlContextLoss();
            state->chart_caption.clear();
            UpdateChartCaption(*state);
        }
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
    // Re-upload nearby finds after EGL recreation, even if the player stayed still.
    state->beach_dirty=state->beach_visible_mask!=0u;
    {
        auto signs=state->walk_session?
            reverie::sitka::HarborShopSigns(state->harbor_sites,*state->walk_session):
            reverie::sitka::TerrainMesh{};
#if defined(REVERIE_SITKA_INTEGRATION_RANGE)
        if(state->integration_range)AddPracticeShore(signs,state->city_sites);
#endif
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
        static_cast<State*>(instance)->chart_pin.Release();
        static_cast<State*>(instance)->geo_text.Release();
        static_cast<State*>(instance)->geo_gl.Release();
        if(static_cast<State*>(instance)->geo_tiles)
            static_cast<State*>(instance)->geo_tiles->AfterGlContextLoss();
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
int32_t PrepareExit(void* instance) {
    auto* state=static_cast<State*>(instance);
    return state && state->fishing.Flush(Store(state)) ? 1 : 0;
}
void Update(void* instance,const ReverieNativeInputV1* input){
    auto* state=static_cast<State*>(instance);
    if(!state||!ReverieNativeInputHasBaseV1(input))return;
    // The default secondary action (Volume Up) plus rightward touch toggles
    // the land chart. A held chord can toggle only once.
    const bool geo_chord=state->geo_tiles &&
        input->secondary_down!=0u && input->primary_down==0u &&
        input->move_x>.80f;
    if(!geo_chord)state->geo_chord_held=false;
    if(geo_chord){
        if(!state->geo_chord_held){
            state->geo_chord_held=true;
            state->geo_view=!state->geo_view;
            state->chart_primary_held=false;
            ReverieNativeRequestFeedbackV1(
                state->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        }
        return;
    }
    if(state->geo_view && state->geo_tiles){
        const double dt=std::isfinite(input->delta_seconds)?
            std::clamp(double(input->delta_seconds),0.,.05):0.;
        const bool chart_press=input->primary_down&&!state->chart_primary_held;
        state->chart_primary_held=input->primary_down!=0u;
        if(input->secondary_down!=0u || input->primary_down!=0u){
            // The secondary button leaves Daydream touch axes live. Gamepads
            // can also hold Select to adjust the geographic observation
            // altitude; the preview is aerial and not collision-grounded.
            state->geo_altitude=std::clamp(
                float(state->geo_altitude-double(input->move_y)*120.*dt),
                65.f,300.f);
        }else{
            state->geo_east+=double(input->move_x)*80.*dt;
            state->geo_north-=double(input->move_y)*80.*dt;
        }
        const double west=state->geo_tiles->origin_east();
        const double north=state->geo_tiles->origin_north();
        const double width=512.*state->geo_tiles->columns();
        const double height=512.*state->geo_tiles->rows();
        state->geo_east=std::clamp(state->geo_east,west+1.,west+width-1.);
        state->geo_north=std::clamp(state->geo_north,north-height+1.,north-1.);
        state->geo_tiles->SetFocus(state->geo_east,state->geo_north);
        state->geo_tiles->Pump([&](const std::string& path,std::string& bytes){
            try{
                bytes=ReadText(state->host,path.c_str(),
                    reverie::sitka::LandTileStream::kTileLimit);
                return true;
            }catch(const std::exception&){return false;}
        });
        if(state->geo_tiles->failed_reads()!=state->geo_last_errors){
            state->geo_last_errors=state->geo_tiles->failed_reads();
            Log(state->host,state->geo_tiles->last_error().c_str());
        }
        const auto station=state->fishing.fishing.journal.chart_stations;
        if(state->chart_route&&chart_press&&!input->secondary_down&&
           std::abs(input->move_y)<.35f&&station<reverie::sitka::kChartStations){
            const auto& site=reverie::sitka::kSurveyRoute[station];
            const bool reached=std::hypot(state->geo_east-site.east,state->geo_north-site.north)
                <=reverie::sitka::kChartSurveyRadius;
            if(reached&&state->geo_tiles->OnDisplayedTile(site.row,site.column)&&
               state->fishing.FileChartStation(station)){
                if(!state->fishing.Flush(Store(state)))Log(state->host,"Sitka chart log save deferred");
                ReverieNativeRequestFeedbackV1(state->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
            }else ReverieNativeRequestFeedbackV1(state->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
        }
        UpdateChartCaption(*state);
        return; // Fishing and boat simulation stay at the berth during the survey.
    }
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
            *state->walk_session,state->integration_range);
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
            state->integration_range?state->fishing.fishing.journal.practice_checkpoints:
                                     state->fishing.fishing.journal.city_sites,
            state->fishing.fishing.journal.observation_level);
        if(found.valid && (state->integration_range?state->fishing.VisitPracticeCheckpoint(found.site):
                           state->fishing.DiscoverCity(found.site,found.kind))){
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
                ray.pointer_origin[1]+foot.EyeY(),
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
        // Touchpad UP and gamepad forward are both negative native Y.
        // Release immediately drops throttle to zero; 30 Hz hull inertia
        // still governs the deceleration of the vessel itself.
        const double throttle=reverie::sitka::SkiffForwardThrottle(input->move_y);
        const double rudder=state->skiff->Active()?
            state->helm_rudder.Advance(input->delta_seconds,input->move_x):0.;
        const auto event=state->skiff->Update(
            input->delta_seconds,throttle,rudder,
            boat_chord,listener_east,listener_north,
            *state->stream,*state->generated,state->waters,
            state->terrain.origin_east,state->terrain.origin_north);
        if(state->skiff->Active()){
            state->instruments=reverie::sitka::HelmReadouts(
                state->helm,state->skiff->Pose(),state->weather,
                state->skiff->Sounder().reading(),
                state->helm_rudder.degrees,throttle,1.);
            // Build real Lato text only when data changes. This executes
            // once in the simulation tick, never from a stereo-eye callback.
            const auto panels=reverie::sitka::BuildBoatPanels(
                state->helm,state->skiff->Pose(),state->weather,
                state->skiff->Sounder().reading(),throttle);
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
                ray_input.pointer_origin[1]+foot.EyeY(),
                ray_input.pointer_origin[2]+foot.z
            };
            const auto found=reverie::sitka::FindBeachTarget(
                source,ray_input.pointer_direction,foot.x,foot.z,
                *state->walk_session,state->beach_sites,
                state->integration_range?state->fishing.fishing.journal.practice_finds:
                                         state->fishing.fishing.journal.beach_sites,
                state->fishing.fishing.journal.shore_kit_level,
                state->fishing.fishing.journal.observation_level);
            if(found.valid){
                // The same world ray supports an explicitly authored,
                // sellable shellfish bundle. Live animal observations and
                // ordinary beach finds remain discovery-only.
                state->beach_primary_latched=true;
                const bool collected=state->integration_range?state->fishing.RecordPracticeFind(found.site):
                    found.shellfish_bundle?
                    state->fishing.CollectShellfishBundle(found.site):
                    state->fishing.Discover(found.site,found.kind);
                if(collected){
                    state->beach_dirty=true;
                    if(!state->fishing.Flush(Store(state)))
                        Log(state->host,"Sitka beach pickup save deferred");
                    ReverieNativeRequestFeedbackV1(
                        state->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
                }else{
                    ReverieNativeRequestFeedbackV1(
                        state->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
                }
            }
        }
    }
    reverie::sitka::FishingCastTarget target;
    if(listener&&state->walk_session&&!state->beach_primary_latched&&
       !boat_chord&&!(state->skiff&&state->skiff->Active())&&
       input->primary_down&&ReverieNativeInputHasPointerV1(input)){
        ReverieNativeInputV1 normalized=*input;
        if(ReverieNativeSanitizePointerV1(&normalized)){
            const float* p=normalized.pointer_origin;
            const float* d=normalized.pointer_direction;
            target=reverie::sitka::FindFishingTarget(
                reverie::sitka::FishingRay{
                    p[0]+foot.x,p[1]+foot.EyeY(),p[2]+foot.z,
                    d[0],d[1],d[2]},
                state->fishing_spots,state->waters);
        }
    }
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
        state->shop_y=foot.EyeY()+input->listener_position[1];
        auto lines=state->harbor_shop.open()?
            reverie::sitka::HarborShopPanel(state->harbor_shop,state->fishing):
            reverie::sitka::ShoreGameplayPanel(state->fishing,aimed_shop>=0);
        if(state->integration_range&&!state->harbor_shop.open()){
            const auto& journal=state->fishing.fishing.journal;
            const unsigned finds=reverie::sitka::BeachSiteCount(journal.practice_finds);
            lines[0]="PRACTICE BAY / "+std::to_string(finds)+"/12 FINDS / "+
                std::to_string(reverie::sitka::BeachSiteCount(journal.practice_checkpoints))+"/6 STOPS";
            if(state->fishing.fishing.phase==reverie::sitka::FishingPhase::Ready){
                const auto next=std::find_if(state->beach_sites.begin(),state->beach_sites.end(),
                    [&](const reverie::sitka::BeachSite& site){return !(journal.practice_finds&(1u<<site.id));});
                if(next!=state->beach_sites.end()){
                    const auto& def=reverie::sitka::Specimen(next->specimen);
                    const double dx=next->x-foot.x,dz=next->z-foot.z,distance=std::hypot(dx,dz);
                    if(distance<=reverie::sitka::kShoreRevealRadiusM)
                        lines[4]=std::string("AIM DOWN + SELECT: ")+def.name;
                    else{
                        const double across=dx*state->shop_rx+dz*state->shop_rz;
                        const double forward=dx*state->shop_rz-dz*state->shop_rx;
                        const char* direction=std::abs(across)>std::abs(forward)?
                            (across>0.?"RIGHT":"LEFT"):(forward>0.?"UP":"DOWN");
                        lines[4]=std::string("FOLLOW BANK: PAD ")+direction+" / "+
                            std::to_string(unsigned(std::ceil(distance)))+" M TO "+def.name;
                    }
                }else lines[4]=journal.practice_submitted==journal.practice_finds?
                    "FIELD LOG COMPLETE / ALL FINDS REGISTERED":"12 FINDS / RETURN TO HARBOR OFFICE TO REGISTER";
            }
        }
        if(state->skiff && !state->harbor_shop.open() && aimed_shop<0 &&
           state->fishing.fishing.phase==reverie::sitka::FishingPhase::Ready &&
           state->skiff->AtBerth(listener_east,listener_north))
            lines[2]="SELECT + VOL UP: BOARD HERE / DOCK WHEN SLOW";
        const bool show_map_hint=state->integration_range && state->geo_tiles &&
            !state->harbor_shop.open();
        const char* map_hint=state->chart_route?"VOL UP + PAD RIGHT: CHART SURVEY (+25 CR / SITE)":
            "VOL UP + PAD RIGHT: NOAA/USGS LAND MAP";
        std::string signature;
        for(const auto& line:lines){signature+=line;signature+='\n';}
        if(show_map_hint)signature+=map_hint;
        if(signature!=state->harbor_last_text){
            state->harbor_text.Clear();
            for(size_t i=0;i<lines.size();++i)
                state->harbor_text.Fit(
                    lines[i].c_str(),0.f,.37f-float(i)*.135f,
                    1.40f,.019f,
                    i==3u?1.f:.84f,
                    i==4u?.90f:.94f,
                    i==4u?.58f:.91f,.015f);
            if(show_map_hint)
                state->harbor_text.Fit(map_hint,0.f,-.44f,1.40f,.019f,
                    .84f,.94f,.91f,.015f);
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
    // A complete 2x2 sourced-land CPU batch is admitted to the GL context
    // ONLY at the frame boundary, never from RenderEye or the input thread.
    // The last valid GPU surface is retained on failed staging/replacement.
    if(state->geo_tiles&&state->geo_gl.ready()&&
       state->geo_tiles->PendingGpuSwap()){
        std::string error;
        if(state->geo_gl.ReplaceSeafloor(
              state->geo_tiles->PendingWorldMeshes(),&error)){
            state->geo_tiles->MarkGpuCommitted();
        }else{
            Log(state->host,error.c_str());
        }
    }
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
            *state->walk_session,state->beach_visible_mask,state->integration_range);
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
    if(state->geo_view && state->geo_tiles &&
       state->geo_gl.ready() && state->geo_gl.batch_count()>0u){
        if(!ReverieNativeEyeRenderableV1(eye))return 0;
        ReverieNativeEyeV1 camera=*eye;
        // The hub's room-scale far plane is 30 m. Extend only the map's
        // perspective depth interval; keep the Cardboard eye's asymmetric
        // frustum, stereo view and original projection for the near HUD.
        if(camera.projection[11]==-1.f && camera.projection[15]==0.f){
            const float near_plane=camera.projection[14]/(camera.projection[10]-1.f);
            constexpr float far_plane=2048.f;
            if(near_plane>0.f && near_plane<far_plane){
                camera.projection[10]=-(far_plane+near_plane)/(far_plane-near_plane);
                camera.projection[14]=-2.f*far_plane*near_plane/(far_plane-near_plane);
            }
        }
        // Chart-land coordinates are rebased to the UTM northwest node.
        // No headset tracking is interpreted as a GPS / surveyed viewpoint.
        const float x=float(state->geo_east-state->geo_tiles->origin_east());
        const float z=float(state->geo_north-state->geo_tiles->origin_north());
        const float pitch=.70f,c=std::cos(pitch),s=std::sin(pitch);
        const float tilt[16]={
            1.f,0.f,0.f,0.f, 0.f,c,s,0.f,
            0.f,-s,c,0.f, 0.f,0.f,0.f,1.f
        };
        const float shift[16]={
            1.f,0.f,0.f,0.f, 0.f,1.f,0.f,0.f,
            0.f,0.f,-1.f,0.f,
            -x,-state->geo_altitude,z-state->geo_altitude*c/s,1.f
        };
        float angled[16];
        ReverieNativeMat4Multiply(angled,tilt,shift);
        ReverieNativeMat4Multiply(camera.view,eye->view,angled);
        if(!state->geo_gl.RenderEye(&camera))return 0;
        const auto station=state->fishing.fishing.journal.chart_stations;
        if(state->chart_route&&station<reverie::sitka::kChartStations){
            const auto& site=reverie::sitka::kSurveyRoute[station];
            if(state->geo_tiles->OnDisplayedTile(site.row,site.column)){
                const float pin[16]={1,0,0,0, 0,1,0,0, 0,0,1,0,
                    float(site.east-state->geo_tiles->origin_east()),site.elevation,
                    float(site.north-state->geo_tiles->origin_north()),1};
                if(!state->chart_pin.RenderEye(&camera,pin))return 0;
            }
        }
        // Actual licensed Lato HUD: 'NOT FOR NAVIGATION' never disappears.
        return state->geo_text.Draw(eye->projection,false)?1:0;
    }
    ReverieNativeEyeV1 shifted{};
    const ReverieNativeEyeV1* world_eye=eye;
    const bool aboard=state->skiff && state->skiff->Active();
    std::array<float,16> boat_model{};
    if(aboard){
        const auto& pose=state->skiff->Pose();
        const auto* water=reverie::sitka::SitkaWorldWater(state->waters,
            state->terrain.origin_east,state->terrain.origin_north,pose.east,pose.north);
        if(!water)return 0;
        try{
            boat_model=reverie::sitka::BoatModelMatrix(pose,state->terrain.origin_east,
                state->terrain.origin_north,water->y);
        }catch(const std::invalid_argument&){return 0;}
        // The helm carries the seat. Only its position follows the vessel;
        // the shell retains head orientation, lean and per-eye separation.
        const auto seat=state->passenger.World(pose);
        constexpr float seated_eye_above_deck=1.22f;
        if(!reverie::sitka::PlaceTrackingEye(eye,shifted,
            float(seat[0]-state->terrain.origin_east),
            boat_model[13]+reverie::sitka::kBoatDeckY+seated_eye_above_deck,
            float(seat[1]-state->terrain.origin_north)))return 0;
        world_eye=&shifted;
    }else if(state->walk_session){
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
    if(!aboard)return 1;
    if(!state->boat_gl.RenderEye(world_eye,boat_model.data()))return 0;
    float vp[16],mvp[16];
    ReverieNativeMat4Multiply(vp,world_eye->projection,world_eye->view);
    ReverieNativeMat4Multiply(mvp,vp,boat_model.data());
    return state->boat_text.Draw(mvp,true)?1:0;
}
const ReverieNativeModuleApiV1 kApi = {
    sizeof(ReverieNativeModuleApiV1), REVERIE_NATIVE_MODULE_ABI_VERSION,
    {sizeof(ReverieNativeModuleDescriptorV1), REVERIE_NATIVE_MODULE_ABI_VERSION,
     "sitka", "Sitka [INTEGRATION TEST]", 2u, 0u},
    Create, Destroy, OnGlCreated, ReleaseGl, Resume, Pause, Update, RenderEye,
    nullptr, PrepareFrame, PrepareExit
};
} // namespace
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1* reverie_native_module_entry_v1(void) { return &kApi; }
