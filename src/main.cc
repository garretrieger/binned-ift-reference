#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#include "argparse.hpp"
#include "woff2/encode.h"

#include "sanitize.h"
#include "config.h"
#include "chunker.h"
#include "client.h"
#include "merger.h"
#include "table_IFTB.h"
#include "sfnt.h"
#include "tag.h"
#include "streamhelp.h"
#include "randtest.h"

std::string loadPathAsString(std::filesystem::path &fpath,
                             bool decompress = true) {
    uint32_t tg;
    std::ifstream ifs(fpath, std::ios::binary);
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string s = ss.str();
    ifs.close();

    if (decompress)
        tg = iftb::decodeBuffer(NULL, 0, s);
    else
        tg = tag(s.data());

    if (tg != 0x00010000 && tg != tag("OTTO") &&
        tg != tag("IFTC") && tg != tag("IFTB") && tg != tag("IFTZ"))
        throw std::runtime_error("Error: Unrecognized file/chunk type");

    return s;
}

void addChunks(iftb::client &cl, std::vector<uint16_t> &chunks,
               bool useRangeFile = false) {
    std::string cs;
    if (useRangeFile) {
        std::filesystem::path rpath = cl.getRangeFileURI();
        std::ifstream rs(rpath, std::ios::binary);
        for (auto cidx: chunks) {
            if (cidx >= cl.getChunkCount()) {
                std::cerr << cidx << " is greater than Chunk Count ";
                std::cerr << cl.getChunkCount() << std::endl;
                continue;
            }
            auto [cstart, cend] = cl.getChunkRange(cidx);
            uint32_t clen = cend - cstart;
            cs.resize(clen);
            rs.seekg(cstart);
            rs.read(cs.data(), clen);
            if (!cl.addChunk(cidx, cs, true)) {
                std::cerr << "Problem merging chunk " << cidx;
                std::cerr << ", stopping." << std::endl;
                std::exit(1);
            }
        }
    } else {
        for (auto cidx: chunks) {
            if (cidx >= cl.getChunkCount()) {
                std::cerr << cidx << " is greater than Chunk Count ";
                std::cerr << cl.getChunkCount() << std::endl;
                continue;
            }
            std::filesystem::path cp = cl.getChunkURI(cidx);
            cs = loadPathAsString(cp, false);
            if (!cl.addChunk(cidx, cs, true)) {
                std::cerr << "Problem merging chunk " << cidx;
                std::cerr << ", stopping." << std::endl;
                std::exit(1);
            }
        }
    }
}

void convertToWOFF2(std::string &s) {
    size_t woff2_size = woff2::MaxWOFF2CompressedSize((uint8_t *)s.data(),
                                                      s.size());
    std::string woff2_out(woff2_size, 0);
    woff2::WOFF2Params params;
    params.preserve_table_order = true;
    if (!woff2::ConvertTTFToWOFF2((uint8_t *)s.data(), s.size(),
                                  (uint8_t *)woff2_out.data(),
                                  &woff2_size, params))
        throw std::runtime_error("Could not WOFF2 compress font");
    woff2_out.resize(woff2_size);
    s.swap(woff2_out);
}

int dispatch(argparse::ArgumentParser &program, iftb::config &conf) {
    int r;
    if (program.is_subcommand_used("check")) {
        auto check = program.at<argparse::ArgumentParser>("check");
        std::filesystem::path fpath = check.get<std::string>("base_file");
        std::string fs = loadPathAsString(fpath);

        r = iftb::sanitize(fs, conf) ? 0 : 1;
        if (r)
            std::cerr << "Error: File failed sanitization" << std::endl;
        else
            std::cerr << "File passed sanitization" << std::endl;
    } else if (program.is_subcommand_used("process")) {
        auto process = program.at<argparse::ArgumentParser>("process");
        std::filesystem::path fpath = process.get<std::string>("font_file");
        std::filesystem::path prefix;
        std::string fs;
        iftb::chunker ck(conf);

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

        iftb::sfnt sft(fs);
        sft.read();
        simplestream ss;
        if (!sft.getTableStream(ss, T_IFTB))
            throw std::runtime_error("No IFTB table in font file");

        iftb::table_IFTB tiftb;
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
                css.str(iftb::decodeChunk(cfz.data(), cfz.size()));
                std::cerr << std::endl << cidx << std::endl;
                iftb::dumpChunk(std::cerr, css);
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
                iftb::dumpChunk(std::cerr, css);
            }
        }
    } else if (program.is_subcommand_used("merge")) {
        auto merge = program.at<argparse::ArgumentParser>("merge");
        auto chunks = merge.get<std::vector<uint16_t>>("indexes");
        std::filesystem::path fpath = merge.get<std::string>("base_file");
        std::string fs = loadPathAsString(fpath);

        iftb::client cl;
        if (!cl.loadFont(fs))
            std::exit(1);

        std::filesystem::path ocwd = std::filesystem::current_path();
        std::filesystem::current_path(fpath.parent_path());

        addChunks(cl, chunks, merge["-r"] == true);

        if (!cl.canMerge()) {
            std::cerr << "Client reports it can't merge, stopping";
            std::cerr << std::endl;
            std::exit(1);
        }
        if (!cl.merge(merge["-l"] == false)) {
            std::cerr << "Problem merging, stopping" << std::endl;
            std::exit(1);
        }
        std::filesystem::current_path(ocwd);
        std::string &nfs = cl.getFontAsString();
        if (merge["-w"] == true)
            convertToWOFF2(nfs);
        std::filesystem::path opath = merge.get<std::string>("-o");
        std::ofstream os;
        os.open(opath, std::ios::out | std::ios::binary);
        os.write(nfs.data(), nfs.size());
        os.close();
        std::cerr << "Wrote output file " << opath << std::endl;
        r = 0;
    } else if (program.is_subcommand_used("preload")) {
        auto preload = program.at<argparse::ArgumentParser>("preload");
        std::filesystem::path fpath = preload.get<std::string>("base_file");
        std::string confTag = preload.get<std::string>("tag");
        std::string fs = loadPathAsString(fpath);

        conf.load(program.get<std::string>("-c"), !program.is_used("-c"));

        std::set<uint32_t> unicodes;
        if (!conf.unicodesForPreload(confTag, unicodes)) {
            std::cerr << "No preloads for tag '" << confTag;
            std::cerr << "', stopping" << std::endl;
            std::exit(1);
        }
        std::vector<uint32_t> unilist, features;
        unilist.reserve(unicodes.size());
        for (auto cp: unicodes)
            unilist.push_back(cp);

        iftb::client cl;
        if (!cl.loadFont(fs))
            std::exit(1);

        cl.setPending(unilist, features);
        std::vector<uint16_t> chunks;
        cl.getPendingChunkList(chunks);

        std::filesystem::path ocwd = std::filesystem::current_path();
        std::filesystem::current_path(fpath.parent_path());

        addChunks(cl, chunks, preload["-r"] == true);

        if (!cl.canMerge()) {
            std::cerr << "Client reports it can't merge, stopping";
            std::cerr << std::endl;
            std::exit(1);
        }
        if (!cl.merge(preload["-l"] == false)) {
            std::cerr << "Problem merging, stopping" << std::endl;
            std::exit(1);
        }
        std::filesystem::current_path(ocwd);

        std::string &nfs = cl.getFontAsString();
        if (preload["-w"] == true)
            convertToWOFF2(nfs);

        std::filesystem::path opath;
        if (auto oname = preload.present("-o")) {
            opath = *oname;
        } else {
            opath = fpath;
            std::filesystem::path ofname = opath.stem();
            ofname += confTag;
            if (preload["-w"] == true)
                ofname.replace_extension("woff2");
            else
                ofname.replace_extension(cl.isCFF() ? "otf" : "ttf");
            opath.replace_filename(ofname);
        }
        std::ofstream os;
        os.open(opath, std::ios::out | std::ios::binary);
        os.write(nfs.data(), nfs.size());
        os.close();
        std::cerr << "Wrote output file " << opath << std::endl;
        r = 0;
    } else if (program.is_subcommand_used("stress-test")) {
        auto stresstest = program.at<argparse::ArgumentParser>("stress-test");
        std::filesystem::path fpath = stresstest.get<std::string>("base_file");
        std::string fs = loadPathAsString(fpath);

        if (iftb::randtest(fs)) {
            std::cerr << "File passed stress tests" << std::endl;
            r = 0;
        } else {
            std::cerr << "File failed stress tests" << std::endl;
            r = 1;
        }
    } else {
        std::cerr << "Error: No command specified" << std::endl;
        std::cerr << program;
        r = 1;
    }
    return r;
}

int main(int argc, char **argv) {
    iftb::config conf;

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
    dumpchunks.add_argument("-r", "--by-range")
              .help("Retrieve the chunk from the range file")
              .default_value(false)
              .implicit_value(true);
    dumpchunks.add_argument("indexes")
              .help("A list of positive integer indexes")
              .nargs(argparse::nargs_pattern::at_least_one)
              .required()
              .scan<'u', uint16_t>();

    argparse::ArgumentParser merge("merge");
    merge.add_description("Merge chunks by index into the base");
    merge.add_argument("base_file")
         .help("The IFTB (binned) input file");
    merge.add_argument("-o", "--output-filename")
         .help("(Mandatory) filename for merged file")
         .required();
    merge.add_argument("indexes")
         .help("A list of positive integer indexes")
         .nargs(argparse::nargs_pattern::at_least_one)
         .required()
         .scan<'u', uint16_t>();
    merge.add_argument("-r", "--by-range")
         .help("Retrieve the chunk from the range file")
         .default_value(false)
         .implicit_value(true);
    merge.add_argument("-l", "--for-loading")
         .help("Set the sfnt version to OpenType/TrueType (instead of IFTB)")
         .default_value(false)
         .implicit_value(true);
    merge.add_argument("-w", "--woff2")
         .help("Output WOFF2")
         .default_value(false)
         .implicit_value(true);

    argparse::ArgumentParser preload("preload");
    preload.add_description("Preload the file by config tag");
    preload.add_argument("base_file")
         .help("The IFTB (binned) input file");
    preload.add_argument("tag")
         .help("The config tag to preload");
    preload.add_argument("-o", "--output-filename")
         .help("filename for preloaded file");
    preload.add_argument("-r", "--by-range")
         .help("Retrieve the chunk from the range file")
         .default_value(false)
         .implicit_value(true);
    preload.add_argument("-l", "--for-loading")
         .help("Set the sfnt version to OpenType/TrueType (instead of IFTB)")
         .default_value(false)
         .implicit_value(true);
    preload.add_argument("-w", "--woff2")
         .help("Output WOFF2")
         .default_value(false)
         .implicit_value(true);

    argparse::ArgumentParser stresstest("stress-test");
    stresstest.add_description("Test binning algorithm against random "
                               "codepoint/feature combinations");
    stresstest.add_argument("base_file")
              .help("The IFTB (binned) input file");

    program.add_subparser(process);
    program.add_subparser(check);
    program.add_subparser(merge);
    program.add_subparser(preload);
    program.add_subparser(dumpchunks);
    program.add_subparser(stresstest);

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
