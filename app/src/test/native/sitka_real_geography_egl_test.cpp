// Actual native Sitka module: real archived NOAA ENC + USGS height meshes.
// Tests physical GLES2 draw, two eyes, public ABI, reversible test-range toggle.
// Render is a SOURCE-DERIVED GEOGRAPHY preview, not simulated chart navigation.
// Match the real unified APK's Android.mk, including the safe optional
// non-geographic fallback when no legacy single-terrain asset exists.
#define REVERIE_SITKA_INTEGRATION_RANGE 1
#include "../../main/jni/native/sitka_terrain_module.cpp"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
std::string asset_root;
std::string ReadFile(const std::string& path) {
    std::ifstream f(path,std::ios::binary);
    if(!f)return {};
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
int32_t Asset(const char* name,void* bytes,uint32_t capacity,uint32_t* actual){
    if(actual)*actual=0;
    if(!name||!actual)return REVERIE_NATIVE_SAVE_ERROR;
    // No fake existing terrain file: the official source must use the
    // 16-tile path, and the game still launches its real test-range loop.
    if(std::string(name)=="sitka/coast/terrain.sitkam")
        return REVERIE_NATIVE_SAVE_NOT_FOUND;
    const std::string root="sitka/coast/";
    if(std::string(name).compare(0,root.size(),root)!=0 ||
       std::string(name).find("..")!=std::string::npos)
        return REVERIE_NATIVE_SAVE_NOT_FOUND;
    std::string source=ReadFile(asset_root+"/"+name);
    if(source.empty())return REVERIE_NATIVE_SAVE_NOT_FOUND;
    *actual=static_cast<uint32_t>(source.size());
    if(!bytes||capacity<source.size())return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    std::memcpy(bytes,source.data(),source.size());
    return REVERIE_NATIVE_SAVE_OK;
}
std::string saved;
bool fail_write=false;
int32_t ReadSave(const char*,void* data,uint32_t capacity,uint32_t* size){
    *size=static_cast<uint32_t>(saved.size());
    if(saved.empty())return REVERIE_NATIVE_SAVE_NOT_FOUND;
    if(!data)return REVERIE_NATIVE_SAVE_OK;
    if(capacity<*size)return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    std::memcpy(data,saved.data(),*size);
    return REVERIE_NATIVE_SAVE_OK;
}
int32_t WriteSave(const char*,const void* data,uint32_t size){
    if(fail_write)return REVERIE_NATIVE_SAVE_ERROR;
    saved.assign(static_cast<const char*>(data),size);
    return REVERIE_NATIVE_SAVE_OK;
}
void LogLine(int32_t level,const char* tag,const char* message) {
    std::fprintf(stderr,"SITKA_NATIVE_LOG level=%d tag=%s msg=%s\n",
        level,tag?tag:"(null)",message?message:"(null)");
}
}

int main(int argc,char** argv){
    assert(argc==3);
    asset_root=argv[1];
    const char* capture=argv[2];
    const EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major=0,minor=0;
    assert(eglInitialize(display,&major,&minor));
    assert(eglBindAPI(EGL_OPENGL_ES_API));
    const EGLint configAttribs[]={
        EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,
        EGL_DEPTH_SIZE,16,EGL_NONE
    };
    EGLConfig config{};
    EGLint count=0;
    assert(eglChooseConfig(display,configAttribs,&config,1,&count));
    assert(count==1);
    const EGLint surfaceAttribs[]={EGL_WIDTH,512,EGL_HEIGHT,256,EGL_NONE};
    const auto surface=eglCreatePbufferSurface(display,config,surfaceAttribs);
    const EGLint contextAttribs[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    const auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextAttribs);
    assert(surface!=EGL_NO_SURFACE&&context!=EGL_NO_CONTEXT);
    assert(eglMakeCurrent(display,surface,surface,context));
    ReverieNativeHostV1 host{};
    host.struct_size=sizeof(host);
    host.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;
    host.log=LogLine;
    host.read_asset=Asset;
    host.read_save=ReadSave;
    host.write_save=WriteSave;
    // Existing installed V5 journal with a prior sale. Preserve its purse,
    // inventory and registration while upgrading inside the same host slot.
    saved="SITKA-FISH-5\n5482442\n100\n0\n0\n0\n0\n0\n0\n0\n0\n0\n0\n0\n0\n0\n0\n"
          "0\n0\n0\n0\n0\n0\n0\n0\n0\n0\n0\n1\n0\n0\n1\n0\n";
    auto* state=static_cast<State*>(Create(&host));
    if(!state){
        std::fprintf(stderr,"SITKA_TEST_CREATE_FAILED (no state)\n");
        return 3;
    }
    if(!state->integration_range||!state->geo_tiles){
        std::fprintf(stderr,
            "SITKA_TEST_SOURCE_VIEW_MISSING range=%d geo=%d\n",
            int(state->integration_range),int(bool(state->geo_tiles)));
        Destroy(state);
        return 4;
    }
    assert(state->geo_tiles->registered_count()==16u);
    assert(!state->geo_view);
    for(const auto& station:reverie::sitka::kSurveyRoute){
        const auto& record=state->geo_tiles->record(station.row,station.column);
        const auto bytes=ReadFile(asset_root+"/sitka/coast/tiles/"+record.filename);
        const auto land=reverie::sitka::ParseTerrainMesh(
            reinterpret_cast<const uint8_t*>(bytes.data()),bytes.size());
        const auto actual=[&](uint32_t index){
            const auto& v=land.vertices[index];
            return land.origin_east+v.x==station.east&&land.origin_north+v.z==station.north&&
                std::abs(v.y-station.elevation)<.001f;
        };
        assert(std::any_of(land.indices.begin(),land.indices.end(),actual));
    }
    assert(OnGlCreated(state));
    assert(state->geo_gl.ready());
    ReverieNativeInputV1 input{};
    input.struct_size=sizeof(input);
    input.delta_seconds=.05f;
    input.secondary_down=1u;
    input.move_x=1.f;
    Update(state,&input);
    assert(state->geo_view);
    const float initial_altitude=state->geo_altitude;
    input.move_x=0.f;input.move_y=-1.f;Update(state,&input);
    assert(state->geo_altitude>initial_altitude);
    input.move_y=1.f;Update(state,&input);
    assert(std::abs(state->geo_altitude-initial_altitude)<.01f);
    input.secondary_down=0u;
    input.move_x=input.move_y=0.f;
    for(int n=0;n<8;++n)Update(state,&input);
    assert(state->geo_tiles->resident_count()==4u);
    assert(state->geo_tiles->PendingGpuSwap());
    assert(PrepareFrame(state));
    assert(!state->geo_tiles->PendingGpuSwap());
    assert(state->geo_gl.batch_count()>=4u);
    assert(state->chart_route&&state->fishing.fishing.journal.credits==100);
    auto fly_to=[&](const reverie::sitka::ChartStation& station){
        for(unsigned i=0;i<650;++i){
            const double dx=station.east-state->geo_east,dn=station.north-state->geo_north;
            if(std::hypot(dx,dn)<4.)break;
            input.move_x=float(std::clamp(dx/4.,-1.,1.));
            input.move_y=float(std::clamp(-dn/4.,-1.,1.));
            Update(state,&input);
        }
        input.move_x=input.move_y=0;
        for(unsigned i=0;i<6;++i)Update(state,&input);
        assert(std::hypot(station.east-state->geo_east,station.north-state->geo_north)<4.);
        assert(PrepareFrame(state));
    };
    input.primary_down=1;Update(state,&input);
    assert(state->fishing.fishing.journal.chart_stations==0); // off-site
    input.primary_down=0;Update(state,&input);
    fly_to(reverie::sitka::kSurveyRoute[0]);
    // The exact same GPU surface is read from TWO actual eye renders, with
    // the native head transform, licensed persistent Lato overlay and no
    // terrain uploads during the second eye.
    float projection[16]={};
    const float tanHalf=std::tan(.698131700797732f);
    projection[0]=projection[5]=1.f/tanHalf;
    // Same near/far planes VrShellRenderer supplies for Sitka on Android.
    projection[10]=-(30.f+.10f)/(30.f-.10f);
    projection[11]=-1.f;
    projection[14]=-(2.f*30.f*.10f)/(30.f-.10f);
    std::vector<uint8_t> rgba(512u*256u*4u);
    glEnable(GL_SCISSOR_TEST);
    for(int i=0;i<2;i++){
        glViewport(i*256,0,256,256);
        glScissor(i*256,0,256,256);
        glDepthMask(GL_TRUE);
        glClearColor(0.f,0.f,0.f,1.f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        ReverieNativeEyeV1 eye{};
        eye.struct_size=sizeof(eye);
        eye.eye_index=static_cast<uint32_t>(i);
        for(int k=0;k<4;k++)eye.view[k*5]=1.f;
        std::copy(projection,projection+16,eye.projection);
        eye.projection[8]=i==0?.025f:-.025f;
        eye.view[12]=i==0?.025f:-.025f;
        assert(RenderEye(state,&eye));
    }
    glDisable(GL_SCISSOR_TEST);
    glFinish();
    assert(glGetError()==GL_NO_ERROR);
    glReadPixels(0,0,512,256,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    assert(glGetError()==GL_NO_ERROR);
    std::array<size_t,2> colored{{0u,0u}};
    std::array<std::array<bool,256>,2> red_levels{};
    for(unsigned y=0;y<256u;++y)for(unsigned x=0;x<512u;++x){
        const size_t at=(size_t(y)*512u+x)*4u;
        const unsigned red=rgba[at],green=rgba[at+1u],blue=rgba[at+2u];
        // Labels occupy the top of the view. Check the lower half for
        // real land so a clipped map with a working HUD cannot pass.
        if(y<128u&&(red+green+blue)>36u){
            ++colored[x/256u];red_levels[x/256u][red]=true;
        }
    }
    assert(colored[0]>100u&&colored[1]>100u);
    for(const auto& levels:red_levels)
        assert(std::count(levels.begin(),levels.end(),true)>8);
    std::ofstream ppm(capture,std::ios::binary);
    assert(ppm.good());
    ppm<<"P6\n512 256\n255\n";
    for(int y=255;y>=0;--y)for(unsigned x=0;x<512u;++x){
        const size_t at=(size_t(y)*512u+x)*4u;
        ppm.write(reinterpret_cast<const char*>(rgba.data()+at),3);
    }
    ppm.close();
    assert(ppm.good());
    for(unsigned i=0;i<reverie::sitka::kChartStations;++i){
        if(i)fly_to(reverie::sitka::kSurveyRoute[i]);
        // On the last site, exercise the same recoverable failure as Android exit.
        fail_write=(i==2);
        input.primary_down=1;Update(state,&input);Update(state,&input);
        assert(state->fishing.fishing.journal.chart_stations==i+1);
        assert(state->fishing.fishing.journal.credits==100+(i+1)*25);
        input.primary_down=0;Update(state,&input);
    }
    assert(state->fishing.write_failed&&PrepareExit(state)==0);
    fail_write=false;assert(PrepareExit(state));
    assert(state->fishing.fishing.journal.submitted_starter_tokens==1);
    assert(state->fishing.fishing.journal.shellfish_inventory==1);
    // Leave the map and restore old gameplay: no second APK / reset save.
    input.secondary_down=1u;
    input.move_x=1.f;
    Update(state,&input);
    assert(!state->geo_view);
    ReleaseGl(state);
    assert(OnGlCreated(state));
    assert(state->geo_tiles->PendingGpuSwap()); // EGL loss reuploads real data
    assert(PrepareFrame(state));
    assert(!state->geo_tiles->PendingGpuSwap());
    ReleaseGl(state);
    Destroy(state);
    state=static_cast<State*>(Create(&host));assert(state&&state->chart_route);
    assert(state->fishing.fishing.journal.chart_stations==3);
    assert(state->fishing.fishing.journal.credits==175);
    Destroy(state);
    assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));
    assert(eglDestroyContext(display,context));
    assert(eglDestroySurface(display,surface));
    assert(eglTerminate(display));
    std::cout<<"PASS genuine Sitka USGS/NOAA 16-tile source: live native Create, "
             <<"four tile GPU swap, two stereoscopic GLES2 previews, "
             <<"three surveyed sites, V5/V6 journal and retry; colored pixels "<<colored[0]
             <<" / "<<colored[1]<<"\n";
}
