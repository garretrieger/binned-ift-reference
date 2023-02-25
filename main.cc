#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#include "argparse.hpp"
#include <woff2/decode.h>

#include "sanitize.h"
#include "config.h"
#include "chunker.h"
#include "unchunk.h"
#include "table_IFTB.h"
#include "sfnt.h"
#include "tag.h"
#include "streamhelp.h"

std::string loadPathAsString(std::filesystem::path &fpath) {
    bool is_woff2 = false, is_compressed_chunk = false;
    uint32_t tg; 

    std::ifstream ifs(fpath, std::ios::binary);
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string s = ss.str();
    ifs.close();

    ss.seekg(0);
    readObject(ss, tg);

    if (tg == tag("wOF2"))
        is_woff2 = true;
    else if (tg == tag("IFTZ"))
        is_compressed_chunk = true;
    else if (tg != 0x00010000 && tg != tag("OTTO") && tg != tag("IFTC"))
        throw std::runtime_error("Error: Unrecognized file/chunk type");

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
    } else if (is_compressed_chunk) {
        std::string t = decodeChunk(s);
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
        for (auto cidx: chunks) {
            if (cidx >= tiftb.chunkCount) {
                std::cerr << cidx << " is greater than Chunk Count ";
                std::cerr << tiftb.chunkCount << std::endl;
                continue;
            }
            std::filesystem::path cp = getChunkPath(fpath, tiftb, cidx);
            std::string cfs = loadPathAsString(cp);
            std::stringstream cfss(cfs);
            std::cerr << std::endl << cidx << ": " << cp << std::endl;
            dumpChunk(std::cerr, cfss);
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
