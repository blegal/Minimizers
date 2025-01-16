#include "merger_in.hpp"

#include "merger_level_0.hpp"
#include "merger_level_1.hpp"
#include "merger_level_2_32.hpp"

void merger_in(
        const std::string& ifile_1,
        const std::string& ifile_2,
        const std::string& o_file,
        const int level)
{
    if( level == 0 )
        merge_level_0(ifile_1, ifile_2, o_file);
    else if( level == 1 )
        merge_level_1(ifile_1, ifile_2, o_file);
    else if( level == 2 )
        merge_level_2_32(ifile_1, ifile_2, o_file, 2);
    else if( level == 4 )
        merge_level_2_32(ifile_1, ifile_2, o_file, 4);
    else if( level == 8 )
        merge_level_2_32(ifile_1, ifile_2, o_file, 8);
    else if( level == 16 )
        merge_level_2_32(ifile_1, ifile_2, o_file, 16);
    else if( level == 32 )
        merge_level_2_32(ifile_1, ifile_2, o_file, 32);
//  else
//        exit( EXIT_FAILURE );
}