#include <vector>
#include <filesystem>

#include "yaml-cpp/yaml.h"

#include "wrappers.h"

#pragma once

namespace iftb {
    struct group_wrapper;
    class config;
    typedef std::vector<std::unique_ptr<group_wrapper>> wrapped_groups;
    class chunker;
}

struct iftb::group_wrapper {
    virtual bool next(uint32_t &gid) = 0;
    virtual ~group_wrapper() = default;
};

class iftb::config {
 public:
    friend class iftb::chunker;
    std::filesystem::path inputPath() { return _inputPath; }
    std::filesystem::path rangePath() { return pathPrefix / rangeFilename; }
    std::filesystem::path chunkPath(uint16_t idx) {
        std::filesystem::path r = pathPrefix;
        char buf[10]; 
        snprintf(buf, sizeof(buf), "%0*x", (int) chunk_hex_digits, (int) idx);
        for (int i = 0; i < chunk_hex_digits - 2; i++) {
            std::string l {buf[i]};
            r /= l;
        }
        r /= "chunk";
        r += buf;
        r += ".br";
        return r;
    }
    void increaseVerbosity() { if (_verbosity < 3) _verbosity++; }
    uint8_t verbosity() { return _verbosity; }
    std::string filesURI() { 
        char buf[10];
        std::string s("./");
        s += pathPrefix.filename();
        s += "/";
        for (int i = chunk_hex_digits; i > 2; i--) {
            snprintf(buf, sizeof(buf), "$%d/", i);
            s += buf;
        }
        s += "chunk";
        for (int i = chunk_hex_digits; i > 0; i--) { 
            snprintf(buf, sizeof(buf), "$%d", i);
            s += buf;
        }
        s += ".br";
        return s;
    }

    std::string rangeFileURI() {
        std::string s("./");
        s += pathPrefix.filename();
        s += "/";
        s += rangeFilename;
        return s;
    }

    std::filesystem::path subsetPath(bool is_cff) {
        std::filesystem::path r = pathPrefix;
        r.replace_extension(is_cff ? "otf" : "ttf");
        return r;
    }
    std::filesystem::path woff2Path() {
        std::filesystem::path r = pathPrefix;
        r.replace_extension("woff2");
        return r;
    }

    void setPathPrefix(std::filesystem::path &p) {
        pathPrefix = p;
        prepDir(pathPrefix, false);
    }

    int load(std::string p, bool is_default);

    void makeChunkDirs() {
        if (chunk_hex_digits > 2)
            makeChunkSubDirs(pathPrefix, chunk_hex_digits - 2);
    }

    void makeChunkSubDirs(std::filesystem::path &p, uint8_t depth) {
        const char *key = "0123456789abcdef";
        for (int i = 0; i < 16; i++) {
            std::string l{key[i]};
            std::filesystem::path a = p / l;
            prepDir(a);
            if (depth > 1)
                makeChunkSubDirs(a, depth-1);
        }
    }

    bool unicodesForPreload(std::string &tag,
                            std::set<uint32_t> &unicodes);
    bool prepDir(std::filesystem::path &p, bool thrw = true);
    void load_points(YAML::Node n, iftb::wr_set &s);
    void load_ordered_points(YAML::Node n, std::vector<uint32_t> &s);
    void setNumChunks(uint16_t numChunks);
    bool subset_feature(uint32_t s) { return s >= feat_subset_cutoff; }
    bool desubroutinize() { return true; }
    bool namelegacy() { return true; }
    bool passunrecognized() { return false; }
    bool allgids() { return false; }
    bool printConfig() { return true; }
    bool noCatch() { return true; }
    uint32_t mini_targ() { return target_chunk_size / 4; }
 private:
    struct point_group_info {
        point_group_info() {}
        std::string name;
        std::vector<std::string> preloads;
        bool is_ordered = false;
        uint16_t index = 0;
    };
    struct vec_wrapper : iftb::group_wrapper {
        vec_wrapper(std::vector<uint32_t> &v) : v(v) {
            i = v.begin();
        }
        virtual ~vec_wrapper() = default;
        bool next(uint32_t &gid) override {
            if (i != v.end()) {
                gid = *i;
                i++;
                return true;
            } else
                return false;
        }
        std::vector<uint32_t> &v;
        std::vector<uint32_t>::iterator i;
    };
    struct set_wrapper : iftb::group_wrapper {
        virtual ~set_wrapper() = default;
        iftb::wr_set &s;
        set_wrapper(iftb::wr_set &s) : s(s) {}
        bool next(uint32_t &gid) override {
            return s.next(gid);
        }
    };
    void get_groups(iftb::wrapped_groups &pg) {
        for (auto &i: ordered_point_groups)
            pg.emplace_back(std::make_unique<vec_wrapper>(i));
        for (auto &i: point_groups)
            pg.emplace_back(std::make_unique<set_wrapper>(i));
    }
    void loadPointGroup(YAML::Node n, bool is_ordered);
    uint8_t _verbosity = 0;
    iftb::wr_set base_points, used_points;
    std::vector<point_group_info> group_info;
    std::vector<std::vector<uint32_t>> ordered_point_groups;
    std::vector<iftb::wr_set> point_groups;
    uint32_t feat_subset_cutoff = 0xFFFF;
    uint32_t target_chunk_size = 0x8FFF;
    uint8_t chunk_hex_digits = 0;
    uint8_t chunk_dir_levels = 0;
    std::string rangeFilename = "rangefile";
    std::filesystem::path _inputPath, pathPrefix;
};
