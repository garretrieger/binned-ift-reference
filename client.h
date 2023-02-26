
#include <string>
#include <vector>

#include "sfnt.h"
#include "table_IFTB.h"
#include "unchunk.h"

#pragma once

class iftb_client {
public:
    iftb_client() {}
    bool loadFont(std::string &s);
    bool loadFont(char *buf, size_t length);
    bool hasFont() { return fontData.size() > 0; }
    bool failure() { return failed; }
    bool error(const char *m) {
        std::cerr << "IFTB Client Error: " << m << std::endl;
        failed = true;
        return false;
    }
    uint16_t getChunkCount();
    bool setPending(std::vector<uint32_t> &unicodes,
                    std::vector<uint32_t> &features);
    bool getPendingChunkList(std::vector<uint16_t> &cl);
    std::string getRangeFileURI();
    std::string getChunkURI(uint16_t cidx);
    std::pair<uint32_t, uint32_t> getChunkRange(uint16_t cidx);
private:
    static constexpr float extraPercent() { return 1.0; }
    table_IFTB tiftb;
    sfnt sf;
    std::vector<uint16_t> pendingChunks;
    merger m;
    std::string fontData;
    bool failed {false};
};
