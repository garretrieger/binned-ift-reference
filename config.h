#include <vector>
#include <filesystem>

#include "yaml-cpp/yaml.h"

#include "wrappers.h"

#pragma once

struct group_wrapper {
    virtual bool next(uint32_t &gid) = 0;
};

struct vec_wrapper : group_wrapper {
    std::vector<uint32_t> &v;
    std::vector<uint32_t>::iterator i;
    vec_wrapper(std::vector<uint32_t> &v) : v(v) {
        i = v.begin();
    }
    virtual bool next(uint32_t &gid) {
        if (i != v.end()) {
            gid = *i;
            i++;
            return true;
        } else
            return false;
    }
};

struct set_wrapper : group_wrapper {
    set &s;
    set_wrapper(set &s) : s(s) {}
    virtual bool next(uint32_t &gid) {
        return s.next(gid);
    }
};

typedef std::vector<std::unique_ptr<group_wrapper>> wrapped_groups;

struct config {
    set base_points, used_points;
    std::vector<std::vector<uint32_t>> ordered_point_groups;
    std::vector<set> point_groups;
    uint32_t feat_subset_cutoff = 0xFFFF;
    uint32_t target_chunk_size = 0x8FFF;
    uint8_t chunk_hex_digits = 0;
    uint8_t chunk_dir_levels = 0;
    std::string rangeFilename = "rangefile";
    std::filesystem::path _inputPath, pathPrefix;
    std::filesystem::path configFilePath = "config.yaml";
    std::filesystem::path path_prefix;
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
    std::string filesURI() { 
        char buf[10];
        std::string s("./");
        s += pathPrefix.filename();
        s += "/";
        for (int i = chunk_hex_digits-1; i >= 2; i--) {
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

    int setArgs(int argc, char **argv);

    void makeChunkDirs() {
        if (chunk_hex_digits > 2)
            makeChunkSubDirs(pathPrefix, chunk_hex_digits - 2);
    }

    void makeChunkSubDirs(std::filesystem::path &p, uint8_t depth) {
        const char *key = "0123456789abcdef";
        for (int i = 0; i < 16; i++) {
            std::string l{key[i]};
            std::filesystem::path a = p / l;
            if (depth > 0)
                makeChunkSubDirs(a, depth-1);
        }
    }

    bool prepDir(std::filesystem::path &p, bool thrw = true);
    void load_points(YAML::Node n, set &s);
    void load_ordered_points(YAML::Node n, std::vector<uint32_t> &s);
    void setNumChunks(uint16_t numChunks);
    bool subset_feature(uint32_t s) { return s >= feat_subset_cutoff; }
    bool desubroutinize() { return true; }
    bool namelegacy() { return true; }
    bool passunrecognized() { return false; }
    bool allgids() { return false; }
    bool printConfig() { return true; }
    bool noCatch() { return true; }
    void get_groups(wrapped_groups &pg) {
        for (auto &i: ordered_point_groups)
            pg.emplace_back(std::make_unique<vec_wrapper>(i));
        for (auto &i: point_groups)
            pg.emplace_back(std::make_unique<set_wrapper>(i));
    }
    uint32_t mini_targ() { return target_chunk_size / 2; }
};
