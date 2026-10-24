#ifndef REVERIE_GUMBALL_SPACE_3D_H
#define REVERIE_GUMBALL_SPACE_3D_H
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
namespace reverie { namespace gumball {
// Fixed-function triangle generator in the SAME 3D position/color vertex
// format as the assembly renderer; no per-frame GLES object creation.
inline void AppendSpaceBox(std::vector<float>& mesh,
                           float x,float y,float z,float sx,float sy,float sz,
                           float r,float g,float b){
    constexpr float corners[8][3]={
        {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
        {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
    constexpr int sides[6][4]={
        {4,5,6,7},{1,0,3,2},{5,1,2,6},
        {0,4,7,3},{7,6,2,3},{0,1,5,4}};
    constexpr float shades[6]={1.f,.55f,.72f,.84f,.94f,.64f};
    constexpr int triangles[6]={0,1,2,0,2,3};
    for(int f=0;f<6;++f)
        for(int t:triangles){
            const int c=sides[f][t];
            mesh.push_back(x+sx*corners[c][0]);
            mesh.push_back(y+sy*corners[c][1]);
            mesh.push_back(z+sz*corners[c][2]);
            mesh.push_back(std::min(1.f,r*shades[f]));
            mesh.push_back(std::min(1.f,g*shades[f]));
            mesh.push_back(std::min(1.f,b*shades[f]));
        }
}
inline uint32_t SpaceNoise(uint32_t v){
    v^=v>>16;v*=0x7feb352du;v^=v>>15;
    v*=0x846ca68bu;return v^(v>>16);
}
// Sixteen actual 3D candy-space rocks and traveling luminous navigation
// beacons. World Z changes with mission time; GLES perspective therefore
// changes projected size and position, rather than scrolling a 2D wallpaper.
inline void AppendSpaceLane(std::vector<float>& mesh,float seconds,uint32_t seed){
    const float elapsed=std::max(0.f,seconds);
    for(unsigned i=0;i<16u;++i){
        const uint32_t h=SpaceNoise(seed+i*1973u+0xCAEDu);
        const float phase=static_cast<float>(h%2400u)*.005f;
        const float range=12.f;
        const float travel=std::fmod(phase+elapsed*1.4f,range);
        const float z=-15.5f+travel;
        const float x=(static_cast<float>((h>>11u)%1000u)/999.f-.5f)*5.0f;
        const float y=(static_cast<float>((h>>21u)%1000u)/999.f-.5f)*3.4f;
        const float radius=.025f+static_cast<float>(h%9u)*.006f;
        const bool beacon=(h&3u)==0u;
        if(beacon)
            AppendSpaceBox(mesh,x,y,z,radius*.55f,radius*.55f,radius*2.0f,
                           .50f,.92f,1.f);
        else
            AppendSpaceBox(mesh,x,y,z,radius,radius*.8f,radius*1.1f,
                           .40f,.25f,.52f);
    }
    for(int side=-1;side<=1;side+=2){
        for(int i=0;i<8;++i){
            const float z=-15.f+std::fmod(i*1.7f+elapsed*1.4f,11.9f);
            AppendSpaceBox(mesh,side*2.25f,-1.45f,z,
                           .018f,.018f,.27f,.65f,.33f,.95f);
        }
    }
}
}}
#endif
