/**
  Copyright (c) 2012-2023 "Bordeaux INP, Bertrand LE GAL"
  bertrand.legal@ims-bordeaux.fr
  [http://legal.vvv.enseirb-matmeca.fr]

  This file is part of a LDPC library for realtime LDPC decoding
  on processor cores.
*/

#ifndef _CFileBuffer_
#define _CFileBuffer_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

class CFileBuffer
{
public:
    std::string fname;          // le nom du fichier existant sur le disque ou viruel si il est en mémoire
    bool in_memory = false;     // le fichier est il en mémoire ?
    std::vector<uint64_t> data; // la structure de données qui contient le fichier si il est en mémoire

    //
    // On construit un fichier à partir de son nom et du nombre de couleurs que le document
    // contient ou va contenir
    //
     CFileBuffer(const std::string name, const int colors);
    ~CFileBuffer();

    int64_t bytes_on_drive  ();                     // taille du fichier en bytes
    int64_t bytes_in_ram    ();                     // nombre d'éléments (minimizer + couleurs)
    int64_t uint64_on_drive ();                     // taille du fichier en bytes
    int64_t uint64_in_ram   ();                     // nombre d'éléments (minimizer + couleurs)
    bool    is_loaded       ();                     // est'il en mémoire ou sur le disque
    void    resize_ram_size (const int n_uint64_t); // redimentionne la taille de la zone méméoire allouée
    void    load_to_ram     ();                     // charge les données et supprime le fichier du disque
    void    write_to_disk   ();                     // crée le fichier sur le disque et supprime les données de la RAM
};

#endif
