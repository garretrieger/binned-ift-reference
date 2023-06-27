/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

/* The iftb::wasm_wrapper and related functions provide a wasm-mediated
   interface to an iftb::client object.  This is only included on the
   client side but will not be included in non-browser clients.
 */

#include <string>
#include <vector>
#include <unordered_map>

#include "client.h"

#pragma once

namespace iftb {
    class wasm_wrapper;
}

class iftb::wasm_wrapper {
 public:
    wasm_wrapper() {}
    char *allocateBuffer(uint16_t cidx, uint32_t length) {
        if (cidx != 0) {
            if (cidx >= getChunkCount()) {
                error("Chunk index too high");
                return NULL;
            } else if (cl.hasChunk(cidx)) {
                return (char *) 1;
            }
        }
        auto i = buffers.emplace(cidx, "");
        i.first->second.resize(length);
        return i.first->second.data();
    }
    bool loadFont() {
        if (buffers.find(0) == buffers.end())
            return error("No font buffer allocated");
        bool r = cl.loadFont(buffers[0]);
        if (r)
            buffers.erase(buffers.find(0));
        return r;
    }
    bool hasFont() { return cl.hasFont(); }
    bool failure() { return cl.failure(); }
    uint16_t getChunkCount() { return cl.getChunkCount(); }
    uint32_t *setUnicodesLen(uint32_t length) {
        unicodes.clear();
        if (length > 0)
            unicodes.resize(length);
        return unicodes.data();
    }
    uint32_t *setFeaturesLen(uint32_t length) {
        features.clear();
        if (length > 0)
            features.resize(length);
        return features.data();
    }
    int setPendingList() { return cl.setPending(unicodes, features) ? 1 : 0; }
    uint16_t getPendingChunkListSize() {
        if (!cl.getPendingChunkList(pendingChunkList))
            return 0;
        return pendingChunkList.size();
    }
    uint16_t *getPendingChunkListLoc() {
        return pendingChunkList.data();
    }
    const char *getRangeFileURI() { return cl.getRangeFileURI().data(); }
    const char *getChunkURI(uint16_t cidx) { return cl.getChunkURI(cidx); }
    uint32_t getChunkOffset(uint16_t cidx) { return cl.getChunkOffset(cidx); }
    bool addChunkFromBuffer(uint16_t cidx, int setPending) {
        if (cidx == 0)
            return error("Attempt to add chunk 0");
        if (buffers.find(cidx) == buffers.end())
            return error("No chunk buffer found");
        bool r = cl.addChunk(cidx, buffers[cidx], setPending);
        if (r)
            buffers.erase(buffers.find(cidx));
        return r;
    }
    bool canMerge() { return cl.canMerge(); }
    bool merge(bool asIFTB = true) { return cl.merge(asIFTB); }
    uint32_t getFontLength() { return cl.fontData.size(); }
    const uint8_t *getFontLoc(bool asIFTB = true) {
        cl.setType(asIFTB);
        return (uint8_t *) cl.fontData.data();
    }
    bool error(const char *m) {
        std::cerr << "IFTB WASM wrapper Error: " << m << std::endl;
        cl.failed = true;
        return false;
    }
 private:
    std::unordered_map<uint16_t, std::string> buffers;
    std::vector<uint32_t> unicodes, features;
    std::vector<uint16_t> pendingChunkList;
    iftb::client cl;
};

extern "C" {

extern void *iftb_new_client();
extern void iftb_delete_client(void *v);
extern uint8_t *iftb_reserve_initial_font_data(void *v, uint32_t length);
extern int iftb_decode_initial_font(void *v);
extern int iftb_has_font(void *v);
extern int iftb_has_failed(void *v);
extern uint16_t iftb_get_chunk_count(void *v);
extern uint32_t *iftb_reserve_unicode_list(void *v, uint32_t length);
extern uint32_t *iftb_reserve_feature_list(void *v, uint32_t length);
extern int iftb_compute_pending(void *v);
extern uint16_t iftb_get_pending_list_count(void *v);
extern uint16_t *iftb_get_pending_list_location(void *v);
extern const char *iftb_range_file_uri(void *v);
extern const char *iftb_chunk_file_uri(void *v, uint16_t cidx);
extern uint32_t iftb_get_chunk_offset(void *v, uint16_t cidx);
extern uint8_t *iftb_reserve_chunk_data(void *v, uint16_t cidx,
                                        uint32_t length);
extern int iftb_use_chunk_data(void *v, uint16_t cidx, int forcePending);
extern int iftb_can_merge(void *v);
extern int iftb_merge(void *v, int as_iftb);
extern uint32_t iftb_get_font_length(void *v);
extern const uint8_t *iftb_get_font_location(void *v, int as_iftb);

}

