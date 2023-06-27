/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

#include "wasm_wrapper.h"

void *iftb_new_client() {
    return new iftb::wasm_wrapper();
}

void iftb_delete_client(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    delete cl;
}

uint8_t *iftb_reserve_initial_font_data(void *v, uint32_t length) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return (uint8_t *) cl->allocateBuffer(0, length);
}

int iftb_decode_initial_font(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->loadFont() ? 1 : 0;
}

int iftb_has_font(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->hasFont() ? 1 : 0;
}

int iftb_has_failed(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->failure() ? 1 : 0;
}

uint16_t iftb_get_chunk_count(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->getChunkCount();
}

uint32_t *iftb_reserve_unicode_list(void *v, uint32_t length) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->setUnicodesLen(length);
}

uint32_t *iftb_reserve_feature_list(void *v, uint32_t length) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->setFeaturesLen(length);
}

int iftb_compute_pending(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->setPendingList() ? 1 : 0;
}

uint16_t iftb_get_pending_list_count(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->getPendingChunkListSize();
}

uint16_t *iftb_get_pending_list_location(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->getPendingChunkListLoc();
}

const char *iftb_range_file_uri(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->getRangeFileURI();
}

const char *iftb_chunk_file_uri(void *v, uint16_t cidx) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->getChunkURI(cidx);
}

uint32_t iftb_get_chunk_offset(void *v, uint16_t cidx) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->getChunkOffset(cidx);
}

uint8_t *iftb_reserve_chunk_data(void *v, uint16_t cidx,
                                        uint32_t length) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    if (cidx == 0) {
        cl->error("Cannot reserve chunk data for index 0");
        return NULL;
    }
    return (uint8_t *) cl->allocateBuffer(cidx, length);
}

int iftb_use_chunk_data(void *v, uint16_t cidx, int forcePending) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->addChunkFromBuffer(cidx, forcePending) ? 1 : 0;
}

int iftb_can_merge(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->canMerge() ? 1 : 0;
}

int iftb_merge(void *v, int as_iftb) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->merge(as_iftb) ? 1 : 0;
}

uint32_t iftb_get_font_length(void *v) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->getFontLength();
}

const uint8_t *iftb_get_font_location(void *v, int as_iftb) {
    iftb::wasm_wrapper *cl = static_cast<iftb::wasm_wrapper *>(v);
    return cl->getFontLoc(as_iftb);
}
