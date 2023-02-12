#include <iostream>
#include <stdexcept>

#include "config.h"
#include "builder.h"

#define FONTNAME "LibertinusSerif-Regular.otf"

int main(int argc, char **argv) {
    config c;
    builder b(c);

    if (argc != 2) {
        std::cerr << "Must supply font name as argument" << std::endl;
        return 1;
    }

    c.load();
    b.process(argv[1]);
    return 0;
}
