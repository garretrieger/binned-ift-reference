#include <string>

#include "config.h"

#pragma once

namespace iftb {
    bool sanitize(std::string &s, iftb::config &conf);
    void info(std::string &s, iftb::config &conf);
}
