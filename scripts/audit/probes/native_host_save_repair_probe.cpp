// Actual production host persistence and prepare-exit dispatch. POSIX file
// operations are real; fsync faults are injected. Android log/assets are
// named test shims, so this is not APK/device acceptance.
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>
namespace fault {
bool fail_file=false, fail_directory=false;
unsigned file_syncs=0, directory_syncs=0;
int Sync(int fd) {
    struct stat info{};
    if(fstat(fd,&info)!=0)return -1;
    bool directory=S_ISDIR(info.st_mode);
    if(directory)++directory_syncs;else ++file_syncs;
    if((directory&&fail_directory)||(!directory&&fail_file)){errno=EIO;return -1;}
    return ::fsync(fd);
}
}
#define fsync fault::Sync
#include "reverie_native_host.cpp"
#undef fsync
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void Check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
unsigned exit_calls=0;
int32_t exit_ready=0;
int32_t Prepare(void*){++exit_calls;return exit_ready;}
}
int main(int argc,char** argv){
    Check(argc==2,"isolated storage argument");
    const std::filesystem::path root(argv[1]);
    Check(!std::filesystem::exists(root),"refuse to reuse preexisting storage");
    std::filesystem::create_directories(root);
    NativeSession session{};session.storage_root=root.string();
    std::vector<uint8_t> old(64,0x31),next(64,0x72),readback(64);
    uint32_t size=0;
    Check(HostWriteSave("progress.v1",old.data(),old.size())==REVERIE_NATIVE_SAVE_ERROR,
          "out-of-callback write rejected");
    {
        CallbackSessionScope scope(&session);
        Check(HostReadSave("progress.v1",readback.data(),readback.size(),&size)==REVERIE_NATIVE_SAVE_NOT_FOUND,
              "absent slot distinguished");
        Check(HostWriteSave("../escape",old.data(),old.size())==REVERIE_NATIVE_SAVE_ERROR,
              "escaping slot rejected");
        Check(HostWriteSave("progress.v1",old.data(),old.size())==REVERIE_NATIVE_SAVE_OK,
              "initial save");
        Check(fault::file_syncs==1&&fault::directory_syncs==1,"file and directory flushed");
        fault::fail_file=true;
        Check(HostWriteSave("progress.v1",next.data(),next.size())==REVERIE_NATIVE_SAVE_ERROR,
              "failed payload flush reported");
        fault::fail_file=false;
        Check(HostReadSave("progress.v1",readback.data(),readback.size(),&size)==REVERIE_NATIVE_SAVE_OK
              &&readback==old,"pre-rename failure preserves old progress");
        Check(!std::filesystem::exists(root/"progress.v1.tmp"),"failed temporary write cleaned");
        fault::fail_directory=true;
        Check(HostWriteSave("progress.v1",next.data(),next.size())==REVERIE_NATIVE_SAVE_ERROR,
              "post-rename durability failure reported");
        Check(HostReadSave("progress.v1",readback.data(),readback.size(),&size)==REVERIE_NATIVE_SAVE_OK
              &&readback==next,"ERROR may expose new bytes without durable acknowledgement");
        fault::fail_directory=false;
        Check(HostWriteSave("progress.v1",next.data(),next.size())==REVERIE_NATIVE_SAVE_OK,
              "retry confirms latest snapshot");
        std::filesystem::create_directory(root/"invalid.v1");
        Check(HostReadSave("invalid.v1",nullptr,0,&size)==REVERIE_NATIVE_SAVE_ERROR,
              "directory cannot masquerade as a save");
        std::vector<uint8_t> limit(REVERIE_NATIVE_SAVE_MAX_BYTES,0x5a);
        Check(HostWriteSave("limit.v1",limit.data(),limit.size())==REVERIE_NATIVE_SAVE_OK,
              "exact 64KiB allowed");
        Check(HostReadSave("limit.v1",nullptr,0,&size)==REVERIE_NATIVE_SAVE_OK
              &&size==limit.size(),"size query exact");
        Check(HostReadSave("limit.v1",readback.data(),readback.size(),&size)==REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL,
              "insufficient read buffer distinguished");
        Check(HostWriteSave("limit.v1",limit.data(),limit.size()+1)==REVERIE_NATIVE_SAVE_ERROR,
              "oversized write rejected before memory read");
    }
    ReverieNativeModuleApiV1 api{};
    api.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;
    api.struct_size=REVERIE_NATIVE_MODULE_API_V1_PREPARE_FRAME_MIN_SIZE;
    api.prepare_exit=Prepare;
    session.api=&api;session.instance=&exit_ready;
    auto handle=static_cast<jlong>(reinterpret_cast<intptr_t>(&session));
    Check(Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativePrepareExit(nullptr,nullptr,handle)==JNI_TRUE
          &&exit_calls==0,"old ABI prefix does not dispatch unseen tail");
    api.struct_size=sizeof(api);
    Check(Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativePrepareExit(nullptr,nullptr,handle)==JNI_FALSE
          &&exit_calls==1&&session.instance==&exit_ready,"refused exit retains instance");
    exit_ready=1;
    Check(Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativePrepareExit(nullptr,nullptr,handle)==JNI_TRUE
          &&exit_calls==2,"successful retry dispatched");
    api.prepare_exit=nullptr;
    Check(Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativePrepareExit(nullptr,nullptr,handle)==JNI_TRUE,
          "null tail remains compatible");
    std::filesystem::remove_all(root);
    std::cout<<"PASS "<<checks<<" actual native-host persistence/exit assertions; real POSIX I/O, injected fsync faults\n";
}
