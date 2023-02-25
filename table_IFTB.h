#include <unordered_map>
#include <string>
#include <vector>
#include <cassert>

#include "streamhelp.h"

#pragma once

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

struct table_IFTB {
    uint16_t majorVersion {0}, minorVersion {1}, flags {0};
    uint32_t id0, id1, id2, id3;
    uint32_t CFFCharStringsOffset {0};
    uint32_t chunkCount {0};
    std::vector<bool> chunkSet;
    std::vector<uint16_t> gidMap;
    std::unordered_map<uint32_t, FeatureMap> featureMap;
    std::vector<uint32_t> chunkOffsets;
    std::string filesURI, rangeFileURI;
    std::unordered_map<uint32_t, uint16_t> uniMap;

    void dumpChunkSet(std::ostream &os);
    void writeChunkSet(std::ostream &os);
    void setChunkCount(uint32_t cc) {
        chunkCount = cc;
        chunkSet.clear();
        chunkSet.resize(chunkCount + 8);
    }
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
    unsigned int compile(std::ostream &o, uint32_t offset = 0);
    void decompile(std::istream &i, uint32_t offset = 0);
    bool addcmap(std::istream &i, uint32_t offset = 0,
                 bool keepGIDmap = false);
    void dump(std::ostream &o, bool full = false);
};
