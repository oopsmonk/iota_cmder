cmake_minimum_required(VERSION 3.11)

cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})

project(iota_cmder DESCRIPTION "The IOTA commander")

enable_language(C)
enable_testing()

# fetch iota.c
include(FetchContent)
FetchContent_Declare(
  iota.c
  GIT_REPOSITORY http://github.com/iotaledger/iota.c.git
  GIT_TAG 3f1c255bd6e7ccc19bd527a83fa4593d342cad32
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

# your source files
add_executable(${CMAKE_PROJECT_NAME}
"iota_cmder.c"
"cli_cmd.c"
"split_argv.c"
)

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
  cclient
  cjson # iota.c
  http_parser # iota.c
  mbedtls # iota.c
  mbedcrypto # iota.c
  mbedx509 # iota.c
  common
  logger # iota_common
  keccak # iota_common
  argtable3
  linenoise
  m # linenoise
)
