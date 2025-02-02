cmake_minimum_required(VERSION 3.10)

option(BUILD_MACOS_INTEL "Build test programs" OFF)
option(BUILD_MACOS_ARM   "Build test programs" ON)
option(BUILD_LINUX_INTEL "Build test programs" OFF)
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
SET(CMAKE_BUILD_TYPE  Release)
#SET(CMAKE_BUILD_TYPE  Debug)

#
# MacOS with ARM M1/M2 processors
#

if(BUILD_MACOS_ARM)
    message("BUILD_MACOS AND BUILD_ARM => MACOS APPLE M1/M2")
    SET (CMAKE_C_COMPILER   /opt/homebrew/opt/llvm/bin/clang)     # LLVM 17
    SET (CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm/bin/clang++)   # LLVM 17

    SET(CMAKE_CXX_FLAGS "-Wall")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native -mtune=native")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -ffast-math -fopenmp -std=c++17")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -fopenmp -std=c++17")
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

project(min_sorter)

file(GLOB_RECURSE source_files ./src/quicksort.cpp ./src/additional_methods.cpp
        ./src/progress/progressbar.cpp
        # sorting algorithms
        ./src/sorting/std_4cores/std_4cores.cpp
        ./src/sorting/std_2cores/std_2cores.cpp
        ./src/sorting/crumsort/crumsort.cpp
        ./src/sorting/crumsort_2cores/crumsort_2cores.cpp
        ./src/minimizer/deduplication.cpp
        src/back/raw/SaveRawToFile.cpp
        ./src/back/txt/SaveMiniToTxtFile.cpp
        # Front-end parsers
        ./src/front/fastx/read_fastx_file.cpp
        ./src/front/fastx_gz/read_fastx_gz_file.cpp
        ./src/front/fastx_bz2/read_fastx_bz2_file.cpp
        # Line counters
        ./src/front/count_file_lines.cpp
        ./src/front/fastx/count_lines_fastx.cpp
        ./src/front/fastx_bz2/count_lines_fastx_bz2.cpp
        # sequence size estimator
        ./src/front/read_k_value.cpp
        ./src/front/fastx/read_fastx_k_value.cpp
        ./src/front/fastx_bz2/read_fastx_bz2_k_value.cpp
        # main
        ./src/min_sorter.cpp)
add_executable(min_sorter ${source_files})

target_include_directories(min_sorter
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

target_link_libraries(min_sorter pthread)
target_link_libraries(min_sorter z)
target_link_libraries(min_sorter bz2)
target_link_libraries(min_sorter curses)

########################################################################################################################

project(min_sorter_v2)

file(GLOB_RECURSE source_files ./src/quicksort.cpp ./src/additional_methods.cpp
        ./src/progress/progressbar.cpp
        # sorting algorithms
        ./src/sorting/std_4cores/std_4cores.cpp
        ./src/sorting/std_2cores/std_2cores.cpp
        ./src/sorting/crumsort/crumsort.cpp
        ./src/sorting/crumsort_2cores/crumsort_2cores.cpp
        ./src/minimizer/deduplication.cpp
        ./src/minimizer/minimizer_v2.cpp
        src/back/raw/SaveRawToFile.cpp
        ./src/back/txt/SaveMiniToTxtFile.cpp
        # Front-end parsers
        src/front/fastx/read_fastx_file.cpp
        src/front/fastx_gz/read_fastx_gz_file.cpp
        src/front/fastx_bz2/read_fastx_bz2_file.cpp
        src/front/fastx_lz4/read_fastx_lz4_file.cpp
        src/front/fastx_lz4/lz4/lz4file.c
        src/front/fastx_lz4/lz4/lz4frame.c
        src/front/fastx_lz4/lz4/xxhash.c
        src/front/fastx_lz4/lz4/lz4hc.c
        src/front/fastx_lz4/lz4/lz4.c
        # Line counters
        ./src/front/count_file_lines.cpp
        ./src/front/fastx/count_lines_fastx.cpp
        ./src/front/fastx_bz2/count_lines_fastx_bz2.cpp
        # sequence size estimator
        ./src/front/read_k_value.cpp
        ./src/front/fastx/read_fastx_k_value.cpp
        ./src/front/fastx_bz2/read_fastx_bz2_k_value.cpp
        # main
        ./src/min_sorter_v2.cpp)

add_executable(min_sorter_v2 ${source_files})

target_include_directories(min_sorter_v2
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

target_link_libraries(min_sorter_v2 pthread)
target_link_libraries(min_sorter_v2 bz2)
target_link_libraries(min_sorter_v2 z)
target_link_libraries(min_sorter_v2 curses)

########################################################################################################################

project(min_sorter_v3)

file(GLOB_RECURSE source_files ./src/quicksort.cpp ./src/additional_methods.cpp
        ./src/progress/progressbar.cpp
        # sorting algorithms
        ./src/sorting/std_4cores/std_4cores.cpp
        ./src/sorting/std_2cores/std_2cores.cpp
        ./src/sorting/crumsort/crumsort.cpp
        ./src/sorting/crumsort_2cores/crumsort_2cores.cpp
        ./src/minimizer/deduplication.cpp
        ./src/minimizer/minimizer_v3.cpp
        src/back/raw/SaveRawToFile.cpp
        ./src/back/txt/SaveMiniToTxtFile.cpp
        # Front-end parsers
        src/front/fastx/read_fastx_file.cpp
        src/front/fastx_gz/read_fastx_gz_file.cpp
        src/front/fastx_bz2/read_fastx_bz2_file.cpp
        src/front/fastx_lz4/read_fastx_lz4_file.cpp
        src/front/fastx_lz4/lz4/lz4file.c
        src/front/fastx_lz4/lz4/lz4frame.c
        src/front/fastx_lz4/lz4/xxhash.c
        src/front/fastx_lz4/lz4/lz4hc.c
        src/front/fastx_lz4/lz4/lz4.c
        # Line counters
        ./src/front/count_file_lines.cpp
        ./src/front/fastx/count_lines_fastx.cpp
        ./src/front/fastx_bz2/count_lines_fastx_bz2.cpp
        # sequence size estimator
        ./src/merger/merger_level_0.cpp
        ./src/front/read_k_value.cpp
        ./src/front/fastx/read_fastx_k_value.cpp
        ./src/front/fastx_bz2/read_fastx_bz2_k_value.cpp
        # main
        ./src/min_sorter_v3.cpp)

add_executable(min_sorter_v3 ${source_files})

target_include_directories(min_sorter_v3
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

target_link_libraries(min_sorter_v3 pthread)
target_link_libraries(min_sorter_v3 bz2)
target_link_libraries(min_sorter_v3 z)
target_link_libraries(min_sorter_v3 curses)

########################################################################################################################

project(min_sorter_ref)

file(GLOB_RECURSE source_files ./src/quicksort.cpp ./src/additional_methods.cpp
        ./src/progress/progressbar.cpp
        ./src/sorting/std_4cores/std_4cores.cpp
        ./src/sorting/std_2cores/std_2cores.cpp
        ./src/sorting/crumsort/crumsort.cpp
        ./src/sorting/crumsort_2cores/crumsort_2cores.cpp
        src/back/raw/SaveRawToFile.cpp
        ./src/back/txt/SaveMiniToTxtFile.cpp
        # Front-end parsers
        ./src/front/fastx/read_fastx_file.cpp
        ./src/front/fastx_gz/read_fastx_gz_file.cpp
        ./src/front/fastx_bz2/read_fastx_bz2_file.cpp
        # Line counters
        ./src/front/count_file_lines.cpp
        ./src/front/fastx/count_lines_fastx.cpp
        ./src/front/fastx_bz2/count_lines_fastx_bz2.cpp
        # sequence size estimator
        ./src/front/read_k_value.cpp
        ./src/front/fastx/read_fastx_k_value.cpp
        ./src/front/fastx_bz2/read_fastx_bz2_k_value.cpp
        # main program
        ./src/min_sorter_ref.cpp)
add_executable(min_sorter_ref ${source_files})

target_include_directories(min_sorter_ref
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

target_link_libraries(min_sorter_ref pthread)
target_link_libraries(min_sorter_ref bz2)
target_link_libraries(min_sorter_ref z)
target_link_libraries(min_sorter_ref curses)

########################################################################################################################

project(merge)

file(GLOB_RECURSE source_files ./src/quicksort.cpp ./src/additional_methods.cpp
        ./src/progress/progressbar.cpp
        ./src/merger/merger_level_s.cpp
        ./src/merger/merger_level_0.cpp
        ./src/merger/merger_level_1.cpp
        ./src/merger/merger_level_2_0.cpp
        ./src/merger/merger_level_2_32.cpp
        ./src/merger/merger_level_64_n.cpp
        ./src/merger/merger_level_n_p.cpp
        ./src/merger/merger_in.cpp
        ./src/merge.cpp)
add_executable(merge ${source_files})

target_include_directories(merge
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

target_link_libraries(merge pthread)
target_link_libraries(merge bz2)
target_link_libraries(merge z)
target_link_libraries(merge curses)

########################################################################################################################

project(raw_dump)

file(GLOB_RECURSE source_files
        ./src/raw_dump.cpp)
add_executable(raw_dump ${source_files})

target_include_directories(raw_dump
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

########################################################################################################################

project(raw_compare)

file(GLOB_RECURSE source_files
        ./src/raw_compare.cpp)
add_executable(raw_compare ${source_files})

target_include_directories(raw_compare
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

########################################################################################################################

project(test_generator)

file(GLOB_RECURSE source_files  ./src/test_generator.cpp)
add_executable(test_generator ${source_files})

########################################################################################################################

project(BreiZHMinimizer)

file(GLOB_RECURSE source_files
        ./src/BreiZHMinimizer.cpp
        ./src/progress/progressbar.cpp
        # Merger implementations
        ./src/merger/merger_in.cpp
        ./src/merger/merger_level_s.cpp
        ./src/merger/merger_level_0.cpp
        ./src/merger/merger_level_1.cpp
        ./src/merger/merger_level_2_0.cpp
        ./src/merger/merger_level_2_32.cpp
        ./src/merger/merger_level_64_n.cpp
        ./src/merger/merger_level_n_p.cpp
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
        ./src/minimizer/minimizer_v2.cpp
        ./src/minimizer/minimizer_v3.cpp
        # Front-end parsers
        src/front/fastx/read_fastx_file.cpp
        src/front/fastx_gz/read_fastx_gz_file.cpp
        src/front/fastx_bz2/read_fastx_bz2_file.cpp
        src/front/fastx_lz4/read_fastx_lz4_file.cpp
        src/front/fastx_lz4/lz4/lz4file.c
        src/front/fastx_lz4/lz4/lz4frame.c
        src/front/fastx_lz4/lz4/xxhash.c
        src/front/fastx_lz4/lz4/lz4hc.c
        src/front/fastx_lz4/lz4/lz4.c
    )
add_executable(BreiZHMinimizer ${source_files})

target_include_directories(BreiZHMinimizer
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

target_link_libraries(BreiZHMinimizer curses)
target_link_libraries(BreiZHMinimizer z)
target_link_libraries(BreiZHMinimizer bz2)

########################################################################################################################

project(extract_color)

file(GLOB_RECURSE source_files
        ./src/extract_color.cpp)
add_executable(extract_color ${source_files})

target_include_directories(extract_color
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

########################################################################################################################

project(extract_all_colors)

file(GLOB_RECURSE source_files
        ./src/extract_all_colors.cpp)
add_executable(extract_all_colors ${source_files})

target_include_directories(extract_all_colors
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

########################################################################################################################

project(color_stats)

file(GLOB_RECURSE source_files
        ./src/color_stats.cpp)
add_executable(color_stats ${source_files})

target_include_directories(color_stats
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

########################################################################################################################
