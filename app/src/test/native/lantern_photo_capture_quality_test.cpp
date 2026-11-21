// Genuine native pixel pipeline acceptance; no invented camera imagery or
// game-world truth proxy. Compiles in both GCC and Clang, C++17, without GLES.
#include "lantern_photo_capture_quality.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
using namespace reverie::lantern::capture;

static unsigned Sum(const std::vector<uint8_t>& v){
    return std::accumulate(v.begin(),v.end(),0u);
}
static uint64_t Difference(const std::vector<uint8_t>& a,
                           const std::vector<uint8_t>& b){
    assert(a.size()==b.size());uint64_t sum=0;
    for(size_t i=0;i<a.size();++i)
        sum+=static_cast<unsigned>(std::abs(int(a[i])-int(b[i])));
    return sum;
}
static GrayPlate ReferencePlate(){
    GrayPlate base{512,512,{}};
    base.pixels.resize(512u*512u);
    // Registered source contains actual fine and medium details, not a
    // model-generated target, detection label, or 16x16 evidence surrogate.
    for(int y=0;y<512;++y)for(int x=0;x<512;++x)
        base.pixels[static_cast<size_t>(y)*512+x]=
            static_cast<uint8_t>((x/2)%2?205:35);
    return base;
}
int main(){
    const auto plate=ReferencePlate();
    assert(plate.Valid());
    Conditions clear{};Camera clean{};
    CapturedMaster baseline;
    assert(Record(plate,clear,clean,&baseline));
    assert(baseline.recorded.pixels==plate.pixels);
    assert(Sum(baseline.masks.cloud)==0u);
    assert(Sum(baseline.masks.precipitation)==0u);
    assert(Sum(baseline.masks.lens_grime)==0u);
    GrayPlate low,mid,full;
    assert(Develop(baseline,0,&low)&&Develop(baseline,1,&mid)&&
           Develop(baseline,4,&full));
    assert(full.pixels==plate.pixels&&low.Valid()&&mid.Valid());
    assert(Difference(low.pixels,full.pixels)>0u);
    assert(Difference(mid.pixels,full.pixels)<Difference(low.pixels,full.pixels));
    assert(baseline.recorded.pixels==plate.pixels);
    assert(!Develop(baseline,5,&full));
    assert(full.pixels==plate.pixels); // invalid tier did not destroy result

    Conditions weather{};
    weather.cloud=Cloud::BrokenCumulus;
    weather.precipitation=Precipitation::Rain;
    weather.weather_seed=793u;
    weather.wind_east_mps=7.f;weather.wind_north_mps=-3.f;
    weather.exposure_seconds=150.f;
    weather.west_m=1200.f;weather.north_m=3400.f;
    weather.meters_per_pixel=3.f;
    weather.ambient_haze=.19f;
    Camera lens{};lens.grime=.58f;lens.grime_seed=513u;lens.sensor_noise=.16f;
    CapturedMaster first,second;
    assert(Record(plate,weather,lens,&first));
    assert(Record(plate,weather,lens,&second));
    assert(first.recorded.pixels==second.recorded.pixels);
    assert(first.masks.cloud==second.masks.cloud);
    assert(first.masks.precipitation==second.masks.precipitation);
    assert(first.masks.lens_grime==second.masks.lens_grime);
    assert(Sum(first.masks.cloud)>0u&&Sum(first.masks.haze)>0u);
    assert(Sum(first.masks.lens_grime)>0u);
    assert(first.masks.width<=256&&first.masks.height<=256);
    assert(first.recorded.pixels!=plate.pixels);
    assert(Develop(first,0,&low)&&Develop(first,4,&full));
    assert(full.pixels==first.recorded.pixels);
    assert(Difference(low.pixels,full.pixels)>0u);
    // Improving the presentation cannot recapture cloud-hidden targets,
    // erase optical grime, or manufacture information from World Truth.
    assert(first.recorded.pixels==second.recorded.pixels);
    assert(first.camera.grime==lens.grime);

    // Identifiable types and real wind/time advection are deterministic.
    const auto originalCloud=first.masks.cloud;
    weather.exposure_seconds+=180.f;
    CapturedMaster later;
    assert(Record(plate,weather,lens,&later));
    assert(later.masks.cloud!=originalCloud);
    assert(later.masks.lens_grime==first.masks.lens_grime);
    weather.exposure_seconds-=180.f;
    weather.wind_east_mps=0.f;weather.wind_north_mps=11.f;
    CapturedMaster turnedWind;
    assert(Record(plate,weather,lens,&turnedWind));
    assert(turnedWind.masks.cloud!=originalCloud);
    assert(turnedWind.masks.lens_grime==first.masks.lens_grime);
    weather.wind_east_mps=7.f;weather.wind_north_mps=-3.f;
    weather.cloud=Cloud::Cirrus;
    CapturedMaster cirrus;
    assert(Record(plate,weather,lens,&cirrus));
    assert(cirrus.masks.cloud!=originalCloud);
    for(Cloud type:{Cloud::ScatteredCumulus,Cloud::Stratus,
                    Cloud::Overcast,Cloud::Squall}){
        weather.cloud=type;
        CapturedMaster variant;
        assert(Record(plate,weather,lens,&variant));
        assert(variant.masks.cloud!=originalCloud);
    }
    weather.cloud=Cloud::BrokenCumulus;
    weather.precipitation=Precipitation::Rain;
    CapturedMaster rain;
    assert(Record(plate,weather,lens,&rain));
    const unsigned rainPower=Sum(rain.masks.precipitation);
    for(Precipitation type:{Precipitation::Storm,Precipitation::HeavyStorm,
                            Precipitation::Snow,Precipitation::Hail}){
        weather.precipitation=type;
        CapturedMaster variant;
        assert(Record(plate,weather,lens,&variant));
        assert(variant.masks.precipitation!=rain.masks.precipitation);
        assert(variant.masks.lens_grime==rain.masks.lens_grime);
        if(type==Precipitation::HeavyStorm)
            assert(Sum(variant.masks.precipitation)>rainPower);
    }

    // Changing lens identity/cleanliness affects grime, never weather.
    Camera newlyClean=lens;newlyClean.grime=0.f;
    CapturedMaster pristine;
    assert(Record(plate,weather,newlyClean,&pristine));
    assert(Sum(pristine.masks.lens_grime)==0u);
    assert(pristine.masks.cloud==rain.masks.cloud);
    Camera otherLens=lens;otherLens.grime_seed++;
    CapturedMaster differentLens;
    assert(Record(plate,weather,otherLens,&differentLens));
    assert(differentLens.masks.lens_grime!=rain.masks.lens_grime);
    assert(differentLens.masks.cloud==rain.masks.cloud);
    // Large sensor masters are allowed; layers remain bounded regardless
    // of resolution and never instantiate full-resolution 5x masks.
    MaskLayers large;
    assert(BuildMasks(4096,4096,weather,lens,&large));
    assert(large.width==256&&large.height==256&&
           large.cloud.size()==256u*256u);
    assert(!BuildMasks(4097,4096,weather,lens,&large));
    assert(large.width==256&&large.height==256);

    const auto before=first.recorded.pixels;
    weather.ambient_haze=std::numeric_limits<float>::quiet_NaN();
    assert(!Record(plate,weather,lens,&first));
    assert(first.recorded.pixels==before);
    weather.ambient_haze=.19f;
    weather.cloud=static_cast<Cloud>(255);
    assert(!Record(plate,weather,lens,&first));
    assert(first.recorded.pixels==before);
    assert(!Record(GrayPlate{},clear,clean,&first));
    assert(first.recorded.pixels==before);
    std::cout<<"PASS Lantern camera: 6 cloud families plus clear, rain/storm/heavy/snow/hail, wind advection, persistent lens grime, seeded capture and progressive no-hallucination resolution\n";
}
