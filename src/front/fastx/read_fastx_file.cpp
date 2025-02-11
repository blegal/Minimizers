#include "read_fastx_file.hpp"
#include "../../tools/colors.hpp"
#include <cstdint>
#include <array>
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
read_fastx_file::read_fastx_file(const std::string filen)
{
    buffer = new char[buff_size];

    f = fopen( filen.c_str(), "r" );
    if( f == NULL )
    {
        error_section();
        printf("(EE) File does not exist (%s))\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }
    n_data = fread(buffer, sizeof(char), buff_size, f);

    no_more_load = ( n_data != buff_size ); //TODO: smaller file than buffer

    //
    // Les fichiers fasta sont normalement équipé d'une en-tete que l'on peut directement
    // skipper...
    //
    if( (buffer[c_ptr] == '>') || (buffer[c_ptr] == '+') || (buffer[c_ptr] == '@') || (buffer[c_ptr] == '>') )
    {
        for(int i = c_ptr; i < n_data; i += 1) {
            if (buffer[i] == '\n')
            {
                c_ptr = i + 1; // on se positionne sur le 1er caractere de la prochaine ligne
                break;
            }
        }
    }
    // Fin de skip du premier commentaire

}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
read_fastx_file::~read_fastx_file()
{
    delete[] buffer;
    fclose( f );
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool read_fastx_file::next_sequence(char* n_kmer)
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
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
std::tuple<int, bool> read_fastx_file::next_sequence(char* n_kmer, int buffer_size, const bool _internal_)
{
    
    if( file_ended == true || is_eof())
    {
        return {0, true}; // aucun octet n'est disponible
    }

    if (buffer[c_ptr] == '\n') c_ptr += 1; //case buffer ended on newline (would else fall into non-DNA case)
    

    //last time left on header, find next sequence and recurse over it
    if (buffer[c_ptr] == '>' || buffer[c_ptr] == '+' || buffer[c_ptr] == '@' || buffer[c_ptr] == '='){
        for(int i = c_ptr; i < n_data; i += 1) {
            if (buffer[i] == '\n') {
                c_ptr      = i + 1;
                return next_sequence(n_kmer, buffer_size, true);
            }
        }
        reload(); //buffer ended inside header
        for(int i = c_ptr; i < n_data; i += 1) {
            if (buffer[i] == '\n') {
                c_ptr      = i + 1;
                return next_sequence(n_kmer, buffer_size, true);
            }
        }
    }

    //last time left on N, go to next char and recurse over it
    if (!(buffer[c_ptr] == 'A') && !(buffer[c_ptr] == 'a') && 
        !(buffer[c_ptr] == 'C') && !(buffer[c_ptr] == 'c') && 
        !(buffer[c_ptr] == 'G') && !(buffer[c_ptr] == 'g') && 
        !(buffer[c_ptr] == 'T') && !(buffer[c_ptr] == 't') && 
        !(buffer[c_ptr] == 'U') && !(buffer[c_ptr] == 'u')){
            c_ptr      += 1; //TODO: look for first non intrus
            return next_sequence(n_kmer, buffer_size, true);
    }


    int cnt = 0;                                        // On recoipie les données que l'on souhaite
    while( buffer[c_ptr] != '\n' )                      // transmettre a la fonction appelante
    {
        n_kmer[cnt++] = buffer[c_ptr++];
    }
    n_kmer[cnt] = 0;                                    // On rajoute un caractere fin de string
    c_ptr      += 1;                                    // pour des raisons de compatibilité (strlen)

    n_lines    += 1;

    //
    // on detecte le moment ou l'on arrive a la fin du fichier
    //
    file_ended = (c_ptr == n_data) && (n_data != buff_size);

    //
    // On indique que la ligne actuelle est la continuité de la séquence précédente
    //
    return {cnt, _internal_};
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

bool read_fastx_file::reload(){
    n_data = fread(buffer, sizeof(char), buff_size, f);
    if (n_data == 0) return false;
    no_more_load = (n_data != buff_size);
    c_ptr        = 0; 
    return true;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool read_fastx_file::is_eof()
{
    if( (no_more_load == true) && (c_ptr >= n_data) )
        return true;
    return false;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
