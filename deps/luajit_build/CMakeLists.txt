# Added LUA_ADD_EXECUTABLE Ryan Phillips <ryan at trolocsis.com>
# This CMakeLists.txt has been first taken from LuaDist
# Copyright (C) 2007-2011 LuaDist.
# Created by Peter Drahoš
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# Debugged and (now seriously) modified by Ronan Collobert, for Torch7
# It then made its way into the LUVIT LUV project,
# then was ported to LOVR with cross compile abilities added.

project(LuaJIT C ASM)

message("Building LuaJIT...")
if (LJ_CROSS)
message("Building LuaJIT as a cross compile")
endif()
if (LJ_CROSS_INNER)
message("Building the tools for a LuaJIT cross compile")
endif()

message("Deletethisdebug-start\nDASM_ARCH ${DASM_ARCH}\nDASM_FLAGS ${DASM_FLAGS}\nDASM_FLAGS_SERIAL ${DASM_FLAGS_SERIAL}\nTARGET_ARCH_SERIAL ${TARGET_ARCH_SERIAL}\nTARGET_ARCH ${TARGET_ARCH}\nTARGET_LJARCH ${TARGET_LJARCH}")

if (WIN32)
    ADD_DEFINITIONS(-DLUA_BUILD_AS_DLL -DLUA_USE_DLOPEN)
endif()

SET(LUAJIT_DIR ${CMAKE_CURRENT_LIST_DIR}/../luajit)

SET(CMAKE_REQUIRED_INCLUDES
  ${LUAJIT_DIR}
  ${LUAJIT_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
)

# We need to pass a list between two CMake invocations, but we're not given a good way to do this.
# So we pick a join separator so weird there is no risk of it occurring naturally.
#set(JOINER "SeP___SeP")

OPTION(WITH_AMALG "Build eveything in one shot (needs memory)" ON)

# Ugly warnings
IF(MSVC)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

# Various includes
INCLUDE(CheckLibraryExists)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckTypeSize)

# LuaJIT specific
option(LUAJIT_DISABLE_FFI "Disable FFI." OFF)
option(LUAJIT_ENABLE_LUA52COMPAT "Enable Lua 5.2 compatibility." ON)
option(LUAJIT_DISABLE_JIT "Disable JIT." OFF)
option(LUAJIT_CPU_SSE2 "Use SSE2 instead of x87 instructions." ON)
option(LUAJIT_CPU_NOCMOV "Disable NOCMOV." OFF)
MARK_AS_ADVANCED(LUAJIT_DISABLE_FFI LUAJIT_ENABLE_LUA52COMPAT LUAJIT_DISABLE_JIT LUAJIT_CPU_SSE2 LUAJIT_CPU_NOCMOV)

IF(LUAJIT_DISABLE_FFI)
  ADD_DEFINITIONS(-DLUAJIT_DISABLE_FFI)
ENDIF()

IF(LUAJIT_ENABLE_LUA52COMPAT)
  ADD_DEFINITIONS(-DLUAJIT_ENABLE_LUA52COMPAT)
ENDIF()

IF(LUAJIT_DISABLE_JIT)
  ADD_DEFINITIONS(-DLUAJIT_DISABLE_JIT)
ENDIF()

IF(LUAJIT_CPU_SSE2)
  ADD_DEFINITIONS(-DLUAJIT_CPU_SSE2)
ENDIF()

IF(LUAJIT_CPU_NOCMOV)
  ADD_DEFINITIONS(-DLUAJIT_CPU_NOCMOV)
ENDIF()
######


CHECK_TYPE_SIZE("void*" SIZEOF_VOID_P)
IF(SIZEOF_VOID_P EQUAL 8)
  ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE)
ENDIF()

if ( WIN32 AND NOT CYGWIN )
  set ( LJVM_MODE peobj )
elseif ( APPLE )
  if (NOT LJ_CROSS_INNER) # It's only the VM that needs this, not the build-host tools
    set ( CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000 ${CMAKE_EXE_LINKER_FLAGS}" )
  endif ()
  set ( LJVM_MODE machasm )
else ()
  set ( LJVM_MODE elfasm )
endif ()

IF(NOT WIN32)
  FIND_LIBRARY(DL_LIBRARY "dl")
  IF(DL_LIBRARY)
    SET(CMAKE_REQUIRED_LIBRARIES ${DL_LIBRARY})
    LIST(APPEND LIBS ${DL_LIBRARY})
  ENDIF(DL_LIBRARY)
  CHECK_FUNCTION_EXISTS(dlopen LUA_USE_DLOPEN)
  IF(NOT LUA_USE_DLOPEN)
    MESSAGE(FATAL_ERROR "Cannot compile a useful lua.
Function dlopen() seems not to be supported on your platform.
Apparently you are not on a Windows platform as well.
So lua has no way to deal with shared libraries!")
  ENDIF(NOT LUA_USE_DLOPEN)
ENDIF(NOT WIN32)

check_library_exists(m sin "" LUA_USE_LIBM)
if ( LUA_USE_LIBM )
  list ( APPEND LIBS m )
endif ()

if ( CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
  list ( APPEND LIBS pthread c++abi )
endif ()

## SOURCES
MACRO(LJ_TEST_ARCH stuff)
  CHECK_C_SOURCE_COMPILES("
#undef ${stuff}
#include \"lj_arch.h\"
#if ${stuff}
int main() { return 0; }
#else
#error \"not defined\"
#endif
" ${stuff})
ENDMACRO()

MACRO(LJ_TEST_ARCH_VALUE stuff value)
  CHECK_C_SOURCE_COMPILES("
#undef ${stuff}
#include \"lj_arch.h\"
#if ${stuff} == ${value}
int main() { return 0; }
#else
#error \"not defined\"
#endif
" ${stuff}_${value})
ENDMACRO()

IF (NOT TARGET_LJARCH) # If no specified arch, discover one
  FOREACH(arch X64 X86 ARM ARM64 PPC PPCSPE MIPS)
    LJ_TEST_ARCH(LJ_TARGET_${arch})
    if(LJ_TARGET_${arch})
      STRING(TOLOWER ${arch} TARGET_LJARCH)
      MESSAGE(STATUS "LuaJIT Target: ${TARGET_LJARCH}")
      BREAK()
    ENDIF()
  ENDFOREACH()
ENDIF ()

IF(NOT TARGET_LJARCH) # If no discovered arch
  MESSAGE(FATAL_ERROR "architecture not supported")
ELSE()
  MESSAGE(STATUS "LuaJIT target ${TARGET_LJARCH}")
ENDIF()

IF (NOT LJ_CROSS_INNER) # Don't create data dirs on the tools pass

  FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/jit)
  FILE(GLOB jit_files ${LUAJIT_DIR}/src/jit/*.lua)
  FILE(COPY ${jit_files} DESTINATION ${CMAKE_BINARY_DIR}/jit)

ENDIF (NOT LJ_CROSS_INNER)

SET(DASM_ARCH ${TARGET_LJARCH})
SET(DASM_FLAGS)

if (TARGET_ARCH_SERIAL) # The semicolons in these lists get turned into spaces by CMake string interpolation. Turn them back
  set(TARGET_ARCH ${TARGET_ARCH_SERIAL})
  separate_arguments(TARGET_ARCH)
  set(DASM_FLAGS ${DASM_FLAGS_SERIAL})
  separate_arguments(DASM_FLAGS)
  message("Deletethis-unpackedserial\nDASM_ARCH ${DASM_ARCH}\nDASM_FLAGS ${DASM_FLAGS}\nTARGET_ARCH ${TARGET_ARCH}\nTARGET_LJARCH ${TARGET_LJARCH}")
endif()

if (NOT TARGET_ARCH) # On tools pass, target arch and dasm flags will be passed in

  SET(TARGET_ARCH)

  # This was in the luvit cmake file, but it doesn't actually make sense
  #if (NOT LJ_CROSS_INNER)
  LIST(APPEND TARGET_ARCH "LUAJIT_TARGET=LUAJIT_ARCH_${TARGET_LJARCH}")
  #endif()
  LJ_TEST_ARCH_VALUE(LJ_ARCH_BITS 64)
  IF(LJ_ARCH_BITS_64)
    SET(DASM_FLAGS ${DASM_FLAGS} -D P64)
  ENDIF()
  LJ_TEST_ARCH_VALUE(LJ_HASJIT 1)
  IF(LJ_HASJIT_1)
    SET(DASM_FLAGS ${DASM_FLAGS} -D JIT)
  ENDIF()
  LJ_TEST_ARCH_VALUE(LJ_HASFFI 1)
  IF(LJ_HASFFI_1)
    SET(DASM_FLAGS ${DASM_FLAGS} -D FFI)
  ENDIF()
  LJ_TEST_ARCH_VALUE(LJ_DUALNUM 1)
  IF(LJ_DUALNUM_1)
    SET(DASM_FLAGS ${DASM_FLAGS} -D DUALNUM)
  ENDIF()
  LJ_TEST_ARCH_VALUE(LJ_ARCH_HASFPU 1)
  IF(LJ_ARCH_HASFPU_1)
    SET(DASM_FLAGS ${DASM_FLAGS} -D FPU)
    LIST(APPEND TARGET_ARCH "LJ_ARCH_HASFPU=1")
  ELSE()
    LIST(APPEND TARGET_ARCH "LJ_ARCH_HASFPU=0")
  ENDIF()
  LJ_TEST_ARCH_VALUE(LJ_ABI_SOFTFP 1)
  IF(NOT LJ_ABI_SOFTFP_1)
    SET(DASM_FLAGS ${DASM_FLAGS} -D HFABI)
    LIST(APPEND TARGET_ARCH "LJ_ABI_SOFTFP=0")
  ELSE()
    LIST(APPEND TARGET_ARCH "LJ_ABI_SOFTFP=1")
  ENDIF()
  IF(WIN32)
    SET(DASM_FLAGS ${DASM_FLAGS} -LN -D WIN)
  ENDIF()
  IF(TARGET_LJARCH STREQUAL "x86")
    LJ_TEST_ARCH_VALUE(__SSE2__ 1)
    IF(__SSE2__1)
      SET(DASM_FLAGS ${DASM_FLAGS} -D SSE)
    ENDIF()
  ENDIF()
  IF(TARGET_LJARCH STREQUAL "x64")
    SET(DASM_ARCH "x86")
  ENDIF()
  IF(TARGET_LJARCH STREQUAL "ppc")
    LJ_TEST_ARCH_VALUE(LJ_ARCH_SQRT 1)
    IF(NOT LJ_ARCH_SQRT_1)
      SET(DASM_FLAGS ${DASM_FLAGS} -D SQRT)
    ENDIF()
    LJ_TEST_ARCH_VALUE(LJ_ARCH_PPC64 1)
    IF(NOT LJ_ARCH_PPC64_1)
      SET(DASM_FLAGS ${DASM_FLAGS} -D GPR64)
    ENDIF()
  ENDIF()

ENDIF(NOT TARGET_ARCH)

message("Deletethisdebug-postflagset\nDASM_ARCH ${DASM_ARCH}\nDASM_FLAGS ${DASM_FLAGS}\nTARGET_ARCH ${TARGET_ARCH}\nTARGET_LJARCH ${TARGET_LJARCH}")

IF(LJ_CROSS)
  # This is a cross compile, instead of building buildvm re-invoke cmake.
  # The "inner" cmake invocation will use the toolchain for the buildsystem's arch/OS instead of the target platform's

  set(BUILDVM ${CMAKE_CURRENT_BINARY_DIR}/luajit_buildtools/buildvm)

  # CMake is currently set to build for Android.
  # However LuaJIT needs us to build some tools that will run on the build machine.
  # We therefore re-invoke this CMake file

  add_custom_command(OUTPUT luajit_buildtools
    COMMAND ${CMAKE_COMMAND} -E make_directory luajit_buildtools
    COMMENT "Creating LuaJIT helper tool directory...")

  add_custom_target(create_luajit_buildtools
                      DEPENDS luajit_buildtools)

  #list(JOIN DASM_FLAGS ${JOINER} DASM_FLAGS_SERIAL_OUT)
  #list(JOIN TARGET_ARCH ${JOINER} TARGET_ARCH_SERIAL_OUT)

  add_custom_command(OUTPUT luajit_buildtools/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
        -DCMAKE_MAKE_PROGRAM="${CMAKE_MAKE_PROGRAM}"
        -DLJ_CROSS_INNER=ON -DLJ_CROSS=OFF -DTARGET_LJARCH=${TARGET_LJARCH}
        -DTARGET_LJARCH_X64=${TARGET_LJARCH_X64} -DTARGET_LJARCH_X86=${TARGET_LJARCH_X86} -DTARGET_LJARCH_ARM=${TARGET_LJARCH_ARM} -DTARGET_LJARCH_ARM64=${TARGET_LJARCH_ARM64} -DTARGET_LJARCH_PPC=${TARGET_LJARCH_PPC} -DTARGET_LJARCH_PPCSPE=${TARGET_LJARCH_PPCSPE} -DTARGET_LJARCH_MIPS=${TARGET_LJARCH_MIPS}
        -DTARGET_ARCH_SERIAL="${TARGET_ARCH}" -DDASM_ARCH=${DASM_ARCH} -DDASM_FLAGS_SERIAL="${DASM_FLAGS}"
        ${CROSS_TOOLCHAIN_FLAGS_LUAJIT} ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY luajit_buildtools
    DEPENDS create_luajit_buildtools
    COMMENT "Configuring LuaJIT helper tools...")

  add_custom_target(configure_luajit_buildtools
                      DEPENDS luajit_buildtools/CMakeCache.txt)

  add_custom_command(OUTPUT luajit_buildtools/buildvm
    COMMAND ${CMAKE_COMMAND} --build .
    WORKING_DIRECTORY luajit_buildtools
    DEPENDS configure_luajit_buildtools
    COMMENT "Building LuaJIT helper tools...")

ELSE(LJ_CROSS)
  # Either this is not a cross compile, or if this is a cross compile this is the inner "tools pass"

  set(BUILDVM buildvm)

  add_executable(minilua ${LUAJIT_DIR}/src/host/minilua.c)

  SET_TARGET_PROPERTIES(minilua PROPERTIES COMPILE_DEFINITIONS "${TARGET_ARCH}")
  CHECK_LIBRARY_EXISTS(m sin "" MINILUA_USE_LIBM)
  if(MINILUA_USE_LIBM)
    TARGET_LINK_LIBRARIES(minilua m)
  endif()

  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h
    COMMAND minilua ${LUAJIT_DIR}/dynasm/dynasm.lua ${DASM_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h ${LUAJIT_DIR}/src/vm_${DASM_ARCH}.dasc
    DEPENDS ${LUAJIT_DIR}/dynasm/dynasm.lua minilua
  )

  file (GLOB_RECURSE SRC_BUILDVM  "${LUAJIT_DIR}/src/host/buildvm*.c")
  list (APPEND SRC_BUILDVM ${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h)

  ## GENERATE
  ADD_EXECUTABLE(buildvm ${SRC_BUILDVM})
  SET_TARGET_PROPERTIES(buildvm PROPERTIES COMPILE_DEFINITIONS "${TARGET_ARCH}")
  if (TARGET_LJARCH_ARM OR TARGET_LJARCH_X86)
    # This is a bit weird: Luajit's buildvm tool cannot cross-compile to a 32-bit platform from a 64-bit build host
    SET_TARGET_PROPERTIES(buildvm PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
  endif()
  add_dependencies(buildvm ${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h)

ENDIF(LJ_CROSS)

## COMPILE
include_directories(
  ${LUAJIT_DIR}/dynasm
  ${LUAJIT_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
)

IF (NOT LJ_CROSS_INNER)
  # Ready to compile the actual luajit binary/lib.

  macro(add_buildvm_target _target _mode)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_target}
      COMMAND ${BUILDVM} ARGS -m ${_mode} -o ${CMAKE_CURRENT_BINARY_DIR}/${_target} ${ARGN}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${BUILDVM} ${ARGN}
    )
  endmacro(add_buildvm_target)

  if (WIN32)
    add_buildvm_target ( lj_vm.obj peobj )
    set (LJ_VM_SRC ${CMAKE_CURRENT_BINARY_DIR}/lj_vm.obj)
  else ()
    add_buildvm_target ( lj_vm.S ${LJVM_MODE} )
    set (LJ_VM_SRC ${CMAKE_CURRENT_BINARY_DIR}/lj_vm.S)
  endif ()
  add_buildvm_target ( lj_ffdef.h   ffdef   ${SRC_LJLIB} )
  add_buildvm_target ( lj_bcdef.h  bcdef  ${SRC_LJLIB} )
  add_buildvm_target ( lj_folddef.h folddef ${LUAJIT_DIR}/src/lj_opt_fold.c )
  add_buildvm_target ( lj_recdef.h  recdef  ${SRC_LJLIB} )
  add_buildvm_target ( lj_libdef.h  libdef  ${SRC_LJLIB} )
  add_buildvm_target ( vmdef.lua  vmdef  ${SRC_LJLIB} )

  ## Source Lists
  SET(SRC_LJLIB
    ${LUAJIT_DIR}/src/lib_base.c
    ${LUAJIT_DIR}/src/lib_math.c
    ${LUAJIT_DIR}/src/lib_bit.c
    ${LUAJIT_DIR}/src/lib_string.c
    ${LUAJIT_DIR}/src/lib_table.c
    ${LUAJIT_DIR}/src/lib_io.c
    ${LUAJIT_DIR}/src/lib_os.c
    ${LUAJIT_DIR}/src/lib_package.c
    ${LUAJIT_DIR}/src/lib_debug.c
    ${LUAJIT_DIR}/src/lib_jit.c
    ${LUAJIT_DIR}/src/lib_ffi.c
  )

  SET(SRC_LIBAUX
    ${LUAJIT_DIR}/src/lib_aux.c
    ${LUAJIT_DIR}/src/lib_init.c
  )
  file (GLOB_RECURSE SRC_LJCORE   "${LUAJIT_DIR}/src/lj_*.c")
  list (APPEND SRC_LJCORE ${SRC_LJLIB} ${SRC_LIBAUX})

  SET(DEPS
    ${LJ_VM_SRC}
    ${CMAKE_CURRENT_BINARY_DIR}/lj_ffdef.h
    ${CMAKE_CURRENT_BINARY_DIR}/lj_bcdef.h
    ${CMAKE_CURRENT_BINARY_DIR}/lj_libdef.h
    ${CMAKE_CURRENT_BINARY_DIR}/lj_recdef.h
    ${CMAKE_CURRENT_BINARY_DIR}/lj_folddef.h
    ${CMAKE_CURRENT_BINARY_DIR}/vmdef.lua
    )

  IF(WITH_SHARED_LUA)
      IF(WITH_AMALG)
  	add_library(luajit-5.1 SHARED ${LUAJIT_DIR}/src/ljamalg.c ${DEPS} )
      ELSE()
  	add_library(luajit-5.1 SHARED ${SRC_LJCORE} ${DEPS} )
      ENDIF()
      SET_TARGET_PROPERTIES(luajit-5.1 PROPERTIES OUTPUT_NAME "lua51")
  ELSE()
      IF(WITH_AMALG)
  	add_library(luajit-5.1 STATIC ${LUAJIT_DIR}/src/ljamalg.c ${DEPS} )
      ELSE()
  	add_library(luajit-5.1 STATIC ${SRC_LJCORE} ${DEPS} )
      ENDIF()
      SET_TARGET_PROPERTIES(luajit-5.1 PROPERTIES
  	PREFIX "lib" IMPORT_PREFIX "lib" OUTPUT_NAME "luajit")
  ENDIF()

  target_link_libraries (luajit-5.1 ${LIBS} )

  IF(WIN32)
    add_executable(luajit ${LUAJIT_DIR}/src/luajit.c)
    target_link_libraries(luajit luajit-5.1)
  ELSE()
    IF(WITH_AMALG)
      add_executable(luajit ${LUAJIT_DIR}/src/luajit.c ${LUAJIT_DIR}/src/ljamalg.c ${DEPS})
    ELSE()
      add_executable(luajit ${LUAJIT_DIR}/src/luajit.c ${SRC_LJCORE} ${DEPS})
    ENDIF()
    target_link_libraries(luajit ${LIBS})
    SET_TARGET_PROPERTIES(luajit PROPERTIES ENABLE_EXPORTS ON)
  ENDIF()

  MACRO(LUAJIT_add_custom_commands luajit_target)
    SET(target_srcs "")
    FOREACH(file ${ARGN})
      IF(${file} MATCHES ".*\\.lua$")
        if(NOT IS_ABSOLUTE ${file})
          set(file "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
        endif()
        set(source_file ${file})
        string(LENGTH ${CMAKE_SOURCE_DIR} _luajit_source_dir_length)
        string(LENGTH ${file} _luajit_file_length)
        math(EXPR _begin "${_luajit_source_dir_length} + 1")
        math(EXPR _stripped_file_length "${_luajit_file_length} - ${_luajit_source_dir_length} - 1")
        string(SUBSTRING ${file} ${_begin} ${_stripped_file_length} stripped_file)

        set(generated_file "${CMAKE_BINARY_DIR}/jitted_tmp/${stripped_file}_${luajit_target}_generated${CMAKE_C_OUTPUT_EXTENSION}")

        add_custom_command(
          OUTPUT ${generated_file}
          MAIN_DEPENDENCY ${source_file}
          DEPENDS luajit
          COMMAND luajit
          ARGS -bg
            ${source_file}
            ${generated_file}
          COMMENT "Building Luajitted ${source_file}: ${generated_file}"
        )

        get_filename_component(basedir ${generated_file} PATH)
        file(MAKE_DIRECTORY ${basedir})

        set(target_srcs ${target_srcs} ${generated_file})
        set_source_files_properties(
          ${generated_file}
          properties
          external_object true # this is an object file
          generated true        # to say that "it is OK that the obj-files do not exist before build time"
        )
      ELSE()
        set(target_srcs ${target_srcs} ${file})
      ENDIF(${file} MATCHES ".*\\.lua$")
    ENDFOREACH(file)
  ENDMACRO()

  MACRO(LUA_ADD_EXECUTABLE luajit_target)
    LUAJIT_add_custom_commands(${luajit_target} ${ARGN})
    add_executable(${luajit_target} ${target_srcs})
  ENDMACRO(LUA_ADD_EXECUTABLE luajit_target)

ENDIF(NOT LJ_CROSS_INNER)
