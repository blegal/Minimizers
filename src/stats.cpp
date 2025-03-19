#include <cstdio>
#include <fstream>
#include <vector>
#include <omp.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <filesystem>

#include "./minimizer/minimizer_v2.hpp"
#include "./minimizer/minimizer_v3.hpp"
#include "./minimizer/minimizer_v4.hpp"
#include "./merger/merger_in.hpp"

#include "./tools/colors.hpp"
#include "./tools/CTimer.hpp"
#include "./tools/file_stats.hpp"

uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}


int main(int argc, char *argv[])
{
    std::string filename  = "";

    static struct option long_options[] = {
            {"filename",      required_argument, 0,  'f'},
            {0, 0, 0, 0}
    };


    /* getopt_long stores the option index here. */
    int option_index = 0;
    int c;
    while( true )
    {
        c = getopt_long (argc, argv, "d:f:t:vhks", long_options, &option_index);
        if (c == -1)
            break;

        switch ( c )
        {
            case 'f':
                filename = optarg;
                break;

            default:
                abort ();
        }
    }

    /*
     * Print any remaining command line arguments (not options).
     * */
    if ( (optind < argc) || (filename.size() == 0))
    {
        printf ("Usage :\n");
        printf ("./stats -f <file with list of file to process>\n");
        putchar ('\n');
        exit( EXIT_FAILURE );
    }
    
    std::vector<std::string> t_files; //replace w/ m_filenames in kam
    std::ifstream infile(filename);
    if (!infile) {
        error_section();
        printf("(EE) Unable to open file: %s\n", filename.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit(EXIT_FAILURE);
    }
    std::string line;
    while (std::getline(infile, line)) {
        t_files.push_back(line);
    }
    infile.close();

    std::vector<uint64_t> results; //(filesize(Bytes), bp, kmer, minmer)

    for(size_t i = 0; i < t_files.size(); i += 1)
    {
        //
        // On mesure la taille des fichiers d'entrée
        //
        const std::string i_file( t_files[i] );
        results.push_back( get_file_size(i_file) );
        minimizer_processing_stats_only(i_file, results);
    }
    
    std::string file_out  = filename + ".stats";
    FILE* outfile = fopen(file_out.c_str(), "w");
    if (!outfile) {
        error_section();
        printf("(EE) Unable to open file: %s\n", file_out.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit(EXIT_FAILURE);
    }
    fwrite(results.data(), sizeof(uint64_t), results.size(), outfile);
    fclose(outfile);
}