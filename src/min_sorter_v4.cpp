#include <cstdio>
#include <algorithm>
#include <omp.h>
#include <getopt.h>

#include "minimizer/minimizer_v4.hpp"

#include "./minimizer/deduplication.hpp"
#include "front/count_file_lines.hpp"

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

std::chrono::steady_clock::time_point begin;

#include <omp.h>

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

int main(int argc, char* argv[])
{

    //std::string i_file = "/home/vlevallo/tmp/test_bertrand/toy.fasta";
    std::string i_file = "/home/vlevallo/tmp/test_bertrand/human/HG002_maternal_f1_assembly_v2_genbank.fasta";
    std::string o_file = "./result";

    int help_flag         = 0;
    int verbose_flag      = 0;
    int file_save_debug   = 0;
    int file_save_output  = 1;

    int limit_in_ram = 1024; // Comme on parle en MB, cela fait 1 GB

    //
    //
    //

    std::string algo = "crumsort";
    

    minimizer_processing_v4(
            i_file,
            o_file,
            algo,
            limit_in_ram,
            file_save_output,
            verbose_flag,
            file_save_debug
    );

    return 0;
}

//
// AHX_ATRIOSF_7_1_C0URMACXX.IND4_clean.fastx => 164939720 lines
// (II) # k-mer            : 1979276616
// (II) # m-mer            : 25730596008
// (II) # minimizers       :  329879436
// (II) Number of skipped minizr :  891047976
// (II) Number of minimizers     : 1699435677
// (II) memory occupancy         :      12965 MB
// (II) - Number of samples (start) = 1699435677
// (II) - Number of samples (stop)  =  844741384
// (II) - Execution time    = 0h01m57s 0h02m33s

// (II) # k-mer            :  989638308
// (II) # minimizers       :  164939718
// (II) Number of ADN sequences  :   82469859
// (II) Number of k-mer          : 1277160061
// (II) Number of skipped minizr :  430079792
// (II) Number of minimizers     :  847080269
// (II) memory occupancy         :       6462 MB
// (II) Launching the simplification step
// (II) - Number of samples (start) =  847080269
// (II) - Number of samples (stop)  =  494471415
// (II) - Execution time    = 0h00m44s 0h00m55s
//(II) Launching the simplification step
//(II) - Number of samples (start) =  852355408
//(II) - Number of samples (stop)  =  483776049
//(II) - Execution time    = 1.686398
// 0h01m03s 0h00m16s
//(II) Document (3) information
//(II) - filename      : tara_2x.merge.raw
//(II) - filesize      : 6757931080 bytes
//(II) -               : 6444 Mbytes
//(II) - #minzer start : 978247464 elements
//(II) -       skipped : 133506079 elements
//(II) -         final : 844741385 elements
//(II) - 8.17


// 4x => 41234930 lines

// 32x => 5154367 lines