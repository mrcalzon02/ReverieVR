#ifndef REVERIE_LANTERN_PHOTO_GL_H
#define REVERIE_LANTERN_PHOTO_GL_H
// Actual GLES2 stereo photo sampler. One immutable registered source plate
// per captured exposure; both eyes use the same GPU texture/crop, with zero
// asset reads and zero image regeneration inside render_eye.
#include "lantern_photo_capture_quality.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_gl_state.h"
#include <GLES2/gl2.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>
namespace reverie { namespace lantern { namespace photo_gl {
inline bool AcceptedPlate(const capture::GrayPlate& p){
    return p.Valid()&&p.width==p.height&&(p.width==512||p.width==1024);
}
constexpr float kImagePlaneZ=-2.901f; // distinct from old flat blocks at -2.92
struct Renderer {
    capture::GrayPlate current,previous;
    bool has_previous=false,dirty=true,initialized=false;
    std::array<std::vector<uint8_t>,2> rgb{};
    GLuint textures[2]{},program=0,quad=0;
    GLint pos=-1,mvp=-1,rect=-1,crop=-1,sampler=-1;
    bool active()const{return initialized&&textures[0]&&current.Valid();}
    void Set(capture::GrayPlate next,capture::GrayPlate before,bool paired,
             const std::array<uint16_t,16>& pins){
        if(!AcceptedPlate(next)){
            current={};previous={};has_previous=false;
            rgb[0].clear();rgb[1].clear();dirty=true;return;
        }
        current=std::move(next);
        previous=std::move(before);
        has_previous=paired&&AcceptedPlate(previous);
        Decorate(pins);
    }
    void Decorate(const std::array<uint16_t,16>& pins){
        if(!current.Valid()){dirty=true;return;}
        for(int slot=0;slot<(has_previous?2:1);++slot){
            const auto& photo=slot==0?current:previous;
            auto& dst=rgb[slot];
            dst.resize(photo.pixels.size()*3u);
            for(size_t i=0;i<photo.pixels.size();++i){
                const uint8_t value=photo.pixels[i];
                dst[i*3u]=dst[i*3u+1u]=dst[i*3u+2u]=value;
            }
        }
        if(!has_previous)rgb[1].clear();
        // World truth/photo pixels remain untouched: markers are rendered
        // into a derivative RGB print, shared by source and loupe.
        for(int sy=0;sy<16;++sy)for(int sx=0;sx<16;++sx){
            if((pins[sy]&(1u<<sx))==0u)continue;
            const int cell=current.width/16;
            const int cx=sx*cell+cell/2,cy=sy*cell+cell/2;
            const int extent=11*cell/32,stroke=std::max(1,2*cell/32);
            for(int dy=-extent;dy<=extent;++dy)for(int dx=-extent;dx<=extent;++dx){
                if(std::max(std::abs(dx),std::abs(dy))<extent-stroke)continue;
                const int x=cx+dx,y=cy+dy;
                if(x<0||y<0||x>=current.width||y>=current.height)continue;
                const size_t i=(static_cast<size_t>(y)*current.width+x)*3u;
                rgb[0][i]=248u;rgb[0][i+1u]=183u;rgb[0][i+2u]=43u;
            }
        }
        dirty=true;
    }
    bool Create(){
        Release();
        ReverieNativeGlStateV1 saved{};
        ReverieNativeGlStateCaptureV1(&saved);
        const char* vs=
            "uniform mat4 u_Mvp; uniform vec4 u_Rect; uniform vec4 u_Crop;"
            "attribute vec2 a_Corner; varying vec2 v_Uv;"
            "void main(){vec2 local=a_Corner+vec2(0.5);"
            "vec2 screen=u_Rect.xy+a_Corner*u_Rect.zw;"
            "v_Uv=u_Crop.xy+vec2(local.x,1.-local.y)*u_Crop.zw;"
            "gl_Position=u_Mvp*vec4(screen,-2.901,1.);}";
        const char* fs=
            "precision mediump float;uniform sampler2D u_Tex;"
            "varying vec2 v_Uv;void main(){gl_FragColor="
            "texture2D(u_Tex,v_Uv);}";
        const ReverieNativeGlAttributeBindingV1 bindings[]={{2u,"a_Corner"}};
        program=ReverieNativeBuildProgram(vs,fs,bindings,1u);
        if(program){
            pos=glGetAttribLocation(program,"a_Corner");
            mvp=glGetUniformLocation(program,"u_Mvp");
            rect=glGetUniformLocation(program,"u_Rect");
            crop=glGetUniformLocation(program,"u_Crop");
            sampler=glGetUniformLocation(program,"u_Tex");
        }
        bool ok=program&&pos==2&&mvp>=0&&rect>=0&&crop>=0&&sampler>=0;
        if(ok){
            constexpr GLfloat quadPoints[]={
                -.5f,-.5f,.5f,-.5f,-.5f,.5f,
                -.5f,.5f,.5f,-.5f,.5f,.5f};
            glGenBuffers(1,&quad);
            ok=quad!=0;
            if(ok){
                glBindBuffer(GL_ARRAY_BUFFER,quad);
                glBufferData(GL_ARRAY_BUFFER,sizeof(quadPoints),
                             quadPoints,GL_STATIC_DRAW);
            }
            if(ok)glGenTextures(2,textures);
            ok=ok&&textures[0]&&textures[1];
        }
        if(ok){
            for(GLuint texture:textures){
                glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            }
            ok=glGetError()==GL_NO_ERROR;
        }
        ReverieNativeGlStateRestoreV1(&saved);
        initialized=ok;
        if(!ok){Release();return false;}
        dirty=true;
        return Upload();
    }
    void Release(){
        if(textures[0]||textures[1])glDeleteTextures(2,textures);
        if(quad)glDeleteBuffers(1,&quad);
        if(program)glDeleteProgram(program);
        textures[0]=textures[1]=quad=program=0;
        pos=mvp=rect=crop=sampler=-1;
        initialized=false;dirty=true;
    }
    bool Upload(){
        if(!dirty||!initialized)return initialized;
        if(!current.Valid()){dirty=false;return true;}
        GLint maxTexture=0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxTexture);
        for(int i=0;i<(has_previous?2:1);++i){
            const auto& plate=i==0?current:previous;
            if(!AcceptedPlate(plate)||plate.width>maxTexture||
               rgb[i].size()!=plate.pixels.size()*3u)return false;
        }
        ReverieNativeGlStateV1 previousState{};
        ReverieNativeGlStateCaptureV1(&previousState);
        GLint alignment=4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glActiveTexture(GL_TEXTURE0);
        for(int i=0;i<(has_previous?2:1);++i){
            glBindTexture(GL_TEXTURE_2D,textures[i]);
            const auto& plate=i==0?current:previous;
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,plate.width,plate.height,0,
                         GL_RGB,GL_UNSIGNED_BYTE,rgb[i].data());
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);
        const bool good=glGetError()==GL_NO_ERROR;
        ReverieNativeGlStateRestoreV1(&previousState);
        if(good)dirty=false;
        return good;
    }
    // Need a current GLES context. This never allocates, uploads, mutates
    // simulation state or changes the stored photograph while either eye draws.
    bool Draw(const float* vp,int crop_x,int crop_y,bool show_loupe){
        if(!active()||dirty||!vp)return true; // legacy film fallback on error
        if(crop_x<0||crop_y<0||crop_x>8||crop_y>8)return false;
        ReverieNativeGlAttribStateV1 attr{};
        ReverieNativeGlAttribCaptureV1(&attr,2u);
        glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
        glUseProgram(program);glBindBuffer(GL_ARRAY_BUFFER,quad);
        glEnableVertexAttribArray(2u);
        glVertexAttribPointer(2u,2,GL_FLOAT,GL_FALSE,2*sizeof(float),
                              reinterpret_cast<const void*>(0));
        glUniformMatrix4fv(mvp,1,GL_FALSE,vp);glUniform1i(sampler,0);
        glActiveTexture(GL_TEXTURE0);
        const auto draw=[&](GLuint texture,float cx,float x,float y,float w,float h){
            glBindTexture(GL_TEXTURE_2D,texture);
            glUniform4f(rect,cx,.16f,1.328f,1.328f);
            glUniform4f(crop,x,y,w,h);
            glDrawArrays(GL_TRIANGLES,0,6);
        };
        draw(textures[0],0.f,0.f,0.f,1.f,1.f);
        if(show_loupe){
            draw(textures[0],1.59f,crop_x/16.f,crop_y/16.f,.5f,.5f);
            if(has_previous)
                draw(textures[1],-1.59f,crop_x/16.f,crop_y/16.f,.5f,.5f);
        }
        const bool good=glGetError()==GL_NO_ERROR;
        ReverieNativeGlAttribRestoreV1(&attr);
        return good;
    }
};
}}} // reverie::lantern::photo_gl
#endif
