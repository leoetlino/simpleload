#include <cstdio>
#include <optional>

#include "loader/Apploader.h"
#include "loader/Boot.h"
#include "loader/DI.h"

static int Fail(const char* message = "failed")
{
  printf("%s\n", message);
  return 1;
}

int main()
{
  printf("Initialising DI, opening partition and reloading IOS...\n");
  if (!Boot::InitAndOpenPartition(true))
    return Fail();

  printf("Initialising DI and opening partition again...\n");
  std::optional<DI::Partition> game_partition = Boot::InitAndOpenPartition(false);
  if (!game_partition)
    return Fail();

  printf("Initialising apploader...\n");
  Boot::GameEntryFunction game_entry = Boot::InitApploader();
  if (!game_entry)
    return Fail();

  printf("Poking constants...\n");
  Boot::PokeConstants(*game_partition);

  printf("Cleaning up before launch...\n");
  if (!Boot::CleanupBeforeLaunch())
    return Fail();

  printf("Launching game (%p)\n", game_entry);
  game_entry();
  return Fail();
}
