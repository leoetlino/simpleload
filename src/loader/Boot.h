#pragma once

#include <optional>
#include <type_traits>

#include "loader/DI.h"

namespace Boot
{
using GameEntryFunction = std::add_pointer_t<void()>;

// Adapted from TinyLoad (GPL, copyright 2008-2009  Hector Martin  <marcan@marcansoft.com>)
// Initialises DI and everything required to open the game partition.
// If reload_ios is true, the IOS title specified in the game's TMD will be launched.
// Returns the game partition.
std::optional<DI::Partition> InitAndOpenPartition(bool reload_ios);

// Adapted from TinyLoad (GPL, copyright 2008-2009  Hector Martin  <marcan@marcansoft.com>)
// Initialises the apploader, loads everything except branching to the game's entry point.
// Returns the game entry function.
GameEntryFunction InitApploader();

// Adapted from Dolphin (GPLv2+)
// Writes various constants that are required by games to low MEM1.
void PokeConstants(const DI::Partition& game_partition);

// Prepares the loader for launching another application.
// This will shut down most IOS services (DI, ES, STM, SD), so only call this right before
// branching to the entry point of another application.
bool CleanupBeforeLaunch();
}  // namespace Boot
