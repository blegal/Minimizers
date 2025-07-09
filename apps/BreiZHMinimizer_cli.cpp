#include "../lib/BreiZHMinimizer.hpp"

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


int main(int argc, char *argv[])
{
    //
    //
    //
    std::string directory = "";
    std::string filename  = "";
    //std::string extension = "";
    std::string file_out  = "result";

    int   verbose_flag        = 0;
    bool  skip_minimizer_step = 0;

    bool  keep_merge_files     = false;
    bool  keep_minimizer_files = false;

    int   help_flag           = 0;
    int   threads_minz        = 1;
    int   threads_merge       = 1;
    int   ram_value           = 1024; //MB
    int   limited_memory      = 1;
    int   merge_step          = 8;

    int   file_limit          = 65536;

    std::string algo = "crumsort";

    static struct option long_options[] = {
            {"help",        no_argument, 0, 'h'},
            {"verbose",     no_argument, 0, 'v'},

            {"skip-minimizer-step",     no_argument, 0, 'S'},
            {"keep-minimizers",     no_argument, 0, 'K'},
            {"keep-temp-files",     no_argument, 0, 'k'},

            {"directory",   required_argument, 0, 'd'},
            {"filename",    required_argument, 0, 'f'},
            {"output",      required_argument, 0, 'o'},

            {"limited-mem",        no_argument, &limited_memory,    1},
            {"unlimited-mem",     no_argument, &limited_memory,    0},
            {"ways",      required_argument, 0, 'w'},

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
        c = getopt_long(argc, argv, "d:f:t:m:g:x:vhkKsSo:w:s:M:G:", long_options, &option_index);

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

            case 'o':
                file_out = optarg;
                break;

            case 'w':
                merge_step = std::atoi( optarg );
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

    auto tmp_dir = "./";

    generate_minimizers(
        filelist,
        file_out,
        tmp_dir,
        threads_minz,
        ram_value,
        31, // k-mer size
        19, // minimizer size
        merge_step,
        algo,
        skip_minimizer_step,
        keep_minimizer_files,
        keep_merge_files,
        verbose_flag
    );


    return 0;
}