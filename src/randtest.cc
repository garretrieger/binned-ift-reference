
#include <iostream>
#include <random>
#include <functional>

#include "client.h"
#include "wrappers.h"

static uint16_t randtt() {
    static auto gen = std::bind(std::uniform_int_distribution<>(0, 9999),
                                std::default_random_engine());
    return gen();
}

bool iftb::randtest(std::string &input_string, uint32_t iterations) {
    using namespace iftb;
    wr_blob inblob;
    wr_face inface, proface;
    wr_set all_features, all_codepoints, some_gids_hb, some_gids_iftb;
    wr_subset_input input;
    hb_set_t *t;
    uint32_t u32;
    unsigned int flags;
    std::vector<uint32_t> some_features, some_codepoints;
    std::vector<uint16_t> pending_chunks;
    std::vector<wr_set> gid_sets;

    inblob.from_string(input_string, true);
    inface.create(inblob);

    iftb::client cl;
    if (!cl.loadFont(input_string, true))
        return false;

    gid_sets.resize(cl.getChunkCount());
    for (int i = 0; i < cl.tiftb.gidMap.size(); i++) {
        auto q = cl.tiftb.gidMap[i];
        gid_sets[cl.tiftb.gidMap[i]].add(i);
    }

    // Read in the feature tags
    {
        hb_tag_t ftags[16];
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

    hb_face_collect_unicodes(inface.f, all_codepoints.s);

    proface.create_preprocessed(inface);

    // Re-initialize input
    flags = HB_SUBSET_FLAGS_DEFAULT;
    flags |= HB_SUBSET_FLAGS_RETAIN_GIDS
             | HB_SUBSET_FLAGS_NAME_LEGACY
             | HB_SUBSET_FLAGS_PASSTHROUGH_UNRECOGNIZED
             | HB_SUBSET_FLAGS_NOTDEF_OUTLINE
             | HB_SUBSET_FLAGS_GLYPH_NAMES
             | HB_SUBSET_FLAGS_NO_PRUNE_UNICODE_RANGES
             ;
    input.set_flags(flags);

    while (iterations) {
        std::cerr << "Iteration " << iterations--;
        t = input.unicode_set();
        hb_set_clear(t);
        some_codepoints.clear();
        u32 = HB_SET_VALUE_INVALID;
        while (all_codepoints.next(u32))
            if (randtt() > 9990) {
                hb_set_add(t, u32);
                some_codepoints.push_back(u32);
            }
        t = input.set(HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
        hb_set_clear(t);
        some_features.clear();
        u32 = HB_SET_VALUE_INVALID;
        while (all_features.next(u32))
            if (randtt() > 4999) {
                hb_set_add(t, u32);
                some_features.push_back(u32);
            }

        // Get transitive gid substitution closure for all points and features
        hb_subset_plan_t *plan = hb_subset_plan_create_or_fail(proface.f, input.i);
        hb_map_t *map = hb_subset_plan_old_to_new_glyph_mapping(plan);
        some_gids_hb.clear();
        hb_map_keys(map, some_gids_hb.s);
        hb_subset_plan_destroy(plan);

        cl.setPending(some_codepoints, some_features);
        cl.getPendingChunkList(pending_chunks);
        std::cerr << ": " << pending_chunks.size() << " chunks" << std::endl;
        some_gids_iftb.copy(gid_sets[0]);
        for (auto ck: pending_chunks)
            some_gids_iftb._union(gid_sets[ck]);

        some_gids_hb.subtract(some_gids_iftb);

        if (!some_gids_hb.is_empty()) {
            u32 = -1;
            std::cerr << "GIDs: ";
            while (some_gids_hb.next(u32))
                std::cerr << u32 << " (" << cl.tiftb.gidMap[u32] << "), ";
            std::cerr << std::endl;
            assert(false);
        }
    }
    return true;
}
