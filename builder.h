#include <unordered_map>

#include "chunk.h"
#include "config.h"
#include "wrappers.h"

#pragma once

class builder {
 public:
    builder(config &c) : conf(c) {}
    void process(const char *fname);
    ~builder() { if (nominal_map) hb_map_destroy(nominal_map); }
 private:
    config &conf;
    blob inblob, outblob;
    face inface, subface, outface;
    font infont;
    subset_input input;
    uint32_t glyph_count;

    std::unordered_map<uint32_t, uint32_t> glyph_sizes;

    std::vector<chunk> chunks;

    hb_map_t *nominal_map = NULL;
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
