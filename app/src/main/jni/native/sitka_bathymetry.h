#ifndef REVERIE_SITKA_BATHYMETRY_H
#define REVERIE_SITKA_BATHYMETRY_H
// Game-private, deterministic source-aware seabed sampler and boat depth gauge.
// Data MUST come from an independently licensed, projected-metre raster with
// datum reconciled to terrain and water. It is never navigation equipment.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
namespace reverie { namespace sitka {

constexpr unsigned kBathyMaxAxis=513u;
constexpr size_t kBathyHeaderBytes=56u;
constexpr unsigned kBathyMaxTiles=16u;
enum class BedQuality : uint8_t { Missing=0, Survey=1, Composite=2, Procedural=3 };
struct BedSample {
    bool known=false;
    double elevation_m=0.;
    BedQuality quality=BedQuality::Missing;
};
inline uint32_t BathyU32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1])<<8u) |
           (uint32_t(p[2])<<16u) | (uint32_t(p[3])<<24u);
}
inline double BathyF64(const uint8_t* p) {
    uint64_t bits=0;
    for(unsigned i=0;i<8;++i)bits|=uint64_t(p[i])<<(8u*i);
    double result;
    std::memcpy(&result,&bits,sizeof(result));
    return result;
}
inline float BathyF32(const uint8_t* p) {
    const uint32_t bits=BathyU32(p);
    float result;
    std::memcpy(&result,&bits,sizeof(result));
    return result;
}
// FNV1a over the canonical source-datum string, used to reject mismatched
// tiles. Not a data integrity hash: manifests carry SHA-256 separately.
inline uint32_t BathyDatumId(const std::string& name) {
    if(name.empty()||name.size()>96)return 0u;
    uint32_t hash=2166136261u;
    for(unsigned char c:name){
        if(c<=32u||c>126u)return 0u;
        hash=(hash^c)*16777619u;
    }
    return hash==0u?0u:hash;
}
struct BathyTile {
    unsigned columns=0,rows=0;
    double first_east=0.,first_north=0.,east_step=0.,north_step=0.;
    uint32_t datum_id=0u;
    std::vector<float> elevations_m;
    std::vector<BedQuality> qualities;
    bool Contains(double east,double north) const {
        if(!std::isfinite(east)||!std::isfinite(north))return false;
        const double a=(east-first_east)/east_step;
        const double b=(first_north-north)/north_step;
        return a>=0.&&b>=0.&&a<double(columns-1)&&b<double(rows-1);
    }
    BedSample Sample(double east,double north) const {
        if(!Contains(east,north))return {};
        const double a=(east-first_east)/east_step;
        const double b=(first_north-north)/north_step;
        const unsigned col=static_cast<unsigned>(std::floor(a));
        const unsigned row=static_cast<unsigned>(std::floor(b));
        if(col+1>=columns||row+1>=rows)return {};
        const size_t i=size_t(row)*columns+col;
        const size_t ids[4]={i,i+1,i+columns,i+columns+1};
        BedQuality worst=BedQuality::Survey;
        for(size_t index:ids){
            if(qualities[index]==BedQuality::Missing)return {};
            if(qualities[index]==BedQuality::Composite)worst=BedQuality::Composite;
        }
        const double u=a-col,v=b-row;
        const double top=(1.-u)*elevations_m[ids[0]]+
                                u*elevations_m[ids[1]];
        const double bottom=(1.-u)*elevations_m[ids[2]]+
                                   u*elevations_m[ids[3]];
        return {true,(1.-v)*top+v*bottom,worst};
    }
};
inline BathyTile ParseBathymetry(const uint8_t* bytes,size_t size,
                                uint32_t expected_datum) {
    static const uint8_t magic[8]={'S','I','T','K','A','B','0','1'};
    if(!bytes||size<kBathyHeaderBytes||
       std::memcmp(bytes,magic,8u)!=0)
        throw std::invalid_argument("invalid Sitka seafloor tile magic");
    const uint32_t cols=BathyU32(bytes+8u),rows=BathyU32(bytes+12u);
    if(cols<2u||cols>kBathyMaxAxis||rows<2u||rows>kBathyMaxAxis)
        throw std::invalid_argument("invalid bathymetry tile dimensions");
    const size_t count=size_t(cols)*size_t(rows);
    if(size!=kBathyHeaderBytes+count*5u)
        throw std::invalid_argument("bathymetry byte length mismatch");
    BathyTile tile;
    tile.columns=cols;tile.rows=rows;
    tile.first_east=BathyF64(bytes+16u);
    tile.first_north=BathyF64(bytes+24u);
    tile.east_step=BathyF64(bytes+32u);
    tile.north_step=BathyF64(bytes+40u);
    tile.datum_id=BathyU32(bytes+48u);
    if(BathyU32(bytes+52u)!=0u||tile.datum_id==0u||
       tile.datum_id!=expected_datum||
       !std::isfinite(tile.first_east)||!std::isfinite(tile.first_north)||
       !std::isfinite(tile.east_step)||!std::isfinite(tile.north_step)||
       std::abs(tile.first_east)>1.e8||std::abs(tile.first_north)>1.e8||
       tile.east_step<=0.||tile.north_step<=0.||
       tile.east_step>500.||tile.north_step>500.)
        throw std::invalid_argument("unreconciled Sitka bathymetry datum/CRS");
    tile.elevations_m.reserve(count);
    tile.qualities.reserve(count);
    unsigned available=0u;
    for(size_t i=0;i<count;++i){
        const uint8_t* p=bytes+kBathyHeaderBytes+i*5u;
        const float z=BathyF32(p);
        const uint8_t quality=p[4];
        if(quality>2u||!std::isfinite(z)||
           (quality!=0u && (z< -12000.f || z>100.f))||
           (quality==0u&&z!=0.f))
            throw std::invalid_argument("invalid Sitka bathymetry sample");
        available+=quality!=0u?1u:0u;
        tile.elevations_m.push_back(z);
        tile.qualities.push_back(static_cast<BedQuality>(quality));
    }
    if(!available)throw std::invalid_argument("empty bathymetric coverage");
    return tile;
}
struct BathyIndex {
    uint32_t datum_id=0u;
    std::string datum_name;
    std::vector<std::string> filenames;
};
inline BathyIndex ParseBathyIndex(const std::string& text){
    constexpr const char* header="SITKA-BATHY-INDEX-1\n";
    constexpr size_t length=20u;
    if(text.size()>2048u||text.compare(0,length,header)!=0)
        throw std::invalid_argument("invalid Sitka bathymetry index version");
    size_t cursor=length;
    const auto next=[&](size_t& pos)->std::string{
        const size_t end=text.find('\n',pos);
        if(end==std::string::npos)throw std::invalid_argument("unterminated index line");
        const std::string value=text.substr(pos,end-pos);
        pos=end+1u;return value;
    };
    BathyIndex index;
    const std::string datum=next(cursor);
    if(datum.compare(0u,6u,"datum ")!=0)
        throw std::invalid_argument("bathymetry index datum absent");
    index.datum_name=datum.substr(6u);
    index.datum_id=BathyDatumId(index.datum_name);
    if(!index.datum_id)throw std::invalid_argument("invalid bathymetry index datum");
    const std::string count=next(cursor);
    if(count.compare(0u,6u,"tiles ")!=0||
       count.size()<7u||count.size()>8u)
        throw std::invalid_argument("invalid bathymetry tile count");
    unsigned total=0u;
    for(char c:count.substr(6u)){
        if(c<'0'||c>'9')throw std::invalid_argument("invalid tile count digits");
        total=total*10u+unsigned(c-'0');
    }
    if(total<1u||total>kBathyMaxTiles)
        throw std::invalid_argument("index exceeds tile limit");
    for(unsigned i=0u;i<total;++i){
        const std::string name=next(cursor);
        if(name.size()!=25u||name.compare(0u,12u,"bathymetry-r")!=0||
           name[14]!='-'||name[15]!='c'||
           name.compare(18u,7u,".sitkab")!=0||
           name[12]<'0'||name[12]>'9'||
           name[13]<'0'||name[13]>'9'||
           name[16]<'0'||name[16]>'9'||
           name[17]<'0'||name[17]>'9')
            throw std::invalid_argument("invalid bathymetry tile filename");
        for(const auto& prior:index.filenames)
            if(prior==name)throw std::invalid_argument("duplicate bathymetry tile");
        index.filenames.push_back(name);
    }
    if(cursor!=text.size())throw std::invalid_argument("trailing bathymetry index bytes");
    return index;
}

class BathyAtlas {
public:
    bool Add(BathyTile tile){
        if(tiles_.size()>=kBathyMaxTiles||tile.columns<2u)return false;
        if(!tiles_.empty()&&tile.datum_id!=tiles_[0].datum_id)return false;
        // Reject overlapping tile regions: no accidental source priority.
        const double a0=tile.first_east,a1=a0+(tile.columns-1)*tile.east_step;
        const double b1=tile.first_north,b0=b1-(tile.rows-1)*tile.north_step;
        for(const auto& old:tiles_){
            const double c0=old.first_east;
            const double c1=c0+(old.columns-1)*old.east_step;
            const double d1=old.first_north;
            const double d0=d1-(old.rows-1)*old.north_step;
            if(a0<c1&&a1>c0&&b0<d1&&b1>d0)return false;
        }
        tiles_.push_back(std::move(tile));return true;
    }
    BedSample Sample(double east,double north)const{
        for(const auto& tile:tiles_)if(tile.Contains(east,north))
            return tile.Sample(east,north);
        return {};
    }
    size_t tile_count()const{return tiles_.size();}
    const std::vector<BathyTile>& Tiles()const{return tiles_;}
private:
    std::vector<BathyTile> tiles_;
};
enum class DepthStatus : uint8_t {
    NoCoverage, OutsideWater, Dry, UnderKeel, Safe
};
struct DepthReading {
    DepthStatus status=DepthStatus::NoCoverage;
    double water_depth_m=0.,under_keel_m=0.;
    BedQuality quality=BedQuality::Missing;
};
template<class BathySampler>
inline DepthReading FindBoatDepth(const BathySampler& atlas,double east,
        double north,double sea_surface_elevation_m,double boat_draft_m,
        bool verified_water){
    if(!verified_water)return {DepthStatus::OutsideWater,0.,0.,BedQuality::Missing};
    if(!std::isfinite(east)||!std::isfinite(north)||
       !std::isfinite(sea_surface_elevation_m)||
       !std::isfinite(boat_draft_m)||boat_draft_m<0.||
       boat_draft_m>10.||std::abs(sea_surface_elevation_m)>100.)
        return {};
    const BedSample bed=atlas.Sample(east,north);
    if(!bed.known)return {};
    const double depth=sea_surface_elevation_m-bed.elevation_m;
    if(depth<=0.)return {DepthStatus::Dry,0.,-boat_draft_m,bed.quality};
    const double clearance=depth-boat_draft_m;
    return {clearance<=0.?DepthStatus::UnderKeel:DepthStatus::Safe,
            depth,clearance,bed.quality};
}
// Simulation update cadence, not one sample per stereo eye.
// Internal readings are stable between ticks, even while moving.
class BoatDepthFinder {
public:
    void Reset(){elapsed_=0.;reading_={};}
    template<class BathySampler>
    bool Update(double dt,const BathySampler& atlas,double east,double north,
                double sea_surface_elevation_m,double boat_draft_m,
                bool verified_water){
        if(!std::isfinite(dt)||dt<0.||dt>.1)return false;
        elapsed_+=dt;
        if(elapsed_+1e-8<.25)return false;
        elapsed_=std::fmod(elapsed_,.25);
        reading_=FindBoatDepth(atlas,east,north,sea_surface_elevation_m,
                               boat_draft_m,verified_water);
        return true;
    }
    const DepthReading& reading()const{return reading_;}
private:
    double elapsed_=0.;
    DepthReading reading_;
};
}} // namespace
#endif
