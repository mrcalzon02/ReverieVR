#include "omega_front_png.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
using namespace reverie;
int main() {
    for(const char *name:{"shingle_bay","iron_quay","blackcap_island","stormwatch_point"}) {
        const std::string path=std::string("app/src/main/assets/native/shared/v1/horizons/")+name+"_skybox_1024x512.png";
        std::ifstream file(path,std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
        assert(!bytes.empty() && bytes.size()<131072u);
        omega_art::Image image;
        // Existing material/sprite callers retain their old 512px limit.
        assert(!omega_art::DecodePng(bytes.data(),bytes.size(),image));
        assert(omega_art::DecodePng(bytes.data(),bytes.size(),image,1024u));
        assert(image.width==1024u && image.height==512u);
        for(unsigned y=0;y<image.height;++y)for(int c=0;c<4;++c)
            assert(image.rgba[(size_t(y)*image.width)*4+c]==
                   image.rgba[(size_t(y)*image.width+image.width-1)*4+c]);
        assert(!omega_art::DecodePng(bytes.data(),bytes.size(),image,2048u));
        bytes[bytes.size()/2]^=1u;
        assert(!omega_art::DecodePng(bytes.data(),bytes.size(),image,1024u));
    }
    std::cout<<"PASS exact 1024px archived horizons, wrap seam, CRC and decoder opt-in bounds\n";
}
