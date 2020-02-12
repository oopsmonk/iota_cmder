
if (NOT __LINENOISE_INCLUDED)
  set(__LINENOISE_INCLUDED TRUE)

  ExternalProject_Add(
    linenoise_download
    PREFIX ${PROJECT_BINARY_DIR}/linenoise
    DOWNLOAD_DIR ${PROJECT_BINARY_DIR}/download
    DOWNLOAD_NAME 4a961c0108720741e2683868eb10495f015ee422.tar.gz
    URL https://github.com/antirez/linenoise/archive/4a961c0108720741e2683868eb10495f015ee422.tar.gz
    URL_HASH SHA256=b2aa29de7480bf93fdd875a19800d5f087b7436ca77f27d0e2177232941e6938
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    BUILD_COMMAND ""
  )

  set(linenoise_cmake_dir ${PROJECT_BINARY_DIR}/linenoise/src/ext_linenoise)
  set(linenoise_src_dir ../linenoise_download)
  set(linenoise_install_include ${CMAKE_INSTALL_PREFIX}/include)
  set(linenoise_install_lib ${CMAKE_INSTALL_PREFIX}/lib)

  file(WRITE ${linenoise_cmake_dir}/CMakeLists.txt
    "cmake_minimum_required(VERSION 3.5)\n"
    "project(linenoise C)\n"
    "set(my_src ${linenoise_src_dir}/linenoise.c)\n"
    "add_library(linenoise STATIC \${my_src})\n"
    "target_include_directories(linenoise\n"
    "PUBLIC ${linenoise_src_dir})\n"
    "install(TARGETS linenoise DESTINATION ${linenoise_install_lib})\n"
    "install(DIRECTORY ${linenoise_src_dir}/ DESTINATION ${linenoise_install_include} FILES_MATCHING PATTERN \"linenoise.h\")\n"
  )

  ExternalProject_Add(
    ext_linenoise
    PREFIX ${PROJECT_BINARY_DIR}/linenoise
    DOWNLOAD_COMMAND ""
    BUILD_IN_SOURCE TRUE
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_INSTALL_PREFIX}
    #  -DCMAKE_TOOLCHAIN_FILE:STRING=${CMAKE_TOOLCHAIN_FILE}
    # for debug
    # LOG_CONFIGURE 1
    # LOG_INSTALL 1
  )
  add_dependencies(ext_linenoise linenoise_download)

endif()