# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.11

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/build

# Include any dependencies generated for this target.
include CMakeFiles/sanity2.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/sanity2.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/sanity2.dir/flags.make

CMakeFiles/sanity2.dir/sfs_test2.c.o: CMakeFiles/sanity2.dir/flags.make
CMakeFiles/sanity2.dir/sfs_test2.c.o: ../sfs_test2.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/sanity2.dir/sfs_test2.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/sanity2.dir/sfs_test2.c.o   -c /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/sfs_test2.c

CMakeFiles/sanity2.dir/sfs_test2.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/sanity2.dir/sfs_test2.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/sfs_test2.c > CMakeFiles/sanity2.dir/sfs_test2.c.i

CMakeFiles/sanity2.dir/sfs_test2.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/sanity2.dir/sfs_test2.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/sfs_test2.c -o CMakeFiles/sanity2.dir/sfs_test2.c.s

CMakeFiles/sanity2.dir/disk_emu.c.o: CMakeFiles/sanity2.dir/flags.make
CMakeFiles/sanity2.dir/disk_emu.c.o: ../disk_emu.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/sanity2.dir/disk_emu.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/sanity2.dir/disk_emu.c.o   -c /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/disk_emu.c

CMakeFiles/sanity2.dir/disk_emu.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/sanity2.dir/disk_emu.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/disk_emu.c > CMakeFiles/sanity2.dir/disk_emu.c.i

CMakeFiles/sanity2.dir/disk_emu.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/sanity2.dir/disk_emu.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/disk_emu.c -o CMakeFiles/sanity2.dir/disk_emu.c.s

CMakeFiles/sanity2.dir/sfs_api.c.o: CMakeFiles/sanity2.dir/flags.make
CMakeFiles/sanity2.dir/sfs_api.c.o: ../sfs_api.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/sanity2.dir/sfs_api.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/sanity2.dir/sfs_api.c.o   -c /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/sfs_api.c

CMakeFiles/sanity2.dir/sfs_api.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/sanity2.dir/sfs_api.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/sfs_api.c > CMakeFiles/sanity2.dir/sfs_api.c.i

CMakeFiles/sanity2.dir/sfs_api.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/sanity2.dir/sfs_api.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/sfs_api.c -o CMakeFiles/sanity2.dir/sfs_api.c.s

# Object files for target sanity2
sanity2_OBJECTS = \
"CMakeFiles/sanity2.dir/sfs_test2.c.o" \
"CMakeFiles/sanity2.dir/disk_emu.c.o" \
"CMakeFiles/sanity2.dir/sfs_api.c.o"

# External object files for target sanity2
sanity2_EXTERNAL_OBJECTS =

sanity2: CMakeFiles/sanity2.dir/sfs_test2.c.o
sanity2: CMakeFiles/sanity2.dir/disk_emu.c.o
sanity2: CMakeFiles/sanity2.dir/sfs_api.c.o
sanity2: CMakeFiles/sanity2.dir/build.make
sanity2: CMakeFiles/sanity2.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C executable sanity2"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/sanity2.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/sanity2.dir/build: sanity2

.PHONY : CMakeFiles/sanity2.dir/build

CMakeFiles/sanity2.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/sanity2.dir/cmake_clean.cmake
.PHONY : CMakeFiles/sanity2.dir/clean

CMakeFiles/sanity2.dir/depend:
	cd /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/build /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/build /home/yuxma/ws/ece427/ece427-operating-system/single_layer_fuse/unorganized/build/CMakeFiles/sanity2.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/sanity2.dir/depend

