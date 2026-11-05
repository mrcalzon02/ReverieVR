#ifndef REVERIE_SITKA_BOAT_PANELS_H
#define REVERIE_SITKA_BOAT_PANELS_H
// CPU-only real-text layout for purchased Sitka instruments. Rendering uses
// the already licensed Lato atlas; never draw invented segmented faux letters.
// UI is explicitly game simulation, not a marine navigation aid.
#include "sitka_helm.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
namespace reverie { namespace sitka {
struct BoatLabel {
    std::string text;
    float x=0.f,y=0.f,z=0.f,width=.32f,scale=.010f;
    float red=.8f,green=1.f,blue=.85f;
};
struct BoatPanelLayout {
    std::array<BoatLabel,16> labels{};
    size_t count=0u;
    void Add(std::string content,float x,float y,float z,
             float width,float scale,float red,float green,float blue){
        if(count<labels.size())
            labels[count++]={std::move(content),x,y,z,width,scale,red,green,blue};
    }
    std::string Signature()const{
        std::string key=std::to_string(count);
        for(size_t i=0;i<count;++i){
            key.push_back('\n');key+=labels[i].text;
        }
        return key;
    }
};
// Physical dashboard glass faces -Z (toward the seated captain), so
// mirror newly emitted Lato glyph XYZ positions, never their atlas UVs.
inline void OrientBoatGlyphsAft(std::vector<float>& vertices,size_t first,float x){
    if(first>vertices.size()||(vertices.size()-first)%8u)
        throw std::invalid_argument("Sitka glyph stride invalid");
    for(size_t i=first;i<vertices.size();i+=8u)vertices[i]=2.f*x-vertices[i];
}
inline std::string HelmNumber(double value,unsigned decimals=1u){
    if(!std::isfinite(value))return "NO DATA";
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out<<std::fixed<<std::setprecision(decimals)<<value;
    return out.str();
}
inline std::string DepthQualityName(BedQuality quality){
    switch(quality){
        case BedQuality::Survey:return "SURVEY";
        case BedQuality::Composite:return "COMPOSITE";
        case BedQuality::Procedural:return "PROCEDURAL";
        default:return "NO DATA";
    }
}
inline std::string DepthConditionName(DepthStatus status){
    switch(status){
        case DepthStatus::Safe:return "SAFE";
        case DepthStatus::UnderKeel:return "SHALLOW";
        case DepthStatus::Dry:return "DRY";
        case DepthStatus::OutsideWater:return "OUTSIDE WATER";
        default:return "NO DATA";
    }
}
inline BoatPanelLayout BuildBoatPanels(const HelmProgress& progress,
    const BoatPose& boat,const Weather& weather,const DepthReading& depth,
    double throttle){
    BoatPanelLayout panel;
    if(!boat.launched)return panel;
    const auto line=[&](const std::string& value,HelmItem item,float offset,
                       float width,float size,float r,float g,float b){
        const auto& mount=kParts[static_cast<unsigned>(item)];
        // The dark glass faces aft. Alpha glyphs sit 1 mm ahead of the
        // actual screen face so depth testing does not clip their strokes.
        panel.Add(value,mount.x,mount.y+offset,mount.z-.041f,
                  width,size,r,g,b);
    };
    if(progress.Has(HelmItem::Sonar)){
        const auto data=BoatSounderLabels(depth);
        line(data[0],HelmItem::Sonar,.080f,.32f,.010f,.72f,1.f,.72f);
        if(depth.status==DepthStatus::Safe||
           depth.status==DepthStatus::UnderKeel){
            line("KEEL "+HelmNumber(depth.under_keel_m)+" m",
                 HelmItem::Sonar,.028f,.32f,.009f,.83f,1.f,.83f);
            line(DepthConditionName(depth.status)+" / "+
                 DepthQualityName(depth.quality),
                 HelmItem::Sonar,-.026f,.32f,.008f,.80f,.96f,.84f);
        }else{
            line(DepthConditionName(depth.status),
                 HelmItem::Sonar,.020f,.32f,.010f,.92f,.85f,.67f);
        }
        line("SIM - NOT FOR NAV",
             HelmItem::Sonar,-.081f,.34f,.006f,.94f,.84f,.65f);
    }
    if(progress.Has(HelmItem::Gps)){
        line("CHARTPLOTTER",HelmItem::Gps,.062f,.32f,.010f,.72f,.90f,.96f);
        line("NO MAP DATA",HelmItem::Gps,-.025f,.32f,.010f,.97f,.80f,.63f);
    }
    if(progress.Has(HelmItem::Radar)){
        line("WEATHER "+weather.kind,HelmItem::Radar,.064f,.32f,.009f,
             .74f,.94f,.80f);
        line("RADAR UNAVAILABLE",HelmItem::Radar,-.026f,.32f,.008f,
             .93f,.80f,.63f);
    }
    if(progress.Has(HelmItem::Overhead)){
        const double turns=boat.heading*180./3.14159265358979323846;
        const int degrees=int(std::lround(std::fmod(
            std::fmod(turns,360.)+360.,360.)))%360;
        const int percent=std::isfinite(throttle)?
            int(std::lround(std::clamp(throttle,0.,1.)*100.)):0;
        std::ostringstream info;
        info.imbue(std::locale::classic());
        info<<"HDG "<<std::setw(3)<<std::setfill('0')<<degrees
            <<"  THR "<<percent<<"%";
        panel.Add(info.str(),0.f,1.484f,.498f,.72f,.012f,.83f,.96f,1.f);
    }
    return panel;
}
}} // namespace reverie::sitka
#endif
