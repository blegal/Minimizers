#include <cstdio>
#include <fstream>
#include <vector>
#include <omp.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <filesystem>


#include "./minimizer/minimizer.hpp"
#include "./merger/merger_in.hpp"
//
//  Récupère la taille en octet du fichier passé en paramètre
//
uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return 0;
    }
    return file_status.st_size;
}

//
//  Récupère la liste des fichiers contenus dans un répertoire
//
std::vector<std::string> file_list_cpp(const std::string path, std::string ext = ".fastx")
{
    std::vector<std::string> paths;
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if( entry.path().string().find_first_of(ext) != std::string::npos )
            paths.push_back(entry.path().string());
    }
    return paths;
}


int main(int argc, char *argv[])
{
    //
    //
    //
    std::string directory = "";
    std::string filename  = "";

    int  verbose_flag        = 0;
    bool  skip_minimizer_step = 0;
    bool  keep_temp_files     = 0;
    bool help_flag           = false;
    int  threads             = 0;

    static struct option long_options[] ={
            {"verbose",     no_argument, 0, 'v'},
            {"skip-minimizer-step",     no_argument, 0, 's'},
            {"keep-temp-files",     no_argument, 0, 'k'},
            {"help",        no_argument, 0, 'h'},
            {"directory",       required_argument, 0, 'd'},
            {"filename",      required_argument, 0,  'f'},
            {"threads",      required_argument, 0,  't'},
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
            case 'd':
                directory = optarg;
                break;

            case 'f':
                filename = optarg;
                break;

            case 's':
                skip_minimizer_step = true;
                break;

            case 'k':
                keep_temp_files = true;
                break;

            case 't':
                threads = std::atoi( optarg );
                omp_set_dynamic(0);
                omp_set_num_threads(threads);
                break;

            case 'h':
                help_flag = true;
                break;

            case '?':
                break;

            default:
                abort ();
        }
    }

    /*
     * Print any remaining command line arguments (not options).
     * */
    if ( (optind < argc) || (help_flag == true) || ((directory.size() == 0) && (filename.size() == 0)))
    {
        printf ("Usage :\n");
        printf ("./BreiZHMinimizer -d <directory to process>              [options]\n");
        printf ("./BreiZHMinimizer -d <file with list of file to process> [options]\n");
        printf ("\n");
        printf ("Options :\n");
        printf ("  -s : --skip-minimizer-step : \n");
        printf ("  -k --keep-temp-files     : \n");
        printf ("  -t  <number>    : threads\n");
        putchar ('\n');
        exit( EXIT_FAILURE );
    }


    //
    // On recuperer la liste des fichiers que l'on doit traiter. Ce nombre de fichiers doit
    // être multiple de 2 pour que la premiere étape de fusion soit faite pour tous les fichier
    // (ajout de la donnée uint64_t pour la couleur). Ce n'est pas un probleme scientifique mais
    // technique, un peu plus de code a developper plus tard...
    //
    std::vector<std::string> l_files = file_list_cpp(directory);
    for( int i = 0; i < l_files.size(); i += 1 )
    {
        if( l_files[i].find(".DS_Store") != std::string::npos )
            l_files.erase( l_files.begin() + i );
    }

    std::sort(l_files.begin(), l_files.end());

    std::vector<std::string> n_files;

    //
    //
    //
    if( skip_minimizer_step == false )
    {
#pragma omp parallel for
        for(int i = 0; i < l_files.size(); i += 1)
        {
            const std::string i_file = l_files[i];
            const uint64_t f_size    = get_file_size(i_file);
            const uint64_t size_mb   = f_size / 1024 / 1024;
            const std::string o_file = "data_n" + std::to_string(i) + ".c0";
            /////
            minimizer_processing(
                     i_file,
                     o_file,
                    "crumsort",         // algo
                    true,               // file_save_output,
                    true,               // worst_case_memory,
                    false,              // verbose_flag,
                    false               // file_save_debug
            );
            /////
            const uint64_t o_size    = get_file_size(o_file);
            const uint64_t sizo_mb   = o_size / 1024 / 1024;
            printf("%5d | %20s | %6lld MB | ==========> | %20s | %6lld MB |\n", i, i_file.c_str(), size_mb, o_file.c_str(), sizo_mb);
            /////
            n_files.push_back( o_file );
            /////
        }
        l_files = n_files;
        n_files.clear();
    }

    //
    //
    //
    std::vector<std::string> vrac_names;
    std::vector<int>         vrac_levels;

    //
    //
    //
    int colors = 1;
    while( l_files.size() > 1 )
    {
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");
        int cnt = 0;
        while( l_files.size() >= 2 )
        {
            const std::string i_file_1 = l_files[0]; l_files.erase( l_files.begin() );
            const std::string i_file_2 = l_files[0]; l_files.erase( l_files.begin() );
            const std::string o_file   = "data_n" + std::to_string(cnt) + "." + std::to_string(2 * colors) + "c";

            const uint64_t i1_size   = get_file_size(i_file_1);
            const uint64_t siz1_kb   = i1_size / 1024;
            const uint64_t siz1_mb   = siz1_kb / 1024;

            const uint64_t i2_size   = get_file_size(i_file_2);
            const uint64_t siz2_kb   = i2_size / 1024;
            const uint64_t siz2_mb   = siz2_kb / 1024;

//            printf("| %20s | + | %20s | %d |\n", i_file_1.c_str(), i_file_2.c_str(), colors);

            merger_in(
                    i_file_1,
                    i_file_2,
                    o_file,
                    colors,     // les 2 fichiers ont un nombre de couleurs homogene donc
                    colors);    // on passe 2 fois le meme parametre

            if( keep_temp_files == 0 )
            {
                std::remove( i_file_1.c_str() ); // delete file
                std::remove( i_file_2.c_str() ); // delete file
            }

            const uint64_t o_size    = get_file_size(o_file);
            const uint64_t sizo_kb   = o_size  / 1024;
            const uint64_t sizo_mb   = sizo_kb / 1024;

            //
            //
            //

            n_files.push_back( o_file );
            if     ( siz1_kb < 10 ) printf("%5d | %20s | %6lld B  ",  cnt, i_file_1.c_str(), i1_size);
            else if( siz1_mb < 10 ) printf("%5d | %20s | %6lld KB ", cnt, i_file_1.c_str(), siz1_kb);
            else                    printf("%5d | %20s | %6lld MB ", cnt, i_file_1.c_str(), siz1_mb);

            //
            //
            //
            if     ( siz2_kb < 10 ) printf("| %20s | %6lld B  | ==========> | ", i_file_2.c_str(), i2_size);
            else if( siz2_mb < 10 ) printf("| %20s | %6lld KB | ==========> | ", i_file_2.c_str(), siz2_kb);
            else                    printf("| %20s | %6lld MB | ==========> | ", i_file_2.c_str(), siz2_mb);

            //
            //
            //
            if     ( sizo_kb < 10 ) printf("%20s | %6lld B  |\n", o_file.c_str(), o_size);
            else if( sizo_mb < 10 ) printf("%20s | %6lld KB |\n", o_file.c_str(), sizo_kb);
            else                    printf("%20s | %6lld MB |\n", o_file.c_str(), sizo_mb);

            cnt += 1;
        }

        //
        // On regarde si des fichiers n'ont pas été traités. Cela peut arriver lorsque l'arbre de fusion
        // n'est pas équilibré. On stocke le fichier
        //
        if( l_files.size() != 0 )
        {
            vrac_names.push_back ( l_files.front() );
            vrac_levels.push_back( colors          );
        }

        l_files = n_files;
        n_files.clear();
        colors *= 2;
    }
    printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

    for(int i = 0; i < vrac_names.size(); i += 1)
    {
        printf("> %5d | %20s | level = %6d ||\n", i, vrac_names[i].c_str(), vrac_levels[i]);
    }

    printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

    //
    // Il faudrait que l'on gere les fichiers que l'on a mis de coté lors du processus de fusion...
    //

    return 0;
}