#include <iostream>
#include <filesystem>
#include <stdexcept>

#include "config.h"
#include "chunker.h"

int main(int argc, char **argv) {
    config conf;
    chunker ck(conf);

    int r = conf.setArgs(argc, argv);
    if (r != 0)
        return r;

    if (conf.noCatch()) {
        r = ck.process();
    } else {
        try {
            r = ck.process();
        } catch (const std::exception &ex ) {
            r = 1;
            std::cerr << ex.what() << std::endl;
        }
    }
    return r;
}
