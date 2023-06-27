/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

/* The iftb::chunk object tracks the data to be added to
   an IFTC chunk string during the encoding process. It is
   only used in the encoder, not the server-side.
 */

#include <iostream>
#include <vector>

#include "merger.h"
#include "wrappers.h"

#pragma once

namespace iftb {
    class chunk;
}

class iftb::chunk {
 public:
    chunk() {}
    friend class iftb::chunker;
    chunk(const chunk &c) = delete;
    chunk(chunk &&c) {
        codepoints.s = c.codepoints.s;
        c.codepoints.s = hb_set_create();
        gids.s = c.gids.s;
        c.gids.s = hb_set_create();
        group = c.group;
        feat = c.feat;
        size = c.size;
        from_min = c.from_min;
        from_max = c.from_max;
        merged_to = c.merged_to;
    }
    void reset() {
        codepoints.clear();
        gids.clear();
        group = 0;
        feat = 0;
        size = 0;
        from_min = from_max = 0;
        merged_to = (uint32_t) -1;
    }
    // Merge the argument into this chunk
    void merge(chunk &c) {
        assert(feat == c.feat);
        assert(c.merged_to == -1);
        codepoints._union(c.codepoints);
        gids._union(c.gids);
        size += c.size;
        assert(feat == 0 || (from_max < c.from_min));
        from_max = c.from_max;
    }
    // True if this chunk be merged with the argument
    bool mergeable(chunk &c, bool permissive = false) {
        if (feat != c.feat)
            return false;
        if (feat != 0) {
            // Only merge directly adjacent feature chunks
            return (from_max + 1) == c.from_min;
        }
        return (permissive || group == c.group);
    }

    // Add the string of this chunk to ostream os at offset offset
    void compile(std::ostream &os, uint16_t idx,
                 uint32_t *id,
                 uint32_t table1,
                 std::vector<iftb::merger::glyphrec> &recs1,
                 uint32_t table2,
                 std::vector<iftb::merger::glyphrec> &recs2,
                 uint32_t offset = 0);
    static std::string encode(std::stringstream &ss);
 private:
    // Unicode codepoints that map to this chunk
    iftb::wr_set codepoints;
    // GIDs included in this chunk
    iftb::wr_set gids;
    uint16_t group = 0;  // Group number (from config)
    uint32_t feat = 0;   // If non-zero, specific to feature with this tag.
    uint32_t size = 0;   // Current (approxmiate) uncompressed size.
    uint16_t from_min = 0, from_max = 0; // If feat != 0, the non-feature
                                         // chunks this chunk maps to.
    // If this chunk was merged into another, the ID of the latter.
    uint32_t merged_to = (uint32_t) -1;
};
