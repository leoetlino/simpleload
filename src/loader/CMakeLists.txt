add_executable(main
  Main.cpp
  Boot.cpp
  DI.cpp
)

target_link_libraries(main
  common
  fat
  ogc
  m
)
