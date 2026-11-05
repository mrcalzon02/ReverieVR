#include "sitka_boat_panels.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::sitka;
static bool has(const BoatPanelLayout& p,const std::string& needle){
    for(size_t i=0;i<p.count;++i)
        if(p.labels[i].text.find(needle)!=std::string::npos)return true;
    return false;
}
int main(){
    HelmProgress progress;
    Weather weather;weather.kind="RAIN";
    BoatPose boat;
    const DepthReading procedural{DepthStatus::Safe,12.,11.5,
                                  BedQuality::Procedural};
    assert(BuildBoatPanels(progress,boat,weather,procedural,0.).count==0u);
    boat.launched=true;boat.heading=1.5707963267948966;
    assert(BuildBoatPanels(progress,boat,weather,procedural,0.).count==0u);
    progress.owned=HelmMask(HelmItem::Battery)|HelmMask(HelmItem::Sonar);
    const auto sonar=BuildBoatPanels(progress,boat,weather,procedural,.5);
    assert(sonar.count==4u&&has(sonar,"DEPTH 12.0 m"));
    assert(has(sonar,"KEEL 11.5 m")&&has(sonar,"PROCEDURAL"));
    assert(has(sonar,"NOT FOR NAV"));
    assert(sonar.Signature()==BuildBoatPanels(
        progress,boat,weather,procedural,.5).Signature());
    for(size_t i=0;i<sonar.count;++i){
        assert(sonar.labels[i].width>.30f);
        assert(sonar.labels[i].z<kParts[unsigned(HelmItem::Sonar)].z-.04f);
    }
    auto missing=BuildBoatPanels(progress,boat,weather,
        {DepthStatus::NoCoverage,0.,0.,BedQuality::Missing},.5);
    assert(has(missing,"NO DATA")&&!has(missing,"KEEL 11.5"));
    progress.owned|=HelmMask(HelmItem::Gps)|
        HelmMask(HelmItem::Roof)|HelmMask(HelmItem::Radar)|
        HelmMask(HelmItem::Overhead);
    auto all=BuildBoatPanels(progress,boat,weather,procedural,.5);
    assert(all.count<=all.labels.size());
    assert(has(all,"CHARTPLOTTER")&&has(all,"NO MAP DATA"));
    assert(has(all,"WEATHER RAIN")&&has(all,"RADAR UNAVAILABLE"));
    assert(has(all,"HDG 090")&&has(all,"THR 50%"));
    assert(!has(all,"FUEL 100")&&!has(all,"STORM DISTANCE"));
    assert(all.Signature()!=sonar.Signature());
    weather.kind="FOG";
    assert(all.Signature()!=BuildBoatPanels(
        progress,boat,weather,procedural,.5).Signature());
    std::vector<float> glyph={.2f,.3f,.4f,.1f,.2f,1.f,1.f,1.f,
                              .4f,.5f,.6f,.3f,.4f,1.f,1.f,1.f};
    OrientBoatGlyphsAft(glyph,0u,.3f);
    assert(std::abs(glyph[0]-.4f)<1.e-6f);
    assert(std::abs(glyph[8]-.2f)<1.e-6f);
    assert(std::abs(glyph[1]-.3f)<1.e-6f&&
           std::abs(glyph[3]-.1f)<1.e-6f);
    std::cout<<"PASS Sitka Lato panel data gates, no fake charts, provenance and boat state\n";
}
