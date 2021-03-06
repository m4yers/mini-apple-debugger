find_package(Threads REQUIRED)

include(ExternalProject)

ExternalProject_Add(
  gtest
  URL https://github.com/google/googletest/archive/master.zip
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
  # Disable install step
  INSTALL_COMMAND "")

ExternalProject_Get_Property(gtest source_dir binary_dir)

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

set(SUFFIX "")

# Set libgtest properties
set_target_properties(libgtest PROPERTIES
  "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/${SUFFIX}/libgtest.a"
  "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

# Create a libgmock target to be used as a dependency by test programs
add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)

# Set libgmock properties
set_target_properties(libgmock PROPERTIES
  "IMPORTED_LOCATION" "${binary_dir}/googlemock/${SUFFIX}/libgmock.a"
  "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

include_directories(
  "${source_dir}/googletest/include"
  "${source_dir}/googlemock/include")

# Add all the tests in the folder
macro(get_subdirlist result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

get_subdirlist(TEST_SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

foreach(test ${TEST_SUBDIRS})
  add_subdirectory(${test})
endforeach()