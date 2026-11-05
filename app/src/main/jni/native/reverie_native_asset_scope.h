#ifndef REVERIE_NATIVE_ASSET_SCOPE_H
#define REVERIE_NATIVE_ASSET_SCOPE_H
// Pure host-owned path classifier; never accepts filesystem paths/URLs.
#include "reverie_native_shared_asset_catalog.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
namespace reverie { namespace native { namespace assets {
inline uint32_t MaximumBytes(const char *id,const char *path){
    if(!id||!path)return 0u;
    const uint32_t common=shared_assets::MaximumBytes(id,path);
    if(common!=0u)return common;
    // Future Sitka module: allow ONLY explicitly named authored data,
    // and strictly bounded geographic depth tile names. This does not
    // register Sitka as playable or permit reading arbitrary filesystem
    // files, private saves, data from other game namespaces or URLs.
    if(std::strcmp(id,"sitka")==0){
        const struct Entry {const char* path;uint32_t limit;} exact[]={
            {"sitka/coast/terrain.sitkam",10000000u},
            {"sitka/lighting/sitka.properties",4096u},
            {"sitka/weather/sitka.properties",4096u},
            {"sitka/water/sitka.water",2000000u},
            {"sitka/fishing/sites.txt",8192u},
            {"sitka/beach/discoveries.txt",8192u},
            {"sitka/city/discoveries.txt",8192u},
            {"sitka/boat/launch.txt",4096u},
            {"sitka/shore/waterfront.sitkash",65536u},
            {"sitka/walk/paths.sitkaw",262144u},
            {"sitka/structures/oconnell.bridge",8192u},
            {"sitka/structures/oconnell_side.png",131072u},
            {"sitka/bathymetry/index.txt",262144u},
            {"sitka/bathymetry/procedural-guides.txt",32000u}
        };
        for(const auto& item:exact)
            if(std::strcmp(path,item.path)==0)return item.limit;
        constexpr const char* root="sitka/bathymetry/";
        const size_t prefix=std::strlen(root);
        const size_t n=strnlen(path,128u);
        if(n!=prefix+25u||std::strncmp(path,root,prefix)!=0)
            return 0u;
        const char* name=path+prefix;
        if(std::strncmp(name,"bathymetry-r",12u)!=0||
           name[14]!='-'||name[15]!='c'||
           std::strcmp(name+18u,".sitkab")!=0)
            return 0u;
        for(unsigned i:{12u,13u,16u,17u})
            if(name[i]<'0'||name[i]>'9')return 0u;
        return 56u+513u*513u*5u; // exact source-only .sitkab max
    }
    // Exactly one authored scene; no arbitrary file access for modules.
    if(std::strcmp(id,"procedural-test-chamber")==0)
        return std::strcmp(path,"native/test-chamber/scene.rvscene")==0
            ? 150000u : 0u;
    // Actual Breakwater imports are BWM1 meshes, already shipped in the
    // bundled APK. Previously the host rejected ALL Breakwater reads.
    if(std::strcmp(id,"breakwater-battery")==0){
        // Only the six approved 64px Breakwater materials, never arbitrary
        // downloaded images or paths from another native-game namespace.
        constexpr const char* tiles[]={
            "native/breakwater/materials/shingle.png",
            "native/breakwater/materials/quay_concrete.png",
            "native/breakwater/materials/black_basalt.png",
            "native/breakwater/materials/oxidized_steel.png",
            "native/breakwater/materials/sandbag_burlap.png",
            "native/breakwater/materials/olive_enamel.png"
        };
        for(const char* approved:tiles)
            if(std::strcmp(path,approved)==0)return 16384u;
        constexpr const char *root="native/breakwater/beckenburg/";
        const size_t r=std::strlen(root),n=strnlen(path,128u);
        if(n>=128u||n<=r+4u||std::strncmp(path,root,r)!=0||
           std::strcmp(path+n-4u,".bwm")!=0||
           std::strstr(path,"..")!=nullptr)return 0u;
        for(size_t i=r;i<n;++i){
            const char c=path[i];
            if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
                (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))
                return 0u;
        }
        return 8u+2048u*27u; // Exact compiled BWM1 mesh upper bound.
    }
    // The Gumball module may read only ten game-bound author-approved
    // derivatives, never arbitrary concept sheets or another game's assets.
    if(std::strcmp(id,"gumball-armada")==0){
        constexpr const char* root="native/gumball/nodes/";
        constexpr const char* allies="native/gumball/allies/";
        const size_t r=std::strlen(root),a=std::strlen(allies);
        const size_t n=strnlen(path,128u);
        if(n>=128u)return 0u;
        constexpr const char* approved[]={
            "gummy_world.rgb565","chocolate_world.rgb565",
            "ice_candy_world.rgb565","citrus_world.rgb565",
            "gas_giant.rgb565","ring_world.rgb565",
            "outpost_station.rgb565","crystal_field.rgb565"};
        if(n>r&&std::strncmp(path,root,r)==0){
            for(const char* name:approved)
                if(std::strcmp(path+r,name)==0)return 16520u;
        }
        if(n>a&&std::strncmp(path,allies,a)==0 &&
           (std::strcmp(path+a,"pip_parfait.rgb565")==0 ||
            std::strcmp(path+a,"lolly_wren.rgb565")==0))
            return 16520u;
        return 0u;
    }
    const bool red=std::strcmp(id,"between-deliveries-red-ledger")==0;
    const bool omega=std::strcmp(id,"omega-front")==0;
    if(!red&&!omega)return 0u;
    const char *root=red?"native/red-ledger/":"native/omega-front/";
    const size_t r=std::strlen(root),n=strnlen(path,128u);
    if(n>=128u||n<=r||std::strncmp(path,root,r)!=0)return 0u;
    const char *leaf=path+r;
    const bool icons=red&&std::strncmp(leaf,"icons/",6u)==0;
    const bool models=red&&std::strncmp(leaf,"models/",7u)==0;
    const bool textures=red&&std::strncmp(leaf,"textures/",9u)==0;
    const bool interiors=red&&std::strncmp(leaf,"beckenburg/",11u)==0;
    const bool terrain=omega&&std::strncmp(leaf,"terrain/",8u)==0;
    const bool units=omega&&std::strncmp(leaf,"units/kenney/",13u)==0;
    const bool shared=omega&&std::strncmp(leaf,"shared/",7u)==0;
    const bool cc0=omega&&std::strncmp(leaf,"rpg_cc0/",8u)==0;
    if(!icons&&!models&&!textures&&!interiors&&!terrain&&!units&&!shared&&!cc0)return 0u;
    const size_t start=r+(icons?6u:models?7u:textures?9u:
        interiors?11u:terrain?8u:units?13u:cc0?8u:7u);
    if(n<=start)return 0u;
    if(omega&&(n<4u||std::strcmp(path+n-4u,".png")!=0))return 0u;
    if(interiors&&(n<7u||std::strcmp(path+n-7u,".rlprop")!=0))return 0u;
    if(std::strstr(path,"..")!=nullptr)return 0u;
    for(size_t i=start;i<n;++i){
        const char c=path[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
            (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0u;
    }
    if(cc0){
        // Admit ONLY the 64 original, indexed CC0 tiles shipped by Omega
        // Front; no arbitrary files in the new read-capable directory.
        const char *name=leaf+8u;
        struct Group {const char* prefix;unsigned count;};
        const Group groups[]={{"tiny_battle_",32u},
                              {"rpg_urban_",16u},
                              {"roguelike_city_",16u}};
        bool approved=false;
        for(const Group &group:groups){
            const size_t prefix=std::strlen(group.prefix);
            if(std::strncmp(name,group.prefix,prefix)!=0)continue;
            if(std::strlen(name)!=prefix+8u ||
               std::strcmp(name+prefix+4u,".png")!=0)return 0u;
            unsigned index=0u;
            for(unsigned i=0;i<4u;++i){
                const char digit=name[prefix+i];
                if(digit<'0'||digit>'9')return 0u;
                index=index*10u+static_cast<unsigned>(digit-'0');
            }
            approved=index<group.count;
            break;
        }
        if(!approved)return 0u;
        return 256u*1024u; // Same bounded buffer as Omega GLES loader.
    }
    return icons?1024u*1024u:3u*1024u*1024u;
}
}}}
#endif
