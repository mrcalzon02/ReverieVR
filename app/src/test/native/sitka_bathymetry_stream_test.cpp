#include "../../main/jni/native/sitka_bathymetry_stream.h"
#include "../../main/jni/native/sitka_boat_navigation.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using namespace reverie::sitka;
static void U32(std::vector<uint8_t>& b,uint32_t x){
    for(int i=0;i<4;++i)b.push_back(static_cast<uint8_t>(x>>(8*i)));
}
static void F64(std::vector<uint8_t>& b,double x){
    uint64_t v=0;std::memcpy(&v,&x,8);
    for(int i=0;i<8;++i)b.push_back(static_cast<uint8_t>(v>>(8*i)));
}
static void F32(std::vector<uint8_t>& b,float x){
    uint32_t v=0;std::memcpy(&v,&x,4);U32(b,v);
}
static std::string Name(unsigned col,unsigned row=0){
    std::ostringstream o;
    o<<"bathymetry-r"<<std::setfill('0')<<std::setw(2)<<row
      <<"-c"<<std::setw(2)<<col<<".sitkab";
    return o.str();
}
static std::string Index(unsigned count){
    std::ostringstream o;o.imbue(std::locale::classic());
    o<<"SITKA-BATHY-INDEX-2\n"<<"datum NAVD88\n"
     <<"tiles "<<count<<"\n";
    for(unsigned c=0;c<count;++c)
        o<<Name(c)<<" "<<(c*200)<<" 1000 100 100 3 3\n";
    return o.str();
}
static std::string Tile(unsigned col,float height=-10.f,bool gap=false){
    std::vector<uint8_t> b={'S','I','T','K','A','B','0','1'};
    U32(b,3u);U32(b,3u);
    F64(b,col*200.);F64(b,1000.);F64(b,100.);F64(b,100.);
    U32(b,BathyDatumId("NAVD88"));U32(b,0u);
    for(unsigned i=0;i<9;++i){
        F32(b,gap&&i==4?0.f:height);
        b.push_back(gap&&i==4?0u:1u);
    }
    return std::string(b.begin(),b.end());
}
static bool Invalid(const std::string& contents){
    try{(void)ParseBathyStreamIndex(contents);return false;}
    catch(const std::invalid_argument&){return true;}
}
int main(int argc,char** argv){
    assert(Invalid("SITKA-BATHY-INDEX-2\ndatum NAVD88\ntiles 1\n../evil 0 1000 100 100 3 3\n"));
    assert(Invalid("SITKA-BATHY-INDEX-2\ndatum NAVD88\ntiles 1\nbathymetry-r00-c00.sitkab 0 1000 100 100 3 3\nEXTRA\n"));
    assert(Invalid("SITKA-BATHY-INDEX-2\ndatum NAVD88\ntiles 2\nbathymetry-r00-c00.sitkab 0 1000 100 100 3 3\nbathymetry-r00-c01.sitkab 100 1000 100 100 3 3\n"));
    assert(Invalid("SITKA-BATHY-INDEX-2\ndatum MLLW\ntiles 1\nbathymetry-r00-c00.sitkab 0 1000 100 100 3 3\nbad\n"));
    const auto dir=ParseBathyStreamIndex(Index(40));
    assert(dir.regions.size()==40u&&dir.Locate(10.,950.)==0);
    assert(dir.Locate(201.,950.)==1&&dir.Locate(7999.,950.)==39);
    assert(dir.Locate(8000.,950.)==-1&&dir.Locate(-1.,950.)==-1);
    BathyStreamCache cache(dir);
    std::ofstream csv;
    if(argc>1){
        csv.open(argv[1],std::ios::trunc);
        assert(csv.good());
        csv<<"event,east_m,requests,loaded,rejected,evicted,resident_tiles,resident_payload_bytes\n";
    }
    const auto trace=[&](const char* event,double east){
        const auto& st=cache.stats();
        if(csv)csv<<event<<","<<east<<","<<st.requests<<","
                  <<st.loaded<<","<<st.rejected<<","<<st.evicted<<","
                  <<st.resident_tiles<<","<<st.resident_bytes<<"\n";
    };
    trace("cold",10.);
    std::map<std::string,std::string> assets;
    for(unsigned c=0;c<40;++c)assets.emplace(Name(c),Tile(c));
    unsigned actual_reads=0;
    auto read=[&](const std::string& name,std::string& bytes){
        ++actual_reads;
        const auto found=assets.find(name);
        if(found==assets.end())return false;
        bytes=found->second;return true;
    };
    assert(!cache.Sample(10.,950.).known); // no eager loading
    assert(cache.stats().requests==0&&cache.stats().resident_tiles==0);
    assert(cache.Pump(10.,950.,4.,0.,read));
    assert(actual_reads==1&&cache.stats().resident_tiles==1);
    trace("first",10.);
    assert(cache.Sample(10.,950.).known);
    assert(cache.stats().resident_bytes==9u*5u);
    for(int i=0;i<90;++i)assert(!cache.Pump(10.,950.,4.,0.,read));
    assert(actual_reads==1); // no repetitive I/O during stable 30Hz ticks
    assert(!cache.Pump(170.,950.,4.,0.,read)); // 20m look-ahead remains in tile
    assert(cache.Pump(183.,950.,4.,0.,read)); // prefetch tile 1
    assert(cache.stats().resident_tiles==2u);
    trace("prefetch",183.);
    assert(cache.Sample(201.,950.).known); // boundary crossing without I/O
    assert(!cache.Pump(201.,950.,4.,0.,read));
    assert(actual_reads==2&&cache.stats().evicted==0u);
    // Boundary jitter does not load the same file again or evict current.
    for(int i=0;i<120;++i){
        const double east=i%2==0?199.9:200.1;
        assert(!cache.Pump(east,950.,0.,0.,read));
        assert(cache.Sample(east,950.).known);
    }
    assert(actual_reads==2);
    trace("boundary_jitter",200.);
    assert(cache.Pump(383.,950.,4.,0.,read)); // one eviction; pin current
    assert(cache.Sample(201.,950.).known);
    assert(cache.Sample(401.,950.).known);
    assert(!cache.Sample(10.,950.).known);
    assert(cache.stats().evicted==1u);
    for(unsigned c=3;c<40;++c){
        const double ahead=(c-1u)*200.+183.;
        assert(cache.Pump(ahead,950.,4.,0.,read));
        const double into=c*200.+1.;
        assert(cache.Sample(into,950.).known);
        assert(!cache.Pump(into,950.,4.,0.,read));
        assert(cache.stats().resident_tiles<=2u);
        assert(cache.stats().resident_bytes<=2u*9u*5u);
        trace("transition",into);
    }
    assert(actual_reads==40u&&cache.stats().loaded==40u);
    assert(cache.stats().evicted==38u);
    trace("route_completed",7990.);

    assert(cache.Sample(7990.,950.).known);
    assert(!cache.Sample(0.,950.).known);
    // Cached metadata never permits a mislabeled source to be installed.
    BathyStreamCache failed(ParseBathyStreamIndex(Index(2)));
    assets[Name(0)]=Tile(1);
    assert(!failed.Pump(10.,950.,0.,0.,read)); // origin mismatch
    assert(!failed.Sample(10.,950.).known);
    const unsigned after_fail=actual_reads;
    for(int i=0;i<14;++i)assert(!failed.Pump(10.,950.,0.,0.,read));
    assert(actual_reads==after_fail);
    assets[Name(0)]=Tile(0);
    assert(failed.Pump(10.,950.,0.,0.,read)); // backoff bounded, retry
    assert(failed.stats().rejected==1u);
    assert(failed.Sample(10.,950.).known);
    // No-data interior source pixel invalidates bilinear interpolation.
    assets[Name(1)]=Tile(1,-10.f,true);
    assert(failed.Pump(183.,950.,4.,0.,read));
    assert(!failed.Sample(250.,950.).known);
    assert(!failed.Sample(225.,995.).known); // hole cannot be bridged
    // Stream cache is consumable by the same boat sounder as the full atlas.
    const auto depth=FindBoatDepth(failed,10.,950.,1.,.5,true);
    assert(depth.status==DepthStatus::Safe&&
           std::abs(depth.water_depth_m-11.)<1e-5);
    SkiffNavigation skiff;
    assert(skiff.Launch(10.,950.,0.,.5,failed,1.,true));
    const auto water=[](double,double){return true;};
    for(int i=0;i<5;++i)skiff.Update(.1,1.,0.,1.,failed,water);
    assert(skiff.boat.launched&&
           skiff.sounder.reading().status==DepthStatus::Safe);
    cache.Clear();
    trace("cleared",7990.);
    assert(cache.stats().resident_tiles==0&&cache.stats().resident_bytes==0);
    assert(!cache.Sample(7990.,950.).known);
    std::cout<<"STREAM_METRICS route_m=8000 tile_count=40"
             <<" requests="<<actual_reads
             <<" streamed="<<cache.stats().loaded
             <<" evictions="<<cache.stats().evicted
             <<" resident_tiles="<<cache.stats().resident_tiles
             <<" resident_payload_bytes="<<cache.stats().resident_bytes
             <<"\n";
    std::cout<<"PASS Sitka 8km streamed bathymetry, 2 tiles, prefetch/jitter/eviction, fault retry, boat-depth safety\n";
}
