#include <iostream>
#include <vector>
#include <unordered_map>

#pragma once

bool readcmap(std::istream &is, uint32_t offset,
              std::unordered_map<uint32_t, uint16_t> &uniMap,
              std::vector<uint16_t> *gidMap);
