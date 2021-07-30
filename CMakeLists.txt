cmake_minimum_required(VERSION 3.11)

cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})

project(iota_cmder DESCRIPTION "IOTA commander")

enable_language(C)
enable_testing()

# fetch iota.c
include(FetchContent)
FetchContent_Declare(
  iota.c
  GIT_REPOSITORY http://github.com/iotaledger/iota.c.git
  GIT_TAG e513f6120106576a1bf7fc2cb4bff21e0bd9990b
)

if(${CMAKE_VERSION} VERSION_LESS 3.14)
    macro(FetchContent_MakeAvailable NAME)
        FetchContent_GetProperties(${NAME})
        if(NOT ${NAME}_POPULATED)
            FetchContent_Populate(${NAME})
            add_subdirectory(${${NAME}_SOURCE_DIR} ${${NAME}_BINARY_DIR})
        endif()
    endmacro()
endif()

message(STATUS "Fetching iota.c")
FetchContent_MakeAvailable(iota.c)

# external libs module
include(ExternalProject)

# libs in the sandbox
link_directories("${CMAKE_INSTALL_PREFIX}/lib")

# external libs
include(cmake/argtable3.cmake)
include(cmake/linenoise.cmake)

if(IOTA_ASAN_ENABLED)
  if(UNIX AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT CMAKE_BUILD_TYPE))
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
      # uthash causes runtime error if enable the unsigned-integer-overflow sanitizer in clang.
      # see: https://github.com/pmodels/mpich/issues/3322
      # add_compile_options("-fsanitize=address,undefined,unsigned-integer-overflow"
      #   "-fno-omit-frame-pointer"
      #   "-fsanitize-address-use-after-scope"
      #   "-fno-sanitize-recover=undefined,unsigned-integer-overflow"
      # )
      add_compile_options("-fsanitize=address,undefined"
        "-fno-omit-frame-pointer"
        "-fno-sanitize=unsigned-integer-overflow"
        "-fsanitize-address-use-after-scope"
        "-fno-sanitize-recover=undefined"
      )
      add_link_options("-fsanitize=address,undefined")
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
      add_compile_options("-fsanitize=address,undefined"
        "-fno-omit-frame-pointer"
        "-fsanitize-address-use-after-scope"
        "-fno-sanitize-recover=undefined"
      )
      add_link_options("-fsanitize=address,undefined")
    endif()
  else()
    message(WARNING "Sanitizer is not supported on Windows or Release build")
  endif()
endif()


# your source files
add_executable(${CMAKE_PROJECT_NAME}
"iota_cmder.c"
"cli_cmd.c"
"split_argv.c"
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES C_STANDARD_REQUIRED NO C_STANDARD 99)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${CMAKE_INSTALL_PREFIX}/include"
  "${CMAKE_INSTALL_PREFIX}/include/cjson"
  "${iota.c_SOURCE_DIR}"
)

add_dependencies(${CMAKE_PROJECT_NAME}
  "ext_argtable3"
  "ext_linenoise"
)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC
  iota_wallet
  argtable3
  linenoise
  m # linenoise
)
