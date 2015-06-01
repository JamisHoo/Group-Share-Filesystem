#include <iostream>
#include "../src/fuse_interface.h"


fuse_operations FUSEInterface::_gsfs_oper;
UserFS* FUSEInterface::_user_fs = nullptr;

int main(int /* argc */, char** argv) {
    using namespace std;


    string mount_point = "test";
    string tmp_dir = "tmp";

    UserFS fs;
    fs.setMaster();
    fs.initDirTree(mount_point, tmp_dir);
    fs.initHost("localhost", 10000, 20);

    FUSEInterface fuse;

    fuse.initialize(&fs);
    
    char* fuse_argv0 = argv[0];
    char* fuse_argv1 = new char[mount_point.length()];
    memcpy(fuse_argv1, mount_point.data(), mount_point.length());

    char* fuse_argv[2] = { fuse_argv0, fuse_argv1 };


    return fuse_main(2, fuse_argv, fuse.get(), NULL);
}
