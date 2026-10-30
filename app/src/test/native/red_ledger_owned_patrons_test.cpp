#include "../../main/jni/native/red_ledger_owned_patrons.h"
#include "../../main/jni/native/red_ledger_beckenburg_props.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using namespace reverie::redledger;
namespace people=reverie::redledger::owned_patrons;
namespace mesh=reverie::redledger::beckenburg;

int main() {
    const PatronKind kinds[people::kCount] = {
        PatronKind::RailWorker, PatronKind::NightClerk,
        PatronKind::MilitiaVeteran, PatronKind::Mechanic,
        PatronKind::DirectorateCourier, PatronKind::SalihorskHauler
    };
    for (size_t i=0; i<people::kCount; ++i) {
        assert(static_cast<size_t>(people::ForPatron(kinds[i]))==i);
        const std::string path=people::kPaths[i];
        assert(path.find("native/red-ledger/beckenburg/patron_")==0u);
        assert(path.size()>=7u && path.substr(path.size()-7u)==".rlprop");
        for (size_t j=0; j<i; ++j)
            assert(path!=people::kPaths[j]);
    }
    // Check the same RLBP decoder used by the GLES2 loader. These bytes are
    // explicitly synthetic test geometry, not claims of imported characters.
    std::vector<uint8_t> data(16u+3u*mesh::kStride*sizeof(float)+
                              mesh::kTextureBytes,0u);
    std::memcpy(data.data(),"RLBP",4u);
    data[4]=1u; // little-endian format version
    data[8]=3u; // one triangle
    data[12]=0u; data[13]=1u; // 256px texture side
    std::vector<float> vertices;
    const uint8_t *texture=nullptr;
    assert(mesh::Decode(data.data(),data.size(),&vertices,&texture));
    assert(vertices.size()==3u*mesh::kStride);
    assert(texture!=nullptr);
    assert(!mesh::Decode(data.data(),data.size()-1u,&vertices,&texture));
    return 0;
}
