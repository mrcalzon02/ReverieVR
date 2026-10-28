#include "../../main/jni/native/reverie_native_asset_scope.h"
#include <cassert>
#include <iostream>
int main(){using reverie::native::assets::MaximumBytes;
    assert(MaximumBytes("procedural-test-chamber","native/test-chamber/scene.rvscene")==150000u);
    assert(MaximumBytes("procedural-test-chamber","native/test-chamber/../../secret.rvscene")==0u);
    assert(MaximumBytes("procedural-test-chamber","native/test-chamber/scene2.rvscene")==0u);
    assert(MaximumBytes("classic-chess","native/test-chamber/scene.rvscene")==0u);
    assert(MaximumBytes("breakwater-battery","native/test-chamber/scene.rvscene")==0u);
    std::cout << "REVSCENE host asset scope: PASS\n";
}