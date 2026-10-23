#include "omega_front_cc0_palette.h"
#include <cassert>
#include <array>
#include <iostream>

int main(){
    using reverie::omega::Ground;
    using reverie::omega_art::GroundVariant;
    // Visual selection is deterministic, bounded and independent of battle
    // mutations. Test every imported tile is addressable on a legal map.
    std::array<bool,reverie::omega_art::kCc0VisualTiles> reachable{};
    const Ground all[]={Ground::Plain,Ground::Road,Ground::Rubble,
                        Ground::Wall,Ground::DeepWater};
    for(int op=0;op<6;++op){
        for(int y=0;y<reverie::omega::kWidth;++y)
        for(int x=0;x<reverie::omega::kWidth;++x)
        for(Ground g:all){
            int tile=GroundVariant(g,op,x,y);
            assert(tile>=-1&&tile<reverie::omega_art::kCc0VisualTiles);
            if(tile>=0)reachable[static_cast<size_t>(tile)]=true;
            assert(tile==GroundVariant(g,op,x,y)); // save/load-safe art
        }
    }
    for(size_t n=0;n<reachable.size();++n) {
        if(!reachable[n])
            std::cerr<<"CC0 source sprite has no gameplay placement: "<<n<<"\n";
        assert(reachable[n]); // reject decorative dead inventory
    }
    assert(GroundVariant(Ground::Road,0,0,0)==-1);
    assert(GroundVariant(Ground::Plain,0,-1,0)==-1);
    assert(GroundVariant(Ground::Wall,2,24,0)==-1);
    assert(GroundVariant(Ground::DeepWater,1,0,0)>=32);
    std::cout<<"PASS: all 64 real CC0 RPG tiles are deterministically used"
                " by Omega Front tactical level themes\n";
}
