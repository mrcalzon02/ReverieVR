#ifndef REVERIE_NATIVE_ASSET_SCOPE_H
#define REVERIE_NATIVE_ASSET_SCOPE_H
// Pure host-owned path classifier; never accepts filesystem paths/URLs.
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace reverie { namespace native { namespace assets {
inline uint32_t MaximumBytes(const char *id,const char *path){
    if(!id||!path)return 0u;
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
    if(!icons&&!models&&!textures&&!interiors&&!terrain&&!units&&!shared)return 0u;
    const size_t start=r+(icons?6u:models?7u:textures?9u:
        interiors?11u:terrain?8u:units?13u:7u);
    if(n<=start)return 0u;
    if(omega&&(n<4u||std::strcmp(path+n-4u,".png")!=0))return 0u;
    if(interiors&&(n<7u||std::strcmp(path+n-7u,".rlprop")!=0))return 0u;
    if(std::strstr(path,"..")!=nullptr)return 0u;
    for(size_t i=start;i<n;++i){
        const char c=path[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
            (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0u;
    }
    return icons?1024u*1024u:3u*1024u*1024u;
}
}}}
#endif
