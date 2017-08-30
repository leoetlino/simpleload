cmake_minimum_required(VERSION 3.5.0)

project(simpleload CXX ASM)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(MACHDEP "-DHW_RVL -DRVL -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} ${MACHDEP}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MACHDEP}")
# Has to be high enough to avoid our own binary being clobbered by game data.
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--section-start,.init=0x80F00000")

include_directories(${LIBOGCDIR}/include)
link_directories(${LIBOGCDIR}/lib/wii)

add_subdirectory(src)
