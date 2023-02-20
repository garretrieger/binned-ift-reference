
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "tag.h"
#include "wrappers.h"
#include "config.h"

void prep(config &conf, const char *fname, bool desubroutinize = true) {
    hb_set_t *t;
    blob inblob;
    face inface;
    font infont;
    subset_input input;
    set tables;
    uint32_t glyph_count;
    bool is_cff = false, is_variable = false;

    if (std::filesystem::exists(conf.output_dir) &&
        !std::filesystem::is_directory(conf.output_dir)) {
        std::string s = "Error: Path '";
        s += conf.output_dir;
        s += "' exists but is not a directory";
        throw std::runtime_error(s);
    }
    std::filesystem::create_directory(conf.output_dir);

    inblob.load(fname);
    inface.create(inblob);
    glyph_count = inface.get_glyph_count();
    infont.create(inface);
    inface.get_table_tags(tables);

    // Determine font type
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

    std::cerr << "Glyph count: " << glyph_count << std::endl;

    int flags = HB_SUBSET_FLAGS_DEFAULT;
    flags |= HB_SUBSET_FLAGS_RETAIN_GIDS 
             | HB_SUBSET_FLAGS_NAME_LEGACY
             | HB_SUBSET_FLAGS_PASSTHROUGH_UNRECOGNIZED
             | HB_SUBSET_FLAGS_NOTDEF_OUTLINE
             | HB_SUBSET_FLAGS_GLYPH_NAMES
             | HB_SUBSET_FLAGS_NO_PRUNE_UNICODE_RANGES
             ;
    if (desubroutinize)
        flags |= HB_SUBSET_FLAGS_DESUBROUTINIZE;
    input.set_flags(flags);

    t = input.unicode_set();
    hb_set_clear(t);
    hb_set_invert(t);
    t = input.set(HB_SUBSET_SETS_NO_SUBSET_TABLE_TAG);
    hb_set_set(t, tables.s);
    hb_set_del(t, T_CFF);
    hb_set_del(t, T_CFF2);
    hb_set_del(t, T_GLYF);
    hb_set_del(t, T_GVAR);
    t = input.set(HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    hb_set_clear(t);
    hb_set_invert(t);
    t = input.set(HB_SUBSET_SETS_GLYPH_INDEX);
    hb_set_clear(t);
    hb_set_invert(t);

    hb_face_t *outface = hb_subset_or_fail(inface.f, input.i);
    hb_blob_t *outblob = hb_face_reference_blob(outface);

    std::filesystem::path filepath = conf.output_dir;
    filepath /= "prepped.otf";
    std::ofstream myfile;
    myfile.open(filepath, std::ios::trunc | std::ios::binary);
    unsigned int size;
    const char *data = hb_blob_get_data(outblob, &size);
    myfile.write(data, size);
    myfile.close();
}

int main(int argc, char **argv) {
    config c;

    if (argc < 2) {
        std::cerr << "Must supply font name as argument" << std::endl;
        return 1;
    }

    try {
        c.load();
        prep(c, argv[1], true);
    } catch (const std::exception &ex ) {
        std::cerr << ex.what() << std::endl;
    }
}
