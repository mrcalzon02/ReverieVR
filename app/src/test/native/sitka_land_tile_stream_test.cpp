// Real archived NOAA/USGS SITKA01 data executed through the same CPU cache
// as the Android module. Rejects bad provenance, CRC, tile path and geometry.
// Synthetic index mutations below are negative TEST FIXTURES only.
#include "../../main/jni/native/sitka_land_tile_stream.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

using reverie::sitka::LandTileStream;
using reverie::sitka::LandTileCrc32;
static std::string Read(const std::string& path){
    std::ifstream file(path,std::ios::binary);
    if(!file.good())throw std::runtime_error("Test cannot open source asset: "+path);
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}
static bool Rejects(const std::string& text){
    try{LandTileStream invalid(text);(void)invalid;return false;}
    catch(const std::invalid_argument&){return true;}
}
static void ChangeOne(std::string& text,const std::string& before,
                      const std::string& after){
    const auto i=text.find(before);
    assert(i!=std::string::npos);
    text.replace(i,before.size(),after);
}

int main(int argc,char** argv){
    assert(argc==2);
    const std::string root=argv[1];
    const std::string index=Read(root+"/sitka/coast/index.txt");
    LandTileStream source(index);
    assert(source.rows()==4u&&source.columns()==4u);
    assert(source.registered_count()==16u);
    assert(source.origin_east()==478720.&&source.origin_north()==6324480.);
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){
        const auto& tile=source.record(r,c);
        assert(tile.row==r&&tile.col==c);
        assert(tile.east==478720.+double(c)*512.);
        assert(tile.north==6324480.-double(r)*512.);
        assert(tile.vertices==16641u);
        assert(tile.triangles>0u);
    }
    auto reader=[&](const std::string& path,std::string& out){
        if(path.find("sitka/coast/tiles/")!=0u)return false;
        out=Read(root+"/"+path);
        return true;
    };
    assert(!source.SetFocus(1.,1.));
    assert(source.SetFocus(479000.,6324200.)); // NOAA coastline study AOI
    assert(!source.PendingGpuSwap());
    // Every Pump performs no more than one file read, no premature GL swap.
    for(unsigned n=0;n<4;++n){
        const auto before=source.io_attempts();
        assert(source.Pump(reader));
        assert(source.io_attempts()==before+1u);
        if(n<3)assert(!source.PendingGpuSwap());
    }
    assert(source.PendingGpuSwap()&&source.resident_count()==4u);
    auto world=source.PendingWorldMeshes();
    assert(world.size()==4u);
    for(const auto& mesh:world){
        assert(mesh.origin_east==source.origin_east());
        assert(mesh.origin_north==source.origin_north());
        assert(mesh.vertices.size()==16641u);
        assert(mesh.indices.size()>500u);
        for(const auto& v:mesh.vertices){
            assert(v.x>=-.01f&&v.x<=2048.01f);
            assert(v.z>=-2048.01f&&v.z<=.01f);
            assert(v.y>=-150.f&&v.y<=6000.f);
        }
    }
    source.MarkGpuCommitted();
    assert(!source.PendingGpuSwap());
    assert(source.SetFocus(480500.,6322650.)); // south/east grid boundary
    unsigned loaded=0;
    while(!source.PendingGpuSwap()&&loaded<5u){
        assert(source.Pump(reader));
        ++loaded;
    }
    assert(source.PendingGpuSwap()&&loaded<=4u);
    assert(source.resident_count()==4u); // bounded resident CPU memory
    assert(source.PendingWorldMeshes().size()==4u);
    source.MarkGpuCommitted();
    assert(!source.PendingGpuSwap());
    source.AfterGlContextLoss();
    assert(source.PendingGpuSwap()); // GPU recreated from intact CPU meshes
    source.MarkGpuCommitted();

    LandTileStream corrupt(index);
    assert(corrupt.SetFocus(478750.,6324450.));
    const bool loadedCorrupt=corrupt.Pump([&](const std::string& path,std::string& out){
        out=Read(root+"/"+path);
        assert(out.size()>100u);
        out[85]^=1; // true source hash mismatch; never coerced into valid land
        return true;
    });
    assert(!loadedCorrupt);
    assert(corrupt.failed_reads()==1u);
    assert(corrupt.resident_count()==0u);
    assert(!corrupt.PendingGpuSwap());
    assert(!corrupt.last_error().empty());

    auto invalid=index;
    ChangeOne(invalid,"crs EPSG32608","crs EPSG4326");
    assert(Rejects(invalid));
    invalid=index;
    ChangeOne(invalid,"datum UNVERIFIED_DYNAMIC_3DEP_MOSAIC",
                      "datum MLLW"); // never invent vertical reconciliation
    assert(Rejects(invalid));
    invalid=index;
    ChangeOne(invalid,"grid 4 4 512 478720 6324480","grid 4 4 512 478720 6324481");
    assert(Rejects(invalid));
    invalid=index;
    ChangeOne(invalid,"terrain-r00-c00.sitkam","../other-game.sitkam");
    assert(Rejects(invalid));
    invalid=index;
    ChangeOne(invalid,"tiles 16","tiles 15");
    assert(Rejects(invalid));
    invalid=index + "unapproved record\n";
    assert(Rejects(invalid));

    std::cout<<"PASS actual NOAA ENC / USGS Sitka 16-tile CPU stream, "
             <<"checked origin/CRC/asset scope, resident cap and GPU rebind\n";
}
