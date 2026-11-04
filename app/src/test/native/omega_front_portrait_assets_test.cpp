#include "omega_front_png.h"
#include "omega_front_portrait_catalog.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using reverie::omega_art::DecodePng;
using reverie::omega_art::Image;
using reverie::omega_art::PortraitAt;
using reverie::omega_art::kPortraitFactionCount;
using reverie::omega_art::kPortraitsPerFaction;

int main(int argc,char **argv){
    if(argc!=2){
        std::cerr<<"USAGE: portrait-assets-test <Android assets directory>\n";
        return 2;
    }
    const std::string root(argv[1]);
    int validated=0;
    for(int faction=0;faction<kPortraitFactionCount;++faction){
        for(int slot=0;slot<kPortraitsPerFaction;++slot){
            const auto *portrait=PortraitAt(faction,slot);
            if(!portrait)return 1;
            const std::string path=root+"/"+portrait->path;
            std::ifstream in(path,std::ios::binary);
            if(!in){
                std::cerr<<"Missing portrait: "<<path<<"\n";
                return 1;
            }
            const std::vector<uint8_t> bytes(
                std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>{});
            // The exact bound used by the native, demand-loaded portrait cache.
            if(bytes.empty() || bytes.size()>512u*1024u){
                std::cerr<<"Portrait asset exceeds runtime read bound: "<<path<<"\n";
                return 1;
            }
            Image image;
            if(!DecodePng(bytes.data(),bytes.size(),image) ||
               image.width!=256u || image.height!=256u ||
               image.rgba.size()!=256u*256u*4u){
                std::cerr<<"Native PNG decoding failed: "<<path<<"\n";
                return 1;
            }
            ++validated;
        }
    }
    if(validated!=100)return 1;
    std::cout<<"PASS Omega Front: "<<validated
             <<" portrait PNGs decoded with Android runtime PNG decoder\n";
}
