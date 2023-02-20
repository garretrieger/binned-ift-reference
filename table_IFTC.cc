#include "table_IFTC.h"

#include <cassert>

constexpr uint32_t relOffsetsOffset = 20;

void table_IFTC::writeChunkMap(std::ostream &os) {
    uint8_t u8 = 0;
    for (int i = 0; i < chunkCount + 8; i++) {
        if (i && i % 8 != 0) {
            writeObject(os, u8);
            u8 = 0;
        }
        u8 = (u8 << 1) + chunkMap[i] ? 1 : 0;
    }
}

uint32_t table_IFTC::compile(std::ostream &os, uint32_t offset) {
    uint32_t gidMapTableOffset = 0, chunkOffsetTableOffset = 0;
    uint32_t featureMapTableOffset = 0;
    os.seekp(offset);
    writeObject(os, majorVersion);
    writeObject(os, minorVersion);
    writeObject(os, (uint16_t) 0);  // reserved
    writeObject(os, flags);
    writeObject(os, chunkCount);
    writeObject(os, gidCount);
    writeObject(os, CFFCharStringsOffset);
    writeObject(os, gidMapTableOffset);
    writeObject(os, chunkOffsetTableOffset);
    writeObject(os, featureMapTableOffset);
    writeChunkMap(os);
    assert(filesURI.length() < 256);
    writeObject(os, (uint8_t) filesURI.length());
    for (auto c: filesURI)
        writeObject(os, c);
    writeObject(os, (uint8_t) 0);
    assert(rangeFileURI.length() < 256);
    writeObject(os, (uint8_t) rangeFileURI.length());
    for (auto c: rangeFileURI)
        writeObject(os, c);
    writeObject(os, (uint8_t) 0);
    gidMapTableOffset = ((uint32_t) os.tellp()) - offset;
    assert(gidMap.size() == gidCount);
    bool writing = false;
    for (uint32_t i = 0; i < gidCount; i++) {
        if (!writing && gidMap[i] == 0)
            continue;
        else if (!writing) {
            writeObject(os, (uint16_t) i);
            writing = true;
        }
        writeChunkIndex(os, gidMap[i]);
    }
    if (chunkOffsets.size() > 0) {
        assert(chunkOffsets.size() == chunkCount);
        chunkOffsetTableOffset = ((uint32_t) os.tellp()) - offset;
        for (auto i: chunkOffsets)
            writeObject(os, i);
    }
    if (featureMap.size() > 0) {
        featureMapTableOffset = ((uint32_t) os.tellp()) - offset;
        writeObject(os, (uint16_t)featureMap.size());
        for (auto &[t, fm]: featureMap) {
            assert(fm.ranges.size() > 0);
            writeObject(os, t);
            writeChunkIndex(os, fm.startIndex);
            writeChunkIndex(os, fm.ranges.size());
        }
        for (auto &[t, fm]: featureMap) {
            for (auto &[start, end]: fm.ranges) {
                writeChunkIndex(os, start);
                writeChunkIndex(os, end);
            }
        }
    }
    uint32_t l = ((uint32_t) os.tellp()) - offset;
    os.seekp(offset + relOffsetsOffset);
    writeObject(os, gidMapTableOffset);
    writeObject(os, chunkOffsetTableOffset);
    writeObject(os, featureMapTableOffset);
    return l;
}

void table_IFTC::decompile(std::istream &is, uint32_t offset) {
    uint32_t gidMapTableOffset, chunkOffsetTableOffset;
    uint32_t featureMapTableOffset;
    uint16_t firstMappedGid;
    is.seekg(offset);
    readObject(is, majorVersion);
    readObject(is, minorVersion);
    readObject<uint16_t>(is);  // reserved
    readObject(is, flags);
    readObject(is, chunkCount);
    readObject(is, gidCount);
    readObject(is, CFFCharStringsOffset);
    readObject(is, gidMapTableOffset);
    readObject(is, chunkOffsetTableOffset);
    readObject(is, featureMapTableOffset);
    uint8_t u8;
    chunkMap.resize(chunkCount + 8);
    for (int i=0; i < chunkCount/8 + 1; i++) {
        readObject(is, u8);
        for (int j = 0; j < 8; j++) {
            chunkMap[i * 8 + j] = u8 & 1;
            u8 >>= 1;
        }
    }
    filesURI.resize(readObject<uint8_t>(is));
    for (int i = 0; i < u8; i++)
        readObject(is, filesURI[i]);
    readObject<uint8_t>(is);  // Null terminator
    readObject(is, u8);
    rangeFileURI.resize(readObject<uint8_t>(is));
    for (int i = 0; i < u8; i++)
        readObject(is, rangeFileURI[i]);
    readObject<uint8_t>(is);  // Null terminator
    is.seekg(offset + gidMapTableOffset);
    readObject(is, firstMappedGid);
    for (uint32_t i = 0; i < gidCount; i++) {
        if (i < firstMappedGid)
            gidMap.push_back(0);
        else {
            gidMap.push_back(readChunkIndex(is));
        }
    }
    chunkOffsets.clear();
    if (chunkOffsetTableOffset != 0) {
        is.seekg(offset + chunkOffsetTableOffset);
        for (int i = 0; i < chunkCount; i++)
            chunkOffsets.push_back(readObject<uint32_t>(is));
    }
    featureMap.clear();
    if (featureMapTableOffset != 0) {
        is.seekg(offset + featureMapTableOffset);
        uint16_t featureMapCount;
        readObject(is, featureMapCount);
        for (int i = 0; i < featureMapCount; i++) {
            uint32_t feat;
            FeatureMap fm;
            readObject(is, feat);
            fm.startIndex = readChunkIndex(is);
            fm.ranges.resize(readChunkIndex(is));
            featureMap.emplace(feat, std::move(fm));
        }
        for (auto &[_, fm]: featureMap) {
            for (auto &[start, end]: fm.ranges) {
                start = readChunkIndex(is);
                end = readChunkIndex(is);
            }
        }
    }
}
