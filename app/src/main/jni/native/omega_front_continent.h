#ifndef REVERIE_OMEGA_FRONT_CONTINENT_H
#define REVERIE_OMEGA_FRONT_CONTINENT_H
// Deterministic, bounded, data-only geography for the strategic command map.
// Does not mutate the 16-sector Hearthfall campaign or serialized state.
#include <array>
#include <cstdint>
#include <cmath>

namespace reverie { namespace omega {
constexpr int kContinentWidth=30;
constexpr int kContinentHeight=20;
constexpr int kContinentCells=kContinentWidth*kContinentHeight;
constexpr int kHearthfallMinX=12;
constexpr int kHearthfallMinY=8;
// These indices are stable and never depend on global random state.
enum class ContinentTerrain : uint8_t {
    Sea, Shoal, Meadow, Forest, Dryland, Badlands, Highland,
    Snow, Marsh, River, Road, Ford, Ruins, Farm
};
struct ContinentalCell {
    ContinentTerrain terrain=ContinentTerrain::Sea;
    uint8_t elevation=0;
};
class Continent {
public:
    explicit Continent(uint32_t seed=414u) : seed_(seed?seed:414u) {
        for(int y=0;y<kContinentHeight;++y)
            for(int x=0;x<kContinentWidth;++x)
                cells_[Index(x,y)]=Make(x,y);
        // An authored, connected Hearthfall rail/road district, not a 4x4
        // rectangle drawn against empty space. The real game sector IDs live
        // in this contiguous subregion; the rest is visual regional geography.
        for(int row=0;row<4;++row)
            for(int col=0;col<4;++col){
                const int x=kHearthfallMinX+col,y=kHearthfallMinY+row;
                auto &cell=cells_[Index(x,y)];
                // Every named sector inherits plausible, authored geography:
                // S005 is the ford, S006 the orchard, S011 the waterworks,
                // S015 the broken viaduct; shattered southern sectors hurt.
                static constexpr ContinentTerrain sites[16]={
                    ContinentTerrain::Badlands,ContinentTerrain::Badlands,
                    ContinentTerrain::Highland,ContinentTerrain::Ruins,
                    ContinentTerrain::Ford,ContinentTerrain::Farm,
                    ContinentTerrain::Road,ContinentTerrain::Road,
                    ContinentTerrain::Badlands,ContinentTerrain::Ruins,
                    ContinentTerrain::Marsh,ContinentTerrain::Ruins,
                    ContinentTerrain::Badlands,ContinentTerrain::Badlands,
                    ContinentTerrain::Ruins,ContinentTerrain::Dryland
                };
                cell.terrain=sites[row*4+col];
                cell.elevation=25;
            }
    }
    static constexpr bool Inside(int x,int y) {
        return x>=0 && y>=0 && x<kContinentWidth && y<kContinentHeight;
    }
    static constexpr int Index(int x,int y){return y*kContinentWidth+x;}
    const ContinentalCell &At(int x,int y) const {
        return cells_[Index(Inside(x,y)?x:0,Inside(x,y)?y:0)];
    }
    static constexpr int SectorX(int sector){return kHearthfallMinX+sector%4;}
    static constexpr int SectorY(int sector){return kHearthfallMinY+sector/4;}
    // Terrain supplies a real, deterministic logistical consequence: heavy
    // badlands/highlands need twice the marching rations. Diplomatic passes
    // and existing viaduct restrictions are still enforced by Frontier.
    int MarchRations(int sector) const {
        if(sector<0||sector>=16)return 0;
        const ContinentTerrain t=At(SectorX(sector),SectorY(sector)).terrain;
        return t==ContinentTerrain::Badlands || t==ContinentTerrain::Highland
            ? 2 : 1;
    }
    static constexpr int SectorAt(int x,int y){
        return x>=kHearthfallMinX && x<kHearthfallMinX+4 &&
            y>=kHearthfallMinY && y<kHearthfallMinY+4?
            (y-kHearthfallMinY)*4+(x-kHearthfallMinX):-1;
    }
    // Local adjacency remains 4-way, with actual existing viaduct gate. This
    // helper makes continental sector geometry available to routing/UX.
    static constexpr bool AdjacentHearthfall(int a,int b){
        return a>=0&&a<16&&b>=0&&b<16&&
            ((SectorX(a)==SectorX(b) &&
              (SectorY(a)+1==SectorY(b)||SectorY(b)+1==SectorY(a))) ||
             (SectorY(a)==SectorY(b) &&
              (SectorX(a)+1==SectorX(b)||SectorX(b)+1==SectorX(a))));
    }
    const std::array<ContinentalCell,kContinentCells> &cells() const {return cells_;}
private:
    uint32_t seed_;
    std::array<ContinentalCell,kContinentCells> cells_{};
    static uint32_t Mix(uint32_t v){
        v^=v>>16;v*=0x7feb352du;v^=v>>15;v*=0x846ca68bu;return v^(v>>16);
    }
    unsigned Noise(int x,int y) const {
        return Mix(seed_ ^ (uint32_t(x+317)*0x9e3779b9u) ^
                   (uint32_t(y+601)*0x85ebca6bu)) % 100u;
    }
    ContinentalCell Make(int x,int y) const {
        // Coastline: warped elliptical mainland with a western gulf, eastern
        // peninsula and northern islands. All coordinates remain deterministic.
        const float nx=(x-14.5f)/15.f,ny=(y-9.5f)/10.f;
        const float warp=std::sin(float(y)*.43f)*.12f+
            std::cos(float(x)*.31f)*.08f;
        const float eccentric=(nx*nx*.75f+ny*ny*1.05f);
        const float bay=(x<12 && y>9 && y<16)?-.22f:0.f;
        const float cape=(x>20 && y>8 && y<17)?+.25f:0.f;
        const float land=.87f-eccentric+warp+bay+cape+
            (int(Noise(x/2,y/2))-50)*.0025f;
        const unsigned n=Noise(x,y);
        if(land<-.035f)return {ContinentTerrain::Sea,0};
        if(land<.06f)return {ContinentTerrain::Shoal,3};
        const float ridge=std::abs(float(x)-(.68f*y+5.0f+
                                    2.5f*std::sin(y*.48f)));
        const float north=1.f-float(y)/float(kContinentHeight-1);
        if(ridge<1.65f && y<15)return {north>.58f?ContinentTerrain::Snow:
                                      ContinentTerrain::Highland,95};
        if(ridge<3.1f && y<15)return {ContinentTerrain::Highland,75};
        // Connected river from mountain headwaters to the southern estuary.
        const int riverX=18+int(2.5f*std::sin(y*.37f));
        if(y>=4 && y<=18 && std::abs(x-riverX)<=0 && land>.08f)
            return {ContinentTerrain::River,5};
        if(y>=12 && y<=18 && std::abs(x-riverX)<=2 && n<34)
            return {ContinentTerrain::Marsh,8};
        // A continent-spanning trade road with a southern branch. It is only
        // drawn on dry land; strategic sector entry remains save-compatible.
        if((y==9 && x>=7 && x<=25) ||
           (x==23 && y>=9 && y<=16))
            return {ContinentTerrain::Road,23};
        if(x>23 && y<9 && n>45)return {ContinentTerrain::Badlands,28};
        if(y<5 && n>48)return {ContinentTerrain::Snow,40};
        if(x<10 && y>12 && n>56)return {ContinentTerrain::Dryland,20};
        if(n<12)return {ContinentTerrain::Ruins,25};
        if(n<41)return {ContinentTerrain::Forest,36};
        if(n<50)return {ContinentTerrain::Farm,21};
        if(n>87)return {ContinentTerrain::Badlands,30};
        return {ContinentTerrain::Meadow,26};
    }
};
}} // namespace reverie::omega
#endif