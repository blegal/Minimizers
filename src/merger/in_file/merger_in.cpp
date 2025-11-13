#include "merger_in.hpp"

#include "merger_level_0.hpp"
#include "merger_level_1.hpp"
#include "merger_level_2_32.hpp"
#include "merger_level_64_n.hpp"
#include "merger_level_2_0.hpp"
#include "merger_level_n_p.hpp"
#include "merger_level_n_p_final.hpp"
#include "merger_level_s.hpp"   // -s- for special case !

void merger_in(
        const std::string& ifile_1, //+grand
        const std::string& ifile_2, //+petit
        const std::string& o_file,
        const int level_1,
        const int level_2)

{
    if( level_1 == level_2 )
    {
        //
        // Cas SPECIAL pour me simplifier la vie lors du merge. On ajoute
        // au fichier d'entrée une couleur
        //
        const int equals = !ifile_1.compare(ifile_2);
        if( (level_1 == 0) && (equals) )
            merge_level_s(ifile_1, o_file); // 0 couleur ne fois devient une couleur
        //
        // On fait un merge sans ajouter de couleur dans le fichier resultat
        //
        else if( level_1 == 0 )
            merge_level_0(ifile_1, ifile_2, o_file); // 0 couleur reste 0 couleur
        //
        // On fait un merge de 2 fichiers incolore et on ajoute 2 bits de couleurs dans un uint64_t
        //
        else if( level_1 ==  1 )
            merge_level_1(ifile_1, ifile_2, o_file); // 1 + 1 => 2
        //
        // Les 2 fichiers d'entrée ont 2 couleurs => 4 couleurs en sortie
        //
        else if( level_1 ==  2 )
            merge_level_2_32(ifile_1, ifile_2, o_file, 2);
        else if( level_1 ==  4 )
            merge_level_2_32(ifile_1, ifile_2, o_file, 4);
        else if( level_1 ==  8 )
            merge_level_2_32(ifile_1, ifile_2, o_file, 8);
        else if( level_1 == 16 )
            merge_level_2_32(ifile_1, ifile_2, o_file, 16);
        else if( level_1 == 32 )
            merge_level_2_32(ifile_1, ifile_2, o_file, 32);
        else if( level_1 >= 64 )
            merge_level_64_n(ifile_1, ifile_2, o_file, level_1);
        else{
            printf("%s %d\n", __FILE__, __LINE__);
            printf("NOT supported yet (%d)\n", level_1);
            exit( EXIT_FAILURE );
        }
    }else{
        //
        // On a un fichier qui a des couleurs (MOINS DE 64 => 1~32) et un fichier sans couleur
        //
        if( (level_1 == 2) && (level_2 == 0) )
            merge_level_2_0(ifile_1, ifile_2, o_file, level_1);
        //
        // On a un fichier qui a des couleurs (PLUS DE 64 => 64~+inf.) et un fichier sans couleur
        //
        else
            merge_level_n_p(ifile_1, ifile_2, o_file, level_1, level_2);
    }
}

void merger_in(
        const std::string& ifile_1, //+grand
        const std::string& ifile_2, //+petit
        const std::string& o_file,
        const std::string& o_file_sparse,
        const int level_1,
        const int level_2)

{
    merge_level_n_p_final(ifile_1, ifile_2, o_file, o_file_sparse, level_1, level_2);
}
