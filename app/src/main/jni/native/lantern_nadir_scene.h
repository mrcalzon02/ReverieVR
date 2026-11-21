#ifndef REVERIE_LANTERN_NADIR_SCENE_H
#define REVERIE_LANTERN_NADIR_SCENE_H
// Temporary, camera-registered grayscale aerial plate for the actual
// Lantern photo texture route, pending approved authored source imagery.
// This is a spatial source image, NOT a truth label or a 16x16 upscale.
#include "lantern_photo_capture_quality.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace reverie { namespace lantern { namespace nadir {
enum class Scene : uint8_t { Rail=0, Orchard=1, River=2 };
constexpr int kPhotoSize=512;
// All geometry uses one immutable perpendicular camera projection.
// Survey source backdrops depend on the sector, never exposure order.
inline float Noise(float x,float y,uint32_t seed){
    return capture::Fractal(x,y,seed);
}
inline float Clamp(float value,float lo=0.f,float hi=1.f){
    return std::max(lo,std::min(hi,value));
}
inline uint8_t Gray(float value){
    return static_cast<uint8_t>(std::lround(Clamp(value,0.f,255.f)));
}
inline float RiverCenter(float v){
    return .49f+.085f*std::sin(v*5.1f+.31f)+.018f*std::sin(v*14.9f);
}
inline float RoadCenter(float u){
    return .54f+.057f*std::sin(u*4.0f+1.7f);
}
inline float Ellipse(float u,float v,float cx,float cy,float rx,float ry){
    const float dx=(u-cx)/rx,dy=(v-cy)/ry;
    return dx*dx+dy*dy;
}
// Render physically contiguous roads/rails, forest, orchards, river,
// shoreline, buildings and small vehicle silhouettes from a FIXED nadir
// plan. "Second exposure" modifies local objects only, never the camera,
// river alignment, land contours, field layout or registered buildings.
inline capture::GrayPlate Make(Scene scene,bool follow_up){
    capture::GrayPlate plate{kPhotoSize,kPhotoSize,{}};
    plate.pixels.resize(static_cast<size_t>(kPhotoSize)*kPhotoSize);
    const uint32_t seed=0xf201ae77u+static_cast<uint32_t>(scene)*101u;
    for(int y=0;y<kPhotoSize;++y)for(int x=0;x<kPhotoSize;++x){
        const float u=(x+.5f)/kPhotoSize,v=(y+.5f)/kPhotoSize;
        const float broad=Noise(u*5.2f,v*5.2f,seed);
        const float texture=Noise(u*47.f,v*47.f,seed+31u);
        const float grains=capture::Random(x,y,seed+93u);
        float light=91.f+77.f*broad+24.f*(texture-.5f)+20.f*(grains-.5f);
        const float grove=Noise(u*12.4f,v*12.4f,seed+207u);
        const float foliage=Noise(u*78.f,v*78.f,seed+299u);
        if(grove>.56f){
            light=67.f+102.f*foliage+24.f*(grains-.5f);
            if(Noise(u*142.f,v*142.f,seed+501u)>.75f)
                light+=25.f;
        }
        // Historic farmland: crop rows vary inside irregular field
        // parcels. Grid extent is fixed to the source coordinate system.
        const float parcel=Noise(u*3.6f,v*3.6f,seed+421u);
        if(parcel>.42f&&grove<.62f){
            const float rows=std::sin((u*.63f+v*.78f)*890.f);
            light+=8.f*rows;
        }
        if(scene==Scene::River){
            const float riverDist=std::abs(u-RiverCenter(v));
            const float riverWidth=.068f+.015f*std::sin(v*11.f+2.3f);
            if(riverDist<riverWidth){
                light=52.f+15.f*Noise(u*28.f,v*65.f,seed+87u)+
                      8.f*(grains-.5f);
            }else if(riverDist<riverWidth+.014f){
                light=183.f-55.f*(riverDist-riverWidth)/.014f+
                      10.f*(texture-.5f);
            }
        }
        // One continuous route, with a correctly registered crossing.
        const float road=RoadCenter(u);
        const float dist=std::abs(v-road);
        if(dist<.021f){
            light=155.f+15.f*texture+
                  4.f*std::sin(u*370.f+v*21.f);
            if(dist>.018f)light=182.f;
            if(dist<.0025f)light-=10.f;
        }else if(dist<.026f)light+=26.f;
        if(scene==Scene::Rail){
            const float trackY=.35f+.025f*std::sin(u*4.6f);
            const float trackDist=std::abs(v-trackY);
            if(trackDist<.007f){
                light=52.f+10.f*texture;
                if(std::fmod(u*310.f,8.0f)<1.2f)light=176.f;
            }
        }
        if(scene==Scene::Orchard){
            // Small civilian orchard canopies at repeated farm spacing.
            for(int row=0;row<4;++row)for(int col=0;col<5;++col){
                const float cx=.13f+col*.158f+row*.009f;
                const float cy=.18f+row*.167f;
                const float e=Ellipse(u,v,cx,cy,.029f,.024f);
                if(e<1.f){
                    light=57.f+112.f*Noise(u*137.f,v*137.f,seed+row*13u+col);
                    if(e>.78f)light-=13.f;
                }
            }
        }
        // Fixed farmhouse roof planes and shadows; building and road
        // do not move or rescale between separate photo exposures.
        if(u>.13f&&u<.235f&&v>.17f&&v<.235f){
            light=u<.185f?181.f:85.f;
            if(v<.177f||v>.229f)light=54.f;
        }
        if(u>.742f&&u<.811f&&v>.77f&&v<.813f)
            light=u<.777f?179.f:77.f;
        if(scene==Scene::River){
            const float riverDist=std::abs(u-RiverCenter(v));
            const float roadDist=std::abs(v-road);
            const bool bridgeArea=riverDist<.105f&&roadDist<.019f;
            if(bridgeArea){
                light=179.f+8.f*texture;
                // Two rails and their narrow center shadows.
                if(roadDist>.016f)light=52.f;
                if(roadDist<.0019f)light=137.f;
                if(!follow_up&&riverDist<.034f){
                    // Broken center section: dark water through the
                    // physical gap with irregular rubble on each end.
                    light=45.f+11.f*texture;
                    const float rubble=Noise(u*135.f,v*141.f,seed+919u);
                    if(riverDist>.024f&&rubble>.52f)light=132.f+74.f*rubble;
                }
            }
        }
        // Reproducible vehicles: their silhouettes are painted on the
        // SAME map and in consistent scale/position, not floating layers.
        const bool convoy=(scene==Scene::Rail&&!follow_up)||
                          (scene==Scene::Orchard&&follow_up);
        if(convoy){
            for(int i=0;i<4;++i){
                const float cx=.39f+i*.095f;
                const float cy=RoadCenter(cx);
                const float du=u-cx,dv=v-cy;
                const float angle=.25f*std::cos(cx*4.f);
                const float rx=du*std::cos(angle)+dv*std::sin(angle);
                const float ry=-du*std::sin(angle)+dv*std::cos(angle);
                if(std::abs(rx)<.027f&&std::abs(ry)<.011f){
                    light=(i%2?68.f:78.f)+11.f*grains;
                    if(std::abs(rx)>.021f)light=126.f;
                    if(std::abs(ry)>.007f)light=38.f;
                    if(std::abs(rx)<.010f&&std::abs(ry)<.006f)
                        light=113.f; // cabin seen only from above
                }
            }
        }
        // A benign first-time orchard structure, which may be mistaken
        // for a vehicle from the real aircraft's limited perspective.
        if(scene==Scene::Orchard&&!follow_up &&
           u>.49f&&u<.54f&&v>.47f&&v<.51f)
            light=u<.515f?177.f:93.f;
        plate.pixels[static_cast<size_t>(y)*kPhotoSize+x]=Gray(light);
    }
    return plate;
}
}}} // reverie::lantern::nadir
#endif
