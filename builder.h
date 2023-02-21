#include <unordered_map>
#include <filesystem>

#include "chunk.h"
#include "merger.h"
#include "config.h"
#include "wrappers.h"

#pragma once

#define DEFOUTDIR "subset_tmp"

class builder {
 public:
    builder(config &c) : conf(c) {}
    void process(std::filesystem::path &fname);
    void check_write();
    void write();
    ~builder() {
        if (nominal_map)
            hb_map_destroy(nominal_map);
        if (all_codepoints)
            hb_map_destroy(all_codepoints);
        if (all_gids)
            hb_map_destroy(all_gids);
    }
 private:
    config &conf;
    blob inblob, subblob;
    face inface, subface, proface;
    font subfont;
    subset_input input;
    uint32_t glyph_count;
    int cff_charstrings_offset = -1;

    uint32_t primaryOffset = 0, secondaryOffset = 0;
    blob primaryBlob, secondaryBlob;
    std::vector<glyphrec> primaryRecs, secondaryRecs;

    std::vector<chunk> chunks;

    hb_map_t *nominal_map = NULL, *all_codepoints = NULL, *all_gids = NULL;
    std::unordered_multimap<uint32_t, uint32_t> nominal_revmap;
    set all_features;
    set tables;
    set unicodes_face, gid_closure, gid_with_point;

    bool is_cff = false, is_variable = false;

    uint32_t gid_size(uint32_t gid);
    uint32_t gid_set_size(const set&s);
    void add_chunk(chunk &ch, hb_map_t *gid_chunk_map);
    void make_group_chunks(int group_num, hb_map_t *gid_chunk_map,
                           set &remaining_points,
                           std::unique_ptr<group_wrapper> &g);
    void process_overlaps(uint32_t gid, uint32_t chid,
                          std::vector<uint32_t> &v);
    void process_candidates(uint32_t last_gid, set &remaining_gids,
                            std::vector<uint32_t> &v);
    void process_feature_candidates(uint32_t feat, set &gids, uint32_t gid,
                                    std::map<uint32_t, chunk> &fchunks,
                                    std::vector<uint32_t> &v);
    chunk &current_chunk(uint32_t &chid);
};
