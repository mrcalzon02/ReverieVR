#ifndef FEATURED_AUDIT_HOST_H
#define FEATURED_AUDIT_HOST_H
#include "reverie_native_module.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace audit {
inline std::map<std::string,std::vector<uint8_t>> saves;
inline std::set<std::string> missing_assets;
inline unsigned reads=0,writes=0,successful_writes=0,feedback=0,sounds=0;
inline bool reject_writes=false;
inline std::string output;
inline void Check(bool value,const char* detail){if(!value)throw std::runtime_error(detail);}
inline void Log(int32_t level,const char* tag,const char* message){std::printf("MODULE_LOG %d %s %s\n",level,tag,message);}
inline int32_t Read(const char* slot,void* buffer,uint32_t capacity,uint32_t* size){
    ++reads;auto it=saves.find(slot);if(it==saves.end())return REVERIE_NATIVE_SAVE_NOT_FOUND;
    *size=static_cast<uint32_t>(it->second.size());
    if(!buffer||capacity<*size)return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    std::memcpy(buffer,it->second.data(),*size);return REVERIE_NATIVE_SAVE_OK;
}
inline int32_t Write(const char* slot,const void* bytes,uint32_t size){
    ++writes;
    if(reject_writes){std::printf("FAULT_INJECTED write_save %s bytes=%u attempt=%u\n",slot,size,writes);return REVERIE_NATIVE_SAVE_ERROR;}
    auto* data=static_cast<const uint8_t*>(bytes);saves[slot]=std::vector<uint8_t>(data,data+size);++successful_writes;
    std::printf("WRITE_OK %s bytes=%u attempt=%u\n",slot,size,writes);return REVERIE_NATIVE_SAVE_OK;
}
inline int32_t Asset(const char* path,void* buffer,uint32_t capacity,uint32_t* size){
    std::ifstream f(std::string("app/src/main/assets/")+path,std::ios::binary|std::ios::ate);
    if(!f){missing_assets.insert(path);return REVERIE_NATIVE_SAVE_NOT_FOUND;}
    *size=static_cast<uint32_t>(f.tellg());
    if(!buffer||capacity<*size)return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    f.seekg(0);f.read(static_cast<char*>(buffer),*size);
    return f?REVERIE_NATIVE_SAVE_OK:REVERIE_NATIVE_SAVE_ERROR;
}
inline ReverieNativeHostV1 Host(){
    ReverieNativeHostV1 host{};host.struct_size=sizeof(host);host.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;
    host.log=Log;host.read_save=Read;host.write_save=Write;host.read_asset=Asset;
    host.request_feedback=[](uint32_t flags){feedback|=flags;};
    host.request_sound=[](uint32_t){++sounds;};return host;
}
inline ReverieNativeInputV1 Input(){ReverieNativeInputV1 input{};input.struct_size=sizeof(input);input.delta_seconds=.05f;return input;}
inline ReverieNativeInputV1 Pointer(float x,float y,float z){
    auto input=Input();input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    const float length=std::sqrt(x*x+y*y+z*z);
    input.pointer_direction[0]=x/length;input.pointer_direction[1]=y/length;input.pointer_direction[2]=z/length;return input;
}
class GL {
public:
    static constexpr int width=960,height=720;
    EGLDisplay display=EGL_NO_DISPLAY;EGLConfig config{};EGLSurface surface=EGL_NO_SURFACE;EGLContext context=EGL_NO_CONTEXT;
    GL(){
        display=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint major=0,minor=0;
        Check(eglInitialize(display,&major,&minor),"EGL initialize");Check(eglBindAPI(EGL_OPENGL_ES_API),"EGL ES bind");
        const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};
        EGLint count=0;Check(eglChooseConfig(display,attributes,&config,1,&count)&&count==1,"EGL config");
        const EGLint surface_attributes[]={EGL_WIDTH,width,EGL_HEIGHT,height,EGL_NONE};
        surface=eglCreatePbufferSurface(display,config,surface_attributes);Check(surface!=EGL_NO_SURFACE,"EGL surface");NewContext();
        std::printf("REAL_DRIVER %s | %s\n",glGetString(GL_VERSION),glGetString(GL_RENDERER));
    }
    void NewContext(){
        const EGLint attributes[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
        context=eglCreateContext(display,config,EGL_NO_CONTEXT,attributes);Check(context!=EGL_NO_CONTEXT,"EGL context");
        Check(eglMakeCurrent(display,surface,surface,context),"EGL make current");glViewport(0,0,width,height);
    }
    void Recreate(){Check(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT),"EGL unbind");Check(eglDestroyContext(display,context),"EGL destroy");context=EGL_NO_CONTEXT;NewContext();}
    ~GL(){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);}
};
inline void Identity(float* data){std::fill(data,data+16,0.f);data[0]=data[5]=data[10]=data[15]=1.f;}
inline ReverieNativeEyeV1 Eye(int side,float scale=1.2f,float pitch=0.f){
    ReverieNativeEyeV1 eye{};eye.struct_size=sizeof(eye);eye.eye_index=side;
    Identity(eye.view);eye.view[12]=side==0?.032f:-.032f;
    eye.view[5]=std::cos(pitch);eye.view[9]=std::sin(pitch);eye.view[6]=-std::sin(pitch);eye.view[10]=std::cos(pitch);
    const float near=.08f,far=120.f;
    eye.projection[0]=scale*GL::height/GL::width;eye.projection[5]=scale;
    eye.projection[10]=-(far+near)/(far-near);eye.projection[11]=-1.f;eye.projection[14]=-2.f*far*near/(far-near);return eye;
}
template<class Renderer> std::vector<uint64_t> Capture(Renderer draw,void* state,const std::string& name,float scale=1.2f,float pitch=0.f){
    std::vector<uint64_t> hashes;
    for(int side=0;side<2;++side){
        auto eye=Eye(side,scale,pitch);glViewport(0,0,GL::width,GL::height);
        glClearColor(.012f,.018f,.025f,1.f);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        Check(draw(state,&eye)==1,"actual module eye rendering");glFinish();Check(glGetError()==GL_NO_ERROR,"actual GL error");
        std::vector<uint8_t> pixels(GL::width*GL::height*4);glPixelStorei(GL_PACK_ALIGNMENT,1);
        glReadPixels(0,0,GL::width,GL::height,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());Check(glGetError()==GL_NO_ERROR,"framebuffer readback");
        uint64_t hash=1469598103934665603ull;size_t bright=0;
        for(size_t i=0;i<pixels.size();++i)hash=(hash^pixels[i])*1099511628211ull;
        for(size_t i=0;i<pixels.size();i+=4)if(pixels[i]>80||pixels[i+1]>80||pixels[i+2]>80)++bright;
        Check(bright>100,"rendered frame empty");hashes.push_back(hash);
        const std::string path=output+"/"+name+(side==0?"-left.ppm":"-right.ppm");
        std::ofstream file(path,std::ios::binary);file<<"P6\n"<<GL::width<<" "<<GL::height<<"\n255\n";
        for(int y=GL::height-1;y>=0;--y)for(int x=0;x<GL::width;++x)file.write(reinterpret_cast<const char*>(pixels.data()+(y*GL::width+x)*4),3);
        Check(bool(file),"capture write");std::printf("CAPTURE %s eye=%d hash=%llu visible_pixels=%zu\n",name.c_str(),side,static_cast<unsigned long long>(hash),bright);
    }
    return hashes;
}
inline void MissingAssets(){std::printf("MISSING_ASSETS %zu\n",missing_assets.size());for(const auto& path:missing_assets)std::printf("MISSING_ASSET %s\n",path.c_str());}
}
#endif
