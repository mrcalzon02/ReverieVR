// Actual module stereo rendering with synthetic terrain; no geographic claim.
#include "../../main/jni/native/sitka_terrain_module.cpp"
#include <EGL/egl.h>
#include <cassert>
#include <iostream>
using namespace reverie::sitka;
int main(){
    const EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major=0,minor=0;assert(eglInitialize(display,&major,&minor));
    assert(eglBindAPI(EGL_OPENGL_ES_API));
    const EGLint config_attribs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_RED_SIZE,8,
        EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,16,EGL_NONE};
    EGLConfig config{};EGLint count=0;
    assert(eglChooseConfig(display,config_attribs,&config,1,&count)&&count==1);
    const EGLint surface_attribs[]={EGL_WIDTH,512,EGL_HEIGHT,256,EGL_NONE};
    const auto surface=eglCreatePbufferSurface(display,config,surface_attribs);
    const EGLint context_attribs[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    const auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,context_attribs);
    assert(eglMakeCurrent(display,surface,surface,context));
    ReverieNativeHostV1 host{};host.struct_size=sizeof(host);
    host.write_save=[](const char*,const void*,uint32_t)->int32_t{
        return REVERIE_NATIVE_SAVE_OK;
    };
    State world;world.host=&host;
    world.terrain.vertices={{-20,0,-20,50,70,50},{20,0,-20,50,70,50},
        {20,0,20,50,70,50},{-20,0,20,50,70,50}};
    world.terrain.indices={0,1,2,0,2,3};
    Water water;water.id="fixture_water";water.shape="OBLONG";
    water.x=-10.f;water.width=4.f;water.depth=4.f;world.waters.push_back(water);
    world.walk_session.reset(new FreeRoamSession(world.terrain,world.waters,
        world.shore_colliders,true,0.,0.));
    world.harbor_sites.push_back({"crescent","fixture_desk",HarborLocationKind::ServiceDesk,1,0});
    world.fishing.fishing.journal=StarterJournal();
    assert(OnGlCreated(&world));
    ReverieNativeInputV1 in{};in.struct_size=sizeof(in);in.delta_seconds=.016f;
    in.listener_valid=1u;in.listener_position[1]=1.4f;in.listener_right[0]=1.f;
    in.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    in.pointer_origin[1]=1.25f;in.pointer_direction[0]=1.f;
    Update(&world,&in);
    auto render=[&]{
        const auto before=SaveJournal(world.fishing.fishing.journal);
        glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        for(int eye_index=0;eye_index<2;++eye_index){
            glViewport(eye_index*256,0,256,256);
            ReverieNativeEyeV1 eye{};eye.struct_size=sizeof(eye);eye.eye_index=eye_index;
            for(int axis=0;axis<4;++axis){eye.view[axis*5]=1.f;eye.projection[axis*5]=1.f;}
            eye.view[12]=eye_index==0?.03f:-.03f;eye.view[13]=-1.4f;
            eye.projection[0]=.5f;eye.projection[5]=.8f;eye.projection[10]=-.05f;
            assert(RenderEye(&world,&eye));
        }
        glFinish();assert(glGetError()==GL_NO_ERROR);
        std::vector<uint8_t> pixels(512u*256u*4u);
        glReadPixels(0,0,512,256,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        unsigned bright[2]={};
        for(unsigned y=0;y<256u;++y)for(unsigned x=0;x<512u;++x){
            const auto i=(y*512u+x)*4u;
            if(pixels[i]>150u&&pixels[i+1u]>150u)++bright[x/256u];
        }
        assert(bright[0]>20u&&bright[1]>20u); // actual licensed text in each eye
        assert(SaveJournal(world.fishing.fishing.journal)==before);
        return pixels;
    };
    const auto shore=render();
    assert(world.harbor_last_text.find("BASIC BAIT: INFINITE")!=std::string::npos);
    in.primary_down=1u;Update(&world,&in);assert(world.harbor_shop.open());
    const auto office=render();assert(office!=shore);
    in.primary_down=0u;Update(&world,&in);
    in.primary_down=1u;Update(&world,&in);
    assert(world.fishing.fishing.journal.credits==100u);
    assert(render()!=office);
    // Read-only stereo passes cannot pay twice or consume more inventory.
    const auto unchanged=SaveJournal(world.fishing.fishing.journal);
    render();render();assert(SaveJournal(world.fishing.fishing.journal)==unchanged);
    ReleaseGl(&world);assert(OnGlCreated(&world));
    in.primary_down=0u;Update(&world,&in);render();ReleaseGl(&world);
    // Retain the existing terrain-only GL preview without a walking session.
    State terrain_only;terrain_only.host=&host;
    terrain_only.terrain=world.terrain;terrain_only.waters=world.waters;
    assert(OnGlCreated(&terrain_only));ReleaseGl(&terrain_only);
    assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));
    assert(eglDestroyContext(display,context));assert(eglDestroySurface(display,surface));
    assert(eglTerminate(display));
    std::cout<<"PASS actual Sitka stereo shore prompts/office/sale text/context recreation\n";
}
