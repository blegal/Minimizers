#include "merger_level_s.hpp"

void merge_level_s(
        const std::string& i_file,
        const std::string& o_file)
{
    std::cout << "merging(" << i_file << ") with nothing => " << o_file << std::endl;

    const int64_t _iBuff_ = 64 * 1024;
    const int64_t _oBuff_ = _iBuff_;

    uint64_t* ibuff = new uint64_t[_iBuff_];
    uint64_t* obuff = new uint64_t[_oBuff_];

    int64_t nElements = 0; // nombre d'éléments chargés en mémoire
    int64_t ndst      = 0; // nombre de données écrites dans le flux

    FILE* fin = fopen(i_file.c_str(), "r");
    if( fin == NULL )
    {
        printf("(EE) Error opening the input file (%s)\n", i_file.c_str());
        printf("(EE) Error was detected in : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    FILE* fou = fopen(o_file.c_str(), "w");
    if( fou == NULL )
    {
        printf("(EE) Error opening the output file (%s)\n", i_file.c_str());
        printf("(EE) Error was detected in : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    uint64_t color = 0x0000000000000001;
    do{
        ndst      = 0;
        nElements = fread(ibuff, sizeof(uint64_t), _iBuff_, fin);
        for(int i = 0; i < nElements; i += 1){
            obuff[ndst++] = ibuff[i];
            obuff[ndst++] = color;
        }
        fwrite(obuff, sizeof(uint64_t), ndst, fou);
    }while(nElements == _iBuff_);

    fclose( fin );
    fclose( fou );

    delete [] ibuff;
    delete [] obuff;
}
