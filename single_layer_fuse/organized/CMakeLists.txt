cmake_minimum_required(VERSION 3.11)
project(mountable_simple_file_system)

# Debug Mode
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "-std=gnu99 ${CMAKE_C_FLAGS_DEBUG} -Wall")
message(STATUS "Verify C compiler flags: " ${CMAKE_C_FLAGS_DEBUG})

# Find Package FUSE
find_package(PkgConfig)
pkg_check_modules(FUSE "fuse")

# MSFS dependency
set(MSFS_SRC_DIR src)
set(MSFS_INC_DIR include)
include_directories(${MSFS_INC_DIR})

# Sanity tests
add_executable(sanity1 ${MSFS_SRC_DIR}/sfs_test.c ${MSFS_SRC_DIR}/disk_emu.c ${MSFS_SRC_DIR}/sfs_api.c)
add_executable(sanity2 ${MSFS_SRC_DIR}/sfs_test2.c ${MSFS_SRC_DIR}/disk_emu.c ${MSFS_SRC_DIR}/sfs_api.c)

# FUSE Mounted Mode
add_executable(mount ${MSFS_SRC_DIR}/fuse_wrappers.c ${MSFS_SRC_DIR}/disk_emu.c ${MSFS_SRC_DIR}/sfs_api.c)
target_compile_options(mount PRIVATE -D_FILE_OFFSET_BITS=64)
target_link_libraries(mount ${FUSE_LIBRARIES})
