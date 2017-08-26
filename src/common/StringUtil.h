#pragma once

#include <string>

#include "CommonTypes.h"

std::string ArrayToString(const u8* data, u32 size, int line_len = 20, bool spaces = true);
