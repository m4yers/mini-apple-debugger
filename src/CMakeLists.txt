file (GLOB_RECURSE CppSource ${CMAKE_SOURCE_DIR}/src/MAD/*.cpp)
file (GLOB_RECURSE ObjCSource ${CMAKE_SOURCE_DIR}/src/MAD/*.mm)

add_executable (main
  ${CMAKE_SOURCE_DIR}/src/Main.cpp
  ${ObjCSource}
  ${CppSource})
