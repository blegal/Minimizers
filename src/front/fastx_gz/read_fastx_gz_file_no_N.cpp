#include "read_fastx_gz_file_no_N.hpp"
#include "../../tools/colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
read_fastx_gz_file_no_N::read_fastx_gz_file_no_N(const std::string filen)
{
    buffer = new char[buff_size];
    stream = fopen( filen.c_str(), "r" );

    if( stream == NULL )
    {
        error_section();
        printf("(EE) File does not exist (%s))\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    streaz = gzdopen(fileno(stream), "r");
    if( streaz == NULL ) {
        error_section();
        printf("(EE) An error happens during gzdopen\n");
        reset_section();
        exit(EXIT_FAILURE);
    }

    n_data = gzread(streaz, buffer, buff_size * sizeof(char));

    if( n_data == Z_STREAM_END ) {
        error_section();
        printf("(EE) Z_STREAM_END\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( n_data == Z_STREAM_ERROR ) {
        error_section();
        printf("(EE) Z_STREAM_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( n_data == Z_DATA_ERROR ) {
        error_section();
        printf("(DD) Z_DATA_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( n_data == Z_NEED_DICT ) {
        error_section();
        printf("(DD) Z_NEED_DICT\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( n_data == Z_MEM_ERROR ) {
        error_section();
        printf("(DD) Z_MEM_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }

    //
    // Les fichiers fasta sont normalement équipé d'une en-tete que l'on peut directement
    // skipper...
    //

    no_more_load = ( n_data != buff_size );

    if ((buffer[c_ptr] == '>') || (buffer[c_ptr] == '@'))
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
read_fastx_gz_file_no_N::~read_fastx_gz_file_no_N()
{
    delete[] buffer;
    gzclose(streaz);
    fclose ( stream );
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool read_fastx_gz_file_no_N::next_sequence(char* n_kmer)
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
std::tuple<int, bool> read_fastx_gz_file_no_N::next_sequence(char* n_kmer, int buffer_size, const bool _internal_)
{
    if( file_ended == true || is_eof())
    {
        return {0, true}; // aucun octet n'est disponible
    }

    if (buffer[c_ptr] == '\n') c_ptr += 1; //case buffer ended on newline (would else fall into non-DNA case)
    

    //last time left on header, find next sequence and recurse over it
    if ((buffer[c_ptr] == '>') || (buffer[c_ptr] == '@') || (buffer[c_ptr] == '+'))
    {
        //start looking for newline in remaining buffer
        for(int i = c_ptr; i < n_data; i += 1) {
            if (buffer[i] == '\n') {
                c_ptr      = i + 1;

                if (buffer[c_ptr] == '+'){ //skip fastq quality line
                    c_ptr += n_qualities;
                    if (c_ptr >= n_data && !reload()){
                        return {0, _internal_};
                        //nothing after quality line
                    }
                } 

                return next_sequence(n_kmer, buffer_size, true);
            }
        }

        //didnt find it yet ? reload buffer and start looking again
        reload();
        for(int i = c_ptr; i < n_data; i += 1) {
            if (buffer[i] == '\n') {
                c_ptr      = i + 1;

                if (buffer[c_ptr] == '+'){ //skip fastq quality line
                    c_ptr += n_qualities;
                    if (c_ptr >= n_data && !reload()){
                        return {0, _internal_};
                        //nothing after quality line
                    }
                } 

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
            n_qualities += 1; //'N' have qualities aswell
            return next_sequence(n_kmer, buffer_size, true);
    }

    //classic loop, add nucleotides to seq until header or 'N'
    int cnt = 0;
    while (cnt < buffer_size){
        if (c_ptr >= n_data){ //check buffer empty ?
            file_ended = !reload();
            if (file_ended) return {cnt, _internal_};
        }
        
        //verify correct char
        if (buffer[c_ptr] == 'A' || buffer[c_ptr] == 'a' || 
            buffer[c_ptr] == 'C' || buffer[c_ptr] == 'c' || 
            buffer[c_ptr] == 'G' || buffer[c_ptr] == 'g' || 
            buffer[c_ptr] == 'T' || buffer[c_ptr] == 't' || 
            buffer[c_ptr] == 'U' || buffer[c_ptr] == 'u'){
            n_kmer[cnt++] = buffer[c_ptr++];
            n_qualities += 1;
        }

        //ignore newlines
        else if (buffer[c_ptr] == '\n'){
            c_ptr += 1;
            continue;
        }

        //found a header 
        else if (buffer[c_ptr] == '>' || buffer[c_ptr] == '+' || buffer[c_ptr] == '@' || buffer[c_ptr] == '='){
            return {cnt, _internal_};

        //found an intruder, or eof
        } else {
            return {cnt, _internal_};
        }
        
    }
    return {cnt, _internal_};
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool read_fastx_gz_file_no_N::reload()
{
    c_ptr        -= n_data; //skip reste of buffer in case of jump
    n_data = gzread(streaz, buffer,  buff_size * sizeof(char));
    if (n_data == 0) return false;
    no_more_load = (n_data != buff_size);
    
    return true;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool read_fastx_gz_file_no_N::is_eof()
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
