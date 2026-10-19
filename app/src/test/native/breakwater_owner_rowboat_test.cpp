#include "../../main/jni/native/breakwater_beckenburg_mesh.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>

using namespace reverie::breakwater;

int main() {
    static_assert(imported::OwnerRowboat==3u,"Owned rowboat index changed");
    static_assert(imported::kAssetPaths.size()==5u,"Missing owner mesh");
    const char *source="app/src/main/assets/native/breakwater/beckenburg/"
                       "owner_boat_rowboat.bwm";
    std::ifstream file(source,std::ios::binary);
    assert(file.is_open());
    const std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(file),
                                     std::istreambuf_iterator<char>()};
    assert(bytes.size()==8u+1180u*27u);
    assert(bytes[0]=='B'&&bytes[1]=='W'&&bytes[2]=='M'&&bytes[3]=='1');
    imported::Model boat;
    assert(boat.Parse(bytes.data(),bytes.size()));
    assert(boat.triangles==1180u);

    std::array<float,3> lo{{1e9f,1e9f,1e9f}},hi{{-1e9f,-1e9f,-1e9f}};
    size_t emitted=0u; bool finite=true;
    const auto collect=[&](auto... components) {
        const std::array<float,sizeof...(components)> values{{
            static_cast<float>(components)...
        }};
        static_assert(sizeof...(components)==12u,"Expected XYZ/XYZ/XYZ/RGB");
        ++emitted;
        for(float v:values)finite=finite&&std::isfinite(v);
        for(unsigned i=0;i<9u;i++) {
            const unsigned axis=i%3u;
            lo[axis]=std::min(lo[axis],values[i]);
            hi[axis]=std::max(hi[axis],values[i]);
        }
        for(unsigned i=9u;i<12u;i++)
            assert(values[i]>=0.f && values[i]<=1.f);
    };
    assert(boat.Emit(2.f,-.77f,-4.f,1.18f,1.57079632679f,collect)==1180u);
    assert(finite && emitted==1180u);
    // The authored GLB is X-forward; yaw rotates its longer 1-unit axis
    // into the campaign Z lane. Keep the visible hull inside the approx.
    // 0.62-unit rowboat aiming radius instead of silently widening hitboxes.
    assert(hi[2]-lo[2]>1.0f && hi[2]-lo[2]<1.3f);
    assert(hi[0]-lo[0]>.5f && hi[0]-lo[0]<.8f);
    assert(lo[1]>-1.10f && hi[1]<-.45f);
    assert(boat.Emit(0.f,-.77f,-4.f,
        std::numeric_limits<float>::quiet_NaN(),0.f,collect)==0u);
    std::vector<uint8_t> broken(bytes.begin(),bytes.end()-1u);
    assert(!boat.Parse(broken.data(),broken.size()));
}
