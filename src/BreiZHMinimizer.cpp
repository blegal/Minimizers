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

    int   verbose_flag        = 0;
    bool  skip_minimizer_step = 0;
    bool  keep_temp_files     = 0;
    int   help_flag           = 0;
    int   threads_minz        = 1;
    int   threads_merge       = 1;
    int   ram_value           = 1024;
    int   limited_memory      = 1;

    std::string algo = "std::sort";

    static struct option long_options[] = {
            {"help",               no_argument, &help_flag,    1},
            {"help",        no_argument, 0, 'h'},

            {"verbose",     no_argument, 0, 'v'},

            {"skip-minimizer-step",     no_argument, 0, 'S'},
            {"keep-temp-files",     no_argument, 0, 'k'},

            {"directory",       required_argument, 0, 'd'},
            {"filename",      required_argument, 0,  'f'},

            {"limited-mem",              no_argument, &limited_memory,    1},
            {"unlimited-mem",              no_argument, &limited_memory,    0},

            {"threads",      required_argument, 0,  't'},
            {"threads-minz", required_argument, 0,  'm'},
            {"threads-merge",required_argument, 0,  'g'},
            {"sorter",      required_argument, 0, 's'},
            {"GB",           required_argument, 0, 'G'},
            {"MB",           required_argument, 0, 'M'},
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

            case 'S':
                skip_minimizer_step = true;
                break;

            case 'k':
                keep_temp_files = true;
                break;

            case 't':
                threads_minz  = std::atoi( optarg );
                threads_merge = std::atoi( optarg );
                break;

            case 'm':
                threads_minz  = std::atoi( optarg );
                break;

            case 'g':
                threads_merge = std::atoi( optarg );
                break;

            case 'h':
                help_flag = true;
                break;

            case 's':
                algo = optarg;
                break;

            case 'M':
                ram_value = std::atoi( optarg );
                break;

            case 'G':
                ram_value = 1024 * std::atoi( optarg );
                break;

            case '?':
                help_flag = true;
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
        printf ("./BreiZHMinimizer -f <file with list of file to process> [options] (NOT WORKING YET !)\n");
        printf ("\n");
        printf ("Common options :\n");
        printf ("  --threads <int>       (-t) : \n");
        printf ("  --skip-minimizer-step (-s) : \n");
        printf ("  --keep-temp-files     (-k) : \n");
        printf ("  --output <string>     (-o) : the name of the output file that containt minimizers at the end\n");

        printf ("\n");
        printf ("Minimizer engine options :\n");
        printf (" --sorter <string> (-s) : the name of the sorting algorithm to apply\n");
        printf("                        + std::sort       :\n");
        printf("                        + std_2cores      :\n");
        printf("                        + std_4cores      :\n");
        printf("                        + crumsort        : default\n");
        printf("                        + crumsort_2cores :\n");
        printf (" --sorter <string>     (-s) : \n");
        printf (" --MB             (-M) [int]    : maximum memory usage in MBytes\n");
        printf (" --GB             (-G) [int]    : maximum memory usage in GBytes\n");
        printf ("\n");

        printf ("Others :\n");
        printf (" --verbose        (-v)          : display debug informations\n");
        printf (" --help           (-h)          : display debug informations\n");
        putchar ('\n');
        exit( EXIT_FAILURE );
/*
        {"skip-minimizer-step",     no_argument, 0, 'S'},
        {"keep-temp-files",     no_argument, 0, 'k'},
        {"directory",       required_argument, 0, 'd'},
        {"filename",      required_argument, 0,  'f'},
        {"limited-mem",              no_argument, &limited_memory,    1},
        {"unlimited-mem",              no_argument, &limited_memory,    0},
        {"threads",      required_argument, 0,  't'},
        {"sorter",      required_argument, 0, 's'},
        {"GB",           required_argument, 0, 'G'},
        {"MB",           required_argument, 0, 'M'},
*/
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
        const auto start = std::chrono::system_clock::now();

        omp_set_num_threads(threads_minz);
#pragma omp parallel for
        for(int i = 0; i < l_files.size(); i += 1)
        {
            const std::string i_file = l_files[i];
            const uint64_t f_size    = get_file_size(i_file);
            const uint64_t size_mb   = f_size / 1024 / 1024;
            const std::string o_file = "data_n" + std::to_string(i) + ".c0";
            /////
            if( limited_memory == true )
            {
                minimizer_processing_v3(
                        i_file,
                        o_file,
                        algo,
                        ram_value,
                        true,
                        verbose_flag,
                        false
                );
            }else{
                minimizer_processing_v2(
                        i_file,
                        o_file,
                        algo,
                        ram_value, // increase by 50% when required
                        true,
                        false,
                        false
                );
            }
            /////
            const uint64_t o_size    = get_file_size(o_file);
            const uint64_t sizo_mb   = o_size / 1024 / 1024;
            if(verbose_flag == true )
            {
                printf("%5d | %20s | %6lld MB | ==========> | %20s | %6lld MB |\n", i, i_file.c_str(), size_mb, o_file.c_str(), sizo_mb);
            }
            /////
            n_files.push_back( o_file );
            /////
        }
        l_files = n_files;
        n_files.clear();

        const auto  end = std::chrono::system_clock::now();
        const float elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.f;
        printf(" - Execution time : %1.2f seconds\n", elapsed);
    }
    //
    //
    //
    std::vector<std::string> vrac_names;
    std::vector<int>         vrac_levels;

    //
    //
    //
    const auto start_merge = std::chrono::system_clock::now();
    int colors = 1;
    while( l_files.size() > 1 )
    {
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");
        const auto start_level = std::chrono::system_clock::now();
        int cnt = 0;
//        omp_set_num_threads(threads_merge);
//#pragma omp parallel for
        for(int ll = 0; ll < l_files.size() - 1; ll += 2)
        {
            const auto start_file = std::chrono::system_clock::now();
            const std::string i_file_1 = l_files[ll   ];
            const std::string i_file_2 = l_files[ll + 1];

            std::string o_file;
//#pragma omp critical
            { // a cause du cnt++
                o_file = "data_n" + std::to_string(cnt++) + "." + std::to_string(2 * colors) + "c";
            };

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

            if( (keep_temp_files == 0) && !((skip_minimizer_step == true) && (colors == 1)) ) // sinon on supprime nos fichier d'entrée !
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
            if     ( sizo_kb < 10 ) printf("%20s | %6lld B  | ", o_file.c_str(), o_size);
            else if( sizo_mb < 10 ) printf("%20s | %6lld KB | ", o_file.c_str(), sizo_kb);
            else                    printf("%20s | %6lld MB | ", o_file.c_str(), sizo_mb);

            const auto  end_file = std::chrono::system_clock::now();
            const float elapsed_file = std::chrono::duration_cast<std::chrono::milliseconds>(end_file - start_file).count() / 1000.f;
            printf(" %6.2fs\n", elapsed_file);
        }

        //
        // On regarde si des fichiers n'ont pas été traités. Cela peut arriver lorsque l'arbre de fusion
        // n'est pas équilibré. On stocke le fichier
        //
        if( l_files.size()%2 )
        {
            vrac_names.push_back ( l_files[l_files.size()-1] );
            vrac_levels.push_back( colors                    );
        }

        //
        // Si c'est le dernier fichier alors on l'ajoute dans la liste des fichiers à fusionner. Si il n'y a que lui
        // il ne se passera rien, sinon on aura une fusion complete des datasets
        //
        if( n_files.size() == 1 )
        {
            vrac_names.push_back ( n_files[0] );
            vrac_levels.push_back( 2 * colors );
        }

        l_files = n_files;
        n_files.clear();
        colors *= 2;
        const auto  end_level = std::chrono::system_clock::now();
        const float elapsed_level = std::chrono::duration_cast<std::chrono::milliseconds>(end_level - start_level).count() / 1000.f;
        printf(" - Execution time : %1.2f seconds\n", elapsed_level);

    }
    const auto  end_merge = std::chrono::system_clock::now();
    const float elapsed_merge = std::chrono::duration_cast<std::chrono::milliseconds>(end_merge - start_merge).count() / 1000.f;
    printf(" - Execution time : %1.2f seconds\n", elapsed_merge);
    printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");



    for(int i = 0; i < vrac_names.size(); i += 1)
    {
        printf("> %5d | %20s | level = %6d ||\n", i, vrac_names[i].c_str(), vrac_levels[i]);
    }

    printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

    if( vrac_names.size() > 1 )
    {
        int cnt = 0;
        std::vector<int> vrac_real_color = vrac_levels;
        //
        // A t'on un cas particulier a gerer (fichier avec 0 couleur)
        //
        if(vrac_levels[0] == 1)
        {
            const std::string i_file = vrac_names[0];
            const std::string o_file   = "data_n" + std::to_string(cnt++) + "." + std::to_string(2) + "c";
            merger_in(i_file,i_file, o_file, 0, 0);
            vrac_names     [0] = o_file;
            vrac_levels    [0] =      2; // les niveaux de fusion auxquels on
            vrac_real_color[0] =      1; // on a une seule couleur
            printf("- %20.20s (%d) + %20.20s (%d)\n", i_file.c_str(), 0, i_file.c_str(), 0);
            std::remove( i_file.c_str() ); // delete file
        }

        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

        while( vrac_names.size() != 1 )
        {
            const std::string i_file_1 = vrac_names[1]; // le plus grand est tjs le second
            const std::string i_file_2 = vrac_names[0]; // la plus petite couleur est le premier
            int color_1     = vrac_levels    [1];
            int color_2     = vrac_levels    [0];
            int r_color_1   = vrac_real_color[1];
            int r_color_2   = vrac_real_color[0];
            int real_color  = r_color_1 + r_color_2;
            int merge_color = color_1 + color_2;

            printf("- %20.20s (%d) + %20.20s (%d)\n", i_file_1.c_str(), r_color_1, i_file_2.c_str(), r_color_2);

            if( (color_1 <= 32) && (color_2 <= 32) ){
                color_2     = color_1;           // on uniformise
                merge_color = color_1 + color_2; // le double du plus gros
            }else if( color_1 >= 64 ){
                if( color_2 < 64 ){
                    color_2     = 64;                 // on est obligé de compter 64 meme si c moins, c'est un uint64_t
                    merge_color = color_1 + color_2;  //
                }else{
                    // on ne change rien !
                    merge_color = color_1 + color_2;  //
                }
            }

            //
            // On lance le processus de merging sur les 2 fichiers
            //
            const std::string o_file   = "data_n" + std::to_string(cnt++) + "." + std::to_string(real_color) + "c";
            printf("  %20.20s (%d) + %20.20s (%d) ===> %20.20s (%d)\n", i_file_1.c_str(), color_1, i_file_2.c_str(), color_2, o_file.c_str(), real_color);
            merger_in(i_file_1,i_file_2, o_file, color_1, color_2);
            vrac_names     [1] = o_file;
            vrac_levels    [1] = merge_color;
            vrac_real_color[1] = real_color;

            //
            // On supprime le premier élément du tableau
            //
            vrac_names.erase     ( vrac_names.begin()      );
            vrac_levels.erase    ( vrac_levels.begin()     );
            vrac_real_color.erase( vrac_real_color.begin() );

            //
            // On supprime les fichiers source
            //
            std::remove( i_file_1.c_str() ); // delete file
            std::remove( i_file_2.c_str() ); // delete file
        }

    }

    for(int i = 0; i < vrac_names.size(); i += 1)
    {
        printf("> %5d | %20s | level = %6d ||\n", i, vrac_names[i].c_str(), vrac_levels[i]);
    }

    //
    // Il faudrait que l'on gere les fichiers que l'on a mis de coté lors du processus de fusion...
    //

    return 0;
}