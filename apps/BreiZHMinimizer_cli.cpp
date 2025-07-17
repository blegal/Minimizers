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
    std::string tmp_dir = "./";

    int   verbose_flag        = 0;
    bool  skip_minimizer_step = 0;

    bool  keep_minimizer_files = false;
    bool  keep_merge_files     = false;
    
    int   help_flag           = 0;
    int   threads             = 4;
    uint64_t   ram_value           = 1024; //MB
    uint64_t   merge_step          = 8;

    int kmer_size = 31; // k-mer size
    int minimizer_size = 19; // minimizer size

    uint64_t   file_limit          = 65536;

    std::string algo = "crumsort";

    static struct option long_options[] = {
            {"help",        no_argument, 0, 'h'},
            {"verbose",     no_argument, 0, 'v'},

            {"skip-minimizer-step",     no_argument, 0, 's'},
            {"keep-minimizer-files",     no_argument, 0, 'n'},
            {"keep-merge-files",     no_argument, 0, 'N'},

            {"directory",   required_argument, 0, 'd'},
            {"filename",    required_argument, 0, 'f'},
            {"output",      required_argument, 0, 'o'},
            {"tmp-dir",      required_argument, 0, 'u'},

            {"kmer-size", required_argument, 0, 'k'},
            {"minimizer-size", required_argument, 0, 'm'},
            {"merge-step",      required_argument, 0, 'w'},

            {"max-files",      required_argument, 0,  'x'},
            {"threads",      required_argument, 0,  't'},
            {"sorter-algo",      required_argument, 0, 'a'},
            {"GB",           required_argument, 0, 'G'},
            {"MB",           required_argument, 0, 'M'},
            {0, 0, 0, 0}
    };


    /* getopt_long stores the option index here. */
    int option_index = 0;
    int c;
    while( true )
    {
        c = getopt_long(argc, argv, "d:f:snNo:k:m:w:t:x:a:M:G:vh", long_options, &option_index);

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

            case 'n':
                keep_minimizer_files = true;
                break;

            case 'N':
                keep_merge_files     = true;
                break;

            case 'o':
                file_out = optarg;
                break;

            case 'k':
                kmer_size = std::atoi( optarg );
                break;

            case 'm':
                minimizer_size = std::atoi( optarg );
                break;

            case 'w':
                merge_step = std::atoi( optarg );
                break;

            case 't':
                threads  = std::atoi( optarg );
                break;

            case 'x':
                file_limit = std::atoi( optarg );
                break;

            case 'u':
                tmp_dir = optarg;
                break;

            case 'a':
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
        printf ("  --output <string>      (-o) : the name of the output file that containt minimizers at the end (default: result)\n");
        printf ("  --kmer-size <int>      (-k) : (default: 31)\n");
        printf ("  --minimizer-size <int> (-m) : (default: 19)\n");
        printf ("  --threads <int>        (-t) : (default: 4)\n");
        printf ("  --tmp-dir <string>     (-u) : (default: ./)\n");
        printf ("  --skip-minimizer-step  (-s) : (default: OFF)\n");
        printf ("  --keep-minimizer-files (-n) : (default: OFF)\n");
        printf ("  --keep-merge-files     (-N) : (default: OFF)\n");
        printf ("  --max-files <int>      (-x) : Max number of files considered in dir or file of files (default: 65536)\n");
        

        printf ("\n");
        printf ("Minimizer engine options :\n");
        printf (" --sorter-algo <string> (-a) : the name of the sorting algorithm to apply\n");
        printf("                        + std::sort       :\n");
        printf("                        + std_2cores      :\n");
        printf("                        + std_4cores      :\n");
        printf("                        + crumsort        : default\n");
        printf("                        + crumsort_2cores :\n");
        printf (" --merge-step     (-w) [int]    : w-way merge, number of files merged together (default: 8)\n");
        printf (" --MB             (-M) [int]    : maximum memory usage in MBytes (default: 1024)\n");
        printf (" --GB             (-G) [int]    : maximum memory usage in GBytes (default: 1)\n");
        printf ("\n");

        printf ("Others :\n");
        printf (" --verbose        (-v)          : display this help message\n");
        printf (" --help           (-h)          : display this help message\n");
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
    for(size_t i = 0; i < t_files.size(); i += 1 )
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
        printf ("(WW) Reducing the number of files to process (file_limit = %ld)\n", file_limit);
        reset_section();
        while (filelist.size() > file_limit) {
            filelist.pop_back();
        }
    }

    generate_minimizers(
        filelist,
        file_out,
        tmp_dir,
        threads,
        ram_value,
        kmer_size, // k-mer size
        minimizer_size, // minimizer size
        merge_step,
        algo,
        skip_minimizer_step,
        keep_minimizer_files,
        keep_merge_files,
        verbose_flag
    );


    return 0;
}