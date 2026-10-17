#ifndef REVERIE_OMEGA_FRONT_TEXTURE_RENDERER_H
#define REVERIE_OMEGA_FRONT_TEXTURE_RENDERER_H

#include "omega_front_png.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_utils.h"
#include <GLES2/gl2.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace reverie { namespace omega_art {

// Fixed 64-pixel art grid, one quad per simulated cell. No atlas UV guesses.
enum TextureId { Soil=0, Road, Rubble, Wall, Water,
                 HearthwellHull, MeridianHull, ScraplineHull,
                 LostNinthHull, IndependentHull,
                 RifleH, RifleR, EngineerH, EngineerR, SalvagerH, SalvagerR,
                 SentinelH, SentinelR, LegionaryH, LegionaryR,
                 RifleHAlt, RifleRAlt, EngineerHAlt, EngineerRAlt,
                 SalvagerHAlt, SalvagerRAlt, SentinelHAlt, SentinelRAlt,
                 LegionaryHAlt, LegionaryRAlt, Count };
constexpr int kTerrainCount=5;
static const char *const kPaths[Count]={
    "native/omega-front/terrain/cracked_earth.png",
    "native/omega-front/terrain/ruined_concrete.png",
    "native/omega-front/terrain/rocky_wasteland.png",
    "native/omega-front/terrain/rocky_scree.png",
    "native/omega-front/terrain/marsh_pool.png",
    "native/omega-front/units/kenney/hearthwell_hull_north.png",
    "native/omega-front/units/kenney/meridian_hull_north.png",
    "native/omega-front/units/kenney/scrapline_hull_north.png",
    "native/omega-front/units/kenney/lost_ninth_hull_north.png",
    "native/omega-front/units/kenney/independent_hull_north.png",
    "native/omega-front/units/kenney/rifle_hearthwell.png",
    "native/omega-front/units/kenney/rifle_raider.png",
    "native/omega-front/units/kenney/engineer_hearthwell.png",
    "native/omega-front/units/kenney/engineer_raider.png",
    "native/omega-front/units/kenney/salvager_hearthwell.png",
    "native/omega-front/units/kenney/salvager_raider.png",
    "native/omega-front/units/kenney/sentinel_hearthwell.png",
    "native/omega-front/units/kenney/sentinel_raider.png",
    "native/omega-front/units/kenney/legionary_hearthwell.png",
    "native/omega-front/units/kenney/legionary_raider.png",
    "native/omega-front/units/kenney/rifle_hearthwell_veteran.png",
    "native/omega-front/units/kenney/rifle_raider_veteran.png",
    "native/omega-front/units/kenney/engineer_hearthwell_specialist.png",
    "native/omega-front/units/kenney/engineer_raider_meridian.png",
    "native/omega-front/units/kenney/salvager_hearthwell_scavenger.png",
    "native/omega-front/units/kenney/salvager_raider_scrapline.png",
    "native/omega-front/units/kenney/sentinel_hearthwell_guard.png",
    "native/omega-front/units/kenney/sentinel_raider_meridian.png",
    "native/omega-front/units/kenney/legionary_hearthwell_guard.png",
    "native/omega-front/units/kenney/legionary_raider_lost_ninth.png"
};
class SpriteRenderer {
public:
    bool upload=true;
    void Clear() {
        for(auto &mesh:mesh_)mesh.clear();
        upload=true;
    }
    bool Place(int slot,float x,float y,float w,float h,float z,
               bool facing_left=false) {
        if(slot<0||slot>=Count||!program_||!textures_[slot]||!buffers_[slot])
            return false;
        auto &v=mesh_[slot];
        const float l=x-w*.5f,r=x+w*.5f,b=y-h*.5f,t=y+h*.5f;
        auto put=[&v](float xx,float yy,float zz,float u,float vv) {
            v.insert(v.end(),{xx,yy,zz,u,vv});
        };
        // PNG row zero is its visual top. Mirror facing in UVs without
        // duplicating PNGs or shifting the one-cell anchor. Defenders look
        // right, opposing troops look left across the battlefield.
        const float ul=facing_left?1.f:0.f,ur=facing_left?0.f:1.f;
        put(l,b,z,ul,1.f);put(r,b,z,ur,1.f);put(r,t,z,ur,0.f);
        put(l,b,z,ul,1.f);put(r,t,z,ur,0.f);put(l,t,z,ul,0.f);
        return true;
    }
    void Initialize(const ReverieNativeHostV1 *host) {
        Shutdown();
        const char *vs="uniform mat4 u_Mvp;attribute vec3 a_Pos;"
            "attribute vec2 a_Uv;varying vec2 v_Uv;"
            "void main(){v_Uv=a_Uv;gl_Position=u_Mvp*vec4(a_Pos,1.0);}";
        const char *fs="precision mediump float;varying vec2 v_Uv;"
            "uniform sampler2D u_Tex;"
            "void main(){gl_FragColor=texture2D(u_Tex,v_Uv);}";
        const ReverieNativeGlAttributeBindingV1 bindings[]={
            {0u,"a_Pos"},{1u,"a_Uv"}
        };
        program_=ReverieNativeBuildProgram(vs,fs,bindings,2);
        if(!program_)return;
        position_=glGetAttribLocation(program_,"a_Pos");
        uv_=glGetAttribLocation(program_,"a_Uv");
        mvp_=glGetUniformLocation(program_,"u_Mvp");
        sampler_=glGetUniformLocation(program_,"u_Tex");
        if(position_<0||uv_<0||mvp_<0||sampler_<0) {
            Shutdown();return;
        }
        GLint old_unit=GL_TEXTURE0,old_binding=0;
        glGetIntegerv(GL_ACTIVE_TEXTURE,&old_unit);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&old_binding);
        glGenBuffers(Count,buffers_);
        int loaded=0;
        for(int slot=0;slot<Count;++slot)
            if(buffers_[slot] && Load(host,slot))++loaded;
        glBindTexture(GL_TEXTURE_2D,static_cast<GLuint>(old_binding));
        glActiveTexture(static_cast<GLenum>(old_unit));
        char status[96];
        std::snprintf(status,sizeof(status),
                      "Tactical PNG textures loaded: %d/%d",loaded,Count);
        ReverieNativeLogV1(host,loaded?REVERIE_NATIVE_LOG_INFO:
                           REVERIE_NATIVE_LOG_WARN,"omega-front",status);
        upload=true;
    }
    void Shutdown() {
        glDeleteTextures(Count,textures_);
        glDeleteBuffers(Count,buffers_);
        if(program_)glDeleteProgram(program_);
        for(int i=0;i<Count;++i)textures_[i]=buffers_[i]=0;
        program_=0;position_=uv_=mvp_=sampler_=-1;
        upload=true;
    }
    // Called after the colored-board batch; caller owns blend and state restore.
    void DrawRange(const float *matrix,int first,int last) {
        if(!program_)return;
        glUseProgram(program_);
        glUniformMatrix4fv(mvp_,1,GL_FALSE,matrix);
        glUniform1i(sampler_,0);
        glActiveTexture(GL_TEXTURE0);
        glEnableVertexAttribArray(static_cast<GLuint>(position_));
        glEnableVertexAttribArray(static_cast<GLuint>(uv_));
        for(int slot=first;slot<last;++slot) {
            const auto &v=mesh_[slot];
            if(!textures_[slot]||!buffers_[slot]||v.empty())continue;
            glBindTexture(GL_TEXTURE_2D,textures_[slot]);
            glBindBuffer(GL_ARRAY_BUFFER,buffers_[slot]);
            if(upload)
                glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(
                    v.size()*sizeof(float)),v.data(),GL_DYNAMIC_DRAW);
            glVertexAttribPointer(static_cast<GLuint>(position_),3,GL_FLOAT,GL_FALSE,
                                  5*sizeof(float),reinterpret_cast<const void *>(0));
            glVertexAttribPointer(static_cast<GLuint>(uv_),2,GL_FLOAT,GL_FALSE,
                                  5*sizeof(float),reinterpret_cast<const void *>(3*sizeof(float)));
            glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(v.size()/5u));
        }
    }
private:
    GLuint program_=0;
    GLuint textures_[Count]{},buffers_[Count]{};
    GLint position_=-1,uv_=-1,mvp_=-1,sampler_=-1;
    std::array<std::vector<float>,Count> mesh_{};
    bool Load(const ReverieNativeHostV1 *host,int slot) {
        if(!host||host->abi_version!=REVERIE_NATIVE_MODULE_ABI_VERSION||
           host->struct_size<REVERIE_NATIVE_HOST_V1_ASSET_MIN_SIZE||
           !host->read_asset)return false;
        // Packaged sprites are small, bounded, and decoded once per GL context.
        std::vector<uint8_t> png(256u*1024u);
        uint32_t length=0;
        if(host->read_asset(kPaths[slot],png.data(),
             static_cast<uint32_t>(png.size()),&length)!=REVERIE_NATIVE_SAVE_OK||
           !length||length>png.size())return false;
        Image image;
        if(!DecodePng(png.data(),length,image)||
           image.width==0u||image.height==0u||
           image.width>64u||image.height>64u)return false;
        // Kenney Shooter originals remain unmodified (33-52x43).
        // Copy their exact RGBA pixels to transparent 64x64 cell canvases
        // in memory at load time, never distort/stretch the original sprite.
        std::array<uint8_t,64u*64u*4u> cell{};
        const unsigned left=(slot>=RifleH && image.width>44u)?
                            10u:(64u-image.width)/2u;
        const unsigned top=(64u-image.height)/2u;
        if(left+image.width>64u||top+image.height>64u)return false;
        for(unsigned y=0;y<image.height;++y){
            std::memcpy(cell.data()+((top+y)*64u+left)*4u,
                image.rgba.data()+size_t(y)*image.width*4u,
                size_t(image.width)*4u);
        }
        glGenTextures(1,&textures_[slot]);
        if(!textures_[slot])return false;
        glBindTexture(GL_TEXTURE_2D,textures_[slot]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,
                     GL_UNSIGNED_BYTE,cell.data());
        if(glGetError()!=GL_NO_ERROR) {
            glDeleteTextures(1,&textures_[slot]);
            textures_[slot]=0;return false;
        }
        return true;
    }
};
}} // namespace reverie::omega_art
#endif
