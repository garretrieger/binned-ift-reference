/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

/* These methods provide information about the correctness or contents of
   an IFTB-encoded font file. They are only included in the encoder.
 */

#include <string>

#include "config.h"

#pragma once

namespace iftb {
    bool sanitize(std::string &s, iftb::config &conf);
    void info(std::string &s, iftb::config &conf);
}
