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
    std::filesystem::path _inputPath;
    std::filesystem::path configFilePath = "config.yaml";
    std::filesystem::path path_prefix;
    std::filesystem::path inputPath() { return _inputPath; }
    std::filesystem::path rangePath() { return ""; }
    std::filesystem::path chunkPath(uint16_t idx) { return ""; }
    std::filesystem::path filesURI() { return ""; }
    std::filesystem::path rangeFileURI() { return ""; }
    std::filesystem::path subsetPath() { return ""; }

    int setArgs(int argc, char **argv);

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
