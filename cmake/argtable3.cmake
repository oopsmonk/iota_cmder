
if (NOT __ARGTABLE3_INCLUDED)
  set(__ARGTABLE3_INCLUDED TRUE)

  ExternalProject_Add(
    argtable3_download
    PREFIX ${PROJECT_BINARY_DIR}/argtable3
    DOWNLOAD_DIR ${PROJECT_BINARY_DIR}/download
    DOWNLOAD_NAME argtable-3.1.5-amalgamation.tar.gz
    URL https://github.com/argtable/argtable3/releases/download/v3.1.5.1c1bb23/argtable-3.1.5-amalgamation.tar.gz
    URL_HASH SHA256=336fadb994cd5aae06fd58b05afa8c53cdff6c9ae2fc30b7910863deedd6746c
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    BUILD_COMMAND ""
  )

  set(argtable3_cmake_dir ${PROJECT_BINARY_DIR}/argtable3/src/ext_argtable3)
  set(argtable3_src_dir ../argtable3_download)
  set(argtable3_install_include ${CMAKE_INSTALL_PREFIX}/include)
  set(argtable3_install_lib ${CMAKE_INSTALL_PREFIX}/lib)

  file(WRITE ${argtable3_cmake_dir}/CMakeLists.txt
    "cmake_minimum_required(VERSION 3.5)\n"
    "project(argtable3 C)\n"
    "set(my_src ${argtable3_src_dir}/argtable3.c)\n"
    "add_library(argtable3 STATIC \${my_src})\n"
    "target_include_directories(argtable3\n"
    "PUBLIC ${argtable3_src_dir})\n"
    "install(TARGETS argtable3 DESTINATION ${argtable3_install_lib})\n"
    "install(DIRECTORY ${argtable3_src_dir}/ DESTINATION ${argtable3_install_include} \n
    FILES_MATCHING PATTERN \"argtable3.h\"\n
    PATTERN tests EXCLUDE\n
    PATTERN examples EXCLUDE)\n"
  )

  ExternalProject_Add(
    ext_argtable3
    PREFIX ${PROJECT_BINARY_DIR}/argtable3
    DOWNLOAD_COMMAND ""
    BUILD_IN_SOURCE TRUE
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_INSTALL_PREFIX}
    #  -DCMAKE_TOOLCHAIN_FILE:STRING=${CMAKE_TOOLCHAIN_FILE}
    # for debug
    # LOG_CONFIGURE 1
    # LOG_INSTALL 1
  )
  add_dependencies(ext_argtable3 argtable3_download)

endif()