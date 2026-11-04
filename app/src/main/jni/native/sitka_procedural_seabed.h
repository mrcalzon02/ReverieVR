#ifndef REVERIE_SITKA_PROCEDURAL_SEABED_H
#define REVERIE_SITKA_PROCEDURAL_SEABED_H
// Pure, coordinate-seeded procedural seabed for GAME coverage gaps.
// Survey/composite data always has priority. Values are never chart facts.
#include "sitka_bathymetry.h"
#include <cmath>
#include <cstdint>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace reverie { namespace sitka {
struct SeabedGuide {
    double east=0.,north=0.,elevation=0.;
};
struct SeabedGeneration {
    uint64_t seed=726392041u;
    uint32_t datum_id=0u;
    double water_reference_m=0.;
    double influence_radius_m=1500.;
    std::vector<SeabedGuide> guides;
};
inline SeabedGeneration ParseSeabedGeneration(
    const std::string& source,uint32_t expected_datum){
    constexpr char magic[]="SITKA-PROCEDURAL-1\n";
    if(source.size()<sizeof(magic)||source.size()>32000u||
       source.compare(0,sizeof(magic)-1u,magic)!=0)
        throw std::invalid_argument("invalid Sitka procedural guide version");
    std::istringstream in(source.substr(sizeof(magic)-1u));
    in.imbue(std::locale::classic());
    std::string key,datum,seedstr,countstr;
    SeabedGeneration g;
    if(!(in>>key>>datum)||key!="datum"||
       (g.datum_id=BathyDatumId(datum))==0u||
       g.datum_id!=expected_datum)
        throw std::invalid_argument("procedural guide vertical datum mismatch");
    if(!(in>>key>>seedstr)||key!="seed"||seedstr.empty()||
       seedstr.size()>20u)
        throw std::invalid_argument("bad procedural world seed");
    uint64_t number=0u;
    for(char c:seedstr){
        if(c<'0'||c>'9')throw std::invalid_argument("nonnumeric world seed");
        const unsigned digit=static_cast<unsigned>(c-'0');
        if(number>(std::numeric_limits<uint64_t>::max()-digit)/10u)
            throw std::invalid_argument("world seed overflow");
        number=number*10u+digit;
    }
    g.seed=number;
    if(!(in>>key>>g.water_reference_m)||key!="water_ref"||
       !std::isfinite(g.water_reference_m)||
       std::abs(g.water_reference_m)>100.)
        throw std::invalid_argument("invalid game datum reference");
    if(!(in>>key>>g.influence_radius_m)||key!="radius"||
       !std::isfinite(g.influence_radius_m)||
       g.influence_radius_m<10.||g.influence_radius_m>5000.)
        throw std::invalid_argument("invalid proximal influence radius");
    if(!(in>>key>>countstr)||key!="anchors"||countstr.empty()||
       countstr.size()>3u)
        throw std::invalid_argument("invalid guidepoint count");
    unsigned count=0u;
    for(char c:countstr){
        if(c<'0'||c>'9')throw std::invalid_argument("invalid count digits");
        count=count*10u+static_cast<unsigned>(c-'0');
    }
    if(count>256u)throw std::invalid_argument("too many guidepoints");
    g.guides.reserve(count);
    for(unsigned i=0u;i<count;++i){
        SeabedGuide point;
        if(!(in>>point.east>>point.north>>point.elevation)||
           !std::isfinite(point.east)||!std::isfinite(point.north)||
           !std::isfinite(point.elevation)||
           std::abs(point.east)>1e8||std::abs(point.north)>1e8||
           point.elevation< -12000.||point.elevation>100.)
            throw std::invalid_argument("invalid proximal guidepoint");
        g.guides.push_back(point);
    }
    if(in>>key)throw std::invalid_argument("unexpected guidepoint payload");
    return g;
}
inline uint64_t SeabedMix(uint64_t x){
    x^=x>>30u;x*=UINT64_C(0xbf58476d1ce4e5b9);
    x^=x>>27u;x*=UINT64_C(0x94d049bb133111eb);
    return x^(x>>31u);
}
class ProceduralSeabed {
public:
    explicit ProceduralSeabed(SeabedGeneration config)
        : settings_(std::move(config)){
        if(!settings_.datum_id||!std::isfinite(settings_.water_reference_m)||
           std::abs(settings_.water_reference_m)>100.||
           !std::isfinite(settings_.influence_radius_m)||
           settings_.influence_radius_m<10.||
           settings_.influence_radius_m>5000.||
           settings_.guides.size()>256u)
            throw std::invalid_argument("invalid procedural world configuration");
    }
    const SeabedGeneration& settings()const{return settings_;}
    BedSample Sample(double east,double north)const{
        if(!std::isfinite(east)||!std::isfinite(north)||
           std::abs(east)>1e8||std::abs(north)>1e8)return {};
        double height=Base(east,north);
        double delta=0.,weight_sum=0.,closest=1e100;
        const double radius2=settings_.influence_radius_m*
                             settings_.influence_radius_m;
        for(const auto& p:settings_.guides){
            const double dx=east-p.east,dz=north-p.north;
            const double d2=dx*dx+dz*dz;
            if(d2<1e-8)
                return {true,p.elevation,BedQuality::Procedural};
            if(d2>=radius2)continue;
            const double u=1.-d2/radius2;
            const double w=(u*u)/(d2+100.);
            delta+=w*(p.elevation-Base(p.east,p.north));
            weight_sum+=w;
            if(d2<closest)closest=d2;
        }
        if(weight_sum>0.){
            const double d=std::sqrt(closest)/settings_.influence_radius_m;
            const double blend=(1.-d)*(1.-d)*(1.+2.*d);
            height+=blend*(delta/weight_sum);
        }
        // Procedural, not claimed hydrographic: bounded so the geometry never
        // catastrophically exceeds low-poly inland/underwater scale.
        return {true,std::max(-500.,std::min(100.,height)),
                BedQuality::Procedural};
    }
    template<class WaterPredicate>
    BathyTile GenerateTile(double east,double north,WaterPredicate&& in_water,
                           double grid_step_m=16.)const{
        if(!std::isfinite(east)||!std::isfinite(north)||
           !std::isfinite(grid_step_m)||grid_step_m<4.||
           grid_step_m>32.||std::abs(east)>1e8||std::abs(north)>1e8)
            throw std::invalid_argument("invalid procedural tile location");
        constexpr double span=256.;
        const double first_east=std::floor(east/span)*span;
        const double first_north=std::ceil(north/span)*span;
        const unsigned steps=static_cast<unsigned>(std::ceil(span/grid_step_m));
        BathyTile tile;
        tile.columns=steps+1u;tile.rows=steps+1u;
        tile.first_east=first_east;tile.first_north=first_north;
        tile.east_step=span/steps;tile.north_step=span/steps;
        tile.datum_id=settings_.datum_id;
        tile.elevations_m.reserve(size_t(tile.columns)*tile.rows);
        tile.qualities.reserve(size_t(tile.columns)*tile.rows);
        for(unsigned row=0;row<tile.rows;++row)
            for(unsigned col=0;col<tile.columns;++col){
                const double x=first_east+col*tile.east_step;
                const double z=first_north-row*tile.north_step;
                const bool water=in_water(x,z);
                const BedSample bed=water?Sample(x,z):BedSample{};
                tile.elevations_m.push_back(
                    water?static_cast<float>(bed.elevation_m):0.f);
                tile.qualities.push_back(
                    water?BedQuality::Procedural:BedQuality::Missing);
            }
        return tile;
    }
private:
    // Stable C++17, world-coordinate hash; no mutable RNG state, order,
    // camera pose or frame timing in generated content.
    double Lattice(int64_t x,int64_t z)const{
        const uint64_t h=SeabedMix(
            settings_.seed^SeabedMix(static_cast<uint64_t>(x))^
            SeabedMix(static_cast<uint64_t>(z)+UINT64_C(0x9e3779b97f4a7c15)));
        return (static_cast<double>(h>>11u)*(1./9007199254740992.))*2.-1.;
    }
    double Noise(double east,double north,double scale)const{
        const double x=east/scale,z=north/scale;
        const double fx=std::floor(x),fz=std::floor(z);
        const int64_t ix=static_cast<int64_t>(fx),iz=static_cast<int64_t>(fz);
        double u=x-fx,v=z-fz;
        u=u*u*u*(u*(u*6.-15.)+10.);
        v=v*v*v*(v*(v*6.-15.)+10.);
        const double lo=Lattice(ix,iz)*(1.-u)+Lattice(ix+1,iz)*u;
        const double hi=Lattice(ix,iz+1)*(1.-u)+Lattice(ix+1,iz+1)*u;
        return lo*(1.-v)+hi*v;
    }
    double Base(double east,double north)const{
        const double broad=Noise(east,north,1536.);
        const double fjord=Noise(east,north,384.);
        const double detail=Noise(east,north,96.);
        const double depth=20.+14.*broad+8.*fjord+3.*detail;
        return settings_.water_reference_m-std::max(2.,depth);
    }
    SeabedGeneration settings_;
};
template<class SurveySource>
class MixedSeabed {
public:
    MixedSeabed(const SurveySource& survey,const ProceduralSeabed& generated)
        : survey_(survey),generated_(generated){}
    BedSample Sample(double east,double north)const{
        const BedSample source=survey_.Sample(east,north);
        return source.known?source:generated_.Sample(east,north);
    }
private:
    const SurveySource& survey_;
    const ProceduralSeabed& generated_;
};
template<class WaterPredicate>
inline BathyTile FillSeabedGaps(const BathyTile& original,
    const ProceduralSeabed& generated,WaterPredicate&& in_water){
    BathyTile filled=original;
    if(generated.settings().datum_id!=filled.datum_id)
        throw std::invalid_argument("procedural and survey datum differ");
    for(unsigned row=0;row<filled.rows;++row)
        for(unsigned col=0;col<filled.columns;++col){
            const size_t i=size_t(row)*filled.columns+col;
            if(filled.qualities[i]!=BedQuality::Missing)continue;
            const double east=filled.first_east+col*filled.east_step;
            const double north=filled.first_north-row*filled.north_step;
            if(!in_water(east,north))continue;
            const BedSample guess=generated.Sample(east,north);
            if(!guess.known)continue;
            filled.elevations_m[i]=static_cast<float>(guess.elevation_m);
            filled.qualities[i]=BedQuality::Procedural;
        }
    return filled;
}
}} // namespace
#endif
