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
    if (argc < 2) {
        return EXIT_FAILURE;
    }

    std::string dir = argv[1];

    //
    // On recuperer la liste des fichiers que l'on doit traiter. Ce nombre de fichiers doit
    // être multiple de 2 pour que la premiere étape de fusion soit faite pour tous les fichier
    // (ajout de la donnée uint64_t pour la couleur). Ce n'est pas un probleme scientifique mais
    // technique, un peu plus de code a developper plus tard...
    //
    std::vector<std::string> l_files = file_list_cpp(dir);
#if 0
    if( l_files.size()%2 == 1 )
    {
        printf("(EE) Le nombre de fichiers a traiter est impair, cela n'est pour le moment pas supporté !\n");
        exit(EXIT_FAILURE);
    }
#endif

    std::vector<std::string> n_files;

    //
    //
    //
#pragma omp parallel for
    for(int i = 0; i < l_files.size(); i += 1)
    {
        const std::string i_file = l_files[i];
        const uint64_t f_size    = get_file_size(i_file);
        const uint64_t size_mb   = f_size / 1024 / 1024;
        const std::string o_file = "file_n" + std::to_string(i) + ".raw";
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

    //
    //
    //
    std::vector<std::string> vrac_names;
    std::vector<int>         vrac_levels;

    //
    //
    //
    int level = 1;
    while( l_files.size() > 1 )
    {
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");
        int cnt = 0;
        while( l_files.size() >= 2 )
        {
            const std::string i_file_1 = l_files[0]; l_files.erase( l_files.begin() );
            const std::string i_file_2 = l_files[0]; l_files.erase( l_files.begin() );
            const std::string o_file   = "lvl_" + std::to_string(level) + "_n" + std::to_string(cnt) + ".raw";

            const uint64_t i1_size   = get_file_size(i_file_1);
            const uint64_t siz1_mb   = i1_size / 1024 / 1024;

            const uint64_t i2_size   = get_file_size(i_file_2);
            const uint64_t siz2_mb   = i2_size / 1024 / 1024;

//          printf("| %20s | + | %20s | %d |\n", i_file_1.c_str(), i_file_2.c_str(), 1 << (level-1));

            merger_in(i_file_1, i_file_2, o_file, 1 << (level-1));

            std::remove( i_file_1.c_str() ); // delete file
            std::remove( i_file_2.c_str() ); // delete file

            const uint64_t o_size    = get_file_size(o_file);
            const uint64_t sizo_mb   = o_size / 1024 / 1024;

            n_files.push_back( o_file );
            printf("%5d | %20s | %6lld MB | %20s | %6lld MB | ==========> | %20s | %6lld MB |\n", cnt, i_file_1.c_str(), siz1_mb, i_file_2.c_str(), i2_size, o_file.c_str(), sizo_mb);
            cnt += 1;
            usleep( 1000 );
        }

        //
        // On regarde si des fichiers n'ont pas été traités. Cela peut arriver lorsque l'arbre de fusion
        // n'est pas équilibré. On stocke le fichier
        //
        if( l_files.size() != 0 )
        {
            vrac_names.push_back ( l_files.front() );
            vrac_levels.push_back( level           );
        }

        l_files = n_files;
        n_files.clear();
        level += 1;
        if( level == 7 ) break;
    }
    printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

    for(int i = 0; i < vrac_names.size(); i += 1)
    {
        printf("%5d | %20s | level = %6d ||\n", i, vrac_names[i].c_str(), vrac_levels[i]);
    }

    printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

    //
    // Il faudrait que l'on gere les fichiers que l'on a mis de coté lors du processus de fusion...
    //

    return 0;
}