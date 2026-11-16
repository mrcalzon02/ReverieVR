#ifndef REVERIE_SITKA_INTEGRATION_RANGE_H
#define REVERIE_SITKA_INTEGRATION_RANGE_H
// Included inside sitka_terrain_module.cpp's anonymous namespace, after State,
// Log and Store are defined. NOT a geographic Sitka map or real bathymetry.
#if defined(REVERIE_SITKA_INTEGRATION_RANGE)
// Visible gravel strip and trail stakes on the fictional field course. This
// is a static display mesh; the authored ground, water and pickup hitboxes
// continue to own collision and the five-foot specimen reveal.
void AddPracticeShore(reverie::sitka::TerrainMesh& mesh,
                      const std::vector<reverie::sitka::CitySite>& stops){
    using Point=std::array<float,2>;
    const std::array<Point,4> inner={{{2,-9},{16,-9},{16,9},{2,9}}};
    const std::array<Point,4> outer={{{-.45f,-11.45f},{18.45f,-11.45f},
                                      {18.45f,11.45f},{-.45f,11.45f}}};
    const auto mix=[](Point a,Point b,float t){return Point{{a[0]+(b[0]-a[0])*t,
                                                            a[1]+(b[1]-a[1])*t}};};
    const auto quad=[&](Point a,Point b,Point c,Point d,float y,
                        uint8_t r,uint8_t g,uint8_t blue){
        const auto first=uint32_t(mesh.vertices.size());
        for(Point p:{a,b,c,d})mesh.vertices.push_back({p[0],y,p[1],r,g,blue});
        mesh.indices.insert(mesh.indices.end(),{first,first+1,first+2,first,first+2,first+3});
    };
    uint32_t seed=61743u;
    const auto random=[&](){seed=1664525u*seed+1013904223u;return float(seed>>8)/16777216.f;};
    for(unsigned side=0;side<4;++side){
        const unsigned end=(side+1u)%4u;
        for(unsigned i=0;i<24;++i){
            const float lo=float(i)/24.f,hi=float(i+1u)/24.f;
            const auto a=mix(inner[side],inner[end],lo),b=mix(inner[side],inner[end],hi);
            const auto d=mix(outer[side],outer[end],lo),c=mix(outer[side],outer[end],hi);
            const auto wet_a=mix(a,d,.29f),wet_b=mix(b,c,.29f);
            quad(a,b,wet_b,wet_a,.004f,82,92,84);
            quad(wet_a,wet_b,c,d,.004f,134,127,105);
            for(unsigned j=0;j<14;++j){
                const float t=random(),u=.13f+.73f*random();
                const auto p=mix(mix(a,b,t),mix(d,c,t),u);
                const float w=.018f+.048f*random(),h=w*(.4f+.6f*random());
                const uint8_t light=uint8_t(74.f+94.f*random());
                quad({p[0]-w,p[1]},{p[0]-.3f*w,p[1]+h},{p[0]+w,p[1]},
                     {p[0]+.3f*w,p[1]-h},.008f,light,uint8_t(light+2),uint8_t(light-7));
            }
        }
    }
    for(const auto& stop:stops){
        const float x=stop.x+(stop.x<2.f?-1.f:stop.x>16.f?1.f:0.f);
        const float z=stop.z+(stop.z<-9.f?-.9f:stop.z>9.f?.9f:0.f);
        const std::array<Point,4> corners={{{x-.07f,z-.07f},{x+.07f,z-.07f},
                                            {x+.07f,z+.07f},{x-.07f,z+.07f}}};
        for(unsigned i=0;i<4;++i){
            const auto a=corners[i],b=corners[(i+1u)%4u];
            const auto start=uint32_t(mesh.vertices.size());
            for(float y:{.01f,.56f,.70f}){
                const bool cap=y>.56f;
                for(auto pt:{a,b})mesh.vertices.push_back({pt[0],y,pt[1],
                    uint8_t(cap?241:106),uint8_t(cap?188:78),uint8_t(cap?57:54)});
            }
            mesh.indices.insert(mesh.indices.end(),{start,start+1,start+2,start+1,start+3,start+2,
                start+2,start+3,start+4,start+3,start+5,start+4});
        }
        quad(corners[0],corners[1],corners[2],corners[3],.70f,241,188,57);
    }
}
void* CreateIntegrationRange(const ReverieNativeHostV1* host) {
    try {
        auto state=std::make_unique<State>();
        state->host=host;
        state->integration_range=true;
        // The freestanding, double-sided colored range marker is visible
        // from the actual first-person spawn even before a controller/head
        // pose is delivered. A horizontal ground alone would be edge-on in
        // the initial no-controller GLES menu capture and appear blank.
        state->terrain.vertices={
            {-32.f,0.f,-24.f,52,92,70},{32.f,0.f,-24.f,52,92,70},
            {32.f,0.f,24.f,76,108,80},{-32.f,0.f,24.f,76,108,80},
            {-8.5f,.20f,-6.f,233,182,74},{-3.5f,.20f,-6.f,47,110,129},
            {-3.5f,3.2f,-6.f,58,170,187},{-8.5f,3.2f,-6.f,238,201,90}
        };
        state->terrain.indices={0u,1u,2u,0u,2u,3u,
            4u,5u,6u,4u,6u,7u,4u,6u,5u,4u,7u,6u};
        reverie::sitka::Water bay;
        bay.id="test_water";bay.shape="SQUARE";
        bay.x=9.f;bay.z=0.f;bay.y=.15f;
        bay.width=14.f;bay.depth=18.f;
        state->waters.push_back(bay);
        state->walk_session.reset(new reverie::sitka::FreeRoamSession(
            state->terrain,state->waters,state->shore_colliders,true,-6.,0.));
        state->harbor_sites.push_back({"crescent","integration_desk",
            reverie::sitka::HarborLocationKind::ServiceDesk,-5.f,0.f});
        state->fishing_spots.push_back({"test_cove","test_water",9.f,0.f,8.f,2.f});
        // A clockwise dry-ground discovery course around the practice bay.
        // These encounters never consume an immutable geographic site ID.
        using reverie::sitka::ShoreSpecimen;
        struct PracticeSite {ShoreSpecimen specimen;float x,z;};
        constexpr PracticeSite course[]={
            {ShoreSpecimen::EmptyClamShell,1.f,0.f},{ShoreSpecimen::PurpleShoreCrab,1.f,-5.f},
            {ShoreSpecimen::BeachPebble,1.f,-10.f},{ShoreSpecimen::SeaGlass,6.f,-10.f},
            {ShoreSpecimen::DriftwoodChip,11.f,-10.f},{ShoreSpecimen::CrabMolt,17.f,-10.f},
            {ShoreSpecimen::SeaStar,17.f,-4.f},{ShoreSpecimen::Limpet,17.f,2.f},
            {ShoreSpecimen::KelpFloat,17.f,10.f},{ShoreSpecimen::Chiton,12.f,10.f},
            {ShoreSpecimen::BarnaclePatch,6.f,10.f},{ShoreSpecimen::SeaSnail,1.f,10.f}
        };
        static_assert(std::size(course)==reverie::sitka::kPracticeKinds.size());
        for(unsigned i=0;i<std::size(course);++i){
            const auto& site=course[i];
            const auto& def=reverie::sitka::Specimen(site.specimen);
            if(def.category!=reverie::sitka::kPracticeKinds[i]||
               !state->walk_session->GroundAt(site.x,site.z).valid)
                throw std::logic_error("Invalid Sitka practice discovery course");
            state->beach_sites.push_back({i,def.category,site.x,site.z,site.specimen,
                                         def.zones&(~def.zones+1u),false});
            if(i%2u==0u)state->city_sites.push_back(
                {i/2u,reverie::sitka::CityFind::Trailhead,site.x,site.z});
        }
        const auto loaded=state->fishing.Load(Store(state.get()));
        if(loaded!=reverie::sitka::FishingLoad::Fresh &&
           loaded!=reverie::sitka::FishingLoad::Restored)
            throw std::runtime_error("Sitka test range save rejected without reset");
        state->helm=reverie::sitka::HelmFromJournal(state->fishing.fishing.journal);
        state->helm_layout=reverie::sitka::AssembleHelm(state->helm);
        if(loaded==reverie::sitka::FishingLoad::Fresh &&
           !state->fishing.Flush(Store(state.get())))
            Log(host,"Sitka test range initial save pending retry");
        // A synthetic directory with a remote, non-overlapping tile is only
        // a bathymetry parser fixture; actual depth is marked PROCEDURAL.
        state->stream.reset(new reverie::sitka::BathyStreamCache(
            reverie::sitka::ParseBathyStreamIndex(
                "SITKA-BATHY-INDEX-2\n"
                "datum NAVD88\n"
                "tiles 1\n"
                "bathymetry-r00-c00.sitkab 900000 7000500 100 100 3 3\n")));
        reverie::sitka::SeabedGeneration seabed;
        seabed.datum_id=state->stream->directory().datum_id;
        seabed.seed=44556677u;
        seabed.water_reference_m=bay.y;
        state->generated.reset(new reverie::sitka::ProceduralSeabed(seabed));
        reverie::sitka::SkiffLaunch berth;
        berth.datum_id=seabed.datum_id;
        berth.water_id=bay.id;berth.east=6.;berth.north=0.;
        berth.heading=1.5707963267948966;berth.radius_m=8.;
        state->skiff.reset(new reverie::sitka::SitkaSkiffSession(berth));
        if(!state->skiff->SetHullTier(state->helm.hull))
            throw std::runtime_error("Sitka test range hull initialization failed");
        ReverieNativeLogV1(host,REVERIE_NATIVE_LOG_INFO,"SitkaTerrain",
            "INTEGRATION TEST RANGE: synthetic, non-geographic, not real Sitka");
        return state.release();
    } catch(const std::exception& e) {
        Log(host,e.what());
        return nullptr;
    }
}
#endif
#endif
