cmake_minimum_required(VERSION 3.3)
project(loopless)
enable_language(C)

include(ExternalProject)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

ExternalProject_Add(
        Lwext4
        GIT_REPOSITORY https://github.com/gkostka/lwext4.git
        GIT_TAG master
        CMAKE_COMMAND ${CMAKE_CMD}
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        INSTALL_COMMAND ""
)

ExternalProject_Get_Property(Lwext4 source_dir binary_dir)

include_directories(${source_dir}/include ${source_dir}/blockdev ${binary_dir}/include)
link_directories(${binary_dir}/src ${binary_dir}/blockdev)

set(CONFIGURE_ARGS CACHE STRING "Arguments for autoconf configure")
set(BINARY_BUILD_PREFIX CACHE STRING "Build command for Syslinux")

ExternalProject_Add(
        Syslinux
        GIT_REPOSITORY git://repo.or.cz/syslinux.git
        GIT_TAG bf6db5b4
        BUILD_IN_SOURCE 1
       # UPDATE_COMMAND git reset --hard
        BUILD_COMMAND ${BINARY_BUILD_PREFIX} ${CMAKE_SOURCE_DIR}/build-syslinux.sh
        CONFIGURE_COMMAND ""
        INSTALL_COMMAND ""
)

ExternalProject_Get_Property(Syslinux source_dir)
include_directories(${source_dir})
set(syslinux_sources
        ${source_dir}/libinstaller/setadv.c
        ${source_dir}/libinstaller/syslxmod.c
        ${source_dir}/bios/libinstaller/bootsect_bin.c
        ${source_dir}/bios/libinstaller/ldlinuxc32_bin.c
        ${source_dir}/bios/libinstaller/ldlinux_bin.c
        )

set(gptmbr "${CMAKE_CURRENT_BINARY_DIR}/gptmbr.c")
add_custom_command(OUTPUT gptmbr.c
        COMMAND cd "${source_dir}/bios/mbr" && xxd -i gptmbr.bin ${gptmbr}
        DEPENDS Syslinux )


ExternalProject_Add(
        Gptfdisk
        GIT_REPOSITORY git://git.code.sf.net/p/gptfdisk/code
        GIT_TAG 846a9e30cda88f75369d175f2f549cad3ea15db2
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
)

ExternalProject_Get_Property(Gptfdisk source_dir)

include_directories(${source_dir})
set(gdisk_sources
        ${source_dir}/crc32.cc
        ${source_dir}/support.cc
        ${source_dir}/guid.cc
        ${source_dir}/gptpart.cc
        ${source_dir}/mbrpart.cc
        ${source_dir}/basicmbr.cc
        ${source_dir}/mbr.cc
        ${source_dir}/gpt.cc
        ${source_dir}/bsd.cc
        ${source_dir}/parttypes.cc
        ${source_dir}/attributes.cc
        ${source_dir}/diskio.cc
        ${source_dir}/diskio-unix.cc
        )

ExternalProject_Add(
        Libuuid
        GIT_REPOSITORY https://git.kernel.org/pub/scm/fs/ext2/e2fsprogs.git
        GIT_TAG v1.42.13
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ./configure ${CONFIGURE_ARGS}
        BUILD_COMMAND ${BINARY_BUILD_PREFIX} make V=99 util/subst && make -C lib/uuid V=99
        INSTALL_COMMAND ""
)

ExternalProject_Get_Property(Libuuid source_dir)

include_directories(${source_dir}/lib/uuid)
link_directories(${source_dir}/lib)

file(STRINGS VERSION APP_VERSION)

add_definitions(-D_FILE_OFFSET_BITS=64 -DLOOPLESS_VERSION="${APP_VERSION}")
# This is needed because we only need a few functions from syslinux
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -undefined dynamic_lookup")
else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--unresolved-symbols=ignore-all")
endif()

set_source_files_properties(${syslinux_sources} ${gdisk_sources} ${gptmbr} PROPERTIES GENERATED true)
add_executable(loopless loopless.c ${syslinux_sources} disks.cpp ${gdisk_sources} ${gptmbr})
target_link_libraries(loopless lwext4 blockdev uuid)
add_dependencies(loopless Lwext4 Syslinux Gptfdisk Libuuid)
