#include <iostream>
#include <filesystem>
#include <stdexcept>

#include "config.h"
#include "builder.h"

int main(int argc, char **argv) {
    config c;
    builder b(c);

    int r = c.setArgs(argc, argv);
    if (r != 0)
        return r;

    if (c.noCatch()) {
        r = b.process();
    } else {
        try {
            r = b.process();
        } catch (const std::exception &ex ) {
            r = 1;
            std::cerr << ex.what() << std::endl;
        }
    }
    return r;
}
