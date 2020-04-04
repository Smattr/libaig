add_library(libaig
  src/bitbuffer.c
  src/free.c
  src/getters.c
  src/infer.c
  src/level.c
  src/load.c
  src/lookup.c
  src/new.c
  src/node.c
  src/node_iter.c
  src/parse.c
  src/sat.c)

target_include_directories(libaig
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  src)

set_target_properties(libaig PROPERTIES PREFIX "")

install(TARGETS libaig EXPORT LibaigConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(TARGETS libaig FILE LibaigConfig.cmake)
