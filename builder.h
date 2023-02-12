#include <unordered_map>

#include "config.h"
#include "wrappers.h"

#pragma once

struct builder {
    config &conf;
    blob inblob, outblob;
    face inface, outface;
    font infont;
    subset_input input;
    uint32_t glyph_count;

    std::unordered_map<uint32_t, uint32_t> glyph_sizes;

    set all_features, subset_features, tables;
    bool is_cff = false, is_variable = false;

    builder(config &c) : conf(c) {}
    void process(const char *fname);
};
