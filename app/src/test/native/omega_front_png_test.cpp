#include "../../main/jni/native/omega_front_png.h"
#include <zlib.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

using reverie::omega_art::DecodePng;
using reverie::omega_art::Image;

static void Be32(std::vector<uint8_t>& bytes,uint32_t value) {
    bytes.push_back(uint8_t(value>>24u));
    bytes.push_back(uint8_t(value>>16u));
    bytes.push_back(uint8_t(value>>8u));
    bytes.push_back(uint8_t(value));
}
static void Chunk(std::vector<uint8_t>& png,const char* tag,
                  const std::vector<uint8_t>& payload) {
    Be32(png,static_cast<uint32_t>(payload.size()));
    const size_t offset=png.size();
    for(int i=0;i<4;++i)png.push_back(uint8_t(tag[i]));
    png.insert(png.end(),payload.begin(),payload.end());
    uLong checksum=crc32(0L,Z_NULL,0);
    checksum=crc32(checksum,png.data()+offset,
                   static_cast<uInt>(payload.size()+4u));
    Be32(png,static_cast<uint32_t>(checksum));
}
static std::vector<uint8_t> MakePng(unsigned width,unsigned height,
                                    unsigned depth,unsigned kind,
                                    const std::vector<uint8_t>& scanlines,
                                    const std::vector<uint8_t>& palette={},
                                    const std::vector<uint8_t>& alpha={}) {
    std::vector<uint8_t> png={137,80,78,71,13,10,26,10};
    std::vector<uint8_t> ihdr;
    Be32(ihdr,width);Be32(ihdr,height);
    ihdr.insert(ihdr.end(),{uint8_t(depth),uint8_t(kind),0,0,0});
    Chunk(png,"IHDR",ihdr);
    if(!palette.empty())Chunk(png,"PLTE",palette);
    if(!alpha.empty())Chunk(png,"tRNS",alpha);
    uLongf bound=compressBound(static_cast<uLong>(scanlines.size()));
    std::vector<uint8_t> packed(static_cast<size_t>(bound));
    assert(compress2(packed.data(),&bound,scanlines.data(),
                     static_cast<uLong>(scanlines.size()),Z_BEST_SPEED)==Z_OK);
    packed.resize(static_cast<size_t>(bound));
    Chunk(png,"IDAT",packed);
    Chunk(png,"IEND",{});
    return png;
}
static void TestPackedIndexed(unsigned depth,unsigned width,
                              const std::vector<uint8_t>& scan,
                              const std::vector<uint8_t>& expected) {
    std::vector<uint8_t> palette;
    for(unsigned i=0;i<(1u<<depth);++i) {
        palette.push_back(uint8_t(i*12u));
        palette.push_back(uint8_t(i*3u));
        palette.push_back(uint8_t(i));
    }
    Image decoded;
    const auto png=MakePng(width,1,depth,3,scan,palette);
    assert(DecodePng(png.data(),png.size(),decoded));
    assert(decoded.width==width && decoded.height==1);
    for(unsigned x=0;x<width;++x) {
        const unsigned offset=x*4u;
        assert(decoded.rgba[offset]==uint8_t(expected[x]*12u));
        assert(decoded.rgba[offset+1u]==uint8_t(expected[x]*3u));
        assert(decoded.rgba[offset+2u]==expected[x]);
        assert(decoded.rgba[offset+3u]==255u);
    }
}
static void SyntheticCoverage() {
    TestPackedIndexed(1,8,{0,0xb4},{1,0,1,1,0,1,0,0});
    TestPackedIndexed(2,4,{0,0x1b},{0,1,2,3});
    TestPackedIndexed(4,3,{0,0x12,0x30},{1,2,3});
    TestPackedIndexed(8,2,{0,2,1},{2,1});
    // Packed indexed colors with tRNS are not accidentally opaque.
    const auto pal=std::vector<uint8_t>{0,0,0,255,32,16};
    const auto opaque=MakePng(2,1,1,3,{0,0x80},pal,{0,255});
    Image image;
    assert(DecodePng(opaque.data(),opaque.size(),image));
    assert(image.rgba[3]==255 && image.rgba[7]==0);
    // Grayscale 2-bit levels expand 0..3 to full 0..255.
    const auto gray=MakePng(4,1,2,0,{0,0x1b});
    assert(DecodePng(gray.data(),gray.size(),image));
    assert(image.rgba[0]==0 && image.rgba[4]==85);
    assert(image.rgba[8]==170 && image.rgba[12]==255);
    // Grayscale 1-bit transparency compares raw sample, not scaled RGB.
    const auto grayAlpha=MakePng(2,1,1,0,{0,0x80},{},{0,1});
    assert(DecodePng(grayAlpha.data(),grayAlpha.size(),image));
    assert(image.rgba[0]==255 && image.rgba[3]==0);
    assert(image.rgba[4]==0 && image.rgba[7]==255);
    // Ordinary true-color PNGs continue to decode identically.
    const auto rgba=MakePng(1,1,8,6,{0,11,22,33,44});
    assert(DecodePng(rgba.data(),rgba.size(),image));
    assert(image.rgba==std::vector<uint8_t>({11,22,33,44}));
    // PNG filters use byte stride=1 for packed samples. Test Sub on a
    // non-byte-aligned 4-bit palette width with two rows.
    std::vector<uint8_t> palette;
    for(unsigned i=0;i<16;++i)
        palette.insert(palette.end(),{uint8_t(i),0u,0u});
    const auto rows=MakePng(3,2,4,3,{0,0x12,0x30,1,0x45,0x1b},palette);
    assert(DecodePng(rows.data(),rows.size(),image));
    assert(image.rgba[12]==4 && image.rgba[16]==5);
    assert(image.rgba[20]==6);
    // A tampered CRC or unsupported bit depth must still be rejected.
    auto bad=rgba;bad[bad.size()-13]^=0x10;
    assert(!DecodePng(bad.data(),bad.size(),image));
    bad=MakePng(1,1,16,6,{0,0,1,2,3,4,5,6,7});
    assert(!DecodePng(bad.data(),bad.size(),image));
}
int main(int argc,char** argv) {
    SyntheticCoverage();
    // The CI passes the actual unmodified source PNGs. Decode EVERY one,
    // not just a synthetic approximation of the art format.
    if(argc>1)assert(argc==65);
    for(int i=1;i<argc;++i) {
        std::ifstream stream(argv[i],std::ios::binary);
        if(!stream) {
            std::fprintf(stderr,"Cannot open CC0 sprite: %s\n",argv[i]);
            return 1;
        }
        std::vector<uint8_t> bytes(
            (std::istreambuf_iterator<char>(stream)),
            std::istreambuf_iterator<char>());
        Image image;
        if(!DecodePng(bytes.data(),bytes.size(),image) ||
           image.width!=16u || image.height!=16u ||
           image.rgba.size()!=16u*16u*4u) {
            std::fprintf(stderr,"CC0 decoder rejected source: %s\n",argv[i]);
            return 1;
        }
    }
    std::printf("PASS: packed PNG (1/2/4/8-bit), RGBA, CRC and %d real CC0 tiles\n",
                argc-1);
    return 0;
}
