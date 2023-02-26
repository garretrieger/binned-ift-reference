#include <unordered_set>

#include <iostream>

#pragma once

constexpr uint32_t tag(const char *s) {
    return s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3];
}

#define T_CFF tag("CFF ")
#define T_CFF2 tag("CFF2")
#define T_GLYF tag("glyf")
#define T_LOCA tag("loca")
#define T_GVAR tag("GVAR")
#define T_CMAP tag("cmap")
#define T_IFTB tag("IFTB")
#define T_HEAD tag("head")

inline void ptag(std::ostream &out, uint32_t tag) {
    char o;
    for (int i = 3; i >= 0; i--) {
        o = (char)(tag >> (8*i) & 0xff);
        out << o;
    }
}

inline uint32_t tagFromBuffer(const char *c) {
    return (c[0] << 24) | (c[1] << 16) | c[2] << 8 | c[3];
}

extern std::unordered_set<uint32_t> default_features;
