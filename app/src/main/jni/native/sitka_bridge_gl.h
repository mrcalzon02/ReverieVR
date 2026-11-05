#ifndef REVERIE_SITKA_BRIDGE_GL_H
#define REVERIE_SITKA_BRIDGE_GL_H

// Game-private textured GLES2 adapter. Exactly three material draws (when
// present); immutable vertex/texture uploads once per GL context, not per eye.
// The side is ONE transparent image on each of TWO quads, not modeled stays.
#include "sitka_bridge_planes.h"
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

struct BridgeArt {
    std::array<omega_art::Image,3> textures;
    bool Valid() const {
        for(size_t i=0;i<textures.size();++i){
            const auto& im=textures[i];
            const bool is_side=i==0u;
            if(im.width!=(is_side?512u:64u) ||
               im.height!=(is_side?128u:64u) ||
               im.rgba.size()!=size_t(im.width)*im.height*4u)
                return false;
            if(is_side){
                bool cutout=false,solid=false;
                for(size_t j=3u;j<im.rgba.size();j+=4u){
                    cutout|=im.rgba[j]<128u;
                    solid|=im.rgba[j]>=128u;
                }
                if(!cutout||!solid)return false;
            }else{
                for(size_t j=3u;j<im.rgba.size();j+=4u)
                    if(im.rgba[j]!=255u)return false;
            }
        }
        return true;
    }
};

// Tiny source-informed PALETTE texture, not a geographic survey/photograph.
// It reproduces the recognizable two tower stations and parallel cable stay
// silhouette without adding a single 3D cable/truss/rivet. This is a fallback
// authoring approximation; an approved PNG can supersede it unchanged.
inline omega_art::Image OriginalBridgeSideSprite(){
    omega_art::Image im;
    im.width=512u;im.height=128u;
    im.rgba.assign(512u*128u*4u,0u);
    const auto pixel=[&](int x,int y,uint8_t r,uint8_t g,uint8_t b){
        if(x<0||y<0||x>=512||y>=128)return;
        const size_t at=(size_t(y)*512u+size_t(x))*4u;
        im.rgba[at]=r;im.rgba[at+1u]=g;im.rgba[at+2u]=b;
        im.rgba[at+3u]=255u;
    };
    const auto line=[&](int x0,int y0,int x1,int y1,
                        uint8_t r,uint8_t g,uint8_t b){
        int dx=std::abs(x1-x0),dy=-std::abs(y1-y0);
        const int sx=x0<x1?1:-1,sy=y0<y1?1:-1;
        int err=dx+dy;
        while(true){
            pixel(x0,y0,r,g,b);
            if(x0==x1&&y0==y1)break;
            const int twice=2*err;
            if(twice>=dy){err+=dy;x0+=sx;}
            if(twice<=dx){err+=dx;y0+=sy;}
        }
    };
    const auto deck=[](int x){
        const float f=float(x)/511.f;
        return 112-static_cast<int>(15.f*std::sin(3.14159265f*f));
    };
    for(int x=0;x<512;++x){
        const int y=deck(x);
        for(int k=0;k<6;++k)pixel(x,y+k,96,119,128);
        pixel(x,y,176,196,201);
    }
    // Approximate two longitudinal stations: main span 450 / total 1255 ft.
    // The TWIN cross-deck pylons collapse to one pole per side elevation.
    for(const int tower:{164,347}){
        for(const int sign:{-1,1})
            for(int i=0;i<3;++i){
                const int tip=tower+sign*(85+14*i);
                line(tower,19+3*i,tip,deck(tip)-1,
                     145,170,181);
                line(tower+1,19+3*i,tip+1,deck(tip)-1,
                     192,211,215);
            }
        for(int x=tower-3;x<=tower+3;++x)
            for(int y=10;y<=deck(tower);++y)
                pixel(x,y,x<tower?191:134,x<tower?209:161,
                      x<tower?216:176);
    }
    return im;
}

inline bool LoadBridgeArt(const ReverieNativeHostV1* host,BridgeArt& art){
    if(!host || !host->read_asset)return false;
    BridgeArt staged;
    for(size_t i=0u;i<3u;++i){
        const auto m=static_cast<BridgePlaneMaterial>(i);
        const char* path=BridgePlaneTexturePath(m);
        constexpr uint32_t kSideMax=131072u,kConcreteMax=16384u;
        const uint32_t cap=i==0u?kSideMax:kConcreteMax;
        uint32_t size=0u;
        const int32_t status=host->read_asset(path,nullptr,0u,&size);
        if(i==0u && status==REVERIE_NATIVE_SAVE_NOT_FOUND){
            staged.textures[i]=OriginalBridgeSideSprite();
            continue;
        }
        if((status!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL &&
            status!=REVERIE_NATIVE_SAVE_OK) || size<45u || size>cap)return false;
        std::vector<uint8_t> bytes(size);
        uint32_t read=0u;
        if(host->read_asset(path,bytes.data(),size,&read)!=
           REVERIE_NATIVE_SAVE_OK || read!=size)return false;
        if(!omega_art::DecodePng(bytes.data(),bytes.size(),
                                 staged.textures[i]))return false;
    }
    if(!staged.Valid())return false;
    art=std::move(staged);
    return true;
}

class BridgeGl {
public:
    BridgeGl()=default;
    BridgeGl(const BridgeGl&)=delete;
    BridgeGl& operator=(const BridgeGl&)=delete;
    bool ready()const{return program_!=0u && buffers_[0]!=0u &&
        buffers_[1]!=0u&&buffers_[2]!=0u&&
        textures_[0]!=0u&&textures_[1]!=0u&&textures_[2]!=0u;}
    size_t quad_count()const{return quads_;}
    size_t upload_count()const{return uploads_;}
    size_t draw_calls_per_eye()const{
        size_t n=0u;for(size_t count:counts_)n+=(count>0u);
        return n;
    }

    bool Create(const BridgePlaneMesh& mesh,const BridgeArt& art,
                float ambient,std::string* error=nullptr){
        if(!art.Valid() || mesh.planes.empty()||
           mesh.planes.size()>2u+2u*kMaxBridgeGradeSegments ||
           !std::isfinite(ambient) || ambient<.5f || ambient>1.5f){
            if(error)*error="Invalid bridge art, geometry or lighting";
            return false;
        }
        // Every visible panel must have exactly one approved material ID.
        std::array<std::vector<BridgePlaneVertex>,3> vertices;
        for(const auto& quad:mesh.planes){
            const size_t index=static_cast<size_t>(quad.material);
            if(index>=3u){if(error)*error="Unknown bridge material";return false;}
            for(const auto& v:quad.vertex)
                if(!std::isfinite(v.x)||!std::isfinite(v.y)||
                   !std::isfinite(v.z)||!std::isfinite(v.u)||
                   !std::isfinite(v.v)){
                    if(error)*error="Bridge vertex is nonfinite";
                    return false;
                }
            auto& dest=vertices[index];
            for(unsigned k:{0u,1u,2u,0u,2u,3u})
                dest.push_back(quad.vertex[k]);
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
        static const char *vs=
            "attribute vec3 a_Position;\n"
            "attribute vec2 a_UV;\n"
            "uniform mat4 u_ViewProjection;\n"
            "varying vec2 v_UV;\n"
            "void main(){v_UV=a_UV;"
            "gl_Position=u_ViewProjection*vec4(a_Position,1.0);}\n";
        static const char *fs=
            "precision mediump float;\n"
            "varying vec2 v_UV;\n"
            "uniform sampler2D u_Texture;\n"
            "uniform float u_Ambient;\n"
            "void main(){vec4 t=texture2D(u_Texture,v_UV);"
            "if(t.a<0.5)discard;"
            "gl_FragColor=vec4(min(t.rgb*u_Ambient,vec3(1.0)),1.0);}\n";
        const ReverieNativeGlAttributeBindingV1 attrs[]={
            {0u,"a_Position"},{1u,"a_UV"}};
        char detail[256]={};
        program_=ReverieNativeBuildProgramDiagnosticV1(
            vs,fs,attrs,2u,detail,sizeof(detail));
        if(program_==0u){
            if(error)*error=detail;
            restore();return false;
        }
        matrix_=glGetUniformLocation(program_,"u_ViewProjection");
        sampler_=glGetUniformLocation(program_,"u_Texture");
        ambient_=glGetUniformLocation(program_,"u_Ambient");
        if(matrix_<0||sampler_<0||ambient_<0){
            if(error)*error="Sitka bridge shader uniforms unavailable";
            Release();restore();return false;
        }
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(3,textures_.data());
        glGenBuffers(3,buffers_.data());
        for(size_t i=0u;i<3u;++i){
            if(!textures_[i]||!buffers_[i]){
                if(error)*error="Bridge static GPU resource allocation failed";
                Release();restore();return false;
            }
            const auto& image=art.textures[i];
            glBindTexture(GL_TEXTURE_2D,textures_[i]);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,
                            i==0u?GL_CLAMP_TO_EDGE:GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,
                            i==0u?GL_CLAMP_TO_EDGE:GL_REPEAT);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,
                         static_cast<GLsizei>(image.width),
                         static_cast<GLsizei>(image.height),0,GL_RGBA,
                         GL_UNSIGNED_BYTE,image.rgba.data());
            glBindBuffer(GL_ARRAY_BUFFER,buffers_[i]);
            glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(vertices[i].size()*
                             sizeof(BridgePlaneVertex)),
                         vertices[i].data(),GL_STATIC_DRAW);
            counts_[i]=vertices[i].size();
            uploads_+=2u;
            if(glGetError()!=GL_NO_ERROR){
                if(error)*error="Bridge GLES2 texture/vertex upload failed";
                Release();restore();return false;
            }
        }
        ambient_scale_=ambient;
        quads_=mesh.planes.size();
        restore();
        return ready();
    }

    bool RenderEye(const ReverieNativeEyeV1* eye)const{
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
        glDisable(GL_CULL_FACE); // double-sided billboard; no duplicate planes
        glDisable(GL_BLEND);     // alpha cutout, depth-write-safe opaque pixels
        glEnable(GL_DEPTH_TEST);
        glUseProgram(program_);
        glUniformMatrix4fv(matrix_,1,GL_FALSE,vp);
        glUniform1i(sampler_,0);
        glUniform1f(ambient_,ambient_scale_);
        glActiveTexture(GL_TEXTURE0);
        glEnableVertexAttribArray(0u);
        glEnableVertexAttribArray(1u);
        for(size_t i=0u;i<3u;++i){
            if(counts_[i]==0u)continue;
            glBindTexture(GL_TEXTURE_2D,textures_[i]);
            glBindBuffer(GL_ARRAY_BUFFER,buffers_[i]);
            glVertexAttribPointer(0u,3,GL_FLOAT,GL_FALSE,
                                  sizeof(BridgePlaneVertex),
                                  reinterpret_cast<const void*>(0));
            glVertexAttribPointer(1u,2,GL_FLOAT,GL_FALSE,
                                  sizeof(BridgePlaneVertex),
                                  reinterpret_cast<const void*>(12));
            glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(counts_[i]));
        }
        ReverieNativeGlAttribRestoreV1(&uv);
        ReverieNativeGlAttribRestoreV1(&pos);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,static_cast<GLuint>(elements));
        ReverieNativeGlStateRestoreV1(&old);
        return glGetError()==GL_NO_ERROR;
    }
    void Release(){
        glDeleteTextures(3,textures_.data());
        glDeleteBuffers(3,buffers_.data());
        if(program_)glDeleteProgram(program_);
        textures_.fill(0u);buffers_.fill(0u);counts_.fill(0u);
        program_=0u;matrix_=sampler_=ambient_=-1;
        uploads_=quads_=0u;ambient_scale_=1.f;
    }
private:
    GLuint program_=0u;
    std::array<GLuint,3> textures_{{0,0,0}},buffers_{{0,0,0}};
    std::array<size_t,3> counts_{{0,0,0}};
    GLint matrix_=-1,sampler_=-1,ambient_=-1;
    float ambient_scale_=1.f;
    size_t uploads_=0u,quads_=0u;
};
}} // namespace reverie::sitka
#endif
