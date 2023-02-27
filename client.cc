#include "client.h"
#include "streamhelp.h"
#include "tag.h"

bool iftb_client::loadFont(std::string &s, bool keepGIDMap) {
    return loadFont(s.data(), s.length(), keepGIDMap);
}

bool iftb_client::loadFont(char *buf, size_t length, bool keepGIDMap) {
    uint32_t tg = decodeBuffer(buf, length, fontData, extraPercent());
    if (tg != 0x00010000 && tg != tag("OTTO") && tg != tag("IFTC"))
        return error("Unrecognized font type.");

    sf.setBuffer(fontData);
    if (!sf.read()) {
        failed = true;
        return false;
    }

    simplestream ss;

    if (!sf.getTableStream(ss, T_IFTB))
        return error("No IFTB table in font");

    if (!tiftb.decompile(ss)) {
        failed = true;
        return false;
    }

    if (!sf.getTableStream(ss, T_CMAP))
        return error("No cmap table in font");

    if (!tiftb.addcmap(ss, keepGIDMap)) {
        failed = true;
        return false;
    }

    return true;
}

uint16_t iftb_client::getChunkCount() {
    if (!hasFont() or failed)
        return 0;
    return tiftb.getChunkCount();
}

bool iftb_client::setPending(const std::vector<uint32_t> &unicodes,
                             const std::vector<uint32_t> &features) {
    if (!hasFont() or failed)
        return false;
    return tiftb.getMissingChunks(unicodes, features,
                                  pendingChunks);
}

bool iftb_client::getPendingChunkList(std::vector<uint16_t> &cl) {
    if (!hasFont() or failed)
        return false;
    if (pendingChunks.size() == 0)
        return false;
    cl.clear();
    std::copy(pendingChunks.begin(), pendingChunks.end(),
              std::back_inserter(cl));
    return true;
}

std::string iftb_client::getRangeFileURI() {
    if (!hasFont() or failed)
        return "";
    return tiftb.getRangeFileURI();
}

std::string iftb_client::getChunkURI(uint16_t cidx) {
    if (!hasFont() or failed)
        return "";
    return tiftb.getChunkURI(cidx);
}

std::pair<uint32_t, uint32_t> iftb_client::getChunkRange(uint16_t cidx) {
    if (!hasFont() or failed)
        std::pair<uint32_t, uint32_t>(0,0);
    return tiftb.getChunkRange(cidx);
}
