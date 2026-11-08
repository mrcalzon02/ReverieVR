// Actual production Update with exactly allocated supported ABI prefixes.
// Compile with AddressSanitizer/UBSan; no rendering callback is invoked.
// CLASSIC_GAME_KIND=1 is sufficient because the copy precedes all eight
// game-specific branches in the shared module.
#include "classic_games_module.cpp"
#include <iostream>
#include <memory>

int main() {
    auto *module=static_cast<Module *>(Create(nullptr));
    if(!module)return 2;
    const uint32_t prefixes[]={
        REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE,
        REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE,
        REVERIE_NATIVE_INPUT_V1_LISTENER_MIN_SIZE
    };
    for(uint32_t size:prefixes){
        auto bytes=std::make_unique<unsigned char[]>(size);
        ReverieNativeInputV1 input{};
        input.struct_size=size;input.delta_seconds=.02f;
        std::memcpy(bytes.get(),&input,size);
        Update(module,reinterpret_cast<const ReverieNativeInputV1 *>(bytes.get()));
        std::cout<<"accepted input prefix="<<size<<" bytes\n";
    }
    Destroy(module);
    std::cout<<"input prefix compatibility PASS\n";
    return 0;
}
