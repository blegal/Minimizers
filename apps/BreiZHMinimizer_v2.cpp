#include <cstdio>
#include <vector>
#include <omp.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <filesystem>

#include "../src/minimizer/minimizer_v2.hpp"
#include "../src/minimizer/minimizer_v3.hpp"
#include "../src/merger/in_file/merger_in.hpp"
#include "../src/merger/CMergeFile.hpp"

#include "../src/merger/in_file/merger_n_files.hpp"
#include "../src/merger/in_file/merger_n_files_lt64.hpp"
#include "../src/merger/in_file/merger_n_files_ge64.hpp"

#include "../src/tools/colors.hpp"
#include "../src/tools/CTimer/CTimer.hpp"
#include "../src/tools/file_stats.hpp"

std::string to_number(int value, const int maxv)
{
    int digits = 1;
    if     (maxv > 100000) digits = 6;
    else if(maxv >  10000) digits = 5;
    else if(maxv >   1000) digits = 4;
    else if(maxv >    100) digits = 3;
    else if(maxv >     10) digits = 2;
    std::string number;
    while(digits--)
    {
        number = std::to_string(value%10) + number;
        value /= 10;
    }
    if( value != 0 )
        number += std::to_string(value) + number;
    return number;
}


//
//  Récupère la liste des fichiers contenus dans un répertoire
//
std::vector<std::string> file_list_cpp(const std::string path, std::string ext = ".fastx")
{
    struct stat path_stat;
    stat(path.c_str(), &path_stat);
    if( (path_stat.st_mode & S_IFDIR) == 0)
    {
        error_section();
        printf("(EE) The provided path is not a directory (%s)\n", path.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    std::vector<std::string> paths;
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if( entry.path().string().find_first_of(ext) != std::string::npos )
            paths.push_back(entry.path().string());
    }
    return paths;
}

std::string shorten(const std::string fname, const int length)
{
    //
    // Removing file path
    //
    int name_pos       = fname.find_last_of("/");
    std::string nstr   = fname;
    if( name_pos != std::string::npos )
        nstr  = fname.substr(name_pos + 1);
/*
    if( nstr.size() > length ){
        int suffix_pos     = nstr.find_last_of(".");
        std::string suffix = nstr.substr(suffix_pos);
        int prefix_length = length - suffix.size() - 3;
        return nstr.substr(0,prefix_length) + ".." + suffix;
    }else{
        return nstr;
    }
*/
    return nstr;
}

int main(int argc, char *argv[])
{
    CTimer timer_full( true );
//  const auto prog_start = std::chrono::steady_clock::now();

    //
    //
    //
    std::string directory = "";
    std::string filename  = "";
    std::string extension = "";
    std::string file_out  = "result";

    int   verbose_flag        = 0;
    bool  skip_minimizer_step = 0;

    bool  keep_merge_files     = false;
    bool  keep_minimizer_files = false;

    int   help_flag           = 0;
    int   threads_minz        = 1;
    int   threads_merge       = 1;
    int   ram_value           = 1024;
    int   limited_memory      = 1;

    int   file_limit          = 65536;

    std::string algo = "std::sort";

    static struct option long_options[] = {
            {"help",        no_argument, 0, 'h'},
            {"verbose",     no_argument, 0, 'v'},

            {"skip-minimizer-step",     no_argument, 0, 'S'},
            {"keep-minimizers",     no_argument, 0, 'K'},
            {"keep-temp-files",     no_argument, 0, 'k'},

            {"directory",   required_argument, 0, 'd'},
            {"filename",    required_argument, 0, 'f'},
            {"compression", required_argument, 0, 'C'},
            {"output",      required_argument, 0, 'O'},

            {"limited-mem",              no_argument, &limited_memory,    1},
            {"unlimited-mem",              no_argument, &limited_memory,    0},

            {"max-files",      required_argument, 0,  'x'},
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

            case 'K':
                keep_minimizer_files = true;
                break;

            case 'k':
                keep_merge_files     = true;
            break;

            case 'O':
                file_out = optarg;
                break;

            case 'C':
                extension  = ".";
                extension += optarg;
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

            case 'x':
                file_limit = std::atoi( optarg );
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

            case 'v':
                verbose_flag = true;
                break;

            case 'h':
                help_flag = true;
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
    std::vector<std::string> t_files = file_list_cpp(directory);
    std::sort(t_files.begin(), t_files.end());

    printf("(II) Searching for files\n");
    printf("(II) - Number of identified files : %zu\n", t_files.size());

    //
    // On filtre les fichiers que l'on souhaite traiter car dans les repertoires il peut y avoir
    // de la doc, etc.
    //
    std::vector<std::string> filelist;
    for( int i = 0; i < t_files.size(); i += 1 )
    {
        std::string t_file = t_files[i];
        if( (skip_minimizer_step == false) &&
                (
                    (t_file.substr(t_file.find_last_of(".") + 1) == "lz4")   ||
                    (t_file.substr(t_file.find_last_of(".") + 1) == "bz2")   ||
                    (t_file.substr(t_file.find_last_of(".") + 1) == "gz")    ||
                    (t_file.substr(t_file.find_last_of(".") + 1) == "fastx") ||
                    (t_file.substr(t_file.find_last_of(".") + 1) == "fasta") ||
                    (t_file.substr(t_file.find_last_of(".") + 1) == "fastq") ||
                    (t_file.substr(t_file.find_last_of(".") + 1) == "fna")
                )
            ){
            filelist.push_back( t_file );
        }else if( (skip_minimizer_step == true) &&
            (
                    (t_file.substr(t_file.find_last_of(".") + 1) == "raw")
            )
            ){
            //
            // raw files are mainly used for debbuging purpose !
            //
            filelist.push_back( t_file );
        }else{
            warning_section();
            printf ("(WW)   > discarding %s\n", t_file.c_str());
            reset_section();
        }
    }

    printf("(II) - Number of selected   files : %zu\n", filelist.size());
    printf("(II)\n");

    //
    // Si nous n'avons aucun fichier c'est que l'utilisateur a du faire une erreur
    //
    if( filelist.size() == 0 )
    {
        error_section();
        printf("(EE) The provided directory does not contain valid file(s) \n");
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    //
    // On limite le nombre de fichiers a traiter, utile lors des phases de debug
    // pour ne pas avoir a traiter tous les fichiers d'un repo
    //
    if ( filelist.size() > file_limit ) {
        warning_section();
        printf ("(WW) Reducing the number of files to process (file_limit = %d)\n", file_limit);
        reset_section();
        while (filelist.size() > file_limit) {
            filelist.pop_back();
        }
    }


    //
    //
    //
    std::vector<CMergeFile> n_files;
    std::vector<CMergeFile> l_files;
    if( skip_minimizer_step == false )
    {
        uint64_t in_mbytes = 0;
        uint64_t ou_mbytes = 0;

        printf("(II) Generating minimizers from DNA - %d thread(s)\n", threads_minz);
        if( limited_memory == true )
            printf("(II) - Limited memory mode : true\n");
        else
            printf("(II) - Limited memory mode : false\n");

        CTimer minzr_timer( true );

        //
        // On predimentionne le vecteur de sortie car on connait sa taille. Cela evite les
        // problemes liés à la fonction push_back qui a l'air incertaine avec OpenMP
        //
        n_files.resize( filelist.size() );

        int counter = 0;
        omp_set_num_threads(threads_minz);
#pragma omp parallel for default(shared)
        for(int i = 0; i < filelist.size(); i += 1)
        {
            CTimer minimizer_t( true );

            //
            // On mesure la taille des fichiers d'entrée
            //
            const file_stats i_file( filelist[i] );
            const std::string t_file = "data_n" + to_number(i, (int)filelist.size()) + ".raw";
            in_mbytes += i_file.size_mb;

            /////
            if( limited_memory == true )
                minimizer_processing_v3(i_file.name, t_file, algo, ram_value, true, false, false);
            else
                minimizer_processing_v2(i_file.name, t_file, algo, ram_value, true, false, false);
            /////

            //
            // On mesure la taille du fichier de sortie
            //
            const file_stats o_file(t_file);
            ou_mbytes += o_file.size_mb;
            if(verbose_flag == true )
            {
                //
                // Mesure du temps d'execution
                //
                std::string nname = shorten(i_file.name, 32);
                counter += 1;
                printf("%5d | %5d/%5d | %32s | %6lld MB | ==========> | %20s | %6lld MB | %5.2f sec.\n", i, counter, filelist.size(), nname.c_str(), i_file.size_mb, o_file.name.c_str(), o_file.size_mb, minimizer_t.get_time_sec());

            }

            /////
            CMergeFile d_file( t_file, 0, 0 );
            n_files[i] = d_file;                          // on stocke le nom du fichier que l'on vient de produire
            /////
        }

        //
        // The S-MER computation stage is now finished, we can prepare the merging ones
        //
        l_files = n_files;
        n_files.clear();

        //
        //
        //
        const float elapsed = minzr_timer.get_time_sec();
        printf("(II) - Information loaded from files : %6d MB\n", (int)(in_mbytes));
        printf("(II)   Information wrote to files    : %6d MB\n", (int)(ou_mbytes));
        printf("(II) - Information throughput (in)   : %6d MB/s\n", (int)((float)(in_mbytes) / elapsed));
        printf("(II)   Information throughput (out)  : %6d MB/s\n", (int)((float)(ou_mbytes) / elapsed));
        printf("(II) - Execution time : %1.2f seconds\n", elapsed);
        printf("(II)\n");
    }

    //
    //
    //
    printf("(II) Tree-based merging of sorted minimizer files - %d thread(s)\n", threads_merge);
    if(verbose_flag == true )
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");
    n_files.resize( (l_files.size() + 63) / 64 ); // pur éviter le push_back qui semble OpenMP unsafe !
    omp_set_num_threads(threads_merge); // on regle le niveau de parallelisme accessible dans cette partie
    int cnt = 0;
#pragma omp parallel for
    for(int ll = 0; ll < l_files.size(); ll += 64)
    {
        const auto start_file = std::chrono::steady_clock::now();
        const int64_t max_files = (l_files.size() - ll) < 64 ? (l_files.size() - ll) : 64;
        std::vector<std::string> liste;
        for(int ff = 0; ff < max_files; ff += 1)        // in this first stage all the file are not colored
            liste.push_back( l_files[ll + ff].name );   // at the input

        std::string t_file = "data_n" + to_number(ll/64, l_files.size()/64) + ".";
        t_file            += std::to_string(max_files) + "c" + extension;

//      printf("(II) %d - Creating %s\n", ll, t_file.c_str());

        merge_n_files_less_than_64_colors( liste, t_file);

        if(keep_minimizer_files == false)
        {
            for(int ff = 0; ff < max_files; ff += 1)
                std::remove( liste[ff].c_str() );
        }


        //
        // On memorise le nom et la nature du fichier que l'on vient de créer
        //
        CMergeFile o_file(t_file, 64, max_files); // the real color depends on the amount of merged files
        n_files[ll/64] = o_file;

        //
        // Information reporting for the user
        //
        if(verbose_flag == true ){
            const file_stats t_file( o_file.name );
            printf("%6d | %s .... ", cnt, l_files[ll            ].name.c_str());
            printf("%s ",                 l_files[ll+max_files-1].name.c_str());
            printf("   == %d x MERGE =>   ", 8);
            t_file.printf_size();
            const auto  end_file = std::chrono::steady_clock::now();
            const float elapsed_file = std::chrono::duration_cast<std::chrono::milliseconds>(end_file - start_file).count() / 1000.f;
            printf("in  %6.2fs\n", elapsed_file);
        }
        cnt += 1;
    }
    if(verbose_flag == true )
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

    //
    // The first merging stage is now ended, it is time to prepare the future ones
    //
    l_files = n_files;
    n_files.clear();

    //
    //
    //
    std::vector<CMergeFile> vrac_names;

    printf("(II) Tree-based merging of sorted minimizer files - %d thread(s)\n", threads_merge);
    const auto start_merge = std::chrono::steady_clock::now();
    int colors = 64;
    omp_set_num_threads(threads_merge); // on regle le niveau de parallelisme accessible dans cette partie
    while( l_files.size() > 1 )
    {
        if( verbose_flag )
            printf("(II)   - Merging %4zu files with %4d color(s)\n", l_files.size(), colors);
        else{
            printf("(II)   - Merging %4zu files | %4d color(s) | ", l_files.size(), colors);
            fflush(stdout);
        }
        const auto start_merge = std::chrono::steady_clock::now();

        if(verbose_flag == true )
            printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

#define MAX_FILES 8

        n_files.resize( (l_files.size() + MAX_FILES - 1) / MAX_FILES ); // pur éviter le push_back qui semble OpenMP unsafe !

        uint64_t in_mbytes = 0; // pour calculer les debits
        uint64_t ou_mbytes = 0; // pour calculer les debits


        int cnt = 0;
#pragma omp parallel for
        for(int ll = 0; ll < l_files.size(); ll += MAX_FILES) // On merge par 8, ce choix est discutable
        {
            //
            // Gathering statistic informations about the merging process
            //
            const auto start_file = std::chrono::steady_clock::now();
            const int64_t max_files = (l_files.size() - ll) < MAX_FILES ? (l_files.size() - ll) : MAX_FILES;

            int64_t local_mb = 0;
            for(int ff = 0; ff < max_files; ff += 1)
            {
                const file_stats file_s( l_files[ll + ff].name );
                in_mbytes += file_s.size_mb;
                local_mb  += file_s.size_mb;
            }

            int final_real_color = 0;
            for(int ff = 0; ff < max_files; ff += 1)
                final_real_color += l_files[ll + ff].real_colors;

            int final_numb_colors = 0;
            for(int ff = 0; ff < max_files; ff += 1)
                final_numb_colors += l_files[ll + ff].numb_colors;

            //
            // Generation of the name of the output file
            //
            std::string t_file   = "data_n";
            t_file += to_number(ll/MAX_FILES, l_files.size()/MAX_FILES) + ".";
            t_file += std::to_string(final_real_color) + "c" + extension;

            //
            // Creation of the filelist
            //
            std::vector<std::string> tmp_list;
            for(int ff = 0; ff < max_files; ff += 1)
                tmp_list.push_back( l_files[ll + ff].name );

            //
            // We apply a 2->1 merging process
            //
//            merger_in(
//                    l_files[ll    ].name,
//                    l_files[ll + 1].name,
//                    t_file,
//                    l_files[ll + 1].numb_colors,
//                    l_files[ll + 1].numb_colors
//                    );    // couleurs homogenes
            merge_n_files_greater_than_64_colors(
                    tmp_list,
                    l_files[ll].numb_colors,
                t_file);


//            if(
//                    (keep_merge_files == false) && !((skip_minimizer_step || keep_minimizer_files) && (colors == 1)) ) // sinon on supprime nos fichier d'entrée !
//            {
            for(int ff = 0; ff < max_files; ff += 1)
                std::remove( l_files[ll+ff].name.c_str() );

            const file_stats o_file( t_file );
            ou_mbytes += o_file.size_mb;

            //
            // Creation of the object associated to the generated file
            //
            CMergeFile cm_file( t_file, final_numb_colors, final_real_color);
            n_files[ll/MAX_FILES] = cm_file;

            //
            // Information reporting for the user
            //
            if(verbose_flag == true ){
                printf("%6d | %s .... ", cnt, l_files[ll            ].name.c_str());
                printf("%s ",                 l_files[ll+max_files-1].name.c_str());
                printf("   == %d x MERGE =>   ", MAX_FILES);
                o_file.printf_size();
                const auto  end_file = std::chrono::steady_clock::now();
                const float elapsed_file = std::chrono::duration_cast<std::chrono::milliseconds>(end_file - start_file).count() / 1000.f;
                printf("in  %6.2fs\n", elapsed_file);
            }

            cnt += 1; // on compte le nombre de données traitées
        }
        const auto  end_merge = std::chrono::steady_clock::now();

        const float elapsed_file = (float)std::chrono::duration_cast<std::chrono::milliseconds>(end_merge - start_merge).count() / 1000.f;
        if( verbose_flag ){
            const float d_in = (float)in_mbytes / elapsed_file;
            const float d_ou = (float)ou_mbytes / elapsed_file;
            printf("(II)     + Step done in %6.2f seconds | in: %6d MB/s | out: %6d |\n", elapsed_file, (int)d_in, (int)d_ou);
        }
        else{
            const float d_in = (float)in_mbytes / elapsed_file;
            const float d_ou = (float)ou_mbytes / elapsed_file;
            printf("%6.2f seconds | in: %6d MB/s | out: %6d]\n", elapsed_file, (int)d_in, (int)d_ou);
        }

        //
        // On regarde si le dernier fichier a le meme nombre de couleurs que les autres fichiers
        // n'est pas équilibré. On stocke le fichier
        //
        if( n_files.size() >= 2 )
        {
            CMergeFile d1_file = n_files[n_files.size()-2];
            CMergeFile d2_file = n_files[n_files.size()-1];
            if( d1_file.numb_colors != d2_file.numb_colors )
            {
                n_files.pop_back();
                vrac_names.push_back ( d2_file );
                warning_section();
                printf("(II)     > Keeping (%s) file for later processing...\n", d2_file.name.c_str());
                reset_section();
            }
        }

        //
        // Si c'est le dernier fichier alors on l'ajoute dans la liste des fichiers à fusionner. Si il n'y a que lui
        // il ne se passera rien, sinon on aura une fusion complete des datasets
        //
        if( n_files.size() == 1 )
        {
            vrac_names.push_back ( n_files[0] );
        }

        l_files = n_files;
        n_files.clear();
        colors *= MAX_FILES;
    }
    const auto  end_merge = std::chrono::steady_clock::now();
    const float elapsed_merge = std::chrono::duration_cast<std::chrono::milliseconds>(end_merge - start_merge).count() / 1000.f;
    printf("(II) - Execution time : %1.2f seconds\n", elapsed_merge);
    printf("(II)\n");

    //
    //
    //
    if(vrac_names.size() == 0)
    {
        //
        // On n'a qu'un seul fichier suite au premier processus de fusion (64), donc on ne passe
        // pas par la seconde étape de fusion => mise à jour du vecteur de sortie
        //
        const CMergeFile single = l_files[0];
        vrac_names.push_back( single );
    }


    if( verbose_flag )
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");


    for(int i = 0; i < vrac_names.size(); i += 1)
    {
        printf("(II) Remaning file : %5d | %20s | num_colors = %6lld | real_colors = %6lld \n", i, vrac_names[i].name.c_str(),  vrac_names[i].numb_colors,  vrac_names[i].real_colors);
    }


    if( verbose_flag )
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");


    //
    //
    //
    if( vrac_names.size() > 1 )
    {
        printf("(II) Comb-based merging of sorted minimizer files\n");
        CTimer timer_merge( true );

        int cnt = 0;

        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

        while( vrac_names.size() != 1 )
        {
            const CMergeFile i_file_1 = vrac_names[1]; // le plus grand est tjs le second
            const CMergeFile i_file_2 = vrac_names[0]; // la plus petite couleur est le premier
                  CMergeFile o_file  ( "", i_file_1, i_file_2 ); // la plus petite couleur est le premier

            o_file.name = "data_n" + std::to_string(cnt++) + "." + std::to_string( o_file.real_colors ) + "c";

            printf("- %20.20s (%lld) + %20.20s (%lld)\n", i_file_1.name.c_str(), i_file_1.real_colors, i_file_2.name.c_str(), i_file_2.real_colors);

            //
            // On lance le processus de merging sur les 2 fichiers
            //
            printf("  %20.20s (%lld) + %20.20s (%lld) ===> %20.20s (%lld)\n",
                   i_file_1.name.c_str(), i_file_1.real_colors,
                   i_file_2.name.c_str(), i_file_2.real_colors,
                   o_file.name.c_str(),   o_file.real_colors);

            merger_in(
                    i_file_1.name,
                    i_file_2.name,
                    o_file.name,
                    i_file_1.numb_colors,
                    i_file_2.numb_colors
            );
            vrac_names[1] = o_file;

            //
            // On supprime le premier élément du tableau
            //
            vrac_names.erase( vrac_names.begin() );

            //
            // On supprime les fichiers source
            //
            if( keep_merge_files == false )
            {
                std::remove( i_file_1.name.c_str() ); // delete file
                std::remove( i_file_2.name.c_str() ); // delete file
            }
        }
        printf("(II) - Execution time : %1.2f seconds\n", timer_merge.get_time_sec());
        printf("(II)\n");
    }


    //
    // Normalement on a un fichier en sortie du processus de fusion, sinon c'est que quelque-chose
    // a merdé dans le processus de fusion
    //
    if( vrac_names.size() == 1 )
    {
        const CMergeFile lastfile = vrac_names[0];
        const std::string o_file = file_out + "." + std::to_string(lastfile.real_colors) + "c" + extension;
        std::rename(lastfile.name.c_str(), o_file.c_str());
        printf("(II) Renaming final file : %s\n", o_file.c_str());
    }else{
        printf("(EE) Something strange happened !!!\n");
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    //
    // Il faudrait que l'on gere les fichiers que l'on a mis de coté lors du processus de fusion...
    //
    printf("(II)\n");
    printf("(II) - Total execution time : %1.2f seconds\n", timer_full.get_time_sec());

    return 0;
}