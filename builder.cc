
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "builder.h"
#include "tag.h"

std::unordered_set<uint32_t> default_features = { tag("abvm"), tag("blwm"), tag("ccmp"),
                                                  tag("locl"), tag("mark"), tag("mkmk"),
                                                  tag("rlig"), tag("calt"), tag("clig"),
                                                  tag("curs"), tag("dist"), tag("kern"),
                                                  tag("liga"), tag("rclt"), tag("numr") };

void builder::process(const char *fname) {
    inblob.load(fname);
    inface.create(inblob);
    glyph_count = inface.get_glyph_count();
    infont.create(inface);
    for (uint32_t i = 0; i < glyph_count; i++) {
        unsigned int l, vl;
        const char *vc;
        infont.get_glyph_content(i, l, vc, vl);
        glyph_sizes.try_emplace(i, l + vl);
    }

    inface.get_table_tags(tables);

    if (tables.has(T_CFF)) {
        if (tables.has(T_CFF2))
            throw std::runtime_error("Font has both CFF and CFF2 tables, exiting");
        else if (tables.has(T_GLYF))
            throw std::runtime_error("Font has both CFF and glyf tables, exiting");
        is_cff = true;
        std::cerr << "Processing CFF font" << std::endl;
    } else if (tables.has(T_CFF2)) {
        if (tables.has(T_GLYF))
            throw std::runtime_error("Font has both CFF and glyf tables, exiting");
        is_cff = is_variable = true;
        std::cerr << "Processing CFF2 font" << std::endl;
    } else if (tables.has(T_GLYF)) {
        if (tables.has(T_GVAR)) {
            is_variable = true;
            std::cerr << "Processing variable glyf font" << std::endl;
        } else {
            std::cerr << "Processing static glyf font" << std::endl;
        }
    } else
        throw std::runtime_error("No CFF, CFF2 or glyf table in font, exiting");

    hb_tag_t ftags[16];
    bool printed = false;
    unsigned int numtags = 16, offset = 0;
    while (true) {
        inface.get_feature_tags(HB_OT_TAG_GSUB, offset, numtags, ftags);
        if (numtags == 0)
            break;
        for (int i = 0; i < numtags; i++)
            all_features.add(ftags[i]);
        offset += numtags;
        numtags = 16;
    }
}
