file (GLOB ProjectSource ${CMAKE_SOURCE_DIR}/src/Debugger/*.cpp)
file (GLOB TestSource *.cpp)

add_executable(debugger ${TestSource} ${ProjectSource})

target_link_libraries(debugger libgtest libgmock)

add_test(NAME debugger COMMAND debugger)
