#include <iostream>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#include "argparse.hpp"
#include <woff2/decode.h>

#include "sanitize.h"

int main(int argc, char **argv) {
    argparse::ArgumentParser ap("ccmd");

    ap.add_argument("font_file")
        .help("The IFTC (chunked) input file");

    ap.add_argument("-o", "--output")
        .help("The file to write changed output to ('-' for stdout)");

    ap.add_argument("-s", "--sanitize")
        .help("Verify the input file is organized correctly")
        .default_value(false)
        .implicit_value(true);

    ap.add_argument("-v", "--verbose")
        .help("Provide verbose messages")
        .default_value(false)
        .implicit_value(true);

    try {
        ap.parse_args(argc, argv);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << ap;
        std::exit(1);
    }

    if (ap["-s"] == true && ap.present("-o")) {
        std::cerr << "Error: --output not relevant to --sanitize" << std::endl;
        std::exit(1);
    }

    std::filesystem::path fpath = ap.get<std::string>("font_file");

    std::string fext = fpath.extension();

    bool is_woff2 = (fext == ".woff2");

    if (!is_woff2 && !(fext == ".otf" || fext == ".ttf")) {
        std::cerr << "Error: Unrecognized input file extension" << std::endl;
        std::exit(1);
    }

    std::ifstream ifs(fpath, std::ios::binary);
    std::string fs(std::istreambuf_iterator<char>(ifs.rdbuf()),
                   std::istreambuf_iterator<char>());

    ifs.close();

    if (is_woff2) {
        const uint8_t *iptr = reinterpret_cast<const uint8_t*>(fs.data());
        std::string t(std::min(woff2::ComputeWOFF2FinalSize(iptr, fs.size()),
                               woff2::kDefaultMaxSize), 0);
        woff2::WOFF2StringOut o(&t);
        auto ok = woff2::ConvertWOFF2ToTTF(iptr, fs.size(), &o);
        if (!ok) {
            std::cerr << "Error: Could not parse WOFF2 input file" << std::endl;
            std::exit(1);
        }
        fs.swap(t);
        t.clear();
    }

    if (ap["-s"] == true) {
        if (iftc_sanitize(fs, ap["-v"] == true)) {
            std::cerr << "File passed sanitization" << std::endl;
            std::exit(0);
        } else {
            std::cerr << "Error: File failed sanitization" << std::endl;
            std::exit(1);
        }
    }

    return 0;
}
