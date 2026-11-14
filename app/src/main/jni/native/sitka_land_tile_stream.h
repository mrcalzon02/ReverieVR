#ifndef REVERIE_SITKA_LAND_TILE_STREAM_H
#define REVERIE_SITKA_LAND_TILE_STREAM_H

// Bounded, real-source land-tile CPU streamer. NO runtime network, no GIS
// interpolation, invented coastline, user GPS, or input/device ABI changes.
// Only source-validated SITKA01 bytes from the existing APK/verified RvPack.
// Geometry is rebased from each 512m UTM tile to one short-float local origin.
// Tile I/O runs in native Update, never in either RenderEye stereo callback.
#include "sitka_terrain_mesh.h"
// Local CRC32 table avoids a zlib header's legacy "far" macro leaking into
// the separate Sitka shoreline shader/geometry headers on Android NDK.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace reverie { namespace sitka {

struct LandTileRecord {
    unsigned row=0u,col=0u;
    std::string filename;
    uint32_t bytes=0u,crc=0u,vertices=0u,triangles=0u;
    double east=0.,north=0.;
};

inline uint32_t LandTileCrc32(const std::string& bytes) {
    // Standard IEEE CRC-32, byte-identical to Python zlib.crc32 and APK pack
    // receipt. No external C interface, runtime allocation, or macro leaks.
    static constexpr std::array<uint32_t,256> table=[]() constexpr {
        std::array<uint32_t,256> result{};
        for(uint32_t i=0u;i<256u;++i){
            uint32_t crc=i;
            for(unsigned j=0;j<8u;++j)
                crc=(crc&1u)?((crc>>1u)^0xedb88320u):(crc>>1u);
            result[i]=crc;
        }
        return result;
    }();
    if(bytes.size()>32u+16641u*15u+32768u*12u)
        throw std::invalid_argument("Sitka geographic tile exceeds CRC budget");
    uint32_t crc=0xffffffffu;
    for(const unsigned char ch:bytes)
        crc=(crc>>8u)^table[(crc^uint32_t(ch))&255u];
    return crc^0xffffffffu;
}

class LandTileStream {
public:
    static constexpr unsigned kMaxRows=8u,kMaxColumns=8u;
    static constexpr unsigned kResidentTiles=4u;
    static constexpr uint32_t kTileLimit=32u+16641u*15u+32768u*12u;
    explicit LandTileStream(const std::string& source) {
        if(source.empty()||source.size()>16384u)
            throw std::invalid_argument("Sitka geographic tile index too large");
        std::istringstream in(source);
        in.imbue(std::locale::classic());
        std::string line;
        if(!std::getline(in,line)||line!="SITKA-LAND-INDEX-1")
            throw std::invalid_argument("Sitka geographic tile index version");
        if(!std::getline(in,line)||line!="crs EPSG32608")
            throw std::invalid_argument("Sitka geographic tile horizontal CRS");
        if(!std::getline(in,line)||
           line!="datum UNVERIFIED_DYNAMIC_3DEP_MOSAIC")
            throw std::invalid_argument("Sitka land source datum requires review");
        if(!std::getline(in,line))
            throw std::invalid_argument("Sitka land geographic grid absent");
        {
            std::istringstream field(line);
            field.imbue(std::locale::classic());
            std::string key,extra;
            if(!(field>>key>>cols_>>rows_>>size_>>west_>>north_)||
               key!="grid" || field>>extra ||
               cols_<2u||cols_>kMaxColumns||rows_<2u||rows_>kMaxRows||
               size_!=512u||!std::isfinite(west_)||!std::isfinite(north_)||
               west_<470000.||west_>490000.||
               north_<6310000.||north_>6340000.)
                throw std::invalid_argument("Sitka geographic grid inconsistent");
        }
        if(!std::getline(in,line))
            throw std::invalid_argument("Sitka tile count absent");
        {
            std::istringstream field(line);
            std::string key,extra;
            unsigned count=0u;
            if(!(field>>key>>count)||key!="tiles"||field>>extra||
               count!=rows_*cols_)
                throw std::invalid_argument("Sitka geographic atlas has gaps");
        }
        records_.resize(rows_*cols_);
        std::vector<bool> occupied(records_.size(),false);
        for(unsigned i=0;i<records_.size();++i){
            if(!std::getline(in,line)||line.empty())
                throw std::invalid_argument("Sitka tile index truncated");
            std::istringstream field(line);
            field.imbue(std::locale::classic());
            LandTileRecord item;
            uint64_t crc=0u;
            std::string extra;
            if(!(field>>item.row>>item.col>>item.filename>>item.bytes>>
                 crc>>item.vertices>>item.triangles>>item.east>>item.north)||
               field>>extra||item.row>=rows_||item.col>=cols_||
               item.bytes<32u+3u*15u+12u||item.bytes>kTileLimit||
               crc>0xffffffffULL||item.vertices<3u||item.vertices>16641u||
               item.triangles<1u||item.triangles>32768u||
               item.bytes!=32u+item.vertices*15u+item.triangles*12u||
               item.east!=west_+double(item.col*size_)||
               item.north!=north_-double(item.row*size_))
                throw std::invalid_argument("Sitka geographic tile extent or size");
            char required[40]={};
            std::snprintf(required,sizeof(required),
                "terrain-r%02u-c%02u.sitkam",item.row,item.col);
            if(item.filename!=required)
                throw std::invalid_argument("Sitka tile filename not admitted");
            const size_t index=size_t(item.row)*cols_+item.col;
            if(occupied[index])
                throw std::invalid_argument("Sitka geographic tile duplicated");
            occupied[index]=true;
            item.crc=static_cast<uint32_t>(crc);
            records_[index]=std::move(item);
        }
        if(std::getline(in,line))
            throw std::invalid_argument("Sitka tile index trailing records");
        for(bool filled:occupied)if(!filled)
            throw std::invalid_argument("Sitka tile index has unlisted cell");
        target_.fill(-1);committed_.fill(-1);
    }

    unsigned rows()const{return rows_;}
    unsigned columns()const{return cols_;}
    size_t registered_count()const{return records_.size();}
    double origin_east()const{return west_;}
    double origin_north()const{return north_;}
    unsigned io_attempts()const{return attempts_;}
    unsigned failed_reads()const{return failed_;}
    unsigned resident_count()const{
        unsigned n=0;for(const auto& s:resident_)if(s.index>=0)++n;
        return n;
    }
    const std::string& last_error()const{return error_;}
    const LandTileRecord& record(unsigned row,unsigned col)const{
        if(row>=rows_||col>=cols_)throw std::out_of_range("Sitka geographic row/column");
        return records_[row*cols_+col];
    }

    bool SetFocus(double east,double north) {
        if(!std::isfinite(east)||!std::isfinite(north)||
           east<west_||east>west_+cols_*size_||
           north>north_||north<north_-rows_*size_)
            return false;
        const unsigned c=std::min(cols_-1,
            static_cast<unsigned>(std::floor((east-west_)/size_)));
        const unsigned r=std::min(rows_-1,
            static_cast<unsigned>(std::floor((north_-north)/size_)));
        const unsigned c0=std::min(cols_-2,c);
        const unsigned r0=std::min(rows_-2,r);
        const std::array<int,kResidentTiles> desired={{
            int(r0*cols_+c0),int(r0*cols_+c0+1),
            int((r0+1)*cols_+c0),int((r0+1)*cols_+c0+1)}};
        if(desired!=target_){
            target_=desired;
            retry_delay_=0u;
        }
        return true;
    }

    // Executes at most ONE checked asset read and triangulation per Update.
    // A corrupt tile cannot replace already uploaded valid geometry.
    template<class Reader> bool Pump(Reader&& reader) {
        if(target_[0]<0)return false;
        if(retry_delay_>0u){--retry_delay_;return false;}
        for(int index:target_) {
            if(Find(index))continue;
            const auto& record=records_[index];
            ++attempts_;
            std::string data;
            const std::string path="sitka/coast/tiles/"+record.filename;
            if(!reader(path,data)){
                Fail("Missing Sitka NOAA land tile: "+path);
                return false;
            }
            try{
                if(data.size()!=record.bytes||LandTileCrc32(data)!=record.crc)
                    throw std::invalid_argument("tile checksum/size mismatch");
                auto mesh=ParseTerrainMesh(
                    reinterpret_cast<const uint8_t*>(data.data()),data.size());
                if(mesh.vertices.size()!=record.vertices||
                   mesh.indices.size()!=size_t(record.triangles)*3u||
                   mesh.origin_east!=record.east||
                   mesh.origin_north!=record.north)
                    throw std::invalid_argument("tile origin/count different from index");
                const float dx=static_cast<float>(record.east-west_);
                const float dz=static_cast<float>(record.north-north_);
                for(auto& p:mesh.vertices){
                    if(p.x < -.01f||p.x > size_+.01f||
                       p.z < -float(size_)-.01f||p.z > .01f||
                       p.y < -150.f||p.y > 6000.f)
                        throw std::invalid_argument("tile vertex outside sourced geographic bounds");
                    p.x+=dx;p.z+=dz;
                }
                mesh.origin_east=west_;
                mesh.origin_north=north_;
                Slot* slot=Victim();
                if(!slot)throw std::logic_error("no available geographic cache slot");
                slot->mesh=std::move(mesh);
                slot->index=index;
                slot->age=++clock_;
            }catch(const std::exception& e){
                Fail(std::string("Rejected sourced Sitka land tile: ")+e.what());
                return false;
            }
            error_.clear();
            // Keep work bounded: the next Update may load another tile.
            return true;
        }
        return true;
    }

    bool PendingGpuSwap() const{
        if(target_[0]<0||target_==committed_)return false;
        for(int index:target_)if(!Find(index))return false;
        return true;
    }

    std::vector<TerrainMesh> PendingWorldMeshes() const {
        if(!PendingGpuSwap())return {};
        std::vector<TerrainMesh> meshes;
        meshes.reserve(kResidentTiles);
        for(int index:target_){
            const Slot* slot=Find(index);
            if(!slot)throw std::logic_error("Sitka geographic CPU tile lost");
            meshes.push_back(slot->mesh);
        }
        return meshes;
    }
    void MarkGpuCommitted(){
        if(!PendingGpuSwap())
            throw std::logic_error("Sitka geographic GPU commit without complete data");
        committed_=target_;
    }
    void AfterGlContextLoss(){committed_.fill(-1);}

private:
    struct Slot{
        int index=-1;
        unsigned age=0u;
        TerrainMesh mesh;
    };
    const Slot* Find(int index) const {
        for(const auto& s:resident_)if(s.index==index)return &s;
        return nullptr;
    }
    Slot* Victim(){
        for(auto& s:resident_)if(s.index<0)return &s;
        // Retain tiles needed for the new complete quad while replacing old.
        Slot* oldest=nullptr;
        for(auto& s:resident_){
            if(std::find(target_.begin(),target_.end(),s.index)!=target_.end())continue;
            if(!oldest||s.age<oldest->age)oldest=&s;
        }
        return oldest;
    }
    void Fail(std::string message){
        error_=std::move(message);
        ++failed_;
        retry_delay_=std::min(90u,4u*failed_);
    }
    unsigned rows_=0u,cols_=0u,size_=0u;
    double west_=0.,north_=0.;
    std::vector<LandTileRecord> records_;
    std::array<Slot,kResidentTiles> resident_{};
    std::array<int,kResidentTiles> target_{},committed_{};
    unsigned attempts_=0u,failed_=0u,retry_delay_=0u,clock_=0u;
    std::string error_;
};
}} // namespace reverie::sitka
#endif
