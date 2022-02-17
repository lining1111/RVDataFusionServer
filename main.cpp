#include <iostream>
#include "log/Log.h"
#include "version.h"

using namespace log;


int main(int argc, char **argv) {

    if (argc == 2 && string(argv[1]) == "--version") {

#if defined( PROJECT_VERSION )
        cout << "" << PROJECT_VERSION << endl;

#else
        cout << "build date:" << __DATE__ << endl;
#endif
        return 0;
    }


    std::cout << "Hello, World!" << std::endl;

//    log::init();
//    int x = 4;
//    printf("x = %d\n", x);
//
//    Fatal("x = %d", x);




    return 0;
}
