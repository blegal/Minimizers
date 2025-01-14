#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>

class fast_kmer_file
{
private:
    const int kmer_size;
    const int buff_size = 64 * 1024 * 1024;
    char* buffer;
    int n_data   = 0;
    int c_ptr    = 0;
    int n_lines        = 0;
    bool no_more_load  = false;
    bool file_ended    = false;
    FILE* f;

public:
    fast_kmer_file(const std::string filen, const int km_size) : kmer_size(km_size)
    {
        printf("(II) Creating fast_kmer_file object\n");

        buffer = new char[buff_size];

        f = fopen( filen.c_str(), "r" );
        if( f == NULL )
        {
            printf("(EE) File does not exist !\n");
            exit( EXIT_FAILURE );
        }
        n_data = fread(buffer, sizeof(char), buff_size, f);
        printf("(II) Creation done\n");
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    ~fast_kmer_file()
    {
        delete[] buffer;
        fclose( f );
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    bool next_kmer(char* n_kmer, char* n_value)
    {
        if( file_ended == true )
        {
            return false;
        }

        //
        // On regarde si l'on a encore une entrée dans le buffer
        //
        int pos_nline = -1;
        for(int i = c_ptr; i < n_data; i += 1)
        {
            if(buffer[i] == '\n')
            {
                pos_nline = i;
                break;
            }
        }

        if( pos_nline == -1 )
        {
            if( is_eof() == true )
                return false;

            if( reload() == true )
            {
                return next_kmer(n_kmer, n_value);
            }else{
                return false;
            }
        }

        //
        // On peut recopier le k-mer dans la structure
        //

        for(int i = 0; i < kmer_size; i += 1)
        {
            n_kmer[i] = buffer[c_ptr + i];
        }
        c_ptr += kmer_size;
        n_kmer[kmer_size] = 0;

        //
        // On verifie que l'on a bien un espace / tabulation
        //

        c_ptr += 1;

        //
        // On recopie la valeur numérique de l'abondance
        //

        int cnt = 0;
        while( buffer[c_ptr] != '\n' )
        {
            n_value[cnt++] = buffer[c_ptr++];
        }
        n_value[cnt] = '\0';
        c_ptr += 1;

        n_lines += 1;

        file_ended = (c_ptr == n_data) && (n_data != buff_size);

        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    bool reload()
    {
//        printf("[%6d] START OF DATA RELOADING !\n", n_lines);
        int reste = n_data - c_ptr;
        for(int i = c_ptr; i < n_data; i += 1)
        {
            buffer[i - c_ptr] = buffer[i];
        }
        int nread = fread(buffer + reste, sizeof(char), buff_size - reste, f);
//        printf("[%6d] ASKED        : %d\n", n_lines, buff_size - reste);
//        printf("[%6d] READ         : %d\n", n_lines, nread);
        no_more_load = ( n_data != buff_size ); // a t'on atteint la fin du fichier ?
//        printf("[%6d] no_more_load : %d\n", n_lines, no_more_load);
        c_ptr        = 0;                       // on remet a zero le pointeur de lecture
        n_data       = nread + reste;           // on met a jour le nombre de données dans le buffer
//        printf("[%6d] n_data       : %d\n", n_lines, n_data);
//        printf("[%6d] END OF DATA RELOADING !\n", n_lines);
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    bool is_eof()
    {
        if( (no_more_load == true) && (c_ptr == n_data) )
            return true;
        return false;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
};
