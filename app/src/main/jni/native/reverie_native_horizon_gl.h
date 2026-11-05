#ifndef REVERIE_NATIVE_HORIZON_GL_H
#define REVERIE_NATIVE_HORIZON_GL_H
// Optional shared 360 atmosphere, backed by four exact-allowlisted PNG assets.
// No eye-side image decoding, mesh baking, dynamic network access, or gameplay
// collision. The full globe is for fictional Breakwater theaters; sky-only is
// for geography-backed Sitka, where generated mountains are NOT survey data.
#include "reverie_native_horizon_geometry.h"
#include "omega_front_png.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include <GLES2/gl2.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace reverie { namespace native { namespace horizon {
constexpr unsigned kTheaters=4u;
constexpr uint32_t kMaxBytes=32768u;
constexpr const char* kPaths[kTheaters]={
    "native/shared/v1/horizons/shingle_bay_skybox_512x256.png",
    "native/shared/v1/horizons/iron_quay_skybox_512x256.png",
    "native/shared/v1/horizons/blackcap_island_skybox_512x256.png",
    "native/shared/v1/horizons/stormwatch_point_skybox_512x256.png"
};

inline bool Load(const ReverieNativeHostV1* host,unsigned theater,
                 omega_art::Image& result) {
    result=omega_art::Image{};
    if(!host||theater>=kTheaters||
       host->struct_size<REVERIE_NATIVE_HOST_V1_ASSET_MIN_SIZE||
       !host->read_asset)return false;
    uint32_t size=0u;
    const char* path=kPaths[theater];
    const int32_t probe=host->read_asset(path,nullptr,0u,&size);
    if((probe!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL &&
        probe!=REVERIE_NATIVE_SAVE_OK)||size<45u||size>kMaxBytes)
        return false;
    std::vector<uint8_t> data(size);
    uint32_t received=0u;
    if(host->read_asset(path,data.data(),size,&received)!=
       REVERIE_NATIVE_SAVE_OK||received!=size)return false;
    if(!omega_art::DecodePng(data.data(),data.size(),result)||
       result.width!=512u||result.height!=256u||
       result.rgba.size()!=512u*256u*4u){result={};return false;}
    for(size_t k=3;k<result.rgba.size();k+=4u){
        if(result.rgba[k]!=255u){result={};return false;}
    }
    return true;
}

class Renderer {
public:
    Renderer()=default;
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    bool ready(unsigned id)const{return id<kTheaters && program_!=0u &&
         vbo_!=0u && textures_[id]!=0u && matrix_>=0 && sampler_>=0 && tint_>=0;}
    unsigned texture_count()const{
        unsigned n=0;for(GLuint t:textures_)n+=t!=0u;return n;
    }
    bool Create(const std::array<omega_art::Image,kTheaters>& images,
                std::string* error=nullptr){
        Release();
        unsigned valid=0u;
        for(const auto& image:images)
            if(image.width==512u&&image.height==256u&&
               image.rgba.size()==512u*256u*4u)++valid;
        if(!valid)return false;
        ReverieNativeGlStateV1 prior{};
        ReverieNativeGlStateCaptureV1(&prior);
        const char* vs=
            "attribute vec3 a_Position;attribute vec2 a_Uv;"
            "uniform mat4 u_Mvp;varying vec2 v_Uv;"
            "void main(){v_Uv=a_Uv;gl_Position=u_Mvp*vec4(a_Position,1.0);"
            "gl_Position.z=gl_Position.w*0.9999;}";
        const char* fs=
            "precision mediump float;uniform sampler2D u_Texture;"
            "uniform vec3 u_Tint;varying vec2 v_Uv;"
            "void main(){gl_FragColor=vec4(texture2D(u_Texture,v_Uv).rgb*u_Tint,1.0);}";
        const ReverieNativeGlAttributeBindingV1 bindings[]={
            {0u,"a_Position"},{1u,"a_Uv"}};
        program_=ReverieNativeBuildProgram(vs,fs,bindings,2u);
        if(program_){
            matrix_=glGetUniformLocation(program_,"u_Mvp");
            sampler_=glGetUniformLocation(program_,"u_Texture");
            tint_=glGetUniformLocation(program_,"u_Tint");
        }
        if(!program_||matrix_<0||sampler_<0||tint_<0){
            if(error)*error="Horizon GLES2 shader initialization failed";
            Release();ReverieNativeGlStateRestoreV1(&prior);return false;
        }
        std::vector<Vertex> mesh;
        Sphere(mesh);
        glGenBuffers(1,&vbo_);
        glBindBuffer(GL_ARRAY_BUFFER,vbo_);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(
            mesh.size()*sizeof(Vertex)),mesh.data(),GL_STATIC_DRAW);
        for(unsigned i=0;i<kTheaters;++i){
            const auto& im=images[i];
            if(im.width!=512u||im.height!=256u||
               im.rgba.size()!=512u*256u*4u)continue;
            glGenTextures(1,&textures_[i]);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,textures_[i]);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,512,256,0,
                         GL_RGBA,GL_UNSIGNED_BYTE,im.rgba.data());
        }
        const GLenum gl_error=glGetError();
        if(gl_error!=GL_NO_ERROR||vbo_==0u){
            if(error)*error="Horizon GLES2 texture/VBO upload failed";
            Release();ReverieNativeGlStateRestoreV1(&prior);return false;
        }
        ReverieNativeGlStateRestoreV1(&prior);
        return true;
    }
    // Draw before all actual terrain, waves, boats, buildings and HUD.
    // Sky is camera-rotation-only and is forced to far depth; no hard 30 m
    // clipping or headset stereo parallax of an artificial nearby wall.
    bool RenderEye(const ReverieNativeEyeV1* eye,unsigned id,bool night,
                   bool sky_only=false)const{
        if(!ready(id))return true; // optional shared art; host still works offline
        if(!ReverieNativeEyeRenderableV1(eye))return false;
        ReverieNativeGlStateV1 prior{};
        ReverieNativeGlAttribStateV1 prior_pos{},prior_uv{};
        ReverieNativeGlStateCaptureV1(&prior);
        ReverieNativeGlAttribCaptureV1(&prior_pos,0u);
        ReverieNativeGlAttribCaptureV1(&prior_uv,1u);
        GLboolean depth_write=GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK,&depth_write);
        float rotation[16],mvp[16];
        std::memcpy(rotation,eye->view,16*sizeof(float));
        rotation[12]=rotation[13]=rotation[14]=0.0f;
        ReverieNativeMat4Multiply(mvp,eye->projection,rotation);
        glDepthMask(GL_FALSE);glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
        glUseProgram(program_);
        glUniformMatrix4fv(matrix_,1,GL_FALSE,mvp);
        glUniform1i(sampler_,0);
        glUniform3f(tint_,night?.43f:1.f,night?.51f:1.f,night?.71f:1.f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,textures_[id]);
        glBindBuffer(GL_ARRAY_BUFFER,vbo_);
        glEnableVertexAttribArray(0u);
        glEnableVertexAttribArray(1u);
        glVertexAttribPointer(0u,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                              reinterpret_cast<void*>(0));
        glVertexAttribPointer(1u,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                              reinterpret_cast<void*>(3*sizeof(float)));
        glDrawArrays(GL_TRIANGLES,0,sky_only?kUpperVertices:kFullVertices);
        const GLenum result=glGetError();
        glDepthMask(depth_write);
        ReverieNativeGlAttribRestoreV1(&prior_uv);
        ReverieNativeGlAttribRestoreV1(&prior_pos);
        ReverieNativeGlStateRestoreV1(&prior);
        return result==GL_NO_ERROR;
    }
    void Release(){
        for(GLuint& tex:textures_){if(tex)glDeleteTextures(1,&tex);tex=0u;}
        if(vbo_)glDeleteBuffers(1,&vbo_);
        if(program_)glDeleteProgram(program_);
        vbo_=program_=0u;matrix_=sampler_=tint_=-1;
    }
private:
    GLuint program_=0u,vbo_=0u;
    std::array<GLuint,kTheaters> textures_{};
    GLint matrix_=-1,sampler_=-1,tint_=-1;
};
}}} // namespace reverie::native::horizon
#endif
