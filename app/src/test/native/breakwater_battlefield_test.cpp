#include "breakwater_coastal_world.h"
#include <cassert>
#include <iostream>
using namespace reverie::breakwater;
using namespace reverie::breakwater::battlefield;
int main() {
    static_assert(kFarClip>5000 && kSeaFar< -3000,"Kilometre scale");
    for(uint8_t location=0;location<Campaign::kLocations;++location) {
        assert(std::abs(Ground(location,0,0)+1.04f)<.001f);
        assert(std::abs(Ground(location,0,kShoreZ)-kSeaY)<.001f);
        assert(Ground(location,0,kShoreZ-30)<kSeaY);
        assert(Ground(location,0,-50)>kSeaY+5.f);
        std::vector<float> textures;
        size_t colored=0;
        auto budget=Coast(location,false,textures,[&](auto... values){
            const float v[]={values...};for(float f:v)assert(std::isfinite(f));++colored;
        });
        assert(budget.textured>300 && budget.textured<1500);
        assert(budget.colored==colored && colored<1500);
        assert(textures.size()==budget.textured*3*5);
        size_t sea=0;float far=0.f;
        Sea(location,false,1.f,[&](float x,float y,float z,float a,float b,float c,
            float d,float e,float f,float r,float g,float blue) {
            for(float v:{x,y,z,a,b,c,d,e,f,r,g,blue})assert(std::isfinite(v));
            far=std::min(far,std::min(z,std::min(c,f)));++sea;
        });
        assert(sea==kSeaAcross*kSeaRows*2 && far< -3399.f);
        // Long-range normalized sight intersects the same placed hull that
        // is rendered. Legacy 40-unit cutoffs must not survive this bridge.
        Enemy enemy{};enemy.active=true;enemy.type=UnitType::LandingCraft;
        enemy.x=1.f;enemy.z=-11.f;
        const auto placement=Vessel(enemy);
        float center[]={enemy.x,-.63f,enemy.z};
        float world[3]={center[0],center[1],center[2]};
        placement.Point(world[0],world[1],world[2]);
        assert(world[2]< -500.f && world[1]< -15.f);
        for(int station=0;station<defense::kPositions;++station) {
            const auto site=defense::At(location,station);
            const float origin[]={site.x,site.y,site.z};
            float ray[]={world[0]-origin[0],world[1]-origin[1],world[2]-origin[2]};
            float length=std::sqrt(ray[0]*ray[0]+ray[1]*ray[1]+ray[2]*ray[2]);
            for(float &v:ray)v/=length;
            float depth=0.f;
            assert(Sphere(placement,origin,ray,0.f,center,.82f,&depth));
            assert(depth>500.f && depth<length);
            ray[0]+=.04f;assert(!Sphere(placement,origin,ray,0.f,center,.82f,&depth));
            // A downward shot must be stopped by actual land/water.
            const float down[]={0.f,-.25f,-1.f};
            assert(Terrain(location,origin,down,0.f,kRange,&depth));
            float point[3];assert(Position(origin,down,depth,0.f,point));
            assert(std::abs(point[1]-std::max(kSeaY,Ground(location,point[0],point[2])))<.002f);
        }
        const Placement person=Land(location,.4f,-1.4f);
        float footX=.4f,footY=-1.04f,footZ=-1.4f;person.Point(footX,footY,footZ);
        assert(std::abs(footY-Ground(location,footX,footZ))<.001f);
        // Cover uses exactly the model's scaled AABB, including its top.
        ShoreCover wall{0.f,-1.17f,100,100};
        const Placement placed=Land(location,wall.x,wall.z,3.f);
        float topX=wall.x,topY=kCoverBaseY+CoverHeight(wall)*.5f,topZ=wall.z;
        placed.Point(topX,topY,topZ);
        const float origin[]={0.f,0.f,0.f};
        float aim[]={topX,topY,topZ};const float norm=std::hypot(aim[1],aim[2]);
        for(float &v:aim)v/=norm;
        float hit=0.f;assert(Cover(location,wall,origin,aim,0.f,&hit));assert(hit>40.f);
    }
    assert(SeaHeight(17,-150,0)!=SeaHeight(17,-150,1));
    const auto site=defense::At(0,0);
    assert(DetailAt(0,-100,site)==Detail::Full);
    assert(DetailAt(0,-300,site)==Detail::Reduced);
    assert(DetailAt(0,-900,site)==Detail::Distant);
    std::cout<<"PASS expanded coastal terrain, bounded LOD, grounded actors, 600m hits and terrain occlusion\n";
}
