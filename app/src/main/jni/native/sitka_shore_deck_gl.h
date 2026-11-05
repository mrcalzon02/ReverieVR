#ifndef REVERIE_SITKA_SHORE_DECK_GL_H
#define REVERIE_SITKA_SHORE_DECK_GL_H

// One source-anchored shoreline texture and ONE persistent VBO. Each harbor
// float has a small draw range; scene LOD chooses ranges, not GPU reuploads.
// The texture lives once in the existing shared coastal catalog and is
// decoded on module Create, never in simulation or either stereo-eye draw.
#include "sitka_shore_deck_planes.h"
#include "omega_front_png.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include <GLES2/gl2.h>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace reverie { namespace sitka {

inline bool LoadShoreDeckArt(const ReverieNativeHostV1* host,
                             omega_art::Image& image){
    if(!host || !host->read_asset)return false;
    constexpr const char* path=
        "native/shared/v1/materials/coastal/dock_planks.png";
    constexpr uint32_t limit=16384u;
    uint32_t size=0u;
    const int32_t probe=host->read_asset(path,nullptr,0u,&size);
    if((probe!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL &&
        probe!=REVERIE_NATIVE_SAVE_OK)||size<45u||size>limit)return false;
    std::vector<uint8_t> bytes(size);
    uint32_t received=0u;
    if(host->read_asset(path,bytes.data(),size,&received)!=
       REVERIE_NATIVE_SAVE_OK || received!=size)return false;
    omega_art::Image decoded;
    if(!omega_art::DecodePng(bytes.data(),bytes.size(),decoded)||
       decoded.width!=64u||decoded.height!=64u||
       decoded.rgba.size()!=64u*64u*4u)return false;
    for(size_t a=3u;a<decoded.rgba.size();a+=4u)
        if(decoded.rgba[a]!=255u)return false;
    image=std::move(decoded);
    return true;
}

class ShoreDeckGl {
public:
    ShoreDeckGl()=default;
    ShoreDeckGl(const ShoreDeckGl&)=delete;
    ShoreDeckGl& operator=(const ShoreDeckGl&)=delete;
    bool ready()const{return program_!=0u&&texture_!=0u&&buffer_!=0u;}
    size_t upload_count()const{return uploads_;}
    size_t float_count()const{return active_float_count_;}
    size_t triangles()const{return vertex_count_/3u;}

    bool Create(const ShoreDeckMesh& mesh,const omega_art::Image& image,
                float ambient,std::string* error=nullptr){
        if(mesh.vertices.empty()||mesh.by_feature.size()>kMaxShoreFeatures||
           mesh.vertices.size()>kMaxShoreDeckVertices||
           mesh.vertices.size()%3u!=0u ||
           !std::isfinite(ambient)||ambient<.5f||ambient>1.5f ||
           image.width!=64u||image.height!=64u||
           image.rgba.size()!=64u*64u*4u){
            if(error)*error="Invalid Sitka waterfront deck texture/mesh";
            return false;
        }
        for(size_t a=3u;a<image.rgba.size();a+=4u)
            if(image.rgba[a]!=255u){
                if(error)*error="Nonopaque dock texture";
                return false;
            }
        size_t cursor=0u;
        size_t active=0u;
        for(const auto& range:mesh.by_feature){
            if(range.first!=cursor||range.count%3u!=0u||
               range.count>mesh.vertices.size()-cursor){
                if(error)*error="Invalid per-float draw range";
                return false;
            }
            cursor+=range.count;
            active+=(range.count!=0u);
        }
        if(cursor!=mesh.vertices.size()){
            if(error)*error="Unaccounted harbor deck triangles";
            return false;
        }
        for(const auto& v:mesh.vertices)
            if(!std::isfinite(v.x)||!std::isfinite(v.y)||
               !std::isfinite(v.z)||!std::isfinite(v.u)||
               !std::isfinite(v.v)){
                if(error)*error="Invalid deck texture vertex";
                return false;
            }
        Release();
        ReverieNativeGlStateV1 old{};
        ReverieNativeGlStateCaptureV1(&old);
        GLint elements=0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&elements);
        const auto restore=[&](){
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLuint>(elements));
            ReverieNativeGlStateRestoreV1(&old);
        };
        static const char* vs=
            "attribute vec3 a_Position;\n"
            "attribute vec2 a_UV;\n"
            "uniform mat4 u_ViewProjection;\n"
            "varying vec2 v_UV;\n"
            "void main(){v_UV=a_UV;"
            "gl_Position=u_ViewProjection*vec4(a_Position,1.0);}\n";
        static const char* fs=
            "precision mediump float;\n"
            "varying vec2 v_UV;\n"
            "uniform sampler2D u_Texture;\n"
            "uniform float u_Ambient;\n"
            "void main(){vec4 tex=texture2D(u_Texture,v_UV);"
            "gl_FragColor=vec4(min(tex.rgb*u_Ambient,vec3(1.)),1.);}\n";
        const ReverieNativeGlAttributeBindingV1 attrs[]={
            {0u,"a_Position"},{1u,"a_UV"}};
        char diagnostic[256]={};
        program_=ReverieNativeBuildProgramDiagnosticV1(
            vs,fs,attrs,2u,diagnostic,sizeof(diagnostic));
        if(!program_){
            if(error)*error=diagnostic;
            restore();return false;
        }
        matrix_=glGetUniformLocation(program_,"u_ViewProjection");
        ambient_=glGetUniformLocation(program_,"u_Ambient");
        sampler_=glGetUniformLocation(program_,"u_Texture");
        if(matrix_<0||ambient_<0||sampler_<0){
            if(error)*error="Sitka deck shader uniforms unavailable";
            Release();restore();return false;
        }
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1,&texture_);
        glGenBuffers(1,&buffer_);
        if(!texture_||!buffer_){
            if(error)*error="Sitka deck GPU allocation failed";
            Release();restore();return false;
        }
        glBindTexture(GL_TEXTURE_2D,texture_);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,
                     GL_RGBA,GL_UNSIGNED_BYTE,image.rgba.data());
        glBindBuffer(GL_ARRAY_BUFFER,buffer_);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mesh.vertices.size()*sizeof(ShoreDeckVertex)),
            mesh.vertices.data(),GL_STATIC_DRAW);
        if(glGetError()!=GL_NO_ERROR){
            if(error)*error="Sitka deck texture/VBO upload failed";
            Release();restore();return false;
        }
        ranges_=mesh.by_feature;
        vertex_count_=mesh.vertices.size();
        active_float_count_=active;
        uploads_=2u; // exactly one GPU texture and one GPU vertex buffer
        ambient_scale_=ambient;
        restore();
        return ready();
    }

    template<class Visible>
    bool RenderEye(const ReverieNativeEyeV1* eye,Visible visible)const{
        if(!ready()||!ReverieNativeEyeRenderableV1(eye))return false;
        ReverieNativeGlStateV1 old{};
        ReverieNativeGlAttribStateV1 pos{},uv{};
        ReverieNativeGlStateCaptureV1(&old);
        ReverieNativeGlAttribCaptureV1(&pos,0u);
        ReverieNativeGlAttribCaptureV1(&uv,1u);
        GLint elements=0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&elements);
        float vp[16]={};
        ReverieNativeMat4Multiply(vp,eye->projection,eye->view);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glUseProgram(program_);
        glUniformMatrix4fv(matrix_,1,GL_FALSE,vp);
        glUniform1f(ambient_,ambient_scale_);
        glUniform1i(sampler_,0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,texture_);
        glBindBuffer(GL_ARRAY_BUFFER,buffer_);
        glEnableVertexAttribArray(0u);
        glEnableVertexAttribArray(1u);
        glVertexAttribPointer(0u,3,GL_FLOAT,GL_FALSE,
                              sizeof(ShoreDeckVertex),
                              reinterpret_cast<const void*>(0));
        glVertexAttribPointer(1u,2,GL_FLOAT,GL_FALSE,
                              sizeof(ShoreDeckVertex),
                              reinterpret_cast<const void*>(12));
        for(size_t i=0u;i<ranges_.size();++i){
            const auto& region=ranges_[i];
            if(region.count!=0u && visible(i))
                glDrawArrays(GL_TRIANGLES,static_cast<GLint>(region.first),
                             static_cast<GLsizei>(region.count));
        }
        ReverieNativeGlAttribRestoreV1(&uv);
        ReverieNativeGlAttribRestoreV1(&pos);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLuint>(elements));
        ReverieNativeGlStateRestoreV1(&old);
        return glGetError()==GL_NO_ERROR;
    }
    void Release(){
        if(texture_)glDeleteTextures(1,&texture_);
        if(buffer_)glDeleteBuffers(1,&buffer_);
        if(program_)glDeleteProgram(program_);
        texture_=buffer_=program_=0u;
        matrix_=ambient_=sampler_=-1;
        ranges_.clear();
        vertex_count_=active_float_count_=uploads_=0u;
        ambient_scale_=1.f;
    }
private:
    GLuint program_=0u,texture_=0u,buffer_=0u;
    GLint matrix_=-1,ambient_=-1,sampler_=-1;
    std::vector<ShoreDeckRange> ranges_;
    float ambient_scale_=1.f;
    size_t vertex_count_=0u,active_float_count_=0u,uploads_=0u;
};

}} // namespace reverie::sitka
#endif
