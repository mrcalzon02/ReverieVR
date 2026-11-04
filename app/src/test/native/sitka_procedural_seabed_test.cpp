#include "../../main/jni/native/sitka_procedural_seabed.h"
#include "../../main/jni/native/sitka_boat_navigation.h"
#include "../../main/jni/native/sitka_seafloor_mesh.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace reverie::sitka;
static bool near(double a,double b,double epsilon=1.e-4){
    return std::abs(a-b)<epsilon;
}
static bool bad(const std::string& text){
    try{(void)ParseSeabedGeneration(text,BathyDatumId("NAVD88"));return false;}
    catch(const std::invalid_argument&){return true;}
}
static BathyTile Survey(){
    BathyTile t;
    t.columns=3u;t.rows=3u;
    t.first_east=0.;t.first_north=20.;
    t.east_step=10.;t.north_step=10.;
    t.datum_id=BathyDatumId("NAVD88");
    t.elevations_m.assign(9u,-10.f);
    t.qualities.assign(9u,BedQuality::Survey);
    t.qualities[4]=BedQuality::Missing;
    t.elevations_m[4]=0.f;
    return t;
}
int main(){
    const std::string spec=
        "SITKA-PROCEDURAL-1\n"
        "datum NAVD88\n"
        "seed 726392041\n"
        "water_ref 0\n"
        "radius 1500\n"
        "anchors 2\n"
        "0 0 -10\n"
        "100 0 -20\n";
    assert(bad("SITKA-PROCEDURAL-1\ndatum MLLW\n"
               "seed 726392041\nwater_ref 0\nradius 1500\n"
               "anchors 0\n"));
    assert(bad("SITKA-PROCEDURAL-1\ndatum NAVD88\n"
               "seed nonsense\nwater_ref 0\nradius 1500\nanchors 0\n"));
    assert(bad(spec+"untrusted\n"));
    const auto config=ParseSeabedGeneration(spec,BathyDatumId("NAVD88"));
    ProceduralSeabed ground(config);
    assert(ground.settings().guides.size()==2u);
    const auto a=ground.Sample(0.,0.);
    const auto b=ground.Sample(100.,0.);
    assert(a.known&&b.known);
    assert(near(a.elevation_m,-10.)&&near(b.elevation_m,-20.));
    assert(a.quality==BedQuality::Procedural);
    const auto arbitrary=ground.Sample(431.25,-612.5);
    assert(near(arbitrary.elevation_m,ground.Sample(431.25,-612.5).elevation_m,1e-12));
    // Traversal and cache request ordering can never change global noise.
    for(int i=0;i<1000;++i)(void)ground.Sample(-5000.+i*12.5,10.+i*3.);
    assert(near(arbitrary.elevation_m,ground.Sample(431.25,-612.5).elevation_m,1e-12));
    assert(!ground.Sample(std::numeric_limits<double>::quiet_NaN(),0.).known);
    auto different=config;different.seed+=1u;
    ProceduralSeabed alt(different);
    assert(!near(arbitrary.elevation_m,alt.Sample(431.25,-612.5).elevation_m,1e-5));
    auto none=config;none.guides.clear();
    ProceduralSeabed without(none);
    assert(near(ground.Sample(9000.,9000.).elevation_m,
                without.Sample(9000.,9000.).elevation_m,1e-8));
    assert(!near(ground.Sample(10.,10.).elevation_m,
                 without.Sample(10.,10.).elevation_m,1e-5));
    // Both tiles share precisely identical world-coordinates at the seam.
    const auto all=[](double,double){return true;};
    const auto left=ground.GenerateTile(255.,128.,all);
    const auto right=ground.GenerateTile(256.,128.,all);
    assert(left.columns==17u&&right.columns==17u);
    assert(left.first_east==0.&&right.first_east==256.);
    for(unsigned row=0;row<17u;++row){
        const size_t li=size_t(row)*17u+16u,ri=size_t(row)*17u;
        assert(left.qualities[li]==BedQuality::Procedural);
        assert(left.elevations_m[li]==right.elevations_m[ri]);
    }
    const auto north=ground.GenerateTile(128.,257.,all);
    const auto south=ground.GenerateTile(128.,255.,all);
    for(unsigned col=0;col<17u;++col)
        assert(north.elevations_m[16u*17u+col]==south.elevations_m[col]);
    // Geographic-water masking MUST keep land out of generated geometry.
    const auto sea=[](double east,double){return east<=128.;};
    const auto masked=ground.GenerateTile(127.,128.,sea);
    assert(masked.qualities[0]==BedQuality::Procedural);
    assert(masked.qualities[16]==BedQuality::Missing);
    const auto mesh=BuildSeafloorTerrain(masked,0.,0.);
    assert(!mesh.indices.empty()&&mesh.indices.size()<16u*16u*6u);
    const BathyTile original=Survey();
    BathyAtlas atlas;assert(atlas.Add(original));
    MixedSeabed<BathyAtlas> mixed(atlas,ground);
    assert(mixed.Sample(1.,19.).quality==BedQuality::Procedural);
    assert(mixed.Sample(5.,15.).quality==BedQuality::Procedural);
    BathyTile covered=original;
    covered.qualities[4]=BedQuality::Survey;
    covered.elevations_m[4]=-10.f;
    BathyAtlas all_survey;assert(all_survey.Add(std::move(covered)));
    MixedSeabed<BathyAtlas> surveyed(all_survey,ground);
    assert(surveyed.Sample(1.,19.).quality==BedQuality::Survey);
    assert(near(surveyed.Sample(1.,19.).elevation_m,-10.));
    assert(mixed.Sample(100000.,100000.).known); // playable without NOAA tile
    const auto filled=FillSeabedGaps(original,ground,all);
    assert(filled.qualities[4]==BedQuality::Procedural);
    assert(filled.qualities[0]==BedQuality::Survey);
    assert(filled.elevations_m[0]==-10.f);
    assert(!BuildSeafloorTerrain(filled,0.,20.).indices.empty());
    const auto nothing=[](double,double){return false;};
    const auto omitted=FillSeabedGaps(original,ground,nothing);
    assert(omitted.qualities[4]==BedQuality::Missing);
    const auto depth=FindBoatDepth(mixed,500.,800.,0.,.35,true);
    assert(depth.status==DepthStatus::Safe&&
           depth.quality==BedQuality::Procedural);
    assert(FindBoatDepth(mixed,500.,800.,0.,.35,false).status==
           DepthStatus::OutsideWater);
    const auto labels=BoatSounderLabels(depth);
    assert(labels[1].find("PROCEDURAL")!=std::string::npos);
    assert(labels[2]=="SIMULATED - NOT FOR NAVIGATION");
    SkiffNavigation skiff;
    assert(skiff.Launch(500.,800.,0.,.35,mixed,0.,true));
    for(int i=0;i<30;++i)
        (void)skiff.Update(.05,1.,0.,0.,mixed,all);
    assert(skiff.boat.north>800.&&
           skiff.sounder.reading().quality==BedQuality::Procedural);
    std::cout<<"PASS Sitka deterministic seeded/proximal seabed, true-data priority, seam continuity, water mask, generated sonar and skiff\n";
}
