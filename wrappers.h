#include <vector>

#include <hb-subset.h>

#pragma once

struct face;

struct set {
    hb_set_t *s;
    set() { s = hb_set_create(); }
    set(const set &st) = delete;
    set(set &&st) {
        s = st.s;
        st.s = hb_set_create();
    }
    ~set() { destroy(); }
    void copy(set &st) {
        destroy();
        s = hb_set_copy(st.s);
    }
    void operator = (const set &st) {
        destroy();
        s = st.s;
        hb_set_reference(s);
    }
    void operator = (hb_set_t *st) {
        destroy();
        s = st;
        hb_set_reference(s);
    }
    bool operator == (const set &st) { return hb_set_is_equal(s, st.s); }
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
    void subtract(const set &st) { hb_set_subtract(s, st.s); }
    void intersect(const set &st) { hb_set_intersect(s, st.s); }
    void _union(const set &st) { hb_set_union(s, st.s); }
    void symmetric_difference(const set &st) {
        hb_set_symmetric_difference(s, st.s);
    }
    void invert() { hb_set_invert(s); }
    bool is_subset(const set &st) const { return hb_set_is_subset(s, st.s); }
    bool next(hb_codepoint_t &cp) const { return hb_set_next(s, &cp); }
 private:
    void destroy() { if (s) hb_set_destroy(s); }
};

struct blob {
    hb_blob_t *b;
    blob() : b(NULL) {}
    blob(hb_blob_t *bl) { b = bl; }
    void operator = (hb_blob_t *bl) {
        destroy();
        b = bl;
        hb_blob_reference(b);
    }
    ~blob() { destroy(); }
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

struct face {
    hb_face_t *f;
    face() { f = hb_face_get_empty(); }
    face(hb_face_t *fa) { f = fa; }
    face(const face &fa) { f = hb_face_reference(fa.f); }
    ~face() { destroy(); }
    void create_preprocessed(const face &fa) {
        destroy();
        f = hb_subset_preprocess(fa.f);
    }
    void create(blob &b) { destroy(); f = hb_face_create(b.b, 0); }
    uint32_t get_glyph_count() { return hb_face_get_glyph_count(f); }
    void get_table_tags(set &ts) {
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

struct font {
    hb_font_t *f;
    font() { f = hb_font_get_empty(); }
    ~font() { destroy(); }
    void create(face &fa) { destroy(); f = hb_font_create(fa.f); }
 private:
    void destroy() { hb_font_destroy(f); }
};

struct subset_input {
    hb_subset_input_t *i;
    subset_input() { i = hb_subset_input_create_or_fail(); }
    ~subset_input() { hb_subset_input_destroy(i); }
    void keep_everything() { hb_subset_input_keep_everything(i); }
    void set_flags(unsigned v) { hb_subset_input_set_flags(i, v); }
    hb_subset_flags_t get_flags() { return hb_subset_input_get_flags(i); }
    hb_set_t *unicode_set() { return hb_subset_input_unicode_set(i); }
    hb_set_t *gid_set() { return hb_subset_input_glyph_set(i); }
    hb_set_t *set(hb_subset_sets_t st) { return hb_subset_input_set(i, st); }
    face subset(face &f) {
        hb_face_t *r = hb_subset_or_fail(f.f, i);
        return face(r);
    }
};

struct plan {
    hb_subset_plan_t *p;
    plan(const face &f, const subset_input &si) {
        p = hb_subset_plan_create_or_fail(f.f, si.i);
    }
    ~plan() { destroy(); }
    hb_map_t *unicode_to_old_glyph_mapping() {
        return hb_subset_plan_unicode_to_old_glyph_mapping(p);
    }
    hb_map_t *new_to_old_glyph_mapping() {
        return hb_subset_plan_new_to_old_glyph_mapping(p);
    }
    hb_map_t *old_to_new_glyph_mapping() {
        return hb_subset_plan_old_to_new_glyph_mapping(p);
    }
 private:
    void destroy() { hb_subset_plan_destroy(p); }
};
