cmake_minimum_required(VERSION 3.10)

option(BUILD_MACOS_INTEL "Build test programs" OFF)
option(BUILD_MACOS_ARM   "Build test programs" OFF)
option(BUILD_LINUX_INTEL "Build test programs" ON)
option(BUILD_LINUX_ARM   "Build test programs" OFF)

option(ENABLE_LTO    "Build test programs" OFF)
option(BUILD_PROFILE "Build test programs" OFF)
option(BUILD_DEBUG   "Build test programs" OFF)
option(BUILD_INFOS   "Build test programs" OFF)

#
#if(BUILD_MACOS_ARM)
#    message("BUILD_MACOS_ARM is not OFF/FALSE/0")
#endif()
#
#if(BUILD_MACOS_INTEL)
#    message("BUILD_MACOS_INTEL is not OFF/FALSE/0")
#endif()
#
#if(BUILD_MACOS_ARM OR BUILD_MACOS_INTEL)
#    if(BUILD_MACOS_ARM)
#        message("- BUILD_MACOS_ARM")
#    else()
#        message("- BUILD_MACOS_INTEL")
#    endif()
#endif()

SET(CMAKE_CXX_STANDARD     17)
#SET(CMAKE_BUILD_TYPE  Release)
SET(CMAKE_BUILD_TYPE  Debug)

#
# MacOS with ARM M1/M2 processors
#

if(BUILD_MACOS_ARM)
    message("BUILD_MACOS AND BUILD_ARM => MACOS APPLE M1/M2")
    SET (CMAKE_C_COMPILER   /opt/homebrew/opt/llvm/bin/clang)     # LLVM 17
    SET (CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm/bin/clang++)   # LLVM 17

    SET(CMAKE_CXX_FLAGS "-Wall")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native -mtune=native")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -ffast-math -fopenmp -std=c++17")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -fopenmp -std=c++17")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flax-vector-conversions") # gcc patch
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/homebrew/include") # gcc patch

    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/local/lib")      # NemaTode
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/opt/homebrew/lib")      # NemaTode
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Accelerate")
endif()

#
# MacOS with INTEL x86 processors
#

if(BUILD_MACOS_INTEL)
    message("BUILD_MACOS_INTEL configuration")
    SET(CMAKE_CXX_FLAGS "-Wall")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -ffast-math")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_MKL_")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
endif ()

#
# Linux with INTEL x86 processors
#

if(BUILD_LINUX_INTEL)
    message("BUILD_LINUX_INTEL configuration")

#    SET(CMAKE_C_COMPILER   /usr/bin/clang-15)
#    SET(CMAKE_CXX_COMPILER /usr/bin/clang++-15)

    SET(CMAKE_CXX_FLAGS "-Wall")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -ffast-math -fopenmp")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_MKL_")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3")  #Uncomment to debug with valgrind
endif ()

#
# Linux with ARM processors
#

if(BUILD_LINUX_ARM)
    message("BUILD_LINUX_ARM configuration")
    #SET (CMAKE_C_COMPILER   /usr/bin/clang-15)
    #SET (CMAKE_CXX_COMPILER /usr/bin/clang++-15)
    SET (CMAKE_C_COMPILER   /opt/homebrew/bin/gcc-14)
    SET (CMAKE_CXX_COMPILER /opt/homebrew/bin/g++-14)

    SET(CMAKE_CXX_FLAGS "-Wall")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native -mtune=native")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -ffast-math")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g3 -ffast-math")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_MKL_")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")

#    SET(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS}        -I/usr/local/include")
#    SET(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS}        -I/opt/homebrew/include")
#    SET(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS}        -I/opt/homebrew/Cellar/uhd/4.4.0.0_1/include/uhd")
#    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/local/lib")
#    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/opt/homebrew/lib")
#    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Accelerate")
endif ()

if(BUILD_PROFILE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Rpass=loop-vectorize") #
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Rpass-missed=loop-vectorize") #
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Rpass-analysis=loop-vectorize") #
endif ()

if(ENABLE_LTO)
    message("LINK time optimization (LTO) is enable")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif ()

########################################################################################################################

########################################################################################################################

project(BreiZHMinimizer)

file(GLOB_RECURSE source_files
        ./src/BreiZHMinimizer.cpp
        # File readers
        #src/files/bz2/reader/file_bz2_reader.cpp
        #src/files/gz/reader/file_gz_reader.cpp
        #src/files/lz4/reader/file_lz4_reader.cpp
        #src/files/raw/reader/file_raw_reader.cpp
        #./src/files/file_reader_library.cpp
        # Merger implementations
        ./src/merger/merger_in.cpp
        ./src/merger/merger_level_s.cpp
        ./src/merger/merger_level_0.cpp
        ./src/merger/merger_level_1.cpp
        ./src/merger/merger_level_2_0.cpp
        ./src/merger/merger_level_2_32.cpp
        ./src/merger/merger_level_64_n.cpp
        ./src/merger/merger_level_n_p.cpp
        ./src/merger/merger_n_files.cpp
        # Output backends
        ./src/back/raw/SaveRawToFile.cpp
        ./src/back/txt/SaveMiniToTxtFile.cpp
        # sorting algorithms
        ./src/sorting/std_4cores/std_4cores.cpp
        ./src/sorting/std_2cores/std_2cores.cpp
        ./src/sorting/crumsort/crumsort.cpp
        ./src/sorting/crumsort_2cores/crumsort_2cores.cpp
        ./src/minimizer/deduplication.cpp
        # Minimizer engines
        ./src/minimizer/deduplication.cpp
        ./src/minimizer/minimizer_v4.cpp
        # Front-end parsers
        src/front/fastx/read_fastx_file.cpp
        src/front/fastx/read_fastx_file_no_N.cpp
        src/front/fastx_gz/read_fastx_gz_file.cpp
        src/front/fastx_gz/read_fastx_gz_file_no_N.cpp
        src/front/fastx_bz2/read_fastx_bz2_file.cpp
        src/front/fastx_lz4/read_fastx_lz4_file.cpp
        src/front/fastx_lz4/lz4/lz4file.c
        src/front/fastx_lz4/lz4/lz4frame.c
        src/front/fastx_lz4/lz4/xxhash.c
        src/front/fastx_lz4/lz4/lz4hc.c
        src/front/fastx_lz4/lz4/lz4.c
        # tools
        ./src/tools/CTimer.cpp
        ./src/tools/file_stats.cpp
    )


add_library(BreiZHMinimizerLib STATIC ${source_files})
target_include_directories(BreiZHMinimizerLib PUBLIC "${PROJECT_SOURCE_DIR}/../src/headers/")
target_link_libraries(BreiZHMinimizerLib z bz2)


########################################################################################################################

########################################################################################################################
