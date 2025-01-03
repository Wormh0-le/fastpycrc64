cmake_minimum_required(VERSION 3.8)
project(crc64)

set(CMAKE_CXX_STANDARD 17)

set(BUILD_PIC ON CACHE BOOL "Needed to require CPU feature PIC build" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
add_subdirectory(cpu_features)
include_directories(cpu_features/include)
include_directories(include)

add_library(crc64 INTERFACE)
target_link_libraries(crc64 INTERFACE)

option(CRC64_ENABLE_PYBIND "Enable Python binding for CRC64" ON)
option(CRC64_ENABLE_TESTS "Enable CRC64 Tests (Require GTest)" OFF)
option(CRC64_ENABLE_BENCHES "Enable CRC64 Benchmark (Require Benchmark)" OFF)
option(CRC64_ENABLE_CAPI "Enable CRC64 CAPI" OFF)
option(CRC64_DISABLE_WERROR "Disable strict mode" ON)
option(CRC64_ENABLE_UNROLL "Enable stronger unrolling" OFF)

# include(CheckCXXCompilerFlag)
# check_cxx_compiler_flag(-mvpclmulqdq CRC64_VPCLMULQDQ_SUPPORT)

if (CRC64_ENABLE_CAPI)
    set(CRC64_CAPI_SRC src/ffi.cpp)
else()
    set(CRC64_CAPI_SRC "")
endif ()

if(NOT CRC64_DISABLE_WERROR)
    set(CRC64_WARNING_FLAGS
            "-Werror"
            "-Wall"
            "-Wextra"
            "-Wpedantic"
            "-Wshadow"
            "-Wcast-align"
            "-Wunused"
            "-Wconversion"
            "-Wsign-conversion"
            "-Wnull-dereference"
            "-Wdouble-promotion"
            "-Wformat=2")
else()
    set(CRC64_WARNING_FLAGS "")
endif()

if (CRC64_ENABLE_UNROLL)
    set(CRC64_ADDITIONAL_FLAGS -funroll-loops)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CRC64_ADDITIONAL_FLAGS
                ${CRC64_ADDITIONAL_FLAGS}
                -fprefetch-loop-arrays
                -fvariable-expansion-in-unroller)
    endif()
else()
    set(CRC64_ADDITIONAL_FLAGS "")
endif()

# After Enable VPCLMULQDQ, computation result is wrong.
# if(CRC64_VPCLMULQDQ_SUPPORT)
#     set(CRC64_VPCLMULQDQ_SRC src/vpclmulqdq_avx512.cpp src/vpclmulqdq_avx2.cpp)
# else()
#     set(CRC64_VPCLMULQDQ_SRC "")
# endif()
set(CRC64_VPCLMULQDQ_SRC "")

configure_file(include/crc64_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/crc64_config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    add_library(crc64-x86 STATIC
            src/simd.cpp
            ${CRC64_VPCLMULQDQ_SRC}
            ${CRC64_CAPI_SRC})
    target_compile_options(crc64-x86 PRIVATE
            ${CRC64_ADDITIONAL_FLAGS}
            ${CRC64_WARNING_FLAGS})
    target_link_libraries(crc64 INTERFACE crc64-x86 cpu_features)
    set_source_files_properties(src/simd.cpp
            PROPERTIES COMPILE_FLAGS "-msse2 -msse4.1 -mpclmul")
    if(CRC64_VPCLMULQDQ_SUPPORT)
        set_source_files_properties(src/vpclmulqdq_avx512.cpp
            PROPERTIES COMPILE_FLAGS "-mvpclmulqdq -mavx512vl -mavx512dq -mpclmul -Wno-ignored-attributes")
        set_source_files_properties(src/vpclmulqdq_avx2.cpp
            PROPERTIES COMPILE_FLAGS "-mvpclmulqdq -mavx2 -mpclmul -Wno-ignored-attributes")
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(arm64)|(ARM64)|(aarch64)|(AARCH64)")
    add_library(crc64-aarch64 STATIC src/simd.cpp ${CRC64_CAPI_SRC})
    target_compile_options(crc64-aarch64 PRIVATE
            -march=armv8-a+simd+fp+crypto
            ${CRC64_ADDITIONAL_FLAGS}
            ${CRC64_WARNING_FLAGS})
    target_link_libraries(crc64 INTERFACE crc64-aarch64 cpu_features)
endif()

if (CRC64_ENABLE_PYBIND)
    find_package(pybind11 CONFIG REQUIRED)
    pybind11_add_module(fastpycrc64 pybind/src/binding.cpp)
    target_link_libraries(fastpycrc64 PRIVATE crc64)
    message(STATUS "Python binding for CRC64 enabled")
endif()

if (CRC64_ENABLE_TESTS)
    find_package(GTest REQUIRED)
    find_package(Threads REQUIRED)
    file(GLOB CRC64_TEST_SRC tests/*.cpp)
    add_executable(crc64-test ${CRC64_TEST_SRC})
    target_include_directories(crc64-test  SYSTEM PRIVATE ${GTEST_INCLUDE_DIRS})
    target_link_libraries(crc64-test ${GTEST_BOTH_LIBRARIES} Threads::Threads crc64)
    target_compile_options(crc64-test PRIVATE -march=native -fno-inline -g3)
    enable_testing()
    add_test(gtest crc64-test)
endif()

if (CRC64_ENABLE_BENCHES)
    find_package(benchmark REQUIRED)
    file(GLOB CRC64_TEST_SRC benches/*.cpp)
    add_executable(crc64-bench ${CRC64_TEST_SRC})
    target_compile_options(crc64-bench PRIVATE -march=native)
    target_link_libraries(crc64-bench crc64 benchmark::benchmark)
endif()

file(GLOB CRC64_FMT_FILES
        benches/*.cpp
        tests/*.cpp
        src/*.cpp
        include/*.hpp
        include/*.h
        include/crc64/*.hpp
        include/crc64/*.h)

add_custom_target(
        format
        COMMAND clang-format
        -i
        ${CRC64_FMT_FILES})
