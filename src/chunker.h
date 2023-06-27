/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

/* This is the encoder implementation, which not used on the client-side.
 */

#include <unordered_map>
#include <filesystem>

#include "chunk.h"
#include "merger.h"
#include "config.h"
#include "wrappers.h"

#pragma once

#define DEFOUTDIR "subset_tmp"

namespace iftb {
    class chunker;
}

class iftb::chunker {
 public:
    chunker(iftb::config &c) : conf(c) {}
    // Builds the IFTB-encoded files from the font in input_string
    int process(std::string &input_string);
    ~chunker() {
        if (nominal_map)
            hb_map_destroy(nominal_map);
        if (all_codepoints)
            hb_map_destroy(all_codepoints);
        if (all_gids)
            hb_map_destroy(all_gids);
    }
 private:
    iftb::config &conf;
    iftb::wr_blob inblob, subblob;
    iftb::wr_face inface, subface, proface;
    iftb::wr_font subfont;
    iftb::wr_subset_input input;
    uint32_t glyph_count;
    int cff_charstrings_offset = -1;

    iftb::wr_blob primaryBlob, secondaryBlob, locaBlob;
    std::vector<iftb::merger::glyphrec> primaryRecs, secondaryRecs;

    std::vector<iftb::chunk> chunks;

    hb_map_t *nominal_map = NULL, *all_codepoints = NULL, *all_gids = NULL;
    std::unordered_multimap<uint32_t, uint32_t> nominal_revmap;
    iftb::wr_set all_features;
    iftb::wr_set tables;
    iftb::wr_set unicodes_face, gid_closure, gid_with_point;

    bool is_cff = false, is_variable = false;

    uint32_t gid_size(uint32_t gid);
    uint32_t gid_set_size(const iftb::wr_set&s);
    void add_chunk(iftb::chunk &ch, hb_map_t *gid_chunk_map);
    void make_group_chunks(int group_num, hb_map_t *gid_chunk_map,
                           iftb::wr_set &remaining_points,
                           std::unique_ptr<iftb::group_wrapper> &g);
    void process_overlaps(uint32_t gid, uint32_t chid,
                          std::vector<uint32_t> &v);
    void process_candidates(uint32_t last_gid, iftb::wr_set &remaining_gids,
                            std::vector<uint32_t> &v);
    void process_feature_candidates(uint32_t feat, iftb::wr_set &gids,
                                    uint32_t gid,
                                    std::map<uint32_t, iftb::chunk> &fchunks,
                                    std::vector<uint32_t> &v);
    iftb::chunk &current_chunk(uint32_t &chid);
};
