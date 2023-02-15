#include <cassert>

#include "wrappers.h"

#pragma once

struct chunk {
    set codepoints;
    set gids;
    uint16_t group = 0;
    uint32_t feat = 0;
    uint32_t size = 0;
    uint16_t from_min = 0, from_max = 0;
    uint32_t merged_to = (uint32_t) -1;
    chunk(const chunk &c) = delete;
    chunk() {}
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
    void merge(chunk &c, bool permissive = false) {
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
};
