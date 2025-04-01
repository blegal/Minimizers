#include "../include/BreiZHMinimizer.hpp"


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


std::string shorten(const std::string fname, const int length)
{
    //
    // Removing file path
    //
    auto name_pos       = fname.find_last_of("/");
    std::string nstr   = fname;
    if( name_pos != std::string::npos )
        nstr  = fname.substr(name_pos + 1);
    return nstr;
}


void generate_minimizers(std::vector<std::string> filenames, const std::string &tmp_dir, const int threads, const int ram_value, const int k, const int m, size_t verbose)
{
    CTimer timer_full( true );

    std::vector<std::string> n_files;
    std::string file_out  = tmp_dir + "/result";

    uint64_t in_mbytes = 0;
    uint64_t ou_mbytes = 0;

    printf("(II) Generating minimizers from DNA - %d thread(s)\n", threads);

    CTimer minzr_timer( true );

    // On predimentionne le vecteur de sortie car on connait sa taille. Cela evite les
    // problemes liés à la fonction push_back qui a l'air incertaine avec OpenMP
    n_files.resize( filenames.size() );

    int counter = 0;
    omp_set_num_threads(threads);
#pragma omp parallel for default(shared)
    for(size_t i = 0; i < filenames.size(); i += 1)
    {
        CTimer minimizer_t( true );

        // On mesure la taille des fichiers d'entrée
        const file_stats i_file( filenames[i] );
        const std::string t_file = tmp_dir + "/data_n" + to_number(i, (int)filenames.size()) + ".c0";
        in_mbytes += i_file.size_mb;

        minimizer_processing_v4(i_file.name, t_file, "crumsort", ram_value, true, false, false, k, m);

        // On mesure la taille du fichier de sortie
        const file_stats o_file(t_file);

        ou_mbytes += o_file.size_mb;
        if(verbose > 2)
        {
            // Mesure du temps d'execution
            std::string nname = shorten(i_file.name, 32);
            counter += 1;
            printf("%zu | %5d/%5zu | %32s | %6zu MB | ==========> | %20s | %6zu MB | %5.2f sec.\n", i, counter, filenames.size(), nname.c_str(), i_file.size_mb, o_file.name.c_str(), o_file.size_mb, minimizer_t.get_time_sec());
        }

        n_files[i] = o_file.name; // on stocke le nom du fichier que l'on vient de produire
    }
    filenames = n_files;
    n_files.clear();

    std::sort(filenames.begin(), filenames.end());

    const float elapsed = minzr_timer.get_time_sec();
    printf("(II) - Information loaded from files : %6d MB\n", (int)(in_mbytes));
    printf("(II)   Information wrote to files    : %6d MB\n", (int)(ou_mbytes));
    printf("(II) - Information throughput (in)   : %6d MB/s\n", (int)((float)(in_mbytes) / elapsed));
    printf("(II)   Information throughput (out)  : %6d MB/s\n", (int)((float)(ou_mbytes) / elapsed));
    printf("(II) - Execution time : %1.2f seconds\n", elapsed);
    printf("(II)\n");
    

    //
    std::vector<std::string> vrac_names;
    std::vector<int>         vrac_levels;
    std::vector<int>         vrac_real_color;
    //

    printf("(II) Tree-based merging of sorted minimizer files - %d thread(s)\n", threads);
    const auto start_merge = std::chrono::steady_clock::now();
    int colors = 1;
    omp_set_num_threads(threads); // on regle le niveau de parallelisme accessible dans cette partie
    while( filenames.size() > 1 )
    {
        if( verbose > 2 )
            printf("(II)   - Merging %4zu files with %4d color(s)\n", filenames.size(), colors);
        else{
            printf("(II)   - Merging %4zu files | %4d color(s) | ", filenames.size(), colors);
            fflush(stdout);
        }
        const auto start_merge = std::chrono::steady_clock::now();

        if(verbose > 2)
            printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

        n_files.resize( filenames.size() / 2 ); // pur éviter le push_back qui semble OpenMP unsafe !

        uint64_t in_mbytes = 0; // pour calculer les debits
        uint64_t ou_mbytes = 0; // pour calculer les debits

        int cnt = 0;
#pragma omp parallel for
        for(size_t ll = 0; ll < filenames.size() - 1; ll += 2)
        {
            const auto start_file = std::chrono::steady_clock::now();
            const file_stats i_file_1( filenames[ll    ] );
            const file_stats i_file_2( filenames[ll + 1] );
            const std::string t_file   = tmp_dir + "/data_n" + to_number(ll/2, filenames.size()/2) + "." + std::to_string(2 * colors) + "c";

            in_mbytes += i_file_1.size_mb;
            in_mbytes += i_file_2.size_mb;

            merger_in( i_file_1.name, i_file_2.name, t_file, colors, colors);    // couleurs homogenes

            std::remove( i_file_1.name.c_str() ); // delete file
            std::remove( i_file_2.name.c_str() ); // delete file
            
            const file_stats o_file( t_file );
            ou_mbytes += o_file.size_mb;

            n_files[ll/2] = o_file.name;

            if(verbose > 2){
                printf("%6d |", cnt);
                i_file_1.printf_size();
                printf("   +   ");
                i_file_2.printf_size();
                printf("   == MERGE =>   ");
                o_file.printf_size();
                const auto  end_file = std::chrono::steady_clock::now();
                const float elapsed_file = std::chrono::duration_cast<std::chrono::milliseconds>(end_file - start_file).count() / 1000.f;
                printf("in  %6.2fs\n", elapsed_file);
            }

            cnt += 1; // on compte le nombre de données traitées
        }

        const auto  end_merge = std::chrono::steady_clock::now();
        const float elapsed_file = (float)std::chrono::duration_cast<std::chrono::milliseconds>(end_merge - start_merge).count() / 1000.f;
        if( verbose > 0 ){
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
        // On trie les fichiers par ordre alphabetique car sinon pour reussir a faire le lien entre couleur et
        // nom du fichier d'entrée est une véritable galère surtout lorsque l'on applique une parallelisation
        // OpenMP !!! On n'a pas besoin de trier les couleurs (int) associés car ils ont tous la meme valeur.
        //
        std::sort(n_files.begin(), n_files.end());

        //
        // On regarde si des fichiers n'ont pas été traités. Cela peut arriver lorsque l'arbre de fusion
        // n'est pas équilibré. On stocke le fichier
        //
        if( filenames.size()%2 )
        {
            vrac_names.push_back ( filenames[filenames.size()-1] );
            vrac_levels.push_back( colors                    );
            warning_section();
            printf("(II)     > Keeping (%s) file for later processing...\n", vrac_names.back().c_str());
            reset_section();
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

        filenames = n_files;
        n_files.clear();
        colors *= 2;
    }
    const auto  end_merge = std::chrono::steady_clock::now();
    const float elapsed_merge = std::chrono::duration_cast<std::chrono::milliseconds>(end_merge - start_merge).count() / 1000.f;
    printf("(II) - Execution time : %1.2f seconds\n", elapsed_merge);
    printf("(II)\n");

    if( verbose > 2 )
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

    // Utile pour les fusions partielles et le renomage du binaire à la fin
    vrac_real_color = vrac_levels;

    if( verbose > 2 )
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");
    //
    if( vrac_names.size() > 1 )
    {
        printf("(II) Comb-based merging of sorted minimizer files\n");
        CTimer timer_merge( true );

        int cnt = 0;
        // A t'on un cas particulier a gerer (fichier avec 0 couleur)
        if(vrac_levels[0] == 1)
        {
            const std::string i_file = vrac_names[0];
            const std::string o_file   = tmp_dir + "/data_n" + std::to_string(cnt++) + "." + std::to_string(2) + "c";
            printf("- No color file processing (%s)\n", i_file.c_str());
            merger_in(i_file,i_file, o_file, 0, 0);
            vrac_names     [0] = o_file;
            vrac_levels    [0] =      2; // les niveaux de fusion auxquels on
            vrac_real_color[0] =      1; // on a une seule couleur
            printf("- %20.20s (%d) + %20.20s (%d)\n", i_file.c_str(), 0, i_file.c_str(), 0);

            std::remove(i_file.c_str()); // delete file
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
            int merge_color =   color_1   + color_2;

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

            // On lance le processus de merging sur les 2 fichiers
            const std::string o_file   = tmp_dir + "/data_n" + std::to_string(cnt++) + "." + std::to_string(real_color) + "c";
            printf("  %20.20s (%d) + %20.20s (%d) ===> %20.20s (%d)\n", i_file_1.c_str(), color_1, i_file_2.c_str(), color_2, o_file.c_str(), real_color);
            merger_in(i_file_1,i_file_2, o_file, color_1, color_2);
            vrac_names     [1] = o_file;
            vrac_levels    [1] = merge_color;
            vrac_real_color[1] = real_color;

            // On supprime le premier élément du tableau
            vrac_names.erase     ( vrac_names.begin()      );
            vrac_levels.erase    ( vrac_levels.begin()     );
            vrac_real_color.erase( vrac_real_color.begin() );

            // On supprime les fichiers source
            std::remove( i_file_1.c_str() ); // delete file
            std::remove( i_file_2.c_str() ); // delete file
        }

        printf("(II) - Execution time : %1.2f seconds\n", timer_merge.get_time_sec());
        printf("(II)\n");
    }

    //
    if( vrac_names.size() == 1 )
    {
        const std::string file   = vrac_names[0]; // le plus grand est tjs le second
        const int real_color     = vrac_real_color[0];
        const std::string o_file = file_out + "." + std::to_string(real_color) + "c";
        std::rename(vrac_names[0].c_str(), o_file.c_str());
        printf("(II) Renaming final file : %s\n", o_file.c_str());
    }else{
        printf("(EE) Something strange happened !!!\n");
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    printf("(II)\n");
    printf("(II) - Total execution time : %1.2f seconds\n", timer_full.get_time_sec());
}