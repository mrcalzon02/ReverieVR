#ifndef REVERIE_TEST_CHAMBER_SHOWCASE_H
#define REVERIE_TEST_CHAMBER_SHOWCASE_H
#include "reverie_native_module.h"
#include <GLES2/gl2.h>
#include <cstdint>
#include <vector>

// Companion-only renderer for actual, owner-approved textured GLB assets.
// It does not modify Test Chamber's existing ABI or the hub's native module.
struct TestShowcaseMesh {
    int model=0,width=0,height=0;
    std::vector<float> xyzuv;
    std::vector<uint8_t> rgba;
    GLuint vbo=0,texture=0;
    GLsizei vertex_count=0;
};
struct TestShowcase {
    std::vector<TestShowcaseMesh> meshes;
    GLuint program=0;
    GLint matrix=-1,sampler=-1,position=-1,uv=-1;
    bool Add(int model,const std::vector<float>&vertices,
             int width,int height,const std::vector<uint32_t>&argb);
    bool HasFiveModels()const;
    bool CreateGl();
    void ReleaseGl();
    bool Draw(const ReverieNativeEyeV1 *eye)const;
};
#endif
