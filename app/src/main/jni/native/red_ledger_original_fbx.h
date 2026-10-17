#ifndef REVERIE_RED_LEDGER_FBX_H
#define REVERIE_RED_LEDGER_FBX_H

// Bounded Unreal FBX 7.3 geometry reader for genuine owner-exported
// StaticMesh assets. Rejects unsupported encodings, collision-only geometry,
// broken array lengths and non-triangle polygon streams. No substitute mesh.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <zlib.h>

namespace reverie { namespace redledger { namespace fbx {
struct Geometry {
    std::vector<double> xyz;
    std::vector<int32_t> poly;
    std::vector<double> uv;
    std::vector<int32_t> uv_indices;
};
inline uint32_t U32(const uint8_t *p) {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|
           (uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
inline uint64_t U64(const uint8_t *p) {
    return uint64_t(U32(p))|(uint64_t(U32(p+4))<<32);
}
struct Node {
    size_t end=0,begin_children=0;
    uint32_t props=0;
    std::string name;
};
inline bool ReadNode(const uint8_t *data,size_t len,size_t at,Node *n) {
    if(!n || at>len || len-at<13u)return false;
    const uint32_t end=U32(data+at);
    const uint32_t props=U32(data+at+4);
    const uint32_t prop_bytes=U32(data+at+8);
    const size_t name_bytes=data[at+12];
    const size_t head=13u+name_bytes;
    if(end<=at || end>len || props>1024u || head>end-at ||
       prop_bytes>end-at-head)return false;
    n->end=end;n->props=props;
    n->name.assign(reinterpret_cast<const char *>(data+at+13),name_bytes);
    n->begin_children=at+head+prop_bytes;
    return true;
}
inline bool SkipProperty(const uint8_t *data,size_t len,size_t *at) {
    if(!at || *at>=len)return false;
    const char type=static_cast<char>(data[(*at)++]);
    size_t bytes=0u;
    switch(type) {
        case 'Y':bytes=2;break;
        case 'C':case 'B':bytes=1;break;
        case 'I':case 'F':bytes=4;break;
        case 'D':case 'L':bytes=8;break;
        case 'S':case 'R':
            if(len-*at<4)return false;
            bytes=4u+U32(data+*at);break;
        case 'f':case 'd':case 'l':case 'i':case 'b':case 'c':
            if(len-*at<12)return false;
            bytes=12u+U32(data+*at+8);break;
        default:return false;
    }
    if(bytes>len-*at)return false;
    *at+=bytes;return true;
}
inline bool ReadArray(const uint8_t *data,size_t len,size_t at,
                      char expected,size_t elem_bytes,
                      std::vector<uint8_t> *out) {
    if(!out || at>=len || data[at]!=uint8_t(expected) ||
       len-at<13u)return false;
    const uint32_t count=U32(data+at+1);
    const uint32_t encoding=U32(data+at+5);
    const uint32_t compressed=U32(data+at+9);
    if(count==0 || count>1000000u || elem_bytes>16u ||
       size_t(count)*elem_bytes>32u*1024u*1024u ||
       compressed>len-at-13u)return false;
    const size_t length=size_t(count)*elem_bytes;
    out->resize(length);
    const uint8_t *source=data+at+13;
    if(encoding==0 && compressed==length) {
        std::memcpy(out->data(),source,length);
    } else if(encoding==1) {
        uLongf actual=static_cast<uLongf>(length);
        if(uncompress(out->data(),&actual,source,compressed)!=Z_OK ||
           actual!=length)return false;
    } else return false;
    return true;
}
inline bool Extract(const uint8_t *data,size_t len,size_t start,
                    size_t limit,int depth,bool in_geo,bool in_uv,const char *model_name,
                    Geometry *out) {
    if(depth>8 || start>limit || limit>len)return false;
    for(size_t p=start;p+13u<=limit;) {
        if(U32(data+p)==0u)break; // FBX null sentinel
        Node n;
        if(!ReadNode(data,len,p,&n) || n.end>limit)return false;
        const size_t prop_at=p+13u+n.name.size();
        bool selected=in_geo,layer_uv=in_uv;
        if(n.name=="Geometry") {
            selected=false;
            // Unreal's UCX collision mesh must never replace the visible mesh.
            size_t q=prop_at;
            if(n.props>=3 && SkipProperty(data,len,&q) &&
               q+5u<=len && data[q]=='S') {
                const uint32_t count=U32(data+q+1);
                const size_t name_at=q+5u;
                if(count>=9u && count<=len-name_at) {
                    std::string name(reinterpret_cast<const char *>(data+name_at),count);
                    const size_t wanted=std::strlen(model_name);
                    selected=wanted>=4u && wanted<=48u &&
                        name.size()>wanted &&
                        name.compare(0,wanted,model_name)==0 &&
                        name[wanted]=='\0';
                }
            }
        }
        if(n.name=="LayerElementUV")layer_uv=true;
        if(selected && n.props==1) {
            std::vector<uint8_t> raw;
            if(n.name=="Vertices" && out->xyz.empty()) {
                if(!ReadArray(data,len,prop_at,'d',8,&raw) ||
                   raw.size()%24u!=0u)return false;
                out->xyz.resize(raw.size()/8u);
                std::memcpy(out->xyz.data(),raw.data(),raw.size());
            } else if(n.name=="PolygonVertexIndex" && out->poly.empty()) {
                if(!ReadArray(data,len,prop_at,'i',4,&raw))return false;
                out->poly.resize(raw.size()/4u);
                std::memcpy(out->poly.data(),raw.data(),raw.size());
            } else if(layer_uv && n.name=="UV" && out->uv.empty()) {
                if(!ReadArray(data,len,prop_at,'d',8,&raw) ||
                   raw.size()%16u!=0u)return false;
                out->uv.resize(raw.size()/8u);
                std::memcpy(out->uv.data(),raw.data(),raw.size());
            } else if(layer_uv && n.name=="UVIndex" && out->uv_indices.empty()) {
                if(!ReadArray(data,len,prop_at,'i',4,&raw))return false;
                out->uv_indices.resize(raw.size()/4u);
                std::memcpy(out->uv_indices.data(),raw.data(),raw.size());
            }
        }
        if(n.begin_children<n.end &&
           !Extract(data,len,n.begin_children,n.end,depth+1,
                    selected,layer_uv,model_name,out))return false;
        p=n.end;
    }
    return true;
}
// Output shares the Red Ledger baked room's XYZ/UV/RGB/tileOrigin
// layout (10 floats/vertex). World transform is already applied.
inline bool BuildWorldTriangles(const uint8_t *data,size_t size,
                                std::vector<float> *vertices,
                                const char *model_name="101draft") {
    if(!data || !vertices || !model_name ||
       std::strncmp(model_name,"UCX_",4)==0 || size<40u ||
       std::memcmp(data,"Kaydara FBX Binary  ",20)!=0 ||
       U32(data+23)!=7300u)return false;
    Geometry g;
    if(!Extract(data,size,27,size,0,false,false,model_name,&g) ||
       g.xyz.size()<9u || g.uv.empty() || g.poly.empty() ||
       g.poly.size()!=g.uv_indices.size() ||
       g.poly.size()>90000u || g.poly.size()%3u!=0u)return false;
    const size_t points=g.xyz.size()/3u;
    const size_t uv_points=g.uv.size()/2u;
    // UE exported visible models in centimeters, z up. Axis mapping
    // X, Z, -Y is a rotation to the native game coordinate system.
    float mins[3]={1e9f,1e9f,1e9f};
    float maxs[3]={-1e9f,-1e9f,-1e9f};
    for(size_t p=0;p<points;++p) {
        const float xyz[3]={float(g.xyz[p*3]),float(g.xyz[p*3+2]),
                            -float(g.xyz[p*3+1])};
        for(int k=0;k<3;++k) {
            if(!std::isfinite(xyz[k]))return false;
            mins[k]=std::min(mins[k],xyz[k]);
            maxs[k]=std::max(maxs[k],xyz[k]);
        }
    }
    float extent=0.f;
    for(int k=0;k<3;++k)extent=std::max(extent,maxs[k]-mins[k]);
    if(extent<.001f || extent>100000.f)return false;
    // Fit the original aspect ratio into a 40cm-wide inspection podium.
    const float scale=.40f/extent;
    const float center[3]={.60f,-.57f,-1.54f};
    std::vector<float> staging;
    staging.reserve(g.poly.size()*10u);
    for(size_t face=0;face<g.poly.size();face+=3u) {
        for(int corner=0;corner<3;++corner) {
            const size_t at=face+size_t(corner);
            const int32_t index=g.poly[at];
            if((corner==2 && index>=0) ||
               (corner!=2 && index<0))return false;
            const int64_t decoded=index<0?-int64_t(index)-1:index;
            const int32_t tex=g.uv_indices[at];
            if(decoded<0 || size_t(decoded)>=points ||
               tex<0 || size_t(tex)>=uv_points)return false;
            const float src[3]={float(g.xyz[decoded*3]),
                float(g.xyz[decoded*3+2]),-float(g.xyz[decoded*3+1])};
            for(int k=0;k<3;++k) {
                staging.push_back(center[k]+
                    (src[k]-(mins[k]+maxs[k])*.5f)*scale);
            }
            const double u=g.uv[size_t(tex)*2u];
            const double v=g.uv[size_t(tex)*2u+1u];
            if(!std::isfinite(u)||!std::isfinite(v))return false;
            staging.push_back(float(u));
            staging.push_back(float(1.0-v));
            staging.push_back(1.f);staging.push_back(1.f);
            staging.push_back(1.f);staging.push_back(0.f);
            staging.push_back(0.f);
        }
    }
    vertices->swap(staging);
    return true;
}
}}}
#endif
