#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#include "argparse.hpp"

#include "sanitize.h"
#include "config.h"
#include "chunker.h"
#include "unchunk.h"
#include "table_IFTB.h"
#include "sfnt.h"
#include "tag.h"
#include "streamhelp.h"

std::string loadPathAsString(std::filesystem::path &fpath) {
    std::ifstream ifs(fpath, std::ios::binary);
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string s = ss.str();
    ifs.close();

    uint32_t tg = decodeBuffer(NULL, 0, s);

    if (tg != 0x00010000 && tg != tag("OTTO") &&
        tg != tag("IFTC") && tg != tag("IFTB"))
        throw std::runtime_error("Error: Unrecognized file/chunk type");

    return s;
}

int dispatch(argparse::ArgumentParser &program, config &conf) {
    int r;
    if (program.is_subcommand_used("check")) {
        auto check = program.at<argparse::ArgumentParser>("check");
        std::filesystem::path fpath = check.get<std::string>("base_file");
        std::string fs = loadPathAsString(fpath);

        r = iftb_sanitize(fs, conf) ? 0 : 1;
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
            prefix += "_iftb";
        }
        conf.setPathPrefix(prefix);

        fs = loadPathAsString(fpath);
        r = ck.process(fs);
    } else if (program.is_subcommand_used("dump-chunks")) {
        auto dumpchunks = program.at<argparse::ArgumentParser>("dump-chunks");
        auto chunks = dumpchunks.get<std::vector<uint16_t>>("indexes");
        std::filesystem::path fpath = dumpchunks.get<std::string>("base_file");
        std::string fs = loadPathAsString(fpath);

        sfnt sft(fs);
        sft.read();
        simplestream ss;
        if (!sft.getTableStream(ss, T_IFTB))
            throw std::runtime_error("No IFTB table in font file");

        table_IFTB tiftb;
        tiftb.decompile(ss);
        std::filesystem::current_path(fpath.parent_path());
        std::stringstream css;
        if (dumpchunks["-r"] == true) {
            std::filesystem::path rpath = tiftb.getRangeFileURI();
            std::ifstream rs(rpath, std::ios::binary);
            for (auto cidx: chunks) {
                if (cidx >= tiftb.getChunkCount()) {
                    std::cerr << cidx << " is greater than Chunk Count ";
                    std::cerr << tiftb.getChunkCount() << std::endl;
                    continue;
                }
                auto [cstart, cend] = tiftb.getChunkRange(cidx);
                uint32_t clen = cend - cstart;
                std::string cfz(clen, 0);
                rs.seekg(cstart);
                rs.read(cfz.data(), clen);
                css.str(decodeChunk(cfz.data(), cfz.size()));
                std::cerr << std::endl << cidx << std::endl;
                dumpChunk(std::cerr, css);
            }
        } else {
            for (auto cidx: chunks) {
                if (cidx >= tiftb.getChunkCount()) {
                    std::cerr << cidx << " is greater than Chunk Count ";
                    std::cerr << tiftb.getChunkCount() << std::endl;
                    continue;
                }
                std::filesystem::path cp = tiftb.getChunkURI(cidx);
                std::string cfs = loadPathAsString(cp);
                css.str(cfs);
                std::cerr << std::endl << cidx << ": " << cp << std::endl;
                dumpChunk(std::cerr, css);
            }
        }
    } else {
        std::cerr << "Error: No command specified" << std::endl;
        std::cerr << program;
        r = 1;
    }
    return r;
}

int main(int argc, char **argv) {
    config conf;

    argparse::ArgumentParser program("iftb");

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
    process.add_description("Reorganize a font into an IFTB baseline "
                            "and chunk set");
    process.add_argument("font_file")
           .help("A TrueType or OpenType input file");
    process.add_argument("-o", "--output-prefix")
         .help("Filename prefix for output files "
               "(default is input path without extension plus \"_iftb\")");

    argparse::ArgumentParser check("check");
    check.add_description("Verify a processed file is organized correctly");
    check.add_argument("base_file")
         .help("The IFTB (binned) input file");

    argparse::ArgumentParser dumpchunks("dump-chunks");
    dumpchunks.add_description("Show the fields in a set of chunks, by index");
    dumpchunks.add_argument("base_file")
              .help("The IFTB (binned) input file");
    dumpchunks.add_argument("indexes")
              .help("A list of positive integer indexes")
              .nargs(argparse::nargs_pattern::at_least_one)
              .required()
              .scan<'u', uint16_t>();
    dumpchunks.add_argument("-r", "--by-range")
              .help("Retrieve the chunk from the range file")
              .default_value(false)
              .implicit_value(true);

    program.add_subparser(process);
    program.add_subparser(check);
    program.add_subparser(dumpchunks);

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
