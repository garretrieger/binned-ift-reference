#include <unordered_set>
#include <iostream>

#pragma once

namespace iftb {
    extern std::unordered_set<uint32_t> default_features;
}

// XXX with C++20 this could be consteval with static assertion.
constexpr uint32_t tag(const char *s) {
    return s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3];
}

#define T_CFF tag("CFF ")
#define T_CFF2 tag("CFF2")
#define T_GLYF tag("glyf")
#define T_LOCA tag("loca")
#define T_GVAR tag("gvar")
#define T_CMAP tag("cmap")
#define T_IFTB tag("IFTB")
#define T_HEAD tag("head")

struct otag {
 public:
    otag() = delete;
    otag(uint32_t tg) : tg(tg) {}
 private:
    uint32_t tg;
    friend std::ostream& operator<<(std::ostream &os, const otag &tg);
};

inline std::ostream& operator<<(std::ostream &os, const otag &p) {
    char o;
    for (int i = 3; i >= 0; i--) {
        o = (char)(p.tg >> (8*i) & 0xff);
        os << o;
    }
    return os;
}
