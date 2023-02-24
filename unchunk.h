#include <iostream>

#include "merger.h"

#pragma once

static uint16_t chunk_addrecs(std::istream &is, merger &m);
static void dump_chunk(std::ostream &os, std::istream &is);
