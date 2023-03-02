#include <unordered_map>
#include <filesystem>

#include "chunk.h"
#include "merger.h"
#include "config.h"
#include "wrappers.h"

#pragma once

#define DEFOUTDIR "subset_tmp"

class iftb_chunker {
 public:
    iftb_chunker(iftb_config &c) : conf(c) {}
    int process(std::string &input_string);
    ~iftb_chunker() {
        if (nominal_map)
            hb_map_destroy(nominal_map);
        if (all_codepoints)
            hb_map_destroy(all_codepoints);
        if (all_gids)
            hb_map_destroy(all_gids);
    }
 private:
    iftb_config &conf;
    wr_blob inblob, subblob;
    wr_face inface, subface, proface;
    wr_font subfont;
    wr_subset_input input;
    uint32_t glyph_count;
    int cff_charstrings_offset = -1;

    wr_blob primaryBlob, secondaryBlob, locaBlob;
    std::vector<iftb_merger::glyphrec> primaryRecs, secondaryRecs;

    std::vector<iftb_chunk> chunks;

    hb_map_t *nominal_map = NULL, *all_codepoints = NULL, *all_gids = NULL;
    std::unordered_multimap<uint32_t, uint32_t> nominal_revmap;
    wr_set all_features;
    wr_set tables;
    wr_set unicodes_face, gid_closure, gid_with_point;

    bool is_cff = false, is_variable = false;

    uint32_t gid_size(uint32_t gid);
    uint32_t gid_set_size(const wr_set&s);
    void add_chunk(iftb_chunk &ch, hb_map_t *gid_chunk_map);
    void make_group_chunks(int group_num, hb_map_t *gid_chunk_map,
                           wr_set &remaining_points,
                           std::unique_ptr<iftb_group_wrapper> &g);
    void process_overlaps(uint32_t gid, uint32_t chid,
                          std::vector<uint32_t> &v);
    void process_candidates(uint32_t last_gid, wr_set &remaining_gids,
                            std::vector<uint32_t> &v);
    void process_feature_candidates(uint32_t feat, wr_set &gids, uint32_t gid,
                                    std::map<uint32_t, iftb_chunk> &fchunks,
                                    std::vector<uint32_t> &v);
    iftb_chunk &current_chunk(uint32_t &chid);
};
