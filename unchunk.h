#include <iostream>
#include <filesystem>

#include "merger.h"
#include "table_IFTB.h"

#pragma once

uint16_t chunkAddRecs(std::istream &is, merger &m);
void dumpChunk(std::ostream &os, std::istream &is);
std::filesystem::path getChunkPath(std::filesystem::path &bp,
                                   table_IFTB &tiftb, uint16_t idx);
std::filesystem::path getRangePath(std::filesystem::path &bp,
                                   table_IFTB &tiftb);
std::string decodeChunk(const std::string &s);
