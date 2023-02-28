#include <map>
#include <string>
#include <set>
#include <vector>
#include <cassert>

#include "streamhelp.h"
#include "cmap.h"

#pragma once

struct FeatureMap {
    friend class chunker;
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
    friend class chunker;
    friend bool randtest(std::string &s, uint32_t iterations);
public:
    uint16_t getChunkCount() { return (uint16_t) chunkCount; }
    std::pair<uint32_t, uint32_t> getChunkRange(uint16_t cidx);
    std::string getRangeFileURI() { return rangeFileURI; }
    std::string getChunkURI(uint16_t idx);
    bool addcmap(std::istream &i, bool keepGIDMap = false) {
        bool r = readcmap(i, uniMap, &gidMap);
        if (r && !keepGIDMap)
            gidMap.clear();
        return r;
    }
    bool getMissingChunks(const std::vector<uint32_t> &unicodes,
                          const std::vector<uint32_t> &features,
                          std::set<uint16_t> &cl);
    void dumpChunkSet(std::ostream &os);
    void writeChunkSet(std::ostream &os, bool seekTo = false);
    void setChunkCount(uint32_t cc) {
        chunkCount = cc;
        chunkSet.clear();
        chunkSet.resize(chunkCount);
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
    uint32_t getGlyphCount() { return glyphCount; }
    unsigned int compile(std::ostream &o, uint32_t offset = 0);
    bool decompile(std::istream &i, uint32_t offset = 0);
    void dump(std::ostream &o, bool full = false);
    uint32_t getCharStringOffset() { return CFFCharStringsOffset; }
    bool updateChunkSet(std::set<uint16_t> &chunks) {
        for (auto idx: chunks) {
            if (idx >= chunkCount)
                return false;
            if (chunkSet[idx])
                return false;
            chunkSet[idx] = true;
        }
        return true;
    }
    uint32_t *getID() { return id; }
private:
    bool error(const char *m) {
        std::cerr << "IFTB table error: " << m << std::endl;
        return false;
    }
    uint16_t majorVersion {0}, minorVersion {1}, flags {0};
    uint32_t id[4];
    uint32_t CFFCharStringsOffset {0};
    uint32_t chunkCount {0}, glyphCount {0};
    std::vector<bool> chunkSet;
    std::vector<uint16_t> gidMap;
    std::map<uint32_t, FeatureMap> featureMap;
    std::vector<uint32_t> chunkOffsets;
    std::string filesURI, rangeFileURI;
    std::unordered_map<uint32_t, uint16_t> uniMap;
};
