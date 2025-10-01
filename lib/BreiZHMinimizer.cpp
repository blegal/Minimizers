#include "BreiZHMinimizer.hpp"

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
    uint64_t name_pos       = fname.find_last_of("/");
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

void generate_minimizers(
    std::vector<std::string> filenames, 
    const std::string &output,
    const std::string &tmp_dir, 
    const int threads, 
    const uint64_t ram_value_MB, 
    const int k, 
    const int m, 
    const uint64_t merge_step,
    const std::string &algo, 
    size_t verbose,
    bool skip_minimizer_step, 
    bool keep_minimizer_files, 
    bool keep_merge_files)
{
    CTimer timer_full( true );


    //
    //
    //
    std::vector<CMergeFile> n_files;
    std::vector<CMergeFile> l_files;
    if( skip_minimizer_step == false )
    {
        uint64_t in_mbytes = 0;
        uint64_t ou_mbytes = 0;

        printf("(II) Generating minimizers from DNA - %d thread(s)\n", threads);

        CTimer minzr_timer( true );

        //
        // On predimentionne le vecteur de sortie car on connait sa taille. Cela evite les
        // problemes liés à la fonction push_back qui a l'air incertaine avec OpenMP
        //
        n_files.resize( filenames.size() );

        int counter = 0;
        omp_set_num_threads(threads);
#pragma omp parallel for default(shared)
        for(size_t i = 0; i < filenames.size(); i += 1)
        {
            CTimer minimizer_t( true );

            //
            // On mesure la taille des fichiers d'entrée
            //
            const file_stats i_file( filenames[i] );
            const std::string t_file = "data_n" + to_number(i, (int)filenames.size()) + ".raw";
            in_mbytes += i_file.size_mb;

            /////
            minimizer_processing_v4(i_file.name, t_file, algo, ram_value_MB, true, false, false, k, m);
            /////

            //
            // On mesure la taille du fichier de sortie
            //
            const file_stats o_file(t_file);
            ou_mbytes += o_file.size_mb;
            if(verbose == true )
            {
                //
                // Mesure du temps d'execution
                //
                std::string nname = shorten(i_file.name, 32);
                counter += 1;
                printf("%5ld | %5d/%5ld | %32s | %6ld MB | ==========> | %20s | %6ld MB | %5.2f sec.\n", i, counter, filenames.size(), nname.c_str(), i_file.size_mb, o_file.name.c_str(), o_file.size_mb, minimizer_t.get_time_sec());

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
    printf("(II) Tree-based %d-ways merging of sorted minimizer files - %d thread(s)\n", 64, threads);
    if(verbose == true )
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");
    n_files.resize( (l_files.size() + 63) / 64 ); // pur éviter le push_back qui semble OpenMP unsafe !
    omp_set_num_threads(threads); // on regle le niveau de parallelisme accessible dans cette partie
    int cnt = 0;
#pragma omp parallel for
    for(size_t ll = 0; ll < l_files.size(); ll += 64)
    {
        const auto start_file = std::chrono::steady_clock::now();
        const int64_t max_files = (l_files.size() - ll) < 64 ? (l_files.size() - ll) : 64;
        std::vector<std::string> liste;
        for(int ff = 0; ff < max_files; ff += 1)        // in this first stage all the file are not colored
            liste.push_back( l_files[ll + ff].name );   // at the input

        std::string t_file = "data_n" + to_number(ll/64, l_files.size()/64) + ".";
        t_file            += std::to_string(max_files) + "c";

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
        if(verbose == true ){
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
    if(verbose == true )
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

    printf("(II) Tree-based %ld-ways merging of first stage files - %d thread(s)\n", merge_step, threads);
    const auto start_merge = std::chrono::steady_clock::now();
    int colors = 64;
    omp_set_num_threads(threads); // on regle le niveau de parallelisme accessible dans cette partie
    while( l_files.size() > 1 )
    {
        if( verbose )
            printf("(II)   - %ld-ways merging %4zu files with %4d color(s)\n", merge_step, l_files.size(), colors);
        else{
            printf("(II)   - %ld-ways merging %4zu files | %4d color(s) | ",   merge_step, l_files.size(), colors);
            fflush(stdout);
        }
        const auto start_merge = std::chrono::steady_clock::now();

        if(verbose == true )
            printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");


        n_files.resize( (l_files.size() + merge_step - 1) / merge_step ); // pur éviter le push_back qui semble OpenMP unsafe !

        uint64_t in_mbytes = 0; // pour calculer les debits
        uint64_t ou_mbytes = 0; // pour calculer les debits


        int cnt = 0;
#pragma omp parallel for
        for(size_t ll = 0; ll < l_files.size(); ll += merge_step) // On merge par 8, ce choix est discutable
        {
            //
            // Gathering statistic informations about the merging process
            //
            const auto start_file = std::chrono::steady_clock::now();
            const int64_t max_files = (l_files.size() - ll) < merge_step ? (l_files.size() - ll) : merge_step;

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
            t_file += to_number(ll/merge_step, l_files.size()/merge_step) + ".";
            t_file += std::to_string(final_real_color) + "c";

            //
            // Creation of the filenames
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
            n_files[ll/merge_step] = cm_file;

            //
            // Information reporting for the user
            //
            if(verbose == true ){
                printf("%6d | %s .... ", cnt, l_files[ll            ].name.c_str());
                printf("%s ",                 l_files[ll+max_files-1].name.c_str());
                printf("   == %ld x MERGE =>   ", merge_step);
                o_file.printf_size();
                const auto  end_file = std::chrono::steady_clock::now();
                const float elapsed_file = std::chrono::duration_cast<std::chrono::milliseconds>(end_file - start_file).count() / 1000.f;
                printf("in  %6.2fs\n", elapsed_file);
            }

            cnt += 1; // on compte le nombre de données traitées
        }
        const auto  end_merge = std::chrono::steady_clock::now();

        const float elapsed_file = (float)std::chrono::duration_cast<std::chrono::milliseconds>(end_merge - start_merge).count() / 1000.f;
        if( verbose ){
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
        colors *= merge_step;
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


    if( verbose )
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");


    for(size_t i = 0; i < vrac_names.size(); i += 1)
    {
        const file_stats t_file( vrac_names[i].name   );
        t_file.printf_size();
        printf("(II) Remaining file : %5ld | %20s | num_colors = %6ld | real_colors = %6ld | size = %6ld MB\n", i, vrac_names[i].name.c_str(),  vrac_names[i].numb_colors,  vrac_names[i].real_colors, t_file.size_mb);
    }


    if( verbose )
        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");


    //
    //
    //
    if( vrac_names.size() > 1 )
    {
        printf("(II) Comb-based 2-ways merging of sorted minimizer files\n");
        CTimer timer_merge( true );

        int cnt = 0;

        printf("------+----------------------+-----------+----------------------+-----------+-------------+----------------------+-----------+\n");

        while( vrac_names.size() != 1 )
        {
            const auto  start_file = std::chrono::steady_clock::now();

            const CMergeFile i_file_1 = vrac_names[1]; // le plus grand est tjs le second
            const CMergeFile i_file_2 = vrac_names[0]; // la plus petite couleur est le premier
                  CMergeFile o_file  ( "", i_file_1, i_file_2 ); // la plus petite couleur est le premier

            o_file.name = "data_n" + std::to_string(cnt++) + "." + std::to_string( o_file.real_colors ) + "c";

//            printf("- %20.20s (%lld) + %20.20s (%lld)\n", i_file_1.name.c_str(), i_file_1.real_colors, i_file_2.name.c_str(), i_file_2.real_colors);

            //
            // On lance le processus de merging sur les 2 fichiers
            //
//            printf("  %20.20s (%lld) + %20.20s (%lld) ===> %20.20s (%lld)\n",
//                   i_file_1.name.c_str(), i_file_1.real_colors,
//                   i_file_2.name.c_str(), i_file_2.real_colors,
//                   o_file.name.c_str(),   o_file.real_colors);

            merger_in(
                    i_file_1.name,
                    i_file_2.name,
                    o_file.name,
                    i_file_1.numb_colors,
                    i_file_2.numb_colors
            );
            vrac_names[1] = o_file;

            //
            // Information reporting for the user
            //
            if(verbose == true ){
                printf("%6d | %s and %s   == 2-way x MERGE =>   ", cnt, i_file_1.name.c_str(), i_file_2.name.c_str());
                const file_stats t_file( o_file.name   );
                t_file.printf_size();
                const auto  end_file = std::chrono::steady_clock::now();
                const float elapsed_file = std::chrono::duration_cast<std::chrono::milliseconds>(end_file - start_file).count() / 1000.f;
                printf("in  %6.2fs\n", elapsed_file);
            }

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
        const std::string o_file = output + "." + std::to_string(lastfile.real_colors) + "c";

        //
        // Pour l'instant: minimizers dédupliqués et colors en désordre, objectif trier par couleur 
        // pour les dedupliquer par la suite
        //

        printf("(II) Sorting final file : %s\n", o_file.c_str());
        CTimer timer_color_sort( true );

        external_sort(
            lastfile.name,
            o_file,
            tmp_dir,
            ram_value_MB,
            false,
            true
        );

        
        printf("(II) - Execution time : %1.2f seconds\n", timer_color_sort.get_time_sec());
    }else{
        printf("(EE) Something strange happened !!!\n");
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    

    


    printf("(II)\n");
    printf("(II) - Total execution time : %1.2f seconds\n", timer_full.get_time_sec());
}