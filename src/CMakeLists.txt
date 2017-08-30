add_definitions(-Wall -fno-exceptions -fno-strict-aliasing)
SET(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")

include(CheckCXXCompilerFlag)
macro(check_and_add_flag var flag)
  CHECK_CXX_COMPILER_FLAG(${flag} FLAG_${var})
  if(FLAG_${var})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
  endif()
endmacro()

# Enable C++17, but fall back to C++14 if it isn't available.
# CMAKE_CXX_STANDARD cannot be used here because we require C++14 or newer, not any standard.
# The Dolphin source uses POSIX features that are disabled if EXTENSIONS is off.
check_and_add_flag(CXX17 -std=gnu++17)
if(NOT FLAG_CXX_CXX17)
  check_and_add_flag(CXX1Z -std=gnu++1z)
  if (NOT FLAG_CXX_CXX1Z)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS ON)
  endif()
endif()

include_directories(SYSTEM common/compat)
include_directories(.)

add_subdirectory(common)
add_subdirectory(loader)
