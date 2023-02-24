#include <iostream>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#include "argparse.hpp"
#include <woff2/decode.h>

#include "sanitize.h"
#include "config.h"
#include "chunker.h"

std::string loadPathAsString(std::filesystem::path &fpath) {
    std::string fext = fpath.extension();
    bool is_woff2 = (fext == ".woff2");

    if (!is_woff2 && !(fext == ".otf" || fext == ".ttf"))
        throw std::runtime_error("Error: Unrecognized input file extension");

    std::ifstream ifs(fpath, std::ios::binary);
    std::string s(std::istreambuf_iterator<char>(ifs.rdbuf()),
                  std::istreambuf_iterator<char>());

    ifs.close();

    if (is_woff2) {
        const uint8_t *iptr = reinterpret_cast<const uint8_t*>(s.data());
        std::string t(std::min(woff2::ComputeWOFF2FinalSize(iptr, s.size()),
                               woff2::kDefaultMaxSize), 0);
        woff2::WOFF2StringOut o(&t);
        auto ok = woff2::ConvertWOFF2ToTTF(iptr, s.size(), &o);
        if (!ok)
            throw std::runtime_error("Error: Could not parse WOFF2 input file");
        s.swap(t);
        t.clear();
    }
    return s;
}

int dispatch(argparse::ArgumentParser &program, config &conf) {
    int r;
    if (program.is_subcommand_used("check")) {
        auto check = program.at<argparse::ArgumentParser>("check");
        std::filesystem::path fpath = check.get<std::string>("base_file");
        std::string fs = loadPathAsString(fpath);

        r = iftc_sanitize(fs, conf) ? 0 : 1;
        if (r)
            std::cerr << "Error: File failed sanitization" << std::endl;
        else
            std::cerr << "File passed sanitization" << std::endl;
    } else if (program.is_subcommand_used("process")) {
        auto process = program.at<argparse::ArgumentParser>("process");
        std::filesystem::path fpath = process.get<std::string>("font_file");
        std::filesystem::path prefix;
        std::string fs;
        chunker ck(conf);

        conf.load(program.get<std::string>("-c"), !program.is_used("-c"));

        if (process.is_used("-o")) {
            prefix = process.get<std::string>("-o");
        } else {
            prefix = fpath;
            prefix.replace_extension();
            prefix += "_iftc";
        }
        conf.setPathPrefix(prefix);

        fs = loadPathAsString(fpath);
        r = ck.process(fs);
    } else {
        std::cerr << "Error: No command specified" << std::endl;
        std::cerr << program;
        r = 1;
    }
    return r;
}

int main(int argc, char **argv) {
    config conf;

    argparse::ArgumentParser program("iftc");

    program.add_argument("-c", "--config-file")
           .help("Path of YAML configuration file")
           .default_value(std::string("config.yaml"))
           .required();
    program.add_argument("-V", "--verbose")
           .help("Provide verbose messages "
                 "(repeat up to 3 times for more info)")
           .action([&](const auto &) { conf.increaseVerbosity(); })
           .append()
           .default_value(false)
           .implicit_value(true)
           .nargs(0);
    program.add_argument("--no-catch")
           .help("Don't catch exceptions (for debugging)")
           .default_value(false)
           .implicit_value(true);

    argparse::ArgumentParser process("process");
    process.add_description("Reorganize a font into an IFTC baseline "
                            "and chunk set");
    process.add_argument("font_file")
           .help("A TrueType or OpenType input file");
    process.add_argument("-o", "--output-prefix")
         .help("Filename prefix for output files "
               "(default is input path without extension plus \"_iftc\")");

    argparse::ArgumentParser check("check");
    check.add_description("Verify a processed file is organized correctly");
    check.add_argument("base_file")
         .help("The IFTC (chunked) input file");

    program.add_subparser(process);
    program.add_subparser(check);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    int r = 0;
    if (program["--no-catch"] == true) {
        r = dispatch(program, conf);
    } else {
        try {
            r = dispatch(program, conf);
        } catch (const std::exception &ex) {
            std::cerr << "Exception thrown: " << ex.what() << std::endl;
            r = 1;
        }
    }
    return r;
}
