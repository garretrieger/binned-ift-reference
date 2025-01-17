/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

/* The iftb::client object is the implemetation of the client side of
   the IFTB system (below the level of WASM). It is also included in the
   encoder for, e.g., preloading.
 */

#include <string>
#include <vector>
#include <set>

#include "sfnt.h"
#include "table_IFTB.h"
#include "tag.h"
#include "merger.h"
#include "streamhelp.h"
#include "randtest.h"

#pragma once

namespace iftb {
    class client;
    class wasm_wrapper;
}

class iftb::client {
 public:
    friend bool iftb::randtest(std::string &s, uint32_t iterations);
    friend class iftb::wasm_wrapper;
    bool loadFont(std::string &s, bool keepGIDMap = false);
    bool loadFont(char *buf, uint32_t length, bool keepGIDMap = false);
    bool hasFont() { return fontData.size() > 0; }
    bool failure() { return failed; }
    uint16_t getChunkCount();
    bool setPending(const std::vector<uint32_t> &unicodes,
                    const std::vector<uint32_t> &features);
    bool getPendingChunkList(std::vector<uint16_t> &cl);
    std::string &getRangeFileURI() { return tiftb.getRangeFileURI(); }
    uint32_t getChunkOffset(uint16_t cidx);
    std::pair<uint32_t, uint32_t> getChunkRange(uint16_t cidx);
    const char *getChunkURI(uint16_t cidx) { return tiftb.getChunkURI(cidx); }
    bool addChunk(uint16_t idx, std::string &s, bool setPending = false) {
        return addChunk(idx, s.data(), s.size(), setPending);
    }
    bool hasChunk(uint16_t idx) { return tiftb.hasChunk(idx); }
    bool addChunk(uint16_t idx, char *buf, uint32_t length,
                  bool setPending = false);
    bool canMerge();
    bool merge(bool asIFTB = true);
    bool setType(bool asIFTB) {
        if (asIFTB != isIFTB) {
            if (!sfnt.write(asIFTB))
                return false;
            isIFTB = asIFTB;
        }
        return true;
    }
    bool isCFF() {
        return !sfnt.has(T_GLYF);
    }
    std::string &getFontAsString() { return fontData; }
 private:
    bool error(const char *m) {
        std::cerr << "IFTB Client Error: " << m << std::endl;
        failed = true;
        return false;
    }
    static constexpr float extraPercent() { return 1.0; }
    iftb::table_IFTB tiftb;
    iftb::sfnt sfnt;
    std::set<uint16_t> pendingChunks;
    iftb::merger merger;
    std::string fontData;
    simplestream ss;
    bool failed {false}, isIFTB = true;
};
