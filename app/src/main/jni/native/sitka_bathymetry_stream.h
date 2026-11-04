#ifndef REVERIE_SITKA_BATHYMETRY_STREAM_H
#define REVERIE_SITKA_BATHYMETRY_STREAM_H
// Georeferenced, bounded, CPU-only lazy seabed streaming for source-approved
// regional maps. Designed for one boat depth tile plus one heading prefetch.
// NO GL calls, disk I/O, mutation or asset decode from stereo eye rendering.
// Host performs at most one approved local read per Pump call during update.
#include "sitka_bathymetry.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
namespace reverie { namespace sitka {
constexpr size_t kBathyStreamMaxRegions=2048u;
constexpr size_t kBathyStreamMaxIndexBytes=256u*1024u;
constexpr size_t kBathyStreamResident=2u;
inline bool ValidBathyTileName(const std::string& name){
    if(name.size()!=25u || name.compare(0,12,"bathymetry-r")!=0||
       name[14]!='-'||name[15]!='c'||name.compare(18,7,".sitkab")!=0)
        return false;
    for(unsigned i : {12u,13u,16u,17u})
        if(name[i]<'0'||name[i]>'9')return false;
    return true;
}
struct BathyRegion {
    std::string filename;
    double first_east=0.,first_north=0.,east_step=0.,north_step=0.;
    unsigned columns=0u,rows=0u;
    double EastEnd()const{return first_east+(columns-1u)*east_step;}
    double SouthEnd()const{return first_north-(rows-1u)*north_step;}
    bool Contains(double east,double north)const{
        return std::isfinite(east)&&std::isfinite(north)&&
            east>=first_east&&east<EastEnd()&&
            north>SouthEnd()&&north<=first_north;
    }
    bool Matches(const BathyTile& t)const{
        return columns==t.columns&&rows==t.rows&&
            first_east==t.first_east&&first_north==t.first_north&&
            east_step==t.east_step&&north_step==t.north_step;
    }
};
struct BathyDirectory {
    uint32_t datum_id=0u;
    std::string datum_name;
    std::vector<BathyRegion> regions;
    int Locate(double east,double north)const{
        for(size_t i=0;i<regions.size();++i)
            if(regions[i].Contains(east,north))return static_cast<int>(i);
        return -1;
    }
};
inline BathyDirectory ParseBathyStreamIndex(const std::string& text){
    constexpr char magic[]="SITKA-BATHY-INDEX-2\n";
    if(text.size()<sizeof(magic)||text.size()>kBathyStreamMaxIndexBytes||
       text.compare(0,sizeof(magic)-1u,magic)!=0)
        throw std::invalid_argument("invalid Sitka streaming index");
    std::istringstream stream(text.substr(sizeof(magic)-1u));
    stream.imbue(std::locale::classic());
    std::string key,datum;
    if(!(stream>>key>>datum)||key!="datum")
        throw std::invalid_argument("Sitka stream datum absent");
    BathyDirectory out;
    out.datum_name=datum;out.datum_id=BathyDatumId(datum);
    if(!out.datum_id)throw std::invalid_argument("invalid Sitka datum");
    std::string count;
    if(!(stream>>key>>count)||key!="tiles"||count.empty()||
       count.size()>4u)throw std::invalid_argument("invalid tile count");
    unsigned number=0u;
    for(char digit:count){
        if(digit<'0'||digit>'9')
            throw std::invalid_argument("invalid tile count digits");
        number=number*10u+static_cast<unsigned>(digit-'0');
    }
    if(number==0u||number>kBathyStreamMaxRegions)
        throw std::invalid_argument("stream index exceeds bounded directory");
    out.regions.reserve(number);
    for(unsigned i=0u;i<number;++i){
        BathyRegion cell;
        if(!(stream>>cell.filename>>cell.first_east>>cell.first_north>>
             cell.east_step>>cell.north_step>>cell.columns>>cell.rows)||
           !ValidBathyTileName(cell.filename)||
           !std::isfinite(cell.first_east)||!std::isfinite(cell.first_north)||
           !std::isfinite(cell.east_step)||!std::isfinite(cell.north_step)||
           std::abs(cell.first_east)>1e8||std::abs(cell.first_north)>1e8||
           cell.east_step<=0.||cell.east_step>500.||
           cell.north_step<=0.||cell.north_step>500.||
           cell.columns<2||cell.columns>kBathyMaxAxis||
           cell.rows<2||cell.rows>kBathyMaxAxis)
            throw std::invalid_argument("invalid stream region descriptor");
        // Shared edge samples are allowed; positive-area overlap is not.
        for(const auto& old:out.regions){
            if(old.filename==cell.filename)
                throw std::invalid_argument("duplicate Sitka stream file");
            if(cell.first_east<old.EastEnd()&&cell.EastEnd()>old.first_east&&
               cell.SouthEnd()<old.first_north&&cell.first_north>old.SouthEnd())
                throw std::invalid_argument("overlapping Sitka stream cells");
        }
        out.regions.push_back(std::move(cell));
    }
    if(stream>>key)throw std::invalid_argument("trailing stream index data");
    return out;
}
struct BathyStreamStats {
    uint32_t requests=0u,loaded=0u,rejected=0u,evicted=0u;
    size_t resident_tiles=0u,resident_bytes=0u;
};
class BathyStreamCache {
public:
    explicit BathyStreamCache(BathyDirectory directory)
        : directory_(std::move(directory)){
        if(!directory_.datum_id||directory_.regions.empty()||
           directory_.regions.size()>kBathyStreamMaxRegions)
            throw std::invalid_argument("invalid streaming bathymetry directory");
    }
    const BathyDirectory& directory()const{return directory_;}
    const BathyStreamStats& stats()const{return stats_;}
    template<class Visitor>
    void VisitResident(Visitor&& visit)const{
        for(const Entry& e:resident_)if(e.region>=0)visit(e.tile);
    }
    // Depth sampling NEVER initiates I/O, mutates scene, or guesses seabed.
    BedSample Sample(double east,double north)const{
        const int id=directory_.Locate(east,north);
        for(const Entry& e:resident_)
            if(e.region==id&&id>=0)return e.tile.Sample(east,north);
        return {};
    }
    const BathyTile* ResidentAt(double east,double north)const{
        const int id=directory_.Locate(east,north);
        for(const Entry& e:resident_)
            if(id>=0&&e.region==id)return &e.tile;
        return nullptr;
    }
    void Clear(){
        for(Entry& entry:resident_)entry=Entry{};
        stats_.resident_tiles=stats_.resident_bytes=0u;
        current_=-1;failed_=-1;retry_ticks_=0u;
    }
    // Predict a single nearby tile from boat horizontal velocity (m/s).
    // One host read maximum per call; caller supplies approved scoped asset
    // reader. API may later hand requests to a worker without changing rules.
    template<class Reader>
    bool Pump(double east,double north,double velocity_east,
              double velocity_north,Reader&& read_asset){
        if(!std::isfinite(east)||!std::isfinite(north)||
           !std::isfinite(velocity_east)||!std::isfinite(velocity_north)||
           std::hypot(velocity_east,velocity_north)>8.)return false;
        current_=directory_.Locate(east,north);
        // Hold a failed tile for a short retry backoff; do not hammer storage.
        if(retry_ticks_>0u)--retry_ticks_;
        int wanted=current_;
        if(wanted>=0&&Has(wanted)){
            // 5 second look-ahead at max virtual skiff speed. Keep two tiles:
            // current + at most one immediate next, never whole-region preload.
            const double next_east=east+velocity_east*5.;
            const double next_north=north+velocity_north*5.;
            const int next=directory_.Locate(next_east,next_north);
            if(next>=0&&next!=current_&&!Has(next))
                wanted=next;
            else return false;
        }
        if(wanted<0||Has(wanted)||
           (wanted==failed_&&retry_ticks_>0u))return false;
        ++stats_.requests;
        const BathyRegion& region=directory_.regions[static_cast<size_t>(wanted)];
        std::string bytes;
        bool valid=false;
        try{
            valid=read_asset(region.filename,bytes);
            if(valid&&bytes.size()<=kBathyHeaderBytes+
                  size_t(kBathyMaxAxis)*kBathyMaxAxis*5u){
                BathyTile tile=ParseBathymetry(
                    reinterpret_cast<const uint8_t*>(bytes.data()),
                    bytes.size(),directory_.datum_id);
                valid=region.Matches(tile);
                if(valid){
                    // Never evict the boat's current tile for a prefetch.
                    size_t place=kBathyStreamResident;
                    for(size_t i=0;i<resident_.size();++i)
                        if(resident_[i].region<0){place=i;break;}
                    if(place==kBathyStreamResident)
                        for(size_t i=0;i<resident_.size();++i)
                            if(resident_[i].region!=current_){place=i;break;}
                    if(place!=kBathyStreamResident){
                        if(resident_[place].region>=0)++stats_.evicted;
                        resident_[place].region=wanted;
                        resident_[place].tile=std::move(tile);
                        ++stats_.loaded;
                        failed_=-1;retry_ticks_=0u;
                        UpdateFootprint();return true;
                    }
                }
            }
        }catch(const std::exception&){valid=false;}
        (void)valid;
        ++stats_.rejected;
        failed_=wanted;
        retry_ticks_=15u; // bounded, retry on subsequent update
        return false;
    }
private:
    struct Entry {int region=-1;BathyTile tile;};
    bool Has(int region)const{
        for(const Entry& x:resident_)if(x.region==region)return true;
        return false;
    }
    void UpdateFootprint(){
        stats_.resident_tiles=stats_.resident_bytes=0;
        for(const Entry& x:resident_)if(x.region>=0){
            ++stats_.resident_tiles;
            stats_.resident_bytes+=x.tile.elevations_m.size()*
                (sizeof(float)+sizeof(BedQuality));
        }
    }
    BathyDirectory directory_;
    std::array<Entry,kBathyStreamResident> resident_{};
    BathyStreamStats stats_;
    int current_=-1,failed_=-1;
    unsigned retry_ticks_=0u;
};
}} // namespace
#endif
