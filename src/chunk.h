#include <cassert>
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
    void merge(chunk &c) {
        assert(feat == c.feat);
        assert(c.merged_to == -1);
        codepoints._union(c.codepoints);
        gids._union(c.gids);
        size += c.size;
        assert(feat == 0 || (from_max < c.from_min));
        from_max = c.from_max;
    }
    bool mergeable(chunk &c, bool permissive = false) {
        if (feat != c.feat)
            return false;
        if (feat != 0) {
            // Only merge directly adjacent feature chunks
            return (from_max + 1) == c.from_min;
        }
        return (permissive || group == c.group);
    }

    void compile(std::ostream &os, uint16_t idx,
                 uint32_t *id,
                 uint32_t table1,
                 std::vector<iftb::merger::glyphrec> &recs1,
                 uint32_t table2,
                 std::vector<iftb::merger::glyphrec> &recs2,
                 uint32_t offset = 0);
    static std::string encode(std::stringstream &ss);
 private:
    iftb::wr_set codepoints;
    iftb::wr_set gids;
    uint16_t group = 0;
    uint32_t feat = 0;
    uint32_t size = 0;
    uint16_t from_min = 0, from_max = 0;
    uint32_t merged_to = (uint32_t) -1;
};
