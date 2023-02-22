#include <iostream>
#include <filesystem>
#include <stdexcept>

#include "config.h"
#include "builder.h"

int main(int argc, char **argv) {
    config c;
    builder b(c);

    if (argc < 2) {
        std::cerr << "Must supply font name as argument" << std::endl;
        return 1;
    }

    try {
        c.load();
        b.check_write();
        /*
        std::filesystem::path filepath = c.output_dir;
        filepath /= "prepped.otf";
        */
        std::filesystem::path filepath {argv[1]};
        b.process(filepath);
    } catch (const std::exception &ex ) {
        std::cerr << ex.what() << std::endl;
    }
    return 0;
}
