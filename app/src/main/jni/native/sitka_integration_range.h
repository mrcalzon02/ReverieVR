#ifndef REVERIE_SITKA_INTEGRATION_RANGE_H
#define REVERIE_SITKA_INTEGRATION_RANGE_H
// Included inside sitka_terrain_module.cpp's anonymous namespace, after State,
// Log and Store are defined. NOT a geographic Sitka map or real bathymetry.
#if defined(REVERIE_SITKA_INTEGRATION_RANGE)
void* CreateIntegrationRange(const ReverieNativeHostV1* host) {
    try {
        auto state=std::make_unique<State>();
        state->host=host;
        state->integration_range=true;
        state->terrain.vertices={
            {-32.f,0.f,-24.f,52,92,70},{32.f,0.f,-24.f,52,92,70},
            {32.f,0.f,24.f,76,108,80},{-32.f,0.f,24.f,76,108,80}
        };
        state->terrain.indices={0u,1u,2u,0u,2u,3u};
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
