#include <cassert>
#include <iostream>
#include <vector>

#include "unchunk.h"
#include "wrappers.h"

#pragma once

class chunk {
public:
    chunk() {}
    friend class chunker;
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
    void compile(std::ostream &os, uint16_t idx,
                 uint32_t *id,
                 uint32_t table1, std::vector<glyphrec> &recs1,
                 uint32_t table2, std::vector<glyphrec> &recs2,
                 uint32_t offset = 0);
    static std::string encode(std::stringstream &ss);
private:
    set codepoints;
    set gids;
    uint16_t group = 0;
    uint32_t feat = 0;
    uint32_t size = 0;
    uint16_t from_min = 0, from_max = 0;
    uint32_t merged_to = (uint32_t) -1;
};
