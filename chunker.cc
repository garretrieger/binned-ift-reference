
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <brotli/encode.h>
#include <woff2/encode.h>

#include "chunker.h"
#include "tag.h"
#include "streamhelp.h"
#include "table_IFTC.h"

std::unordered_set<uint32_t> default_features = { tag("abvm"), tag("blwm"),
                                                  tag("ccmp"), tag("locl"),
                                                  tag("mark"), tag("mkmk"),
                                                  tag("rlig"), tag("calt"),
                                                  tag("clig"), tag("curs"),
                                                  tag("dist"), tag("kern"),
                                                  tag("liga"), tag("rclt"),
                                                  tag("numr") };

uint32_t chunker::gid_size(uint32_t gid) {
    uint32_t r = primaryRecs[gid].length;
    if (!is_cff && is_variable) {
        assert(secondaryRecs.size() > gid);
        r += secondaryRecs[gid].length;
    }
    return r;
}

uint32_t chunker::gid_set_size(const set &s) {
    uint32_t r = 0, gid = HB_SET_VALUE_INVALID;

    while (s.next(gid)) {
        r += gid_size(gid);
    }
    return r;
}

void chunker::add_chunk(chunk &ch, hb_map_t *gid_chunk_map) {
    uint32_t group_num = ch.group;
    uint32_t idx = chunks.size();
    uint32_t k = HB_SET_VALUE_INVALID;
    while (ch.gids.next(k)) {
        assert(!hb_map_has(gid_chunk_map, k));
        hb_map_set(gid_chunk_map, k, idx);
    }
    chunks.push_back(std::move(ch));
    ch.reset();
    ch.group = group_num;
}

void chunker::make_group_chunks(int group_num, hb_map_t *gid_chunk_map,
                                set &remaining_points,
                                std::unique_ptr<group_wrapper> &g) {
    chunk ch;
    ch.group = group_num;
    uint32_t codepoint = HB_SET_VALUE_INVALID;
    while(g->next(codepoint)) {
        assert(!chunks[0].codepoints.has(codepoint));
        if (!unicodes_face.has(codepoint))
            continue;
        remaining_points.del(codepoint);
        uint32_t gid = hb_map_get(nominal_map, codepoint);
        assert(gid != HB_MAP_VALUE_INVALID);
        uint32_t chid = hb_map_get(gid_chunk_map, gid);
        if (chid != HB_MAP_VALUE_INVALID) {
            auto &ech = chunks[chid];
            std::cerr << "Found gid for codepoint " << codepoint;
            std::cerr << " in earlier chunk " << chid << ", moving.";
            std::cerr << std::endl;
            ech.codepoints.add(codepoint);
            continue;
        }
        uint32_t size = gid_size(gid);
        if (!ch.codepoints.is_empty() && (size + ch.size > conf.mini_targ()))
            add_chunk(ch, gid_chunk_map);
        ch.codepoints.add(codepoint);
        ch.gids.add(gid);
        ch.size += size;
    }
    if (!ch.codepoints.is_empty())
        add_chunk(ch, gid_chunk_map);
}

chunk &chunker::current_chunk(uint32_t &chid) {
    chunk *r = &chunks[chid];
    while (r->merged_to != -1) {
        chid = r->merged_to;
        r = &chunks[chid];
    }
    return *r;
}

void chunker::process_overlaps(uint32_t gid, uint32_t chid,
                               std::vector<uint32_t> &v) {
    bool move = false;
    uint32_t nid;
    auto &ch = current_chunk(chid);
    assert(chid != 0);
    if (v.size() > 3)
        move = true;
    else {
        int size = ch.size;
        for (auto it: v) {
            nid = it;
            auto &chv = current_chunk(nid);
            if (nid != chid)
                size += chv.size;
        }
        move = (size > 2 * conf.target_chunk_size);
    }

    if (move) {
        // Move the gid and its codepoints into the base
        std::cerr << "Moving gid " << gid << " to chunk 0" << std::endl;
        assert(ch.gids.has(gid));
        ch.gids.del(gid);
        chunks[0].gids.add(gid);
        auto [start, end] = nominal_revmap.equal_range(gid);
        for (auto it = start; it != end; ++it) {
            assert(ch.codepoints.has(it->second));
            ch.codepoints.del(it->second);
            chunks[0].codepoints.add(it->second);
        }
        ch.size -= gid_size(gid);
    } else {
        for (auto nid: v) {
            auto &chv = current_chunk(nid);
            if (chid != nid) {
                std::cerr << "Merging chunk " << nid << " into chunk ";
                std::cerr << chid << std::endl;
                ch.merge(chv, true);
                chv.merged_to = chid;
            }
        }
    }
}

void chunker::process_candidates(uint32_t gid, set &remaining_gids,
                                 std::vector<uint32_t> &v) {
    uint32_t size = std::numeric_limits<uint32_t>::max();
    chunk *c;
    for (auto i: v) {
        if (size > chunks[i].size) {
            size = chunks[i].size;
            c = &chunks[i];
        }
    }
    c->gids.add(gid);
    c->size += gid_size(gid);
    assert(remaining_gids.has(gid));
    remaining_gids.del(gid);
}

void chunker::process_feature_candidates(uint32_t feat, set &gids,
                                         uint32_t gid,
                                         std::map<uint32_t, chunk> &fchunks,
                                         std::vector<uint32_t> &v) {
    uint32_t size = std::numeric_limits<uint32_t>::max();
    uint32_t cidx = (uint32_t) -1;
    assert(gids.has(gid));
    bool add = false;
    for (auto i: v) {
        if (i == 0) {
            cidx = 0;
            break;
        }
        auto j = fchunks.find(i);
        if (j == fchunks.end()) {
            cidx = i;
            add = true;
            break;
        } else if (j->second.size < size) {
            cidx = i;
        }
    }
    if (add) {
        chunk c;
        c.feat = feat;
        c.size = gid_size(gid);
        c.from_min = c.from_max = cidx;
        c.gids.add(gid);
        fchunks.emplace(cidx, std::move(c));
    } else {
        auto i = fchunks.find(cidx);
        assert(i != fchunks.end());
        auto &c = i->second;
        c.size += gid_size(gid);
        c.gids.add(gid);
    }
    gids.del(gid);
}


int chunker::process() {
    uint32_t codepoint, gid, feat, last_gid;
    set scratch1, scratch2, remaining_gids, remaining_points;
    int idx;
    unsigned flags;
    bool printed;
    hb_set_t *t;
    hb_subset_plan_t *plan;
    hb_map_t *map, *gid_chunk_map;
    chunk base;
    simpleistream sis;
    simplestream ss;
    std::vector<chunk> tchunks;
    wrapped_groups wg;
    std::unordered_map<uint32_t, set> feature_gids;
    std::unique_ptr<group_wrapper> wrap_rp;
    std::unordered_multimap<uint32_t, uint32_t> chunk_overlap;
    std::unordered_multimap<uint32_t, uint32_t> candidate_chunks;
    std::unordered_map<uint32_t, std::unordered_multimap<uint32_t, uint32_t>>
        feature_candidate_chunks;
    std::vector<uint32_t> v;

    std::filesystem::path ip = conf.inputPath();
    inblob.load(ip.c_str());
    inface.create(inblob);
    inface.get_table_tags(tables);

    // Determine font type
    if (tables.has(T_CFF)) {
        if (tables.has(T_CFF2))
            throw std::runtime_error("Font has both CFF and CFF2 tables, "
                                     "exiting");
        else if (tables.has(T_GLYF))
            throw std::runtime_error("Font has both CFF and glyf tables, "
                                     "exiting");
        is_cff = true;
        std::cerr << "Processing CFF font" << std::endl;
    } else if (tables.has(T_CFF2)) {
        if (tables.has(T_GLYF))
            throw std::runtime_error("Font has both CFF2 and glyf tables, "
                                     "exiting");
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
        throw std::runtime_error("No CFF, CFF2 or glyf table in font, "
                                 "exiting");

    glyph_count = inface.get_glyph_count();
    std::cerr << "Initial glyph count: " << glyph_count << std::endl;

    // Initial subset clears out unused data and implements table
    // requirements.  We don't retain GIDs here
    flags = HB_SUBSET_FLAGS_DEFAULT;
    flags |= HB_SUBSET_FLAGS_GLYPH_NAMES
             | HB_SUBSET_FLAGS_NOTDEF_OUTLINE
             | HB_SUBSET_FLAGS_NO_PRUNE_UNICODE_RANGES
             | HB_SUBSET_FLAGS_IFTC_REQUIREMENTS
             ;
    if (conf.desubroutinize())
        flags |= HB_SUBSET_FLAGS_DESUBROUTINIZE;
    if (conf.namelegacy())
        flags |= HB_SUBSET_FLAGS_NAME_LEGACY;
    if (conf.passunrecognized())
        flags |= HB_SUBSET_FLAGS_PASSTHROUGH_UNRECOGNIZED;

    input.set_flags(flags);

    t = input.unicode_set();
    hb_set_clear(t);
    hb_set_invert(t);
    t = input.set(HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    hb_set_clear(t);
    hb_set_invert(t);
    if (conf.allgids()) {
        t = input.set(HB_SUBSET_SETS_GLYPH_INDEX);
        hb_set_clear(t);
        hb_set_invert(t);
    }

    subface.f = hb_subset_or_fail(inface.f, input.i);
    subblob.b = hb_face_reference_blob(subface.f);
    subfont.create(subface);
    glyph_count = subface.get_glyph_count();

    std::cerr << "Preliminary subset glyph count: " << glyph_count;
    std::cerr << std::endl;

    // Read in glyph content locations
    if (is_cff) {
        cff_charstrings_offset = hb_font_get_cff_charstrings_offset(subfont.f);
        assert(cff_charstrings_offset != -1);
        std::cerr << "CharStrings Offset: " << cff_charstrings_offset;
        std::cerr << std::endl;
        if (is_variable)
            primaryBlob = hb_face_reference_table(subface.f, T_CFF2);
        else
            primaryBlob = hb_face_reference_table(subface.f, T_CFF);
        unsigned int l;
        const char *b = hb_blob_get_data(primaryBlob.b, &l);
        sis.rdbuf()->pubsetbuf((char *) b, l);
        sis.seekg(cff_charstrings_offset);
        uint16_t icount = readObject<uint16_t>(sis);
        assert(icount == glyph_count);
        uint8_t ioffsize = readObject<uint8_t>(sis);
        assert(ioffsize == 4);
        uint32_t lastioff = readObject<uint32_t>(sis), ioff;
        for (int i = 0; i < glyph_count; i++) {
            readObject(sis, ioff);
            primaryRecs.emplace_back(lastioff, ioff - lastioff);
            lastioff = ioff;
        }
        primaryOffset = ((uint32_t) sis.tellg()) - 1;
    } else {
        {
            blob headBlob = hb_face_reference_table(subface.f, tag("head"));
            unsigned int hl;
            uint16_t tt;
            const char *hb = hb_blob_get_data(headBlob.b, &hl);
            sis.rdbuf()->pubsetbuf((char *) hb, hl);
            sis.seekg(0);
            assert(readObject<uint16_t>(sis) == 1);
            assert(readObject<uint16_t>(sis) == 0);
            sis.seekg(50);
            int16_t i2lf = readObject<int16_t>(sis);
            assert(i2lf == 1);
            sis.clear();
        }
        primaryBlob = hb_face_reference_table(subface.f, T_GLYF);
        primaryOffset = 0;
        locaBlob = hb_face_reference_table(subface.f, T_LOCA);
        unsigned int l;
        const char *b = hb_blob_get_data(locaBlob.b, &l);
        std::cerr << "loca length: " << l << std::endl;
        sis.rdbuf()->pubsetbuf((char *) b, l);
        sis.seekg(0);
        uint32_t lastioff = readObject<uint32_t>(sis), ioff;
        for (int i = 0; i < glyph_count; i++) {
            readObject(sis, ioff);
            primaryRecs.emplace_back(lastioff, ioff - lastioff);
            lastioff = ioff;
        }
        if (is_variable) {
            sis.clear();
            secondaryBlob = hb_face_reference_table(subface.f, T_GVAR);
            b = hb_blob_get_data(secondaryBlob.b, &l);
            sis.rdbuf()->pubsetbuf((char *) b, l);
            sis.seekg(0);
            uint16_t u16;
            readObject(sis, u16);
            assert(u16 == 1);
            readObject(sis, u16);
            assert(u16 == 0);
            sis.seekg(16);
            readObject(sis, u16);
            assert(u16 == glyph_count);
            readObject(sis, u16);
            assert(u16 & 0x1);
            readObject(sis, secondaryOffset);
            readObject(sis, lastioff);
            for (int i = 0; i < glyph_count; i++) {
                readObject(sis, ioff);
                secondaryRecs.emplace_back(lastioff, ioff - lastioff);
                lastioff = ioff;
            }
        }
    }

    // Read in the feature tags
    {
        hb_tag_t ftags[16];
        unsigned int numtags = 16, offset = 0;
        while (true) {
            subface.get_feature_tags(HB_OT_TAG_GSUB, offset, numtags, ftags);
            if (numtags == 0)
                break;
            for (int i = 0; i < numtags; i++)
                all_features.add(ftags[i]);
            offset += numtags;
            numtags = 16;
        }
    }

    nominal_map = hb_map_create();
    hb_face_collect_nominal_glyph_mapping(subface.f, nominal_map,
                                          unicodes_face.s);
    hb_map_values(nominal_map, gid_with_point.s);
    idx = -1;
    while (hb_map_next(nominal_map, &idx, &codepoint, &gid))
        nominal_revmap.emplace(gid, codepoint);

    std::cerr << "Unicodes defined in cmap: " << unicodes_face.size();
    std::cerr << std::endl;

    proface.create_preprocessed(subface);

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

    t = input.unicode_set();
    hb_set_set(t, unicodes_face.s);
    /*
    t = input.set(HB_SUBSET_SETS_NO_SUBSET_TABLE_TAG);
    hb_set_set(t, tables.s);
    hb_set_del(t, T_CFF);
    hb_set_del(t, T_CFF2);
    hb_set_del(t, T_GLYF);
    hb_set_del(t, T_GVAR);
    */
    t = input.set(HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    hb_set_set(t, all_features.s);

    // Get transitive gid substitution closure for all points and features
    plan = hb_subset_plan_create_or_fail(proface.f, input.i);
    map = hb_subset_plan_old_to_new_glyph_mapping(plan);
    hb_map_keys(map, gid_closure.s);
    hb_subset_plan_destroy(plan);

    // Determine which features to encode separately
    feat = HB_SET_VALUE_INVALID;
    t = input.set(HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    while (all_features.next(feat)) {
        if (default_features.find(feat) != default_features.end())
            continue;
        hb_set_set(t, all_features.s);
        hb_set_del(t, feat);
        plan = hb_subset_plan_create_or_fail(proface.f, input.i);
        map = hb_subset_plan_old_to_new_glyph_mapping(plan);
        // set scratch2 to the set of GIDs missing when the feature
        // is omitted
        scratch1.clear();
        hb_map_keys(map, scratch1.s);
        hb_subset_plan_destroy(plan);
        scratch2.copy(gid_closure);
        scratch2.subtract(scratch1);
        for (auto &i: feature_gids)
            scratch2.subtract(i.second);
        if (conf.subset_feature(gid_set_size(scratch2))) {
            std::unordered_multimap<uint32_t, uint32_t> blah;
            feature_gids.insert(std::make_pair(feat, std::move(scratch2)));
            feature_candidate_chunks.insert(std::make_pair(feat,
                                                           std::move(blah)));
        }
    }

    remaining_gids.copy(gid_closure);
    remaining_gids.subtract(gid_with_point);

    if (feature_gids.size() > 0) {
        std::cerr << "Subsetting " << feature_gids.size();
        std::cerr << " features separately: (";
        printed = false;
        for (auto &a: feature_gids) {
            hb_set_del(t, a.first);
            if (printed)
                std::cerr << ", ";
            ptag(std::cerr, a.first);
            printed = true;
            std::cerr << " (" << a.second.size() << ")";
            remaining_gids.subtract(a.second);
        }
        std::cerr << ")" << std::endl;
    }

    base.codepoints.copy(conf.base_points);
    base.codepoints.intersect(unicodes_face);
    t = input.unicode_set();
    hb_set_set(t, base.codepoints.s);

    // input now has the font's base unicodes and all features we aren't
    // subsetting separately

    plan = hb_subset_plan_create_or_fail(proface.f, input.i);
    map = hb_subset_plan_old_to_new_glyph_mapping(plan);
    hb_map_keys(map, base.gids.s);
    hb_subset_plan_destroy(plan);
    remaining_gids.subtract(base.gids);

    // Keep notdef in chunk 0
    base.gids.add(0);

    // We now have the starting point of the base unicode set and its
    // corresponding set of gids, from the closure. remaining_gids is
    // the set of gids under the closure of all unicode points and the
    // features we aren't subsetting, minus the initial baseline unicodes
    // and any gid with a direct unicode encoding. These are the gids we
    // need to find a home for in some non-base chunk, or in the base if
    // there's no good place for it.

    // Make initial vector of "mini-chunks"
    remaining_points.copy(unicodes_face);
    remaining_points.subtract(base.codepoints);
    gid_chunk_map = hb_map_create();
    add_chunk(base, gid_chunk_map);
    conf.get_groups(wg);
    int group_num = 1;
    for (auto &g: wg) {
        make_group_chunks(group_num, gid_chunk_map, remaining_points, g);
        group_num++;
    }
    scratch1.copy(remaining_points);
    wrap_rp = std::make_unique<set_wrapper>(scratch1);
    make_group_chunks(group_num, gid_chunk_map, remaining_points, wrap_rp);
    assert(remaining_points.is_empty());
    wg.clear();
    wrap_rp = nullptr;

    std::cerr << "Starting mini-chunk count: " << chunks.size() << std::endl;

    // Try to reorganize the chunks so that each cmapped gid "belongs to"
    // only one chunk. We have two options: move a gid with its codepoints
    // into the base, or merge chunks together.
    t = input.unicode_set();
    hb_set_set(t, chunks[0].codepoints.s);
    idx = -1;
    for (auto &i: chunks) {
        idx++;
        hb_set_union(t, i.codepoints.s);
        plan = hb_subset_plan_create_or_fail(proface.f, input.i);
        map = hb_subset_plan_old_to_new_glyph_mapping(plan);
        scratch1.clear();
        hb_map_keys(map, scratch1.s);
        scratch1.subtract(chunks[0].gids);
        scratch1.subtract(chunks[idx].gids);
        scratch1.intersect(gid_with_point);
        uint32_t gid = HB_SET_VALUE_INVALID;
        while (scratch1.next(gid))
            chunk_overlap.emplace(gid, idx);
        hb_subset_plan_destroy(plan);
        hb_set_subtract(t, i.codepoints.s);
    }
    for (auto &i: chunk_overlap) {
        if (v.size() > 0 && last_gid != i.first) {
            process_overlaps(last_gid, hb_map_get(gid_chunk_map, last_gid), v);
            v.clear();
        }
        last_gid = i.first;
        v.push_back(i.second);
    }
    if (v.size() > 0)
        process_overlaps(last_gid, hb_map_get(gid_chunk_map, last_gid), v);
    v.clear();

    chunk_overlap.clear();

    // Now we need to check each mini-chunk for two things:
    // 1) Are the gids for the codepoints only for those codepoints, or
    //    are they substituted for other codepoints somewhere? If the latter,
    //    move those points to the base chunk and redo the plan.
    // 2) Which, if any, of the remaining_gids have a hard dependency on
    //    one of the codepoints in the chunk? Those gids are candidates
    //    for grouping with this chunk.

    t = input.set(HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    hb_set_set(t, all_features.s);
    t = input.unicode_set();
    idx = -1;
    for (auto &i: chunks) {
        idx++;
        if (i.group == 0 || i.merged_to != -1 || i.codepoints.size() == 0)
            // Skip the base chunk and merged chunks
            continue;
        hb_set_set(t, unicodes_face.s);
        hb_set_subtract(t, i.codepoints.s);
        plan = hb_subset_plan_create_or_fail(proface.f, input.i);
        map = hb_subset_plan_old_to_new_glyph_mapping(plan);
        scratch1.clear();
        hb_map_keys(map, scratch1.s);
        scratch2.copy(scratch1);
        scratch2.intersect(i.gids);
        if (!scratch2.is_empty()) {
            gid = HB_SET_VALUE_INVALID;
            while (scratch2.next(gid)) {
                std::cerr << "Second stage: moving gid " << gid;
                std::cerr << " to chunk 0" << std::endl;
                assert(i.gids.has(gid));
                i.gids.del(gid);
                chunks[0].gids.add(gid);
                auto [start, end] = nominal_revmap.equal_range(gid);
                for (auto it = start; it != end; ++it) {
                    assert(i.codepoints.has(it->second));
                    i.codepoints.del(it->second);
                    chunks[0].codepoints.add(it->second);
                }
                i.size -= gid_size(gid);
            }
            if (i.codepoints.is_empty())
                continue;
            hb_set_set(t, unicodes_face.s);
            hb_set_subtract(t, i.codepoints.s);
            hb_subset_plan_destroy(plan);
            plan = hb_subset_plan_create_or_fail(proface.f, input.i);
            map = hb_subset_plan_old_to_new_glyph_mapping(plan);
            scratch1.clear();
            hb_map_keys(map, scratch1.s);
        }
        scratch2.copy(gid_closure);
        scratch2.subtract(scratch1);
        scratch2.intersect(remaining_gids);
        gid = HB_SET_VALUE_INVALID;
        while(scratch2.next(gid)) {
            // std::cerr << gid << std::endl;
            candidate_chunks.emplace(gid, idx);
        }
        hb_subset_plan_destroy(plan);
    }

    for (auto &i: candidate_chunks) {
        if (v.size() > 0 && last_gid != i.first) {
            process_candidates(last_gid, remaining_gids, v);
            v.clear();
        }
        last_gid = i.first;
        v.push_back(i.second);
    }
    if (v.size() > 0)
        process_candidates(last_gid, remaining_gids, v);
    v.clear();

    candidate_chunks.clear();
    std::cerr << "Remaining gid count after assignment: ";
    std::cerr << remaining_gids.size() << std::endl;

    chunks[0].gids._union(remaining_gids);

    for (auto &i: chunks) {
        if (i.merged_to != -1 || i.gids.size() == 0)
            continue;
        if (tchunks.size() == 0)
            tchunks.push_back(std::move(i));
        else {
            chunk &tch = tchunks.back();
            if (tch.size + i.size <= conf.target_chunk_size)
                tch.merge(i, true);
            else
                tchunks.push_back(std::move(i));
        }
    }
    chunks.swap(tchunks);
    tchunks.clear();
    hb_map_destroy(gid_chunk_map);

    t = input.unicode_set();
    idx = -1;
    if (feature_gids.size() > 0) {
        std::cerr << "Processing subsetted features" << std::endl;
        for (auto &i: chunks) {
            idx++;
            std::cerr << "For non-feature chunk " << idx << std::endl;
            if (idx == 0) {
                hb_set_set(t, i.codepoints.s);
            } else {
                hb_set_set(t, unicodes_face.s);
                hb_set_subtract(t, i.codepoints.s);
            }
            plan = hb_subset_plan_create_or_fail(proface.f, input.i);
            map = hb_subset_plan_old_to_new_glyph_mapping(plan);
            scratch1.clear();
            hb_map_keys(map, scratch1.s);
            for (auto &f: feature_gids) {
                std::unordered_multimap<uint32_t, uint32_t> &fmmap =
                                            feature_candidate_chunks[f.first];
                if (idx == 0) {
                    scratch2.copy(scratch1);
                } else {
                    scratch2.copy(gid_closure);
                    scratch2.subtract(scratch1);
                }
                scratch2.intersect(f.second);
                gid = HB_SET_VALUE_INVALID;
                while (scratch2.next(gid))
                    fmmap.emplace(gid, idx);
            }
            hb_subset_plan_destroy(plan);
        }
    }

    uint32_t nonfeat_chunkcount = chunks.size();

    for (auto &f: feature_candidate_chunks) {
        set &gids = feature_gids[f.first];
        std::map<uint32_t, chunk> fchunks;
        base.reset();
        base.feat = f.first;
        fchunks.emplace(0, std::move(base));
        for (auto &i: f.second) {
            if (v.size() > 0 && last_gid != i.first) {
                process_feature_candidates(f.first, gids, last_gid, fchunks,
                                           v);
                v.clear();
            }
            last_gid = i.first;
            v.push_back(i.second);
        }

        if (v.size() > 0)
            process_feature_candidates(f.first, gids, last_gid, fchunks, v);
        v.clear();

        if (gids.size() > 0) {
            std::cerr << f.second.size() << " gids remaining in feature ";
            ptag(std::cerr, f.first);
            std::cerr << std::endl;
            gid = HB_SET_VALUE_INVALID;
            while (gids.next(gid)) {
                fchunks[0].gids.add(gid);
                fchunks[0].size += gid_size(gid);
            }
        }

        for (auto &ii: fchunks) {
            auto &bch = chunks.back();
            auto &i = ii.second;
            if (   bch.feat != i.feat
                || bch.size + i.size > conf.target_chunk_size) {
                if (i.gids.size() > 0)
                    chunks.push_back(std::move(i));
            } else
                bch.merge(i);
        }
    }

    // If any gids fall outside of the closure, add them as final chunks
    scratch1.clear();
    scratch1.add_range(0, glyph_count-1);
    scratch1.subtract(gid_closure);
    if (scratch1.size() > 0) {
        base.reset();
        gid = HB_SET_VALUE_INVALID;
        while(scratch1.next(gid)) {
            uint32_t size = gid_size(gid);
            if (   base.gids.size() > 0
                && base.size + size > conf.target_chunk_size) {
                chunks.push_back(std::move(base));
                base.reset();
            }
            base.gids.add(gid);
            base.size += size;
        }
        if (base.gids.size() > 0)
            chunks.push_back(std::move(base));
    }

    all_codepoints = hb_map_create();
    all_gids = hb_map_create();

    std::cerr << std::endl << std::endl << "---- Chunk Report ----";
    std::cerr << std::endl << std::endl;
    idx = -1;
    for (auto &i: chunks) {
        idx++;
        std::cerr << "Chunk " << idx << ": ";
        if (i.feat) {
            std::cerr << "feature ";
            ptag(std::cerr, i.feat);
            std::cerr << ", ";
        }
        std::cerr << i.codepoints.size() << " codepoints, ";
        std::cerr << i.gids.size() << " gids, ";
        std::cerr << i.size << " bytes" << std::endl;
        codepoint = HB_SET_VALUE_INVALID;
        while (i.codepoints.next(codepoint)) {
            assert(!hb_map_has(all_codepoints, codepoint));
            hb_map_set(all_codepoints, codepoint, idx);
        }
        gid = HB_SET_VALUE_INVALID;
        while (i.gids.next(gid)) {
            assert(!hb_map_has(all_gids, gid));
            hb_map_set(all_gids, gid, idx);
        }
    }
    scratch1.clear();
    hb_map_keys(all_codepoints, scratch1.s);
    assert(unicodes_face == scratch1);
    scratch1.clear();
    scratch1.add_range(0, glyph_count-1);
    scratch2.clear();
    hb_map_keys(all_gids, scratch2.s);
    assert(scratch1 == scratch2);

    table_IFTC tiftc;
    tiftc.CFFCharStringsOffset = cff_charstrings_offset;
    tiftc.setChunkCount(chunks.size());
    tiftc.chunkSet[0] = true;
    for (uint32_t i = 0; i < glyph_count; i++)
        tiftc.gidMap.push_back(hb_map_get(all_gids, i));

    uint32_t curr_feat = 0;
    FeatureMap fm;
    for (uint32_t i = nonfeat_chunkcount; i < chunks.size(); i++) {
        chunk &c = chunks[i];
        if (curr_feat != c.feat) {
            if (curr_feat != 0)
                tiftc.featureMap.emplace(curr_feat, std::move(fm));
            curr_feat = c.feat;
            fm.startIndex = i;
        }
        std::pair<uint16_t, uint16_t> range(c.from_min, c.from_max);
        fm.ranges.push_back(range);
    }
    if (curr_feat != 0)
        tiftc.featureMap.emplace(curr_feat, std::move(fm));

    conf.setNumChunks(chunks.size());

    uint32_t table1 = T_GLYF, table2 = 0;
    if (is_cff && is_variable)
        table1 = T_CFF2;
    else if (is_cff)
        table1 = T_CFF;
    else if (is_variable)
        table2 = T_GVAR;

    idx = -1;
    std::stringstream css;
    size_t encoded_size;
    uint8_t *encoded_buffer;
    std::vector<uint8_t> encode_buf;
    std::ofstream rangefile;
    rangefile.open(conf.rangePath(), std::ios::trunc | std::ios::binary);
    uint32_t chunkOffset = 0;
    for (auto &c: chunks) {
        idx++;
        if (idx == 0)
            continue;
        std::cerr << "Compiling and compressing chunk " << idx << std::endl;
        css.str("");
        css.clear();
        c.compile(css, idx, table1, primaryOffset, primaryBlob,
                  primaryRecs, table2, secondaryOffset, secondaryBlob,
                  secondaryRecs);
        std::string cs = css.str();
        encoded_size = BrotliEncoderMaxCompressedSize(cs.size());
        if (encode_buf.size() < encoded_size)
            encode_buf.resize(encoded_size);
        if (!BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_DEFAULT_WINDOW,
                                   BROTLI_MODE_FONT, cs.size(),
                                   (const uint8_t *) cs.data(), &encoded_size,
                                   encode_buf.data()))
            throw std::runtime_error("Could not compress chunk");
        std::ofstream cfile;
        cfile.open(conf.chunkPath(idx), std::ios::trunc | std::ios::binary);
        cfile.write((char *) encode_buf.data(), encoded_size);
        cfile.close();
        rangefile.write((char *) encode_buf.data(), encoded_size);
        tiftc.chunkOffsets.push_back(chunkOffset);
        chunkOffset += encoded_size;
    }
    tiftc.chunkOffsets.push_back(chunkOffset);
    rangefile.close();

    set &c0g = chunks[0].gids;

    ss.clear();

    hb_blob_t *newPrimaryBlob, *newSecondaryBlob = NULL, *newLocaBlob = NULL;

    if (is_cff) {
        uint8_t endchar = 14;  // CFF operator
        unsigned int l;
        const char *b = hb_blob_get_data(primaryBlob.b, &l);
        char *nb = (char *) malloc(l);
        memcpy(nb, b, cff_charstrings_offset + 3);
        ss.rdbuf()->pubsetbuf(nb, l);
        ss.seekp(cff_charstrings_offset + 3);
        uint32_t nboff = 1;
        char *nbinsert = nb + cff_charstrings_offset + 3 +
                         (glyph_count + 1) * 4;
        const char *bbase = b + cff_charstrings_offset + 3 +
                            (glyph_count + 1) * 4 - 1;
        for (uint32_t i = 0; i < glyph_count; i++) {
            const char *from = NULL;
            uint32_t froml = 0;
            if (c0g.has(i)) {
                from = bbase + primaryRecs[i].offset;
                froml = primaryRecs[i].length;
            } else if (!is_variable) {
                from = (char *) &endchar;
                froml = 1;
            }
            if (froml > 0)
                memcpy(nbinsert, from, froml);
            writeObject(ss, nboff);
            nboff += froml;
            nbinsert += froml;
        }
        writeObject(ss, nboff);
        newPrimaryBlob = hb_blob_create(nb, nbinsert - nb,
                                        HB_MEMORY_MODE_READONLY, NULL, NULL);
    } else {
        unsigned int l;
        const char *b = hb_blob_get_data(primaryBlob.b, &l);
        char *nglyf = (char *) malloc(l);
        char *nloca = (char *) malloc(glyph_count * 4);
        ss.rdbuf()->pubsetbuf(nloca, glyph_count * 4);
        ss.seekp(0);
        char *ninsert = nglyf;
        for (uint32_t i = 0; i < glyph_count; i++) {
            const char *from = NULL;
            uint32_t froml = 0;
            if (c0g.has(i)) {
                from = b + primaryRecs[i].offset;
                froml = primaryRecs[i].length;
            }
            if (froml > 0)
                memcpy(ninsert, from, froml);
            writeObject(ss, (uint32_t) (ninsert - nglyf));
            ninsert += froml;
        }
        newPrimaryBlob = hb_blob_create(nglyf, ninsert - nglyf,
                                        HB_MEMORY_MODE_READONLY, NULL, NULL);
        newLocaBlob = hb_blob_create(nloca, glyph_count * 4,
                                     HB_MEMORY_MODE_READONLY, NULL, NULL);
    }

    tiftc.filesURI = conf.filesURI();
    tiftc.rangeFileURI = conf.rangeFileURI();

    hb_face_t *fbldr = hb_face_builder_create();

    std::vector<uint32_t> tagOrder;
    css.str("");
    css.clear();
    tiftc.compile(css);
    std::string iftc_str = css.str();
    hb_blob_t *iftcblob = hb_blob_create(iftc_str.data(), iftc_str.size(),
                                         HB_MEMORY_MODE_READONLY, NULL, NULL);
    hb_face_builder_add_table(fbldr, T_IFTC, iftcblob);
    tagOrder.push_back(T_IFTC);
    hb_blob_destroy(iftcblob);

    if (tables.has(T_CMAP)) {
        hb_face_builder_add_table(fbldr, T_CMAP,
                                  hb_face_reference_table(subface.f, T_CMAP));
        tagOrder.push_back(T_CMAP);
    } else {
        std::cerr << "Warning: Input font has no cmap table!" << std::endl;
    }

    uint32_t tbl = HB_SET_VALUE_INVALID;
    while (tables.next(tbl)) {
        if (tbl == tag("FFTM") || tbl == tag("DSIG"))
           continue;
        if (tbl == T_CMAP || tbl == T_CFF || tbl == T_CFF2 || tbl == T_GLYF ||
            tbl == T_LOCA || tbl == T_GVAR)
            continue;
        hb_face_builder_add_table(fbldr, tbl,
                                  hb_face_reference_table(subface.f, tbl));
        tagOrder.push_back(tbl);
    }

    if (is_cff) {
        if (is_variable) {
            hb_face_builder_add_table(fbldr, T_CFF2, newPrimaryBlob);
            tagOrder.push_back(T_CFF2);
        } else {
            hb_face_builder_add_table(fbldr, T_CFF, newPrimaryBlob);
            tagOrder.push_back(T_CFF);
        }
    } else {
        if (is_variable) {
            hb_face_builder_add_table(fbldr, T_GVAR, newSecondaryBlob);
            tagOrder.push_back(T_GVAR);
        }
        hb_face_builder_add_table(fbldr, T_GLYF, newPrimaryBlob);
        tagOrder.push_back(T_GLYF);
        hb_face_builder_add_table(fbldr, T_LOCA, newLocaBlob);
        tagOrder.push_back(T_LOCA);
    }

    tagOrder.push_back(HB_TAG_NONE);
    hb_face_builder_sort_tables(fbldr, (hb_tag_t *)tagOrder.data());

    hb_blob_t *outblob = hb_face_reference_blob(fbldr);
    unsigned int size;
    const char *data = hb_blob_get_data(outblob, &size);
    css.clear();
    ss.rdbuf()->pubsetbuf((char *)data, size);
    ss.seekp(0);
    writeObject(ss, T_IFTC);
    std::ofstream myfile;
    myfile.open(conf.subsetPath(is_cff), std::ios::trunc | std::ios::binary);
    myfile.write(data, size);
    myfile.close();

    size_t woff2_size = woff2::MaxWOFF2CompressedSize((uint8_t *)data, size);
    std::string woff2_out(woff2_size, 0);

    woff2::WOFF2Params params;
    if (!woff2::ConvertTTFToWOFF2((uint8_t *)data, size, (uint8_t *)woff2_out.data(), &woff2_size, params))
        throw std::runtime_error("Could not WOFF2 compress font");
    woff2_out.resize(woff2_size);

    myfile.open(conf.woff2Path(), std::ios::trunc | std::ios::binary);
    myfile.write(woff2_out.data(), woff2_size);
    myfile.close();

    return 0;
}