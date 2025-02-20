/**
  Copyright (c) 2012-2023 "Bordeaux INP, Bertrand LE GAL"
  bertrand.legal@ims-bordeaux.fr
  [http://legal.vvv.enseirb-matmeca.fr]

  This file is part of a LDPC library for realtime LDPC decoding
  on processor cores.
*/
#include "CFileBuffer.hpp"
#include "../colors.hpp"
#include <sys/stat.h>
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CFileBuffer::CFileBuffer(const std::string name, const int n_colors)
{
    fname     = name;
    in_memory = false;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CFileBuffer::~CFileBuffer()
{

}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int64_t CFileBuffer::bytes_on_drive()
{
    if( is_loaded() == false ){
        //
        // La taille du fichier sur le disque dur
        //
        struct stat file_status;
        if (stat(fname.c_str(), &file_status) < 0) {
            error_section();
            printf("(EE) CFileBuffer::bytes_on_drive() was called whereas the file does not exist\n");
            printf("(EE) The current filename is : %s\n", fname.c_str());
            printf("(EE) Error location          : %s %d\n", __FILE__, __LINE__);
            reset_section();
            exit( EXIT_FAILURE );
        }
        return file_status.st_size;
    }else{
        //
        //
        //
        printf("(EE) CFileBuffer::bytes_on_drive() was called whereas the file is loaded in RAM\n");
        printf("(EE) The current filename is : %s\n", fname.c_str());
        printf("(EE) Error location          : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
        return -1;
    }
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int64_t CFileBuffer::bytes_in_ram()
{
    if( is_loaded() == true ){
        return data.size();
    }else{
        printf("(EE) CFileBuffer::bytes_in_ram() was called whereas the file is loaded in RAM\n");
        printf("(EE) The current filename is : %s\n", fname.c_str());
        printf("(EE) Error location          : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
        return -1;
    }
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool CFileBuffer::is_loaded()
{
    return in_memory;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int64_t CFileBuffer::uint64_on_drive()
{
    return bytes_on_drive() / sizeof (uint64_t);
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int64_t CFileBuffer::uint64_in_ram()
{
    return bytes_in_ram() / sizeof (uint64_t);
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void CFileBuffer::resize_ram_size(const int n_uint64_t)
{
    if( in_memory == false )
    {
        warning_section();
        printf("(WW) Allocating memory  (%s)\n", fname.c_str());
        reset_section();
        in_memory = true; // on a chargé le fichier en mémoire
    }

    data.resize(n_uint64_t);
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void CFileBuffer::load_to_ram()
{
    //
    //
    //
    if( is_loaded() == true ){
        error_section();
        printf("(EE) The file was already loaded in memory (%s)\n", fname.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }
    //
    //
    //
    FILE* stream = fopen( fname.c_str(), "rb" );
    if( stream == NULL )
    {
        error_section();
        printf("(EE) It is impossible to open the file (%s))\n", fname.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    //
    //
    //
    const int nElems = bytes_on_drive();            // on recupère la taille du fichier
    data.resize( nElems / sizeof(uint64_t) );   // on redimentionne l'espace mémoire en RAM

    //
    //
    //
    const int nread  = fread(data.data(), sizeof(uint64_t), nElems, stream);
    fclose( stream );

    const bool is_ok = (nread != nElems);
    if( is_ok == false )
    {
        error_section();
        printf("(EE) Something strange happened during data loading (%s)\n", fname.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }
    in_memory = true; // on a chargé le fichier en mémoire
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void CFileBuffer::write_to_disk()
{
    if( is_loaded() == false ){
        error_section();
        printf("(EE) The file was not loaded in RAM memory (%s)\n", fname.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    //
    //
    //
    FILE* stream = fopen( fname.c_str(), "wb" );
    if( stream == NULL )
    {
        error_section();
        printf("(EE) It is impossible to open the file (%s))\n", fname.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    const int nwrite = fwrite(data.data(), sizeof(uint64_t), data.size(), stream);
    fclose( stream );

    const bool is_ok = (nwrite != data.size());
    if( is_ok == false )
    {
        error_section();
        printf("(EE) Something strange happened during data writing (%s)\n", fname.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    data.resize( 2 ); // on éfface le buffer que nous avions alloué
    data.clear();         // en RAM
    in_memory = false;    // on a déchargé le fichier de la mémoire
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
