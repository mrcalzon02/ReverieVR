#include "../../main/jni/native/sitka_bathymetry.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace reverie::sitka;

static void U32(std::vector<uint8_t>& a,uint32_t n){
    for(unsigned i=0;i<4;++i)a.push_back(static_cast<uint8_t>(n>>(8u*i)));
}
static void F32(std::vector<uint8_t>& a,float n){
    uint32_t bits=0;std::memcpy(&bits,&n,4u);U32(a,bits);
}
static void F64(std::vector<uint8_t>& a,double n){
    uint64_t bits=0;std::memcpy(&bits,&n,8u);
    for(unsigned i=0;i<8;++i)a.push_back(static_cast<uint8_t>(bits>>(8u*i)));
}
static bool near(double x,double y){return std::abs(x-y)<1.e-5;}
static bool bad(const std::vector<uint8_t>& a,uint32_t datum){
    try{(void)ParseBathymetry(a.data(),a.size(),datum);return false;}
    catch(const std::invalid_argument&){return true;}
}
static bool badIndex(const std::string& t){
    try{(void)ParseBathyIndex(t);return false;}
    catch(const std::invalid_argument&){return true;}
}
static std::vector<uint8_t> Tile(double east=100.,bool gap=false){
    std::vector<uint8_t> buf={'S','I','T','K','A','B','0','1'};
    U32(buf,3u);U32(buf,3u);
    F64(buf,east);F64(buf,200.);F64(buf,10.);F64(buf,10.);
    U32(buf,BathyDatumId("NAVD88"));U32(buf,0u);
    const float z[9]={-4.f,-6.f,-8.f,-8.f,-10.f,-12.f,-14.f,-16.f,-18.f};
    for(unsigned i=0;i<9u;++i){
        F32(buf,gap&&i==4u?0.f:z[i]);
        buf.push_back(gap&&i==4u?0u:i>=6u?2u:1u);
    }
    assert(buf.size()==kBathyHeaderBytes+45u);
    return buf;
}
int main(){
    const auto datum=BathyDatumId("NAVD88");
    assert(datum!=0u&&datum!=BathyDatumId("MLLW"));
    const auto idx=ParseBathyIndex(
        "SITKA-BATHY-INDEX-1\n"
        "datum NAVD88\n"
        "tiles 2\n"
        "bathymetry-r00-c00.sitkab\n"
        "bathymetry-r00-c01.sitkab\n");
    assert(idx.datum_id==datum&&idx.filenames.size()==2u);
    assert(badIndex("SITKA-BATHY-INDEX-1\ndatum NAVD88\ntiles 1\n../escape.sitkab\n"));
    assert(badIndex("SITKA-BATHY-INDEX-1\ndatum NAVD88\ntiles 1\nbathymetry-r00-c00.sitkab\nextra\n"));
    assert(badIndex("SITKA-BATHY-INDEX-1\ndatum NAVD88\ntiles 2\nbathymetry-r00-c00.sitkab\nbathymetry-r00-c00.sitkab\n"));
    auto bytes=Tile();
    assert(bad(bytes,BathyDatumId("MLLW")));
    auto first=ParseBathymetry(bytes.data(),bytes.size(),datum);
    assert(first.Contains(105.,195.)&&!first.Contains(120.,180.));
    const auto middle=first.Sample(105.,195.);
    assert(middle.known&&near(middle.elevation_m,-7.));
    assert(middle.quality==BedQuality::Survey);
    assert(first.Sample(105.,185.).quality==BedQuality::Composite);
    auto missing=Tile(100.,true);
    const auto cut=ParseBathymetry(missing.data(),missing.size(),datum);
    assert(!cut.Sample(105.,195.).known);
    assert(!cut.Sample(120.,180.).known);
    assert(!cut.Sample(100000.,0.).known);
    assert(bad(std::vector<uint8_t>(bytes.begin(),bytes.end()-1u),datum));
    auto corrupt=bytes;corrupt[52]=1u;assert(bad(corrupt,datum));
    corrupt=bytes;corrupt[56+4u]=3u;assert(bad(corrupt,datum));
    corrupt=bytes;corrupt[56+4u]=0u;assert(bad(corrupt,datum));
    corrupt=bytes;corrupt[56+5u*4u+4u]=0u;assert(bad(corrupt,datum));
    corrupt=bytes;corrupt[56+5u*4u+4u]=1u;
    const uint32_t nan_bits=0x7fc00000u;
    for(unsigned i=0;i<4u;++i)
        corrupt[56+5u*4u+i]=static_cast<uint8_t>(nan_bits>>(i*8u));
    assert(bad(corrupt,datum));
    BathyAtlas atlas;
    assert(atlas.Add(std::move(first)));
    assert(!atlas.Add(ParseBathymetry(bytes.data(),bytes.size(),datum))); // overlap
    auto second=Tile(120.);
    assert(atlas.Add(ParseBathymetry(second.data(),second.size(),datum)));
    assert(atlas.tile_count()==2);
    assert(atlas.Sample(125.,195.).known);
    const auto depth=FindBoatDepth(atlas,105.,195.,1.,.5,true);
    assert(depth.status==DepthStatus::Safe);
    assert(near(depth.water_depth_m,8.)&&near(depth.under_keel_m,7.5));
    assert(depth.quality==BedQuality::Survey);
    assert(FindBoatDepth(atlas,105.,195.,-7.,.5,true).status==DepthStatus::Dry);
    assert(FindBoatDepth(atlas,105.,195.,-6.8,.5,true).status==DepthStatus::UnderKeel);
    assert(FindBoatDepth(atlas,105.,195.,0.,.5,false).status==DepthStatus::OutsideWater);
    assert(FindBoatDepth(atlas,1000.,195.,1.,.5,true).status==DepthStatus::NoCoverage);
    assert(FindBoatDepth(atlas,105.,195.,0.,-1.,true).status==DepthStatus::NoCoverage);
    BoatDepthFinder finder;
    for(int i=0;i<4;++i)
        assert(!finder.Update(.05,atlas,105.,195.,1.,.5,true));
    assert(finder.Update(.05,atlas,105.,195.,1.,.5,true));
    assert(near(finder.reading().water_depth_m,8.));
    assert(!finder.Update(.05,atlas,105.,195.,1.,.5,true));
    assert(near(finder.reading().water_depth_m,8.)); // stable until next update
    finder.Reset();
    assert(finder.reading().status==DepthStatus::NoCoverage);
    assert(!finder.Update(std::numeric_limits<double>::quiet_NaN(),atlas,
                          105.,195.,1.,.5,true));
    std::cout<<"PASS Sitka surveyed/composite seabed, datum, NoData, tiles, depth, draft and tick cadence\n";
}
