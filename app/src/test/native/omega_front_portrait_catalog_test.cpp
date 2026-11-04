#include "omega_front_png.h"
#include "omega_front_portrait_catalog.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

using reverie::omega_art::DecodePng;
using reverie::omega_art::Image;
using reverie::omega_art::PortraitAt;
using reverie::omega_art::kPortraitFactionCount;
using reverie::omega_art::kPortraitFactionIds;
using reverie::omega_art::kPortraitsPerFaction;

int main(int argc,char **argv) {
    if(argc!=2) {
        std::fprintf(stderr,"Usage: omega_front_portrait_catalog_test <apk-assets-directory>\n");
        return 2;
    }
    const std::string asset_root=argv[1];
    constexpr const char *ids[]={"F06","F07","F08","F09","F10"};
    static_assert(kPortraitFactionCount==5,"Expected five staged faction galleries");
    static_assert(kPortraitsPerFaction==20,"Expected twenty square icons per faction");
    assert(PortraitAt(-1,0)==nullptr);
    assert(PortraitAt(kPortraitFactionCount,0)==nullptr);
    assert(PortraitAt(0,-1)==nullptr);
    assert(PortraitAt(0,kPortraitsPerFaction)==nullptr);
    std::set<std::string> unique_paths;
    std::set<std::string> unique_roles;
    int decoded=0;
    for(int faction=0;faction<kPortraitFactionCount;++faction){
        assert(std::strcmp(kPortraitFactionIds[faction],ids[faction])==0);
        const std::string prefix="native/omega-front/ui/icons/f"+
            std::string(ids[faction]+1)+"/";
        for(int slot=0;slot<kPortraitsPerFaction;++slot){
            const auto *entry=PortraitAt(faction,slot);
            assert(entry&&entry->role&&entry->path);
            const std::string role=entry->role,relative=entry->path;
            if(role.empty() || relative.rfind(prefix,0)!=0 ||
               relative.size()<4 || relative.substr(relative.size()-4)!=".png" ||
               !unique_paths.insert(relative).second ||
               !unique_roles.insert(std::string(ids[faction])+":"+role).second){
                std::fprintf(stderr,"Invalid or duplicate Faction Gallery entry %d:%d\n",
                             faction,slot);
                return 1;
            }
            const std::string path=asset_root+"/"+relative;
            std::ifstream stream(path,std::ios::binary);
            if(!stream){
                std::fprintf(stderr,"Missing portrait asset: %s\n",path.c_str());
                return 1;
            }
            std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(stream)),
                                       std::istreambuf_iterator<char>());
            // Match the native renderer's bounded read_asset buffer.
            if(bytes.empty() || bytes.size()>512u*1024u){
                std::fprintf(stderr,"Oversize or empty portrait: %s (%zu bytes)\n",
                             path.c_str(),bytes.size());
                return 1;
            }
            Image image;
            if(!DecodePng(bytes.data(),bytes.size(),image) ||
               image.width!=256u || image.height!=256u ||
               image.rgba.size()!=256u*256u*4u){
                std::fprintf(stderr,"Native PNG decoder rejected portrait: %s\n",
                             path.c_str());
                return 1;
            }
            ++decoded;
        }
    }
    assert(decoded==100 && unique_paths.size()==100);
    std::printf("PASS: all %d Omega Front gallery portraits decode as 256x256 RGBA\n",decoded);
    return 0;
}
