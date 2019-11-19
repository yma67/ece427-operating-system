cmake_minimum_required(VERSION 3.11)

project(mountable_simple_file_system)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -g -Wall")

find_package(PkgConfig)
pkg_check_modules(FUSE "fuse")

include_directories(${PROJECT_SOURCE_DIR})

# Sanity tests
add_executable(sanity1 sfs_test.c disk_emu.c sfs_api.c)
add_executable(sanity2 sfs_test2.c disk_emu.c sfs_api.c)

# FUSE Mounted Mode
add_executable(mount fuse_wrappers.c disk_emu.c sfs_api.c)
target_compile_options(mount PRIVATE -D_FILE_OFFSET_BITS=64)
target_link_libraries(mount ${FUSE_LIBRARIES})
