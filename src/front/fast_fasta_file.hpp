#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>

class fast_fasta_file
{
private:
    const int buff_size = 64 * 1024 * 1024;
    char* buffer;
    int n_data   = 0;
    int c_ptr    = 0;
    int n_lines        = 0;
    bool no_more_load  = false;
    bool file_ended    = false;
    FILE* f;

public:
    fast_fasta_file(const std::string filen)
    {
        printf("(II) Creating fast_fasta_file object\n");

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

    ~fast_fasta_file()
    {
        delete[] buffer;
        fclose( f );
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    bool next_sequence(char* n_kmer)
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
                return next_sequence(n_kmer);
            }else{
                return false;
            }
        }


        int cnt = 0;
        while( buffer[c_ptr] != '\n' )
        {
            n_kmer[cnt++] = buffer[c_ptr++];
        }
        n_kmer[cnt] = 0;
        c_ptr += 1; // on passe le retour à la ligne

        n_lines += 1;

        file_ended = (c_ptr == n_data) && (n_data != buff_size);

        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    bool reload()
    {
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
