#include <cassert>
#include <iostream>
#include <vector>

#include "merger.h"
#include "wrappers.h"

#pragma once

class iftb_chunk {
 public:
    iftb_chunk() {}
    friend class iftb_chunker;
    iftb_chunk(const iftb_chunk &c) = delete;
    iftb_chunk(iftb_chunk &&c) {
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
    void merge(iftb_chunk &c, bool permissive = false) {
        assert(feat == c.feat);
        if (!permissive)
            assert(group == c.group);
        assert(c.merged_to == -1);
        codepoints._union(c.codepoints);
        gids._union(c.gids);
        size += c.size;
        assert(feat == 0 || (from_max < c.from_min));
        from_max = c.from_max;
    }
    void compile(std::ostream &os, uint16_t idx,
                 uint32_t *id,
                 uint32_t table1,
                 std::vector<iftb_merger::glyphrec> &recs1,
                 uint32_t table2,
                 std::vector<iftb_merger::glyphrec> &recs2,
                 uint32_t offset = 0);
    static std::string encode(std::stringstream &ss);
 private:
    wr_set codepoints;
    wr_set gids;
    uint16_t group = 0;
    uint32_t feat = 0;
    uint32_t size = 0;
    uint16_t from_min = 0, from_max = 0;
    uint32_t merged_to = (uint32_t) -1;
};
