cmake_minimum_required(VERSION 3.10)

option(BUILD_MACOS_INTEL "Build test programs" OFF)
option(BUILD_MACOS_ARM   "Build test programs" OFF)
option(BUILD_LINUX_INTEL "Build test programs" ON)
option(BUILD_LINUX_ARM   "Build test programs" OFF)

option(ENABLE_LTO    "Build test programs" ON)
option(BUILD_PROFILE "Build test programs" OFF)
option(BUILD_DEBUG   "Build test programs" OFF)
option(BUILD_INFOS   "Build test programs" OFF)


if(NOT DEFINED N_COLORS)
    set(N_COLORS 64)  # default
    message(WARNING "N_COLORS not set. Using default value: ${N_COLORS}. You can override with -DN_COLORS=<value>.")
endif()

math(EXPR N_UINT_PER_COLOR "((${N_COLORS} + 63) / 64)")

# Pass both values to the compiler
add_compile_definitions(N_COLORS=${N_COLORS})
add_compile_definitions(N_UINT_PER_COLOR=${N_UINT_PER_COLOR})

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
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
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
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -ffast-math -fopenmp")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -fsanitize=address -fno-omit-frame-pointer -fopenmp")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_MKL_")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
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

    # Only set if not already defined by the parent project/toolchain
    if(NOT CMAKE_AR)
        find_program(CMAKE_AR gcc-ar)
    endif()

    if(NOT CMAKE_RANLIB)
        find_program(CMAKE_RANLIB gcc-ranlib)
    endif()

    if(NOT CMAKE_NM)
        find_program(CMAKE_NM gcc-nm)
    endif()

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif ()



project(BreiZHMinimizer)

# BreiZHMinimizer LIB
file(GLOB_RECURSE src_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*)
file(GLOB_RECURSE lib_sources ${CMAKE_CURRENT_SOURCE_DIR}/lib/*)
add_library(BreiZHMinimizerLib STATIC ${src_sources} ${lib_sources})
target_link_libraries(BreiZHMinimizerLib PUBLIC z bz2)


# --- Helper macro to add executables using common sources + app sources ---

macro(add_app exe_name app_source)
    file(GLOB_RECURSE app_sources "${app_source}")
    add_executable(${exe_name} ${src_sources} ${lib_sources} ${app_sources})
    target_link_libraries(${exe_name} PRIVATE z bz2)
endmacro()

# --- Main executable, uses the lib ---

add_executable(BreiZHMinimizer apps/BreiZHMinimizer_cli.cpp)
target_link_libraries(BreiZHMinimizer PRIVATE BreiZHMinimizerLib)

# --- All other executables, standalone tools ---
# UNCOMMENT TO BUILD

#add_app(min_sorter          "apps/min_sorter.cpp")
#add_app(min_sorter_v2       "apps/min_sorter_v2.cpp")
#add_app(min_sorter_v3       "apps/min_sorter_v3.cpp")
#add_app(min_sorter_ref      "apps/min_sorter_ref.cpp")
#add_app(merge               "apps/merge.cpp")
#add_app(raw_dump            "apps/raw_dump.cpp")
#add_app(raw_compare         "apps/raw_compare.cpp")
#add_app(test_generator      "apps/test_generator.cpp")
#add_app(extract_color       "apps/extract_all_colors.cpp")
#add_app(extract_all_colors  "apps/extract_all_colors.cpp")
#add_app(color_stats         "apps/color_stats.cpp")
#add_app(color_sorter        "apps/color_sorter.cpp")
#add_app(color_sort_and_dump "apps/color_sort_and_dump.cpp")
#add_app(kmer_sorter         "apps/kmer_sorter.cpp")
#add_app(merge_all           "apps/merge_all.cpp")
#add_app(ext_sort            "src/sorting/external_sort/external_sort.cpp") #main() is commented out

# --- Special case raw_dump: includes extra include dirs ---
# UNCOMMENT TO BUILD raw_dump exe

#target_include_directories(raw_dump PUBLIC "${PROJECT_SOURCE_DIR}/../src/headers/")
