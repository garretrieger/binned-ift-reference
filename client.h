
#include <string>
#include <vector>
#include <set>

#include "sfnt.h"
#include "table_IFTB.h"
#include "merger.h"
#include "streamhelp.h"

#pragma once

class iftb_client {
 public:
    friend bool randtest(std::string &s, uint32_t iterations);
    iftb_client() {}
    bool loadFont(std::string &s, bool keepGIDMap = false);
    bool loadFont(char *buf, uint32_t length, bool keepGIDMap = false);
    bool hasFont() { return fontData.size() > 0; }
    bool failure() { return failed; }
    bool error(const char *m) {
        std::cerr << "IFTB Client Error: " << m << std::endl;
        failed = true;
        return false;
    }
    uint16_t getChunkCount();
    bool setPending(const std::vector<uint32_t> &unicodes,
                    const std::vector<uint32_t> &features);
    bool getPendingChunkList(std::vector<uint16_t> &cl);
    std::string getRangeFileURI();
    std::pair<uint32_t, uint32_t> getChunkRange(uint16_t cidx);
    std::string getChunkURI(uint16_t cidx);
    bool addChunk(uint16_t idx, std::string &s, bool setPending = false) {
        return addChunk(idx, s.data(), s.size(), setPending);
    }
    bool addChunk(uint16_t idx, char *buf, uint32_t length,
                  bool setPending = false);
    bool canMerge();
    bool merge(bool asIFTB = true);
    std::string &getFontAsString() { return fontData; }
 private:
    static constexpr float extraPercent() { return 1.0; }
    table_IFTB tiftb;
    iftb_sfnt sfnt;
    std::set<uint16_t> pendingChunks;
    iftb_merger merger;
    std::string fontData;
    simplestream ss;
    bool failed {false};
};
