#include <iostream>
#include "../src/fuse_interface.h"


fuse_operations FUSEInterface::_gsfs_oper;
UserFS* FUSEInterface::_user_fs = nullptr;

int main(int argc, char** argv) {
    using namespace std;

    if (argc == 1) {
        std::cout << "./" << argv[0] << " [ client | server ]. \n";
        return 1;
    }


    string mount_point;
    string tmp_dir = "tmp";
    bool is_master = 0;

    if (std::string(argv[1]) == "client1")
        mount_point = "test2";
    else if (std::string(argv[1]) == "server")
        mount_point = "test", is_master = 1;
    else if (std::string(argv[1]) == "client")
        mount_point = "test1";
    else {
        std::cout << "./" << argv[0] << " [ client | server ]. \n";
        return 1;
    }



    UserFS fs;
    if (is_master) fs.setMaster();
    fs.initDirTree(mount_point, tmp_dir);
    fs.initHost("127.0.0.1", 10000, 20);
    fs.initTCPNetwork("127.0.0.1", 10000);

    FUSEInterface fuse;

    fuse.initialize(&fs);
    
    std::string fuse_argvs[] = { argv[0], mount_point, "-f" };

    char* fuse_argv[3] = { &fuse_argvs[0][0], 
                           &fuse_argvs[1][0],
                           &fuse_argvs[2][0] };


    return fuse_main(3, fuse_argv, fuse.get(), NULL);
}
