cmake_minimum_required(VERSION 2.8.7)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(AppendCompilerFlags)

project(LIBCSD)
set(CMAKE_PROJECT_NAME "LIBCSD")

#Change to "Debug" when you need it
set(CMAKE_BUILD_TYPE "Release")

# C++11 compiler Check
if(NOT CMAKE_CXX_COMPILER_VERSION) # work around for cmake versions smaller than 2.8.10
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION)
endif()
if(CMAKE_CXX_COMPILER MATCHES ".*clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()
if( (CMAKE_COMPILER_IS_GNUCXX AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.7) OR
(CMAKE_COMPILER_IS_CLANGXX AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 3.2))
    message(FATAL_ERROR "Your C++ compiler does not support C++11. Please install g++ 4.7 (or greater) or clang 3.2 (or greater)")
else()
    message(STATUS "Compiler is recent enough to support C++11.")
endif()
if( CMAKE_COMPILER_IS_GNUCXX )
    append_cxx_compiler_flags("-fpermissive -std=c++11 -Wall -Wextra " "GCC" CMAKE_CXX_FLAGS)
    append_cxx_compiler_flags("-O3 -ffast-math -funroll-loops -fno-omit-frame-pointer -g" "GCC" CMAKE_CXX_FLAGS_RELEASE)
else()
    append_cxx_compiler_flags("-fpermissive -std=c++11" "CLANG" CMAKE_CXX_FLAGS)
    append_cxx_compiler_flags("-stdlib=libc++" "CLANG" CMAKE_CXX_FLAGS)
    append_cxx_compiler_flags("-O3 -ffast-math -funroll-loops -DNDEBUG" "CLANG" CMAKE_CXX_FLAGS_RELEASE)
endif()
include(CheckSSE)
FindSSE ()
if( SSE4_2_FOUND )
    if( CMAKE_COMPILER_IS_GNUCXX )
        append_cxx_compiler_flags("-msse4.2" "GCC" CMAKE_CXX_FLAGS)
    else()
        append_cxx_compiler_flags("-msse4.2" "CLANG" CMAKE_CXX_FLAGS)
    endif()
    message(STATUS "CPU does support SSE4.2.")
else()
    message(STATUS "CPU does NOT support SSE4.2")
endif()

include_directories(${CMAKE_HOME_DIRECTORY})

add_subdirectory(libcds)

set(LIBCSD_SOURCES
    StringDictionary.cpp
    StringDictionaryHASHHF.cpp
    StringDictionaryHASHRPF.cpp
    StringDictionaryHASHUFFDAC.cpp
    StringDictionaryHASHRPDAC.cpp
    StringDictionaryPFC.cpp
    StringDictionaryRPFC.cpp
    StringDictionaryHTFC.cpp
    StringDictionaryHHTFC.cpp
    StringDictionaryRPHTFC.cpp
    StringDictionaryRPDAC.cpp
    StringDictionaryFMINDEX.cpp
    StringDictionaryXBW.cpp
    utils/VByte.cpp
    utils/LogSequence.cpp
    utils/DAC_VLS.cpp
    utils/DAC_BVLS.cpp
    utils/Coder/StatCoder.cpp
    utils/Coder/DecodingTableBuilder.cpp
    utils/Coder/DecodingTable.cpp
    utils/Coder/DecodingTree.cpp
    utils/Coder/BinaryNode.cpp
    HuTucker/HuTucker.cpp
    Huffman/huff.cpp
    Huffman/Huffman.cpp
    RePair/RePair.cpp
    RePair/Coder/arrayg.cpp
    RePair/Coder/basics.cpp
    RePair/Coder/hash.cpp
    RePair/Coder/heap.cpp
    RePair/Coder/records.cpp
    RePair/Coder/dictionary.cpp
    RePair/Coder/IRePair.cpp
    RePair/Coder/CRePair.cpp
    Hash/Hash.cpp
    Hash/HashDAC.cpp
    Hash/Hashdh.cpp
    Hash/HashBdh.cpp
    Hash/HashBBdh.cpp
    FMIndex/SuffixArray.cpp
    FMIndex/SSA.cpp
    XBW/TrieNode.cpp
    XBW/XBW.cpp
)

add_library(libcsd STATIC ${LIBCSD_SOURCES})
target_link_libraries(libcsd cds)
target_include_directories(libcsd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libcds/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/libcds/src/static/bitsequence
    ${CMAKE_CURRENT_SOURCE_DIR}/libcds/src/static/mapper
    ${CMAKE_CURRENT_SOURCE_DIR}/libcds/src/static/coders
    ${CMAKE_CURRENT_SOURCE_DIR}/libcds/src/static/sequence
    ${CMAKE_CURRENT_SOURCE_DIR}/libcds/src/static/permutation
    ${CMAKE_CURRENT_SOURCE_DIR}/libcds/src/static/textindex
    ${CMAKE_CURRENT_SOURCE_DIR}/libcds/src/static/suffixtree
)

add_executable(Build Build.cpp)
target_link_libraries(Build libcsd cds)
set_target_properties(Build PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(Test Test.cpp)
target_link_libraries(Test libcsd cds)
set_target_properties(Test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
