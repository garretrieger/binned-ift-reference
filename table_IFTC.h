#include <unordered_map>
#include <string>
#include <vector>

#include "streamhelp.h"

struct FeatureMap {
    FeatureMap() {}
    FeatureMap(const FeatureMap &fm) = delete;
    FeatureMap(FeatureMap &&fm) {
        startIndex = fm.startIndex;
        ranges.swap(fm.ranges);
    }
    uint16_t startIndex = 0;
    std::vector<std::pair<uint16_t, uint16_t>> ranges;
};

class table_IFTC {
    uint16_t majorVersion {0}, minorVersion {0}, flags {0};
    uint32_t chunkCount, gidCount;
    uint32_t CFFCharStringsOffset;
    std::vector<bool> chunkMap;
    std::vector<uint16_t> gidMap;
    std::unordered_map<uint32_t, FeatureMap> featureMap;
    std::vector<uint32_t> chunkOffsets;
    std::string filesURI, rangeFileURI;
    void writeChunkMap(std::ostream &os);
    uint16_t readChunkIndex(std::istream &i) {
        uint8_t i8;
        uint16_t i16;
        if (chunkCount > 256) {
            readObject(i, i16);
            return i16;
        } else {
            readObject(i, i8);
            return i8;
        }
    }
    void writeChunkIndex(std::ostream &o, uint16_t idx) {
        uint8_t i8 = (uint8_t) idx;
        if (chunkCount > 256)
            writeObject(o, idx);
        else
            writeObject(o, i8);
    }
    unsigned int compile(std::ostream &o, uint32_t offset);
    void decompile(std::istream &i, uint32_t offset);
};
