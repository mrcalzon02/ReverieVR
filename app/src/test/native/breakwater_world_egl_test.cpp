// Executes the ACTUAL game module with packaged art on a real GLES driver.
// Controlled campaign setup is a fixture, not evidence of an installed APK.
#include "../../main/jni/native/breakwater_module.cpp"
#include <EGL/egl.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstdlib>
static void Check(bool okay,const char *message) {
    if(!okay)throw std::runtime_error(message);
}
static int32_t ReadAsset(const char *path,void *buffer,uint32_t capacity,uint32_t *size) {
    const std::string full=std::string("app/src/main/assets/")+path;
    std::ifstream f(full,std::ios::binary|std::ios::ate);
    if(!f)return REVERIE_NATIVE_SAVE_NOT_FOUND;
    *size=static_cast<uint32_t>(f.tellg());
    if(!buffer || capacity<*size)return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    f.seekg(0);f.read(static_cast<char *>(buffer),*size);
    return f?REVERIE_NATIVE_SAVE_OK:REVERIE_NATIVE_SAVE_ERROR;
}
static void Log(int32_t,const char *tag,const char *text) {
    std::cout<<tag<<": "<<text<<"\n";
}
static void Identity(float *m){std::fill(m,m+16,0.f);m[0]=m[5]=m[10]=m[15]=1.f;}
static void Save(const std::string &path,int w,int h) {
    std::vector<unsigned char> bytes(static_cast<size_t>(w*h*4));
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,bytes.data());
    std::ofstream f(path,std::ios::binary);f<<"P6\n"<<w<<" "<<h<<"\n255\n";
    for(int y=h-1;y>=0;--y)for(int x=0;x<w;++x)
        f.write(reinterpret_cast<const char *>(bytes.data()+(y*w+x)*4),3);
    Check(bool(f),"capture write failed");
}
int main(int argc,char **argv) {
    const EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major=0,minor=0;Check(eglInitialize(display,&major,&minor),"EGL initialization");
    Check(eglBindAPI(EGL_OPENGL_ES_API),"ES API");
    const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};
    EGLConfig config=nullptr;EGLint count=0;
    Check(eglChooseConfig(display,attributes,&config,1,&count)&&count==1,"EGL config");
    constexpr int width=1280,height=800;
    const EGLint size[]={EGL_WIDTH,width,EGL_HEIGHT,height,EGL_NONE};
    EGLSurface surface=eglCreatePbufferSurface(display,config,size);
    const EGLint contextAttributes[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextAttributes);
    Check(eglMakeCurrent(display,surface,surface,context),"EGL current");
    std::cout<<"Driver: "<<glGetString(GL_VERSION)<<"\n";
    using DebugCallback=void (*)(GLenum,GLenum,GLuint,GLenum,GLsizei,const GLchar*,const void*);
    using SetDebug=void (*)(DebugCallback,const void*);
    auto debug=reinterpret_cast<SetDebug>(eglGetProcAddress("glDebugMessageCallbackKHR"));
    if(debug){glEnable(0x92E0);debug([](GLenum,GLenum,GLuint,GLenum,GLsizei,const GLchar *message,const void*){
        std::cerr<<"GL: "<<message<<"\n";},nullptr);}
    ReverieNativeHostV1 host{};host.struct_size=sizeof(host);host.abi_version=1;
    host.log=Log;host.read_asset=ReadAsset;
    auto *state=static_cast<State *>(Create(&host));Check(state,"game create");LoadState(state);
    Check(state->authored_materials.count==16,"all authored material PNGs must load");
    for(const auto &image:state->horizon_art)
        Check(image.width==1024u && image.height==512u,"full archived panorama resolution must load");
    Check(GlCreate(state),"actual Breakwater shader/resource creation");
    state->slot_picker=false;Check(state->campaign.Start(),"start campaign");
    Check(state->campaign.BeginWave(),"begin wave");
    state->campaign.Tick(4.f);state->scene_dirty=true;PrepareFrame(state);
    // Controlled showcase wave using the real unit builders, NOT concept art.
    auto &vessels=const_cast<std::array<Enemy,Campaign::kMaxEnemies>&>(state->campaign.enemies());
    const UnitType kinds[]={UnitType::Rowboat,UnitType::LandingCraft,UnitType::Gunboat,
                           UnitType::ArmoredTransport,UnitType::Aircraft};
    for(int i=0;i<5;++i){auto &e=vessels[i];e={};e.active=true;e.type=kinds[i];
        e.x=(i-2)*.85f;e.z=-2.4f-i*.85f;e.hp=EnemyStartingHp(e.type);e.passengers=4;
        if(e.type==UnitType::Aircraft)e.aircraft_pattern=AircraftPattern::Bomber;}
    // Real update input is essential: old capture fixtures never enabled
    // aim_ray_valid and therefore could not prove the trajectory was visible.
    ReverieNativeInputV1 live{};live.struct_size=sizeof(live);live.delta_seconds=.016f;
    live.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    live.pointer_direction[1]=-.08f;live.pointer_direction[2]=-.9967949f;
    Update(state,&live);Update(state,&live);
    Check(state->aim_ray_valid,"live input must enable trajectory preview");
    state->scene_dirty=true;PrepareFrame(state);
    const auto coast=state->coast_mesh,textured=state->coast_sprite_mesh;
    Check(!coast.empty()&&!textured.empty(),"static coast");
    ReverieNativeEyeV1 eye{};eye.struct_size=sizeof(eye);
    Identity(eye.view);std::fill(eye.projection,eye.projection+16,0.f);
    const float near=.08f,far=battlefield::kFarClip,scale=1.25f;
    eye.projection[0]=scale*height/width;eye.projection[5]=scale;
    eye.projection[10]=-(far+near)/(far-near);eye.projection[11]=-1.f;
    eye.projection[14]=-2.f*far*near/(far-near);
    const std::string directory=argc>1?argv[1]:"/tmp";
    glViewport(0,0,width,height);
    for(int station=0;station<3;++station) {
        state->defense_points.current=station;Update(state,&live);
        Check(state->aim_ray_valid,"station aim preview");
        state->scene_dirty=true;PrepareFrame(state);
        for(int side=0;side<2;++side) {
            eye.eye_index=side;eye.view[12]=side==0?.032f:-.032f;
            glClearColor(.2f,.4f,.5f,1.f);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            Check(Draw(state,&eye)==1,"actual stereo module render");
            Check(glGetError()==GL_NO_ERROR,"GL error");
            std::vector<unsigned char> withGuide(width*height*4),withoutGuide(width*height*4);
            glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,withGuide.data());
            state->aim_ray_valid=false;state->scene_dirty=true;PrepareFrame(state);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);Check(Draw(state,&eye)==1,"guide off render");
            glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,withoutGuide.data());
            size_t changed=0;for(size_t i=0;i<withGuide.size();i+=4)
                if(withGuide[i+1]>withoutGuide[i+1]+30 && withGuide[i+2]>withoutGuide[i+2]+30)++changed;
            Check(changed>120,"trajectory guide must visibly cover screen pixels in both eyes");
            std::cout<<"Guide visible pixels station "<<station<<" eye "<<side<<": "<<changed<<"\n";
            state->aim_ray_valid=true;state->scene_dirty=true;PrepareFrame(state);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);Check(Draw(state,&eye)==1,"guide on render");
            if(side==0)Save(directory+"/breakwater-station-"+std::to_string(station)+".ppm",width,height);
        }
    }
    Check(state->coast_mesh==coast && state->coast_sprite_mesh==textured,"static coast changed between eyes/stations");
    Check(!state->coast_gpu_dirty,"static GPU upload did not settle");
    // Verify all support pick coordinates after the physical console mapping.
    for(float x:{-2.f,0.f,2.f}) {
        float wx=x,wy=.97f,wz=-1.8f;console::Point(wx,wy,wz);
        ReverieNativeInputV1 input{};input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
        input.pointer_direction[0]=wx;input.pointer_direction[1]=wy;input.pointer_direction[2]=wz;
        float pickedX=0.f,pickedY=0.f;
        Check(AimAtPlane(ConsolePointer(input),-1.8f,&pickedX,&pickedY)>0.f,"console plane");
        Check(std::abs(pickedX-x)<.001f && std::abs(pickedY-.97f)<.001f,"console visible/pick parity");
    }
    // Side and downward views prove world placement around the operator.
    state->defense_points.current=0;state->scene_dirty=true;PrepareFrame(state);
    const auto captureView=[&](float yaw,float pitch,const char *name) {
        const float cy=std::cos(yaw),sy=std::sin(yaw),cp=std::cos(pitch),sp=std::sin(pitch);
        Identity(eye.view);
        eye.view[0]=cy;eye.view[4]=0;eye.view[8]=sy;
        eye.view[1]=-sy*sp;eye.view[5]=cp;eye.view[9]=cy*sp;
        eye.view[2]=-sy*cp;eye.view[6]=-sp;eye.view[10]=cy*cp;
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        Check(Draw(state,&eye)==1,"turned view");
        Save(directory+"/"+name+".ppm",width,height);
    };
    captureView(0.f,-.23f,"breakwater-shore");
    captureView(1.85f,-.10f,"breakwater-console");
    captureView(-1.825f,-.10f,"breakwater-rack");
    const auto remaining=state->campaign.enemies();
    eye.eye_index=0;Check(Draw(state,&eye)==1,"repeat render");
    for(size_t i=0;i<remaining.size();++i)
        Check(remaining[i].z==state->campaign.enemies()[i].z,"render advanced simulation");
    // Renderer acceptance beyond CPU mesh/labels: use paid campaign ownership
    // and actual tracked-controller rack presses to exchange the *mounted* gun.
    // Keep world time, camera and gaze-guide identical for before/after eyes.
    state->campaign=Campaign{};
    Check(state->campaign.Start(),"paid-rack shop start");
    const int32_t credits_before=state->campaign.credits();
    Check(state->campaign.PurchaseWeapon(WeaponKind::HeavyGun),"paid-rack purchase");
    Check(state->campaign.owns_weapon(WeaponKind::HeavyGun) &&
          state->campaign.credits()==credits_before-
              WeaponFor(WeaponKind::HeavyGun).purchase_cost,
          "heavy gun actually paid for");
    Check(state->campaign.BeginWave(),"paid-rack combat wave");
    state->slot_picker=false;state->flash=0.f;
    state->combat_input.Release();
    const auto rackRay=[](uint8_t id) {
        ReverieNativeInputV1 sample{};
        sample.struct_size=sizeof(sample);
        sample.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
        const auto slot=gunmesh::RackLocation(id);
        const float length=std::sqrt(slot.x*slot.x+slot.y*slot.y+slot.z*slot.z);
        sample.pointer_direction[0]=slot.x/length;
        sample.pointer_direction[1]=slot.y/length;
        sample.pointer_direction[2]=slot.z/length;
        return sample;
    };
    const auto rackPress=[&](uint8_t id) {
        auto sample=rackRay(id);
        Check(gunmesh::HitRack(sample.pointer_origin,sample.pointer_direction)==id,
              "physical rack ray hit");
        Update(state,&sample); // release any preceding press
        sample.primary_down=1u;
        Update(state,&sample); // REAL module controller edge, not SelectWeapon()
    };
    const auto selected=state->campaign.selected_weapon();
    rackPress(2); // unowned Flak is not a free swap
    Check(state->campaign.selected_weapon()==selected &&
          !state->campaign.owns_weapon(WeaponKind::Flak),
          "locked rack must not equip");
    auto forward=live;forward.delta_seconds=0.f;
    forward.primary_down=0u;forward.secondary_down=0u;
    forward.pointer_direction[0]=0.f;forward.pointer_direction[1]=0.f;
    forward.pointer_direction[2]=-1.f;
    const auto stereoMount=[&](const char *label) {
        Update(state,&forward); // clear rack hover and release trigger
        Check(state->rack_focus==-1,"mounted-gun snapshot has no rack hover");
        state->aim_ray_valid=false; // omit weapon-dependent trajectory overlay
        state->ghost_on=false;
        state->scene_dirty=true;
        Check(PrepareFrame(state)==1,"mounted-gun production PrepareFrame");
        std::array<std::vector<unsigned char>,2> frames;
        for(int side=0;side<2;++side) {
            // The seated forward/downward view includes the near-field breech.
            const float pitch=-.23f,cp=std::cos(pitch),sp=std::sin(pitch);
            Identity(eye.view);
            eye.view[5]=cp;eye.view[9]=sp;
            eye.view[6]=-sp;eye.view[10]=cp;
            eye.eye_index=side;eye.view[12]=side==0?.032f:-.032f;
            glClearColor(.2f,.4f,.5f,1.f);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            Check(Draw(state,&eye)==1,"paid-rack real stereo draw");
            Check(glGetError()==GL_NO_ERROR,"paid-rack GLES draw error");
            auto &pixels=frames[static_cast<size_t>(side)];
            pixels.resize(static_cast<size_t>(width*height*4));
            glPixelStorei(GL_PACK_ALIGNMENT,1);
            glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
            Check(glGetError()==GL_NO_ERROR,"paid-rack framebuffer readback");
            Save(directory+"/breakwater-paid-"+label+
                 (side==0?"-left.ppm":"-right.ppm"),width,height);
        }
        return frames;
    };
    rackPress(0);
    Check(state->campaign.selected_weapon()==WeaponKind::MachineGun,
          "real rack press must select original machine gun");
    const auto machinePixels=stereoMount("machine");
    rackPress(1);
    Check(state->campaign.selected_weapon()==WeaponKind::HeavyGun &&
          state->campaign.credits()==credits_before-
              WeaponFor(WeaponKind::HeavyGun).purchase_cost,
          "real rack press must equip paid heavy gun without rebilling");
    const auto heavyPixels=stereoMount("heavy");
    for(int side=0;side<2;++side) {
        size_t changed=0;
        // Center-lower framebuffer: physical gun muzzle and breech in front
        // of the player, not the right-side weapon-name HUD or left rack.
        for(int y=0;y<height/2;++y)for(int x=width*3/10;x<width*7/10;++x) {
            const size_t p=static_cast<size_t>((y*width+x)*4);
            const auto &a=machinePixels[static_cast<size_t>(side)];
            const auto &b=heavyPixels[static_cast<size_t>(side)];
            if(std::abs(int(a[p])-int(b[p]))>24 ||
               std::abs(int(a[p+1])-int(b[p+1]))>24 ||
               std::abs(int(a[p+2])-int(b[p+2]))>24)++changed;
        }
        Check(changed>100,"paid selected gun must change mounted GLES pixels in both eyes");
        std::cout<<"PAID_RACK_MOUNT_PIXELS eye="<<side<<" changed="<<changed<<"\n";
    }
    // An unheld forward trigger really fires the selected paid gun.
    const auto ammo=state->campaign.magazine_left();
    forward.primary_down=1u;Update(state,&forward);
    Check(state->campaign.selected_weapon()==WeaponKind::HeavyGun &&
          state->campaign.magazine_left()+1u==ammo && state->flash>0.f,
          "selected heavy mount fires with actual controller input");
    Check(PrepareFrame(state)==1,"firing frame prepared");
    std::cout<<"PASS: Breakwater paid rack GLES2 selected-mount stereo raster and fire\n";
    GlRelease(state);
    Check(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT),"unbind old context");
    Check(eglDestroyContext(display,context),"destroy old context");
    context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextAttributes);
    Check(eglMakeCurrent(display,surface,surface,context),"new GL context");
    glViewport(0,0,width,height);
    Check(GlCreate(state),"GL context resource recreation");
    Check(Draw(state,&eye)==1,"render after resource recreation");
    GlRelease(state);Destroy(state);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
    std::cout<<"PASS actual Breakwater GLES2: 16 materials, stereo, 3 stations, immutable coast and context resource recreation\n";
}
