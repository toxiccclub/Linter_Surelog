cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(MY_LINTER VERSION 1.0 LANGUAGES CXX)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type" FORCE)
endif()

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(external/Surelog)

set(LINTER_SRC
    linter/src/lint.cpp
    linter/src/class_variable_lifetime.cpp
    linter/src/dpi_decl_string.cpp
    linter/src/fatal_rule.cpp
    linter/src/hierarchical_interface_identifier.cpp
    linter/src/implicit_data_type.cpp
    linter/src/parametr_dynamic_array.cpp
    linter/src/prototype_return_data_type.cpp
    linter/src/repet_in_sequence.cpp
    linter/src/rule_distpatcher.cpp
)

foreach(f ${LINTER_SRC})
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
    message(FATAL_ERROR "Linter source file not found: ${f}\nCheck for typos/incorrect path in CMakeLists.txt")
  endif()
endforeach()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_executable(lint ${LINTER_SRC})

target_include_directories(lint
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/linter/include>
)

target_link_libraries(lint PUBLIC surelog)

if(MSVC)
    target_compile_options(lint PRIVATE /W4 /bigobj)
else()
    target_compile_options(lint PRIVATE -Wall -O3 -Wextra -Wpedantic)
endif()

# Install rule
include(GNUInstallDirs)
install(TARGETS lint
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

