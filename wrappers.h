#include <vector>

#include <hb-subset.h>

#pragma once

namespace iftb {
    struct wr_set;
    struct wr_blob;
    struct wr_face;
    struct wr_font;
    struct wr_subset_input;
}

struct iftb::wr_set {
    hb_set_t *s;
    wr_set() { s = hb_set_create(); }
    wr_set(const wr_set &st) = delete;
    wr_set(wr_set &&st) {
        s = st.s;
        st.s = hb_set_create();
    }
    ~wr_set() { destroy(); }
    void copy(wr_set &st) {
        destroy();
        s = hb_set_copy(st.s);
    }
    void operator = (const wr_set &st) {
        destroy();
        s = st.s;
        hb_set_reference(s);
    }
    void operator = (hb_set_t *st) {
        destroy();
        s = st;
        hb_set_reference(s);
    }
    bool operator == (const wr_set &st) { return hb_set_is_equal(s, st.s); }
    void clear() { hb_set_clear(s); }
    bool has(hb_codepoint_t cp) const { return hb_set_has(s, cp); }
    void add(hb_codepoint_t cp) { hb_set_add(s, cp); }
    void add_range(hb_codepoint_t f, hb_codepoint_t l) {
        hb_set_add_range(s, f, l);
    }
    void del(hb_codepoint_t cp) { hb_set_del(s, cp); }
    void del_range(hb_codepoint_t f, hb_codepoint_t l) {
        hb_set_del_range(s, f, l);
    }
    hb_codepoint_t min() const { return hb_set_get_min(s); }
    hb_codepoint_t max() const { return hb_set_get_max(s); }
    unsigned int size() const { return hb_set_get_population(s); }
    bool is_empty() const { return hb_set_is_empty(s); }
    void subtract(const wr_set &st) { hb_set_subtract(s, st.s); }
    void intersect(const wr_set &st) { hb_set_intersect(s, st.s); }
    void _union(const wr_set &st) { hb_set_union(s, st.s); }
    void symmetric_difference(const wr_set &st) {
        hb_set_symmetric_difference(s, st.s);
    }
    void invert() { hb_set_invert(s); }
    bool is_subset(const wr_set &st) const {
        return hb_set_is_subset(s, st.s);
    }
    bool next(hb_codepoint_t &cp) const { return hb_set_next(s, &cp); }
 private:
    void destroy() { if (s) hb_set_destroy(s); }
};

struct iftb::wr_blob {
    hb_blob_t *b;
    wr_blob() : b(NULL) {}
    wr_blob(hb_blob_t *bl) { b = bl; }
    void operator = (hb_blob_t *bl) {
        destroy();
        b = bl;
        hb_blob_reference(b);
    }
    ~wr_blob() { destroy(); }
    void load(const char *fname) {
        destroy();
        b = hb_blob_create_from_file_or_fail(fname);
    }
    void from_string(std::string &s, bool read_only = false) {
        destroy();
        b = hb_blob_create(s.data(), s.size(),
                           read_only ? HB_MEMORY_MODE_READONLY
                                     : HB_MEMORY_MODE_WRITABLE,
                           NULL, NULL);
    }
private:
    void destroy() { if (b) hb_blob_destroy(b); }
};

struct iftb::wr_face {
    hb_face_t *f;
    wr_face() { f = hb_face_get_empty(); }
    wr_face(hb_face_t *fa) { f = fa; }
    wr_face(const wr_face &fa) { f = hb_face_reference(fa.f); }
    ~wr_face() { destroy(); }
    void create_preprocessed(const wr_face &fa) {
        destroy();
        f = hb_subset_preprocess(fa.f);
    }
    void create(wr_blob &b) { destroy(); f = hb_face_create(b.b, 0); }
    uint32_t get_glyph_count() { return hb_face_get_glyph_count(f); }
    void get_table_tags(wr_set &ts) {
        unsigned int l;
        std::vector<uint32_t> t;
        l = hb_face_get_table_tags(f, 0, NULL, NULL);
        t.resize(l);
        hb_face_get_table_tags(f, 0, &l, t.data());
        for (auto i: t)
            ts.add(i);
    }
    unsigned int get_feature_tags(hb_tag_t table_tag,
                                  unsigned int start_offset,
                                  unsigned int &feature_count,
                                  hb_tag_t *feature_tags) {
        return hb_ot_layout_table_get_feature_tags(f, table_tag, start_offset,
                                                   &feature_count,
                                                   feature_tags);
    }
 private:
    void destroy() { hb_face_destroy(f); }
};

struct iftb::wr_font {
    hb_font_t *f;
    wr_font() { f = hb_font_get_empty(); }
    ~wr_font() { destroy(); }
    void create(wr_face &fa) { destroy(); f = hb_font_create(fa.f); }
 private:
    void destroy() { hb_font_destroy(f); }
};

struct iftb::wr_subset_input {
    hb_subset_input_t *i;
    wr_subset_input() { i = hb_subset_input_create_or_fail(); }
    ~wr_subset_input() { hb_subset_input_destroy(i); }
    // void keep_everything() { hb_subset_input_keep_everything(i); }
    void set_flags(unsigned v) { hb_subset_input_set_flags(i, v); }
    hb_subset_flags_t get_flags() { return hb_subset_input_get_flags(i); }
    hb_set_t *unicode_set() { return hb_subset_input_unicode_set(i); }
    hb_set_t *gid_set() { return hb_subset_input_glyph_set(i); }
    hb_set_t *set(hb_subset_sets_t st) { return hb_subset_input_set(i, st); }
    wr_face subset(wr_face &f) {
        hb_face_t *r = hb_subset_or_fail(f.f, i);
        return wr_face(r);
    }
};
