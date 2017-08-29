add_executable(main
  Main.cpp
  Boot.cpp
  CodeHandler.cpp
  DI.cpp
  executable_readers/Reader.cpp
  executable_readers/ElfReader.cpp
)

target_link_libraries(main
  common
  fat
  ogc
  m
)
