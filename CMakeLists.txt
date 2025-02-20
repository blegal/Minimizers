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
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -fopenmp -std=c++17")
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
        src/kmer_list/smer_deduplication.cpp
        ./src/back/raw/SaveRawToFile.cpp
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
        apps/min_sorter.cpp)
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
        src/kmer_list/smer_deduplication.cpp
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
        apps/min_sorter_v2.cpp)

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
        src/kmer_list/smer_deduplication.cpp
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
        src/merger/in_file/merger_level_0.cpp
        ./src/front/read_k_value.cpp
        ./src/front/fastx/read_fastx_k_value.cpp
        ./src/front/fastx_bz2/read_fastx_bz2_k_value.cpp
        # main
        apps/min_sorter_v3.cpp)

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
        apps/min_sorter_ref.cpp)
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
        src/merger/in_file/merger_level_s.cpp
        src/merger/in_file/merger_level_0.cpp
        src/merger/in_file/merger_level_1.cpp
        src/merger/in_file/merger_level_2_0.cpp
        src/merger/in_file/merger_level_2_32.cpp
        src/merger/in_file/merger_level_64_n.cpp
        src/merger/in_file/merger_level_n_p.cpp
        src/merger/in_file/merger_in.cpp
        apps/merge.cpp)
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
        apps/raw_dump.cpp)
add_executable(raw_dump ${source_files})

target_include_directories(raw_dump
        PUBLIC
        "${PROJECT_SOURCE_DIR}/../src/headers/"
)

########################################################################################################################

project(raw_compare)

file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/apps/raw_compare.cpp")

add_executable(raw_compare ${source_files})

target_link_libraries(raw_compare z)
target_link_libraries(raw_compare bz2)
target_link_libraries(raw_compare curses)

########################################################################################################################

project(test_generator)

file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/apps/test_generator.cpp")

add_executable(test_generator ${source_files})

target_link_libraries(test_generator z)
target_link_libraries(test_generator bz2)
target_link_libraries(test_generator curses)

########################################################################################################################

project(BreiZHMinimizer)

file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/apps/BreiZHMinimizer.cpp")

add_executable(BreiZHMinimizer ${source_files})

target_link_libraries(BreiZHMinimizer z)
target_link_libraries(BreiZHMinimizer bz2)
target_link_libraries(BreiZHMinimizer curses)

########################################################################################################################

project(BreiZHMinimizer_v2)

file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/apps/BreiZHMinimizer_v2.cpp")

add_executable(BreiZHMinimizer_v2 ${source_files})

target_link_libraries(BreiZHMinimizer_v2 z)
target_link_libraries(BreiZHMinimizer_v2 bz2)
target_link_libraries(BreiZHMinimizer_v2 curses)

########################################################################################################################

project(extract_color)

file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/apps/extract_all_colors.cpp")

add_executable(extract_color ${source_files})

target_link_libraries(extract_color z)
target_link_libraries(extract_color bz2)
target_link_libraries(extract_color curses)

########################################################################################################################

project(extract_all_colors)

file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/apps/extract_all_colors.cpp")

add_executable(extract_all_colors ${source_files})

target_link_libraries(extract_all_colors z)
target_link_libraries(extract_all_colors bz2)
target_link_libraries(extract_all_colors curses)

########################################################################################################################

project(color_stats)

file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/apps/color_stats.cpp")

add_executable(color_stats ${source_files})

target_link_libraries(color_stats z)
target_link_libraries(color_stats bz2)
target_link_libraries(color_stats curses)

########################################################################################################################

project(color_sorter)

file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/apps/color_sorter.cpp")

add_executable(color_sorter ${source_files})

target_link_libraries(color_sorter z)
target_link_libraries(color_sorter bz2)
target_link_libraries(color_sorter curses)

########################################################################################################################

project(kmer_sorter)

file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/apps/kmer_sorter.cpp")

add_executable(kmer_sorter ${source_files})

target_link_libraries(kmer_sorter z)
target_link_libraries(kmer_sorter bz2)
target_link_libraries(kmer_sorter curses)

########################################################################################################################

project(merge_all)

file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/apps/merge_all.cpp")

add_executable(merge_all ${source_files})

target_link_libraries(merge_all z)
target_link_libraries(merge_all bz2)
target_link_libraries(merge_all curses)

########################################################################################################################
