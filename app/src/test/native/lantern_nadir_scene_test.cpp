#include "lantern_nadir_scene.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
using namespace reverie::lantern;
int main(){
 for(auto scene:{nadir::Scene::Rail,nadir::Scene::Orchard,nadir::Scene::River}){
    const auto before=nadir::Make(scene,false);
    const auto after=nadir::Make(scene,true);
    assert(before.Valid()&&after.Valid());
    assert(before.width==512&&after.width==512);
    assert(before.pixels!=after.pixels);
    assert(before.pixels==nadir::Make(scene,false).pixels);
    uint64_t modified=0;
    for(int y=0;y<512;++y)for(int x=0;x<512;++x){
        const auto index=static_cast<size_t>(y)*512u+x;
        if(before.pixels[index]!=after.pixels[index]){
            ++modified;
            const float u=(x+.5f)/512.f,v=(y+.5f)/512.f;
            if(scene==nadir::Scene::River)
                assert(std::abs(u-nadir::RiverCenter(v))<.106f &&
                       std::abs(v-nadir::RoadCenter(u))<.02f);
            else if(scene==nadir::Scene::Rail)
                assert(u>.35f&&u<.72f && v>.47f&&v<.64f);
            else
                assert((u>.35f&&u<.72f&&v>.47f&&v<.64f) ||
                       (u>.49f&&u<.54f&&v>.47f&&v<.51f));
        }
    }
    assert(modified>10 && modified<15000);
    // A fixed nadir camera must not teleport any unrelated geography:
    // at the corners and river banks the same source RGB remains exact.
    for(int y:{10,80,470})for(int x:{15,490})
        assert(before.pixels[static_cast<size_t>(y)*512+x]==
               after.pixels[static_cast<size_t>(y)*512+x]);
 }
 std::cout<<"PASS Lantern 512px nadir survey: stable geographic registration, localized bridge loss, convoy and civilian lookalike\n";
}
