#include <cstdio>
#include <fstream>
#include <vector>
#include <omp.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>

uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}

std::vector<std::string> file_list_cpp(std::string path)
{
    std::vector<std::string> paths;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
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

    std::vector<std::string> l_files = file_list_cpp(dir);
    std::vector<std::string> n_files;

    for(int i = 0; i < l_files.size(); i += 1)
    {
        const std::string i_file = l_files[i];
        const uint64_t f_size    = get_file_size(i_file);
        const uint64_t size_mb   = f_size / 1024 / 1024;
        const std::string o_file = "file_n" + std::to_string(i) + ".raw";
        /////

        /////
        const uint64_t o_size    = get_file_size(i_file);
        const uint64_t sizo_mb   = o_size / 1024 / 1024;
        printf("%5d | %20s | %6lld MB | ==========> | %20s | %6lld MB |\n", i, i_file.c_str(), size_mb, o_file.c_str(), sizo_mb);
        /////
        n_files.push_back( o_file );
        /////
    }

    l_files = n_files;
    n_files.clear();

    std::vector<std::string> vrac_names;
    std::vector<std::string> vrac_levels;

    int level = 1;
    while( l_files.size() > 1 )
    {
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");
        int cnt = 0;
        while( l_files.size() >= 2 )
        {
            const std::string i_file_1 = l_files[0]; l_files.erase( l_files.begin() );
            const std::string i_file_2 = l_files[0]; l_files.erase( l_files.begin() );
            const std::string o_file   = "merge_lvl_" + std::to_string(level) + "_n" + std::to_string(cnt) + ".raw";

            n_files.push_back( o_file );
            printf("%5d | %20s | %6lld MB | %20s | %6lld MB | ==========> | %20s | %6lld MB |\n", cnt, i_file_1.c_str(), 0, i_file_2.c_str(), 0, o_file.c_str(), 0);
            cnt += 1;
            sleep( 1 );
        }
        l_files = n_files;
        n_files.clear();
        level += 1;
    }
    printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

    return 0;
}