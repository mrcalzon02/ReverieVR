// Real Mesa/EGL GLES2 driver regression for the unregistered Sitka terrain module.
// Synthetic triangle is a test fixture, NEVER approved Sitka geography.
#include "../../main/jni/native/sitka_terrain_gl.h"
#include "../../main/jni/native/sitka_water_gl.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

static void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
static void NoGlError(const char* phase) {
    if (glGetError() != GL_NO_ERROR) throw std::runtime_error(std::string("GLES2 error: ") + phase);
}

struct Driver {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig config = nullptr;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    Driver() {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        Check(display != EGL_NO_DISPLAY, "EGL display unavailable");
        EGLint major = 0, minor = 0;
        Check(eglInitialize(display, &major, &minor) == EGL_TRUE, "EGL initialize failed");
        Check(eglBindAPI(EGL_OPENGL_ES_API) == EGL_TRUE, "EGL ES API unavailable");
        const EGLint attributes[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
            EGL_DEPTH_SIZE, 16, EGL_NONE
        };
        EGLint count = 0;
        Check(eglChooseConfig(display, attributes, &config, 1, &count) == EGL_TRUE && count == 1,
              "EGL GLES2 pbuffer config unavailable");
        const EGLint size[] = {EGL_WIDTH, 128, EGL_HEIGHT, 64, EGL_NONE};
        surface = eglCreatePbufferSurface(display, config, size);
        Check(surface != EGL_NO_SURFACE, "EGL pbuffer unavailable");
        CreateContext();
    }
    void CreateContext() {
        const EGLint attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, attributes);
        Check(context != EGL_NO_CONTEXT, "EGL GLES2 context unavailable");
        Check(eglMakeCurrent(display, surface, surface, context) == EGL_TRUE,
              "EGL make-current failed");
        const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        Check(version != nullptr && std::string(version).find("OpenGL ES") != std::string::npos,
              "not a real OpenGL ES driver");
        std::cout << "GLES driver: " << version << "\n";
    }
    void DestroyContext() {
        if (context != EGL_NO_CONTEXT) {
            Check(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_TRUE,
                  "EGL unbind failed");
            Check(eglDestroyContext(display, context) == EGL_TRUE, "EGL context destroy failed");
            context = EGL_NO_CONTEXT;
        }
    }
    ~Driver() {
        if (display != EGL_NO_DISPLAY) {
            if (context != EGL_NO_CONTEXT) {
                eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                eglDestroyContext(display, context);
            }
            if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
            eglTerminate(display);
        }
    }
};

struct Snapshot {
    GLint program = 0, array = 0, element = 0, active = 0, texture0 = 0;
    GLint viewport[4] = {};
    GLint attrib_enabled[2] = {}, attrib_buffer[2] = {};
    void* attrib_pointer[2] = {};
    GLboolean depth = GL_FALSE, blend = GL_FALSE, cull = GL_FALSE;
};
static Snapshot Capture() {
    Snapshot s;
    glGetIntegerv(GL_CURRENT_PROGRAM, &s.program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &s.array);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &s.element);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &s.active);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &s.texture0);
    glActiveTexture(static_cast<GLenum>(s.active));
    glGetIntegerv(GL_VIEWPORT, s.viewport);
    s.depth = glIsEnabled(GL_DEPTH_TEST);
    s.blend = glIsEnabled(GL_BLEND);
    s.cull = glIsEnabled(GL_CULL_FACE);
    for (GLuint i = 0; i < 2; ++i) {
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &s.attrib_enabled[i]);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &s.attrib_buffer[i]);
        glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &s.attrib_pointer[i]);
    }
    NoGlError("capture state");
    return s;
}
static void Same(const Snapshot& a, const Snapshot& b) {
    Check(a.program == b.program && a.array == b.array && a.element == b.element &&
          a.active == b.active && a.texture0 == b.texture0, "GL binding leaked");
    Check(a.depth == b.depth && a.blend == b.blend && a.cull == b.cull,
          "GL capability leaked");
    for (int i = 0; i < 4; ++i) Check(a.viewport[i] == b.viewport[i], "host viewport changed");
    for (int i = 0; i < 2; ++i)
        Check(a.attrib_enabled[i] == b.attrib_enabled[i] &&
              a.attrib_buffer[i] == b.attrib_buffer[i] &&
              a.attrib_pointer[i] == b.attrib_pointer[i], "GL vertex attribute leaked");
}
static ReverieNativeEyeV1 Eye(int index) {
    ReverieNativeEyeV1 eye = {};
    eye.struct_size = sizeof(eye);
    eye.eye_index = index;
    for (int i = 0; i < 4; ++i) {
        eye.view[i * 5] = 1.0f;
        eye.projection[i * 5] = 1.0f;
    }
    return eye;
}
static std::array<GLubyte,4> Pixel(int x, int y) {
    std::array<GLubyte,4> pixel = {};
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    NoGlError("pixel readback");
    return pixel;
}
static void Exercise(reverie::sitka::TerrainGl& terrain, int generation) {
    GLuint buffers[2] = {};
    GLuint textures[2] = {};
    glGenBuffers(2, buffers);
    glGenTextures(2, textures);
    Check(buffers[0] && buffers[1] && textures[0] && textures[1], "host sentinel allocation failed");
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, 64, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 64, nullptr, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 16, reinterpret_cast<void*>(16));
    glEnableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glViewport(0, 0, 128, 64);
    NoGlError("host setup");
    const Snapshot before = Capture();
    reverie::sitka::TerrainMesh mesh;
    mesh.vertices = {{-0.7f,-0.7f,0.0f,220,25,25},
                     { 0.7f,-0.7f,0.0f,220,25,25},
                     { 0.0f, 0.7f,0.0f,220,25,25}};
    mesh.indices = {0,1,2};
    std::string error;
    Check(terrain.Create(mesh, &error), "real-driver terrain Create failed");
    Check(terrain.batch_count() == 1u && terrain.upload_count() == 1u,
          "static VBO/EBO batch not uploaded once");
    Same(before, Capture());
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (int index = 0; index < 2; ++index) {
        glViewport(index * 64, 0, 64, 64);
        const Snapshot eye_before = Capture();
        const auto eye = Eye(index);
        Check(terrain.RenderEye(&eye), "real GLES2 indexed eye draw failed");
        Same(eye_before, Capture());
        Check(terrain.upload_count() == 1u, "per-eye terrain buffer reupload");
    }
    glFinish();
    const auto left = Pixel(32, 32), right = Pixel(96, 32), background = Pixel(4, 4);
    Check(left[0] > 120 && left[1] < 80 && left[2] < 80, "left eye did not rasterize terrain");
    Check(right[0] > 120 && right[1] < 80 && right[2] < 80, "right eye did not rasterize terrain");
    Check(background[0] < 20 && background[1] < 20 && background[2] < 20,
          "unexpected background pixel");
    // Test-only synthetic water, not approved geographic data.
    const auto waters=reverie::sitka::ParseWater(
        "version=1\nlevel=sitka\ncount=1\n"
        "water.0.id=test-sea\nwater.0.shape=SQUARE\n"
        "water.0.x=0\nwater.0.y=0\nwater.0.z=0\n"
        "water.0.width=1.5\nwater.0.depth=1.5\nwater.0.yaw=0\n"
        "water.0.setting.wave.height=0.16\nwater.0.setting.wave.grid=0.75\n"
        "water.0.setting.wave.speed=0.8\nwater.0.setting.layer2.speed=0.12\n");
    const auto weather=reverie::sitka::ParseWeather(
        "version=1\ncondition=RAIN\nhour=13\nwind_mps=6\nwind_heading_deg=90\n");
    const auto clear=reverie::sitka::ParseWeather(
        "version=1\ncondition=CLEAR\nhour=13\nwind_mps=0\nwind_heading_deg=90\n");
    const float sample=reverie::sitka::WeatherWaveHeight(waters[0],weather,.3f,.4f,3.f);
    Check(std::abs(sample-reverie::sitka::WeatherWaveHeight(
        waters[0],weather,.3f,.4f,3.f))<1.e-6f,"wave nondeterministic");
    Check(std::abs(sample-reverie::sitka::WeatherWaveHeight(
        waters[0],clear,.3f,.4f,3.f))>1.e-4f,"wind does not affect wave");
    Check(std::abs(sample-reverie::sitka::WeatherWaveHeight(
        waters[0],weather,.3f,.4f,3.5f))>1.e-4f,"wave time static");
    Check(reverie::sitka::WaterTint(weather)!=reverie::sitka::WaterTint(clear),
          "weather tint static");
    Check(reverie::sitka::WaterSurfaceVertices(waters[0]).size()==12u,
          "water polygon vertex count wrong");
    reverie::sitka::WaterGl water;
    const Snapshot before_water=Capture();
    Check(water.Create(waters,weather,&error),"water GL create failed");
    Check(water.ready()&&water.upload_count()==1u,"water static VBO missing");
    Same(before_water,Capture());
    water.Update(.08f);
    Check(std::abs(water.time_seconds()-.08f)<1.e-5f,"wave update failed");
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    for(int index=0;index<2;++index){
        glViewport(index*64,0,64,64);
        const Snapshot before_eye=Capture();
        auto eye=Eye(index);
        eye.view[5]=0.f;eye.view[9]=1.f;
        eye.view[6]=-1.f;eye.view[10]=0.f; // Overhead: world X/Z to screen X/Y.
        Check(water.RenderEye(&eye),"real GLES2 water eye failed");
        Same(before_eye,Capture());
        Check(water.upload_count()==1u,"per-eye water static reupload");
        Check(std::abs(water.time_seconds()-.08f)<1.e-5f,"per-eye time advanced");
    }
    glFinish();
    const auto wl=Pixel(32,32),wr=Pixel(96,32),bg=Pixel(4,4);
    Check(wl[2]>75&&wl[2]>wl[0]*2u&&wl[1]>wl[0],
          "left eye did not rasterize blue water");
    Check(wr[2]>75&&wr[2]>wr[0]*2u&&wr[1]>wr[0],
          "right eye did not rasterize blue water");
    Check(bg[0]<20&&bg[1]<20&&bg[2]<20,"water outside footprint");
    water.Release();
    Check(!water.ready()&&water.upload_count()==0u,"water GL release failed");
    NoGlError("water release");
    terrain.Release();
    Check(!terrain.ready() && terrain.upload_count() == 0u, "context release retained buffers");
    glDeleteBuffers(2, buffers);
    glDeleteTextures(2, textures);
    NoGlError("release");
    std::cout << "PASS: real GLES2 context " << generation
              << " stereo rasterization, caller state and static uploads\n";
}
int main() {
    try {
        Driver driver;
        reverie::sitka::TerrainGl terrain;
        Exercise(terrain, 1);
        driver.DestroyContext();
        driver.CreateContext(); // New shareless context: all previous GPU objects are invalid.
        Exercise(terrain, 2);
        std::cout << "PASS: Sitka EGL/GLES2 driver render and context recreation\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: Sitka real GLES2: " << e.what() << "\n";
        return 1;
    }
}
