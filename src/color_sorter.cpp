#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <omp.h>
#include <sstream>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>

uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
#if defined(__SSE4_2__)
#include <immintrin.h>
inline int popcount_u64_x86(const uint64_t _val)
{
    uint64_t val = _val;
	asm("popcnt %[val], %[val]" : [val] "+r" (val) : : "cc");
	return val;
}
#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
inline uint64_t popcount_u64_arm(const uint64_t val)
{
    return vaddlv_u8(vcnt_u8(vcreate_u8((uint64_t) val)));
}
#endif
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
template<int N>
struct element {
    uint64_t minizr;    // le minimizer
    uint64_t colors[N]; // les couleurs associées
};

template<int N>
bool Sgreater_func (const element<N>& e1, const element<N>& e2)
{
//    printf("e1 : %16.16llX |\e[0;32m %16.16llX \e[0;37m|\n", e1.minizr, e1.colors[0]);
//    printf("e2 : %16.16llX |\e[0;32m %16.16llX \e[0;37m|\n", e2.minizr, e2.colors[0]);
    for(int i = 0; i < N; i += 1)
    {
        if( e1.colors[i] != e2.colors[i] )
        {
            return e1.colors[i] < e2.colors[i];
        }
    }
    return e1.minizr > e2.minizr;
}

//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
inline uint64_t popcount_u64_builtin(const uint64_t val)
{
    return __builtin_popcountll(val);
}
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
template<int N>
void process(const std::string& ifile, const std::string& ofile, int n_colors)
{
    const uint64_t size_bytes  = get_file_size(ifile);
    const uint64_t n_elements  = size_bytes / sizeof(uint64_t);
    uint64_t n_minimizr;
    uint64_t n_uint64_c;

    if( n_colors < 64  ){
        n_uint64_c  = 1;
        n_minimizr  = n_elements / (1 + n_uint64_c);
    }else{
        n_uint64_c  = ((n_colors + 63) / 64);
        n_minimizr  = n_elements / (1 + n_uint64_c);
    }

//    const int e_size = 1 + n_uint64_c;

    printf("(II) file size in bytes  : %llu\n", size_bytes);
    printf("(II) # uint64_t elements : %llu\n", n_elements);
    printf("(II) # minimizers        : %llu\n", n_minimizr);
    printf("(II) # of colors         : %d\n",   n_colors  );
    printf("(II) # uint64_t/colors   : %llu\n", n_uint64_c);

    ////////////////////////////////////////////////////////////////////////////////////

//    const int n_buff_mini = ((64 * 1024 * 1024) / e_size); // 64 Mbytes
//    const int buffer_size = n_buff_mini * e_size;

    FILE* fi = fopen( ifile.c_str(), "r" );
    if( fi == NULL )
    {
        printf("(EE) An error corrured while openning the file (%s)\n", ifile.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    //
    // On cree le vecteur qui va nous permettre de calculer l'histogramme
    //

    std::vector< element<N> > buff_t( n_minimizr );

    const int n_reads = fread(buff_t.data(), sizeof(uint64_t), n_elements, fi);
    if( n_reads != n_elements )
    {
        printf("(EE) Le nombre de données lues n'est pas correct !\n");
        printf("(EE) -  n_reads    : %d\n", n_reads   );
        printf("(EE) -  n_elements : %llu\n", n_elements);
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }
    fclose( fi );

    //
    // On remplace les minimizers par des numéros
    //

    for(uint64_t i = 0; i < n_minimizr; i += 1)
    {
        buff_t[i].minizr = i;
    }

    //
    // On remplace les minimizers par des numéros
    //
    std::sort( buff_t.begin(), buff_t.end(), &Sgreater_func<N> );

    //
    // On remplace les minimizers par des numéros
    //
    FILE* fo = fopen( ofile.c_str(), "w" );
    if( fo == NULL )
    {
        printf("(EE) An error corrured while openning the file (%s)\n", ofile.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }
    fwrite(buff_t.data(), sizeof(uint64_t), n_elements, fo);
    fclose( fo );
}
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
int main(int argc, char *argv[]) {

    std::string ifile;
    std::string ofile;
    int n_colors = 0;
    int verbose_flag  = 0;
    int help_flag     = 0;

    static struct option long_options[] ={
            {"verbose",     no_argument, &verbose_flag, 1},
            {"help",        no_argument, 0, 'h'},
            {"file",        required_argument, 0,  'f'},
            {"output",       required_argument, 0,  'o'},
            {"colors",      required_argument, 0,  'c'},
            {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;
    while( true )
    {
        c = getopt_long (argc, argv, "f:c:o:vh", long_options, &option_index);
        if (c == -1)
            break;

        switch ( c )
        {
            case 'f':
                ifile = optarg;
                break;

            case 'o':
                ofile = optarg;
                break;

            case 'c':
                n_colors = std::atoi( optarg );
                break;

            case 'h':
                help_flag = true;
                break;

            case '?':
                break;

            default:
                abort ();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////

    if ( (optind < argc) || (help_flag == true) || (ifile.size() == 0) || (n_colors < 0) )
    {
        printf ("\n");
        printf ("Usage :\n");
        printf ("  ./color_stats -f <string> -c <int>\n");
        printf ("\n");
        printf ("Options :\n");
        printf ("  -f (--file)   : The raw file to analyze\n");
        printf ("  -o (--output) : The raw file to analyze\n");
        printf ("  -c (--colors) : The number of colors in the file\n");
        printf ("\n");
        exit( EXIT_FAILURE );
    }

         if( n_colors <=  64 ) process<1>(ifile, ofile, n_colors);
    else if( n_colors <= 128 ) process<2>(ifile, ofile, n_colors);
    else if( n_colors <= 192 ) process<3>(ifile, ofile, n_colors);
    else if( n_colors <= 256 ) process<4>(ifile, ofile, n_colors);
    else if( n_colors <= 320 ) process<5>(ifile, ofile, n_colors);
    else if( n_colors <= 384 ) process<6>(ifile, ofile, n_colors);
    else if( n_colors <= 448 ) process<7>(ifile, ofile, n_colors);
    else if( n_colors <= 512 ) process<8>(ifile, ofile, n_colors);
    else if( n_colors <= 576 ) process<9>(ifile, ofile, n_colors);
    else if( n_colors <= 640 ) process<10>(ifile, ofile, n_colors);
    else if( n_colors <= 704 ) process<11>(ifile, ofile, n_colors);
    else if( n_colors <= 768 ) process<12>(ifile, ofile, n_colors);
    else if( n_colors <= 832 ) process<13>(ifile, ofile, n_colors);
    else if( n_colors <= 896 ) process<14>(ifile, ofile, n_colors);
    else if( n_colors <= 960 ) process<15>(ifile, ofile, n_colors);
    else if( n_colors <= 1024 ) process<16>(ifile, ofile, n_colors);
    else if( n_colors <= 1088 ) process<17>(ifile, ofile, n_colors);
    else if( n_colors <= 1152 ) process<18>(ifile, ofile, n_colors);
    else if( n_colors <= 1216 ) process<19>(ifile, ofile, n_colors);
    else if( n_colors <= 1280 ) process<20>(ifile, ofile, n_colors);
    else if( n_colors <= 1344 ) process<21>(ifile, ofile, n_colors);
    else if( n_colors <= 1408 ) process<22>(ifile, ofile, n_colors);
    else if( n_colors <= 1472 ) process<23>(ifile, ofile, n_colors);
    else if( n_colors <= 1536 ) process<24>(ifile, ofile, n_colors);
    else if( n_colors <= 1600 ) process<25>(ifile, ofile, n_colors);
    else if( n_colors <= 1664 ) process<26>(ifile, ofile, n_colors);
    else if( n_colors <= 1728 ) process<27>(ifile, ofile, n_colors);
    else if( n_colors <= 1792 ) process<28>(ifile, ofile, n_colors);
    else if( n_colors <= 1856 ) process<29>(ifile, ofile, n_colors);
    else if( n_colors <= 1920 ) process<30>(ifile, ofile, n_colors);
    else if( n_colors <= 1984 ) process<31>(ifile, ofile, n_colors);
    else if( n_colors <= 2048 ) process<32>(ifile, ofile, n_colors);
    else if( n_colors <= 2112 ) process<33>(ifile, ofile, n_colors);
    else if( n_colors <= 2176 ) process<34>(ifile, ofile, n_colors);
    else if( n_colors <= 2240 ) process<35>(ifile, ofile, n_colors);
    else if( n_colors <= 2304 ) process<36>(ifile, ofile, n_colors);
    else if( n_colors <= 2368 ) process<37>(ifile, ofile, n_colors);
    else if( n_colors <= 2432 ) process<38>(ifile, ofile, n_colors);
    else if( n_colors <= 2496 ) process<39>(ifile, ofile, n_colors);
    else if( n_colors <= 2560 ) process<40>(ifile, ofile, n_colors);
    else if( n_colors <= 2624 ) process<41>(ifile, ofile, n_colors);
    else if( n_colors <= 2688 ) process<42>(ifile, ofile, n_colors);
    else if( n_colors <= 2752 ) process<43>(ifile, ofile, n_colors);
    else if( n_colors <= 2816 ) process<44>(ifile, ofile, n_colors);
    else if( n_colors <= 2880 ) process<45>(ifile, ofile, n_colors);
    else if( n_colors <= 2944 ) process<46>(ifile, ofile, n_colors);
    else if( n_colors <= 3008 ) process<47>(ifile, ofile, n_colors);
    else if( n_colors <= 3072 ) process<48>(ifile, ofile, n_colors);
    else if( n_colors <= 3136 ) process<49>(ifile, ofile, n_colors);
    else if( n_colors <= 3200 ) process<50>(ifile, ofile, n_colors);
    else if( n_colors <= 3264 ) process<51>(ifile, ofile, n_colors);
    else if( n_colors <= 3328 ) process<52>(ifile, ofile, n_colors);
    else if( n_colors <= 3392 ) process<53>(ifile, ofile, n_colors);
    else if( n_colors <= 3456 ) process<54>(ifile, ofile, n_colors);
    else if( n_colors <= 3520 ) process<55>(ifile, ofile, n_colors);
    else if( n_colors <= 3584 ) process<56>(ifile, ofile, n_colors);
    else if( n_colors <= 3648 ) process<57>(ifile, ofile, n_colors);
    else if( n_colors <= 3712 ) process<58>(ifile, ofile, n_colors);
    else if( n_colors <= 3776 ) process<59>(ifile, ofile, n_colors);
    else if( n_colors <= 3840 ) process<60>(ifile, ofile, n_colors);
    else if( n_colors <= 3904 ) process<61>(ifile, ofile, n_colors);
    else if( n_colors <= 3968 ) process<62>(ifile, ofile, n_colors);
    else if( n_colors <= 4032 ) process<63>(ifile, ofile, n_colors);
    else if( n_colors <= 4096 ) process<64>(ifile, ofile, n_colors);
    else if( n_colors <= 4160 ) process<65>(ifile, ofile, n_colors);
    else if( n_colors <= 4224 ) process<66>(ifile, ofile, n_colors);
    else if( n_colors <= 4288 ) process<67>(ifile, ofile, n_colors);
    else if( n_colors <= 4352 ) process<68>(ifile, ofile, n_colors);
    else if( n_colors <= 4416 ) process<69>(ifile, ofile, n_colors);
    else if( n_colors <= 4480 ) process<70>(ifile, ofile, n_colors);
    else if( n_colors <= 4544 ) process<71>(ifile, ofile, n_colors);
    else if( n_colors <= 4608 ) process<72>(ifile, ofile, n_colors);
    else if( n_colors <= 4672 ) process<73>(ifile, ofile, n_colors);
    else if( n_colors <= 4736 ) process<74>(ifile, ofile, n_colors);
    else if( n_colors <= 4800 ) process<75>(ifile, ofile, n_colors);
    else if( n_colors <= 4864 ) process<76>(ifile, ofile, n_colors);
    else if( n_colors <= 4928 ) process<77>(ifile, ofile, n_colors);
    else if( n_colors <= 4992 ) process<78>(ifile, ofile, n_colors);
    else if( n_colors <= 5056 ) process<79>(ifile, ofile, n_colors);
    else if( n_colors <= 5120 ) process<80>(ifile, ofile, n_colors);
    else if( n_colors <= 5184 ) process<81>(ifile, ofile, n_colors);
    else if( n_colors <= 5248 ) process<82>(ifile, ofile, n_colors);
    else if( n_colors <= 5312 ) process<83>(ifile, ofile, n_colors);
    else if( n_colors <= 5376 ) process<84>(ifile, ofile, n_colors);
    else if( n_colors <= 5440 ) process<85>(ifile, ofile, n_colors);
    else if( n_colors <= 5504 ) process<86>(ifile, ofile, n_colors);
    else if( n_colors <= 5568 ) process<87>(ifile, ofile, n_colors);
    else if( n_colors <= 5632 ) process<88>(ifile, ofile, n_colors);
    else if( n_colors <= 5696 ) process<89>(ifile, ofile, n_colors);
    else if( n_colors <= 5760 ) process<90>(ifile, ofile, n_colors);
    else if( n_colors <= 5824 ) process<91>(ifile, ofile, n_colors);
    else if( n_colors <= 5888 ) process<92>(ifile, ofile, n_colors);
    else if( n_colors <= 5952 ) process<93>(ifile, ofile, n_colors);
    else if( n_colors <= 6016 ) process<94>(ifile, ofile, n_colors);
    else if( n_colors <= 6080 ) process<95>(ifile, ofile, n_colors);
    else if( n_colors <= 6144 ) process<96>(ifile, ofile, n_colors);
    else if( n_colors <= 6208 ) process<97>(ifile, ofile, n_colors);
    else if( n_colors <= 6272 ) process<98>(ifile, ofile, n_colors);
    else if( n_colors <= 6336 ) process<99>(ifile, ofile, n_colors);
    else if( n_colors <= 6400 ) process<100>(ifile, ofile, n_colors);
    else if( n_colors <= 6464 ) process<101>(ifile, ofile, n_colors);
    else if( n_colors <= 6528 ) process<102>(ifile, ofile, n_colors);
    else if( n_colors <= 6592 ) process<103>(ifile, ofile, n_colors);
    else if( n_colors <= 6656 ) process<104>(ifile, ofile, n_colors);
    else if( n_colors <= 6720 ) process<105>(ifile, ofile, n_colors);
    else if( n_colors <= 6784 ) process<106>(ifile, ofile, n_colors);
    else if( n_colors <= 6848 ) process<107>(ifile, ofile, n_colors);
    else if( n_colors <= 6912 ) process<108>(ifile, ofile, n_colors);
    else if( n_colors <= 6976 ) process<109>(ifile, ofile, n_colors);
    else if( n_colors <= 7040 ) process<110>(ifile, ofile, n_colors);
    else if( n_colors <= 7104 ) process<111>(ifile, ofile, n_colors);
    else if( n_colors <= 7168 ) process<112>(ifile, ofile, n_colors);
    else if( n_colors <= 7232 ) process<113>(ifile, ofile, n_colors);
    else if( n_colors <= 7296 ) process<114>(ifile, ofile, n_colors);
    else if( n_colors <= 7360 ) process<115>(ifile, ofile, n_colors);
    else if( n_colors <= 7424 ) process<116>(ifile, ofile, n_colors);
    else if( n_colors <= 7488 ) process<117>(ifile, ofile, n_colors);
    else if( n_colors <= 7552 ) process<118>(ifile, ofile, n_colors);
    else if( n_colors <= 7616 ) process<119>(ifile, ofile, n_colors);
    else if( n_colors <= 7680 ) process<120>(ifile, ofile, n_colors);
    else if( n_colors <= 7744 ) process<121>(ifile, ofile, n_colors);
    else if( n_colors <= 7808 ) process<122>(ifile, ofile, n_colors);
    else if( n_colors <= 7872 ) process<123>(ifile, ofile, n_colors);
    else if( n_colors <= 7936 ) process<124>(ifile, ofile, n_colors);
    else if( n_colors <= 8000 ) process<125>(ifile, ofile, n_colors);
    else if( n_colors <= 8064 ) process<126>(ifile, ofile, n_colors);
    else if( n_colors <= 8128 ) process<127>(ifile, ofile, n_colors);
    else if( n_colors <= 8192 ) process<128>(ifile, ofile, n_colors);
    else if( n_colors <= 8256 ) process<129>(ifile, ofile, n_colors);
    else if( n_colors <= 8320 ) process<130>(ifile, ofile, n_colors);
    else if( n_colors <= 8384 ) process<131>(ifile, ofile, n_colors);
    else if( n_colors <= 8448 ) process<132>(ifile, ofile, n_colors);
    else if( n_colors <= 8512 ) process<133>(ifile, ofile, n_colors);
    else if( n_colors <= 8576 ) process<134>(ifile, ofile, n_colors);
    else if( n_colors <= 8640 ) process<135>(ifile, ofile, n_colors);
    else if( n_colors <= 8704 ) process<136>(ifile, ofile, n_colors);
    else if( n_colors <= 8768 ) process<137>(ifile, ofile, n_colors);
    else if( n_colors <= 8832 ) process<138>(ifile, ofile, n_colors);
    else if( n_colors <= 8896 ) process<139>(ifile, ofile, n_colors);
    else if( n_colors <= 8960 ) process<140>(ifile, ofile, n_colors);
    else if( n_colors <= 9024 ) process<141>(ifile, ofile, n_colors);
    else if( n_colors <= 9088 ) process<142>(ifile, ofile, n_colors);
    else if( n_colors <= 9152 ) process<143>(ifile, ofile, n_colors);
    else if( n_colors <= 9216 ) process<144>(ifile, ofile, n_colors);
    else if( n_colors <= 9280 ) process<145>(ifile, ofile, n_colors);
    else if( n_colors <= 9344 ) process<146>(ifile, ofile, n_colors);
    else if( n_colors <= 9408 ) process<147>(ifile, ofile, n_colors);
    else if( n_colors <= 9472 ) process<148>(ifile, ofile, n_colors);
    else if( n_colors <= 9536 ) process<149>(ifile, ofile, n_colors);
    else if( n_colors <= 9600 ) process<150>(ifile, ofile, n_colors);
    else if( n_colors <= 9664 ) process<151>(ifile, ofile, n_colors);
    else if( n_colors <= 9728 ) process<152>(ifile, ofile, n_colors);
    else if( n_colors <= 9792 ) process<153>(ifile, ofile, n_colors);
    else if( n_colors <= 9856 ) process<154>(ifile, ofile, n_colors);
    else if( n_colors <= 9920 ) process<155>(ifile, ofile, n_colors);
    else if( n_colors <= 9984 ) process<156>(ifile, ofile, n_colors);
    else if( n_colors <= 10048 ) process<157>(ifile, ofile, n_colors);
    else if( n_colors <= 10112 ) process<158>(ifile, ofile, n_colors);
    else if( n_colors <= 10176 ) process<159>(ifile, ofile, n_colors);
    else if( n_colors <= 10240 ) process<160>(ifile, ofile, n_colors);
    else if( n_colors <= 10304 ) process<161>(ifile, ofile, n_colors);
    else if( n_colors <= 10368 ) process<162>(ifile, ofile, n_colors);
    else if( n_colors <= 10432 ) process<163>(ifile, ofile, n_colors);
    else if( n_colors <= 10496 ) process<164>(ifile, ofile, n_colors);
    else if( n_colors <= 10560 ) process<165>(ifile, ofile, n_colors);
    else if( n_colors <= 10624 ) process<166>(ifile, ofile, n_colors);
    else if( n_colors <= 10688 ) process<167>(ifile, ofile, n_colors);
    else if( n_colors <= 10752 ) process<168>(ifile, ofile, n_colors);
    else if( n_colors <= 10816 ) process<169>(ifile, ofile, n_colors);
    else if( n_colors <= 10880 ) process<170>(ifile, ofile, n_colors);
    else if( n_colors <= 10944 ) process<171>(ifile, ofile, n_colors);
    else if( n_colors <= 11008 ) process<172>(ifile, ofile, n_colors);
    else if( n_colors <= 11072 ) process<173>(ifile, ofile, n_colors);
    else if( n_colors <= 11136 ) process<174>(ifile, ofile, n_colors);
    else if( n_colors <= 11200 ) process<175>(ifile, ofile, n_colors);
    else if( n_colors <= 11264 ) process<176>(ifile, ofile, n_colors);
    else if( n_colors <= 11328 ) process<177>(ifile, ofile, n_colors);
    else if( n_colors <= 11392 ) process<178>(ifile, ofile, n_colors);
    else if( n_colors <= 11456 ) process<179>(ifile, ofile, n_colors);
    else if( n_colors <= 11520 ) process<180>(ifile, ofile, n_colors);
    else if( n_colors <= 11584 ) process<181>(ifile, ofile, n_colors);
    else if( n_colors <= 11648 ) process<182>(ifile, ofile, n_colors);
    else if( n_colors <= 11712 ) process<183>(ifile, ofile, n_colors);
    else if( n_colors <= 11776 ) process<184>(ifile, ofile, n_colors);
    else if( n_colors <= 11840 ) process<185>(ifile, ofile, n_colors);
    else if( n_colors <= 11904 ) process<186>(ifile, ofile, n_colors);
    else if( n_colors <= 11968 ) process<187>(ifile, ofile, n_colors);
    else if( n_colors <= 12032 ) process<188>(ifile, ofile, n_colors);
    else if( n_colors <= 12096 ) process<189>(ifile, ofile, n_colors);
    else if( n_colors <= 12160 ) process<190>(ifile, ofile, n_colors);
    else if( n_colors <= 12224 ) process<191>(ifile, ofile, n_colors);
    else if( n_colors <= 12288 ) process<192>(ifile, ofile, n_colors);
    else if( n_colors <= 12352 ) process<193>(ifile, ofile, n_colors);
    else if( n_colors <= 12416 ) process<194>(ifile, ofile, n_colors);
    else if( n_colors <= 12480 ) process<195>(ifile, ofile, n_colors);
    else if( n_colors <= 12544 ) process<196>(ifile, ofile, n_colors);
    else if( n_colors <= 12608 ) process<197>(ifile, ofile, n_colors);
    else if( n_colors <= 12672 ) process<198>(ifile, ofile, n_colors);
    else if( n_colors <= 12736 ) process<199>(ifile, ofile, n_colors);
    else if( n_colors <= 12800 ) process<200>(ifile, ofile, n_colors);
    else if( n_colors <= 12864 ) process<201>(ifile, ofile, n_colors);
    else if( n_colors <= 12928 ) process<202>(ifile, ofile, n_colors);
    else if( n_colors <= 12992 ) process<203>(ifile, ofile, n_colors);
    else if( n_colors <= 13056 ) process<204>(ifile, ofile, n_colors);
    else if( n_colors <= 13120 ) process<205>(ifile, ofile, n_colors);
    else if( n_colors <= 13184 ) process<206>(ifile, ofile, n_colors);
    else if( n_colors <= 13248 ) process<207>(ifile, ofile, n_colors);
    else if( n_colors <= 13312 ) process<208>(ifile, ofile, n_colors);
    else if( n_colors <= 13376 ) process<209>(ifile, ofile, n_colors);
    else if( n_colors <= 13440 ) process<210>(ifile, ofile, n_colors);
    else if( n_colors <= 13504 ) process<211>(ifile, ofile, n_colors);
    else if( n_colors <= 13568 ) process<212>(ifile, ofile, n_colors);
    else if( n_colors <= 13632 ) process<213>(ifile, ofile, n_colors);
    else if( n_colors <= 13696 ) process<214>(ifile, ofile, n_colors);
    else if( n_colors <= 13760 ) process<215>(ifile, ofile, n_colors);
    else if( n_colors <= 13824 ) process<216>(ifile, ofile, n_colors);
    else if( n_colors <= 13888 ) process<217>(ifile, ofile, n_colors);
    else if( n_colors <= 13952 ) process<218>(ifile, ofile, n_colors);
    else if( n_colors <= 14016 ) process<219>(ifile, ofile, n_colors);
    else if( n_colors <= 14080 ) process<220>(ifile, ofile, n_colors);
    else if( n_colors <= 14144 ) process<221>(ifile, ofile, n_colors);
    else if( n_colors <= 14208 ) process<222>(ifile, ofile, n_colors);
    else if( n_colors <= 14272 ) process<223>(ifile, ofile, n_colors);
    else if( n_colors <= 14336 ) process<224>(ifile, ofile, n_colors);
    else if( n_colors <= 14400 ) process<225>(ifile, ofile, n_colors);
    else if( n_colors <= 14464 ) process<226>(ifile, ofile, n_colors);
    else if( n_colors <= 14528 ) process<227>(ifile, ofile, n_colors);
    else if( n_colors <= 14592 ) process<228>(ifile, ofile, n_colors);
    else if( n_colors <= 14656 ) process<229>(ifile, ofile, n_colors);
    else if( n_colors <= 14720 ) process<230>(ifile, ofile, n_colors);
    else if( n_colors <= 14784 ) process<231>(ifile, ofile, n_colors);
    else if( n_colors <= 14848 ) process<232>(ifile, ofile, n_colors);
    else if( n_colors <= 14912 ) process<233>(ifile, ofile, n_colors);
    else if( n_colors <= 14976 ) process<234>(ifile, ofile, n_colors);
    else if( n_colors <= 15040 ) process<235>(ifile, ofile, n_colors);
    else if( n_colors <= 15104 ) process<236>(ifile, ofile, n_colors);
    else if( n_colors <= 15168 ) process<237>(ifile, ofile, n_colors);
    else if( n_colors <= 15232 ) process<238>(ifile, ofile, n_colors);
    else if( n_colors <= 15296 ) process<239>(ifile, ofile, n_colors);
    else if( n_colors <= 15360 ) process<240>(ifile, ofile, n_colors);
    else if( n_colors <= 15424 ) process<241>(ifile, ofile, n_colors);
    else if( n_colors <= 15488 ) process<242>(ifile, ofile, n_colors);
    else if( n_colors <= 15552 ) process<243>(ifile, ofile, n_colors);
    else if( n_colors <= 15616 ) process<244>(ifile, ofile, n_colors);
    else if( n_colors <= 15680 ) process<245>(ifile, ofile, n_colors);
    else if( n_colors <= 15744 ) process<246>(ifile, ofile, n_colors);
    else if( n_colors <= 15808 ) process<247>(ifile, ofile, n_colors);
    else if( n_colors <= 15872 ) process<248>(ifile, ofile, n_colors);
    else if( n_colors <= 15936 ) process<249>(ifile, ofile, n_colors);
    else if( n_colors <= 16000 ) process<250>(ifile, ofile, n_colors);
    else if( n_colors <= 16064 ) process<251>(ifile, ofile, n_colors);
    else if( n_colors <= 16128 ) process<252>(ifile, ofile, n_colors);
    else if( n_colors <= 16192 ) process<253>(ifile, ofile, n_colors);
    else if( n_colors <= 16256 ) process<254>(ifile, ofile, n_colors);
    else if( n_colors <= 16320 ) process<255>(ifile, ofile, n_colors);
    else{
        printf ("(EE) Color set is not yet supported)\n");
        printf ("\n");
        exit( EXIT_FAILURE );
    }
    //
    // On estime les parametres qui vont être employés par la suite
    //

#if 0
    std::vector<uint64_t> histo( n_colors + 1 );
    for(int i = 0; i < histo.size(); i += 1)
    {
        histo[i] = 0;
    }

    //
    // On parcours l'ensemble des minimisers
    //
    auto t1 = std::chrono::high_resolution_clock::now();

    bool stop = false;
    while( stop == false )
    {
        //
        // on precharge un sous ensemble de données
        //
        const int n_reads = fread(liste.data(), sizeof(uint64_t), buffer_size, fi);
        const int elements = n_reads / (1 + n_uint64_c);
        stop = (n_reads != buffer_size);

        //
        // On parcours l'ensemble des minimizer que l'on a chargé
        //
        for(int m = 0; m < elements; m += 1) // le nombre total de minimizers
        {
            //
            // On compte le nombre de couleurs associé au minimizer
            //
            int colors = 0;
            for(int c = 0; c < n_uint64_c; c += 1)
            {
                uint64_t value = liste[e_size * m + 1 + c];
                colors += popcount_u64_builtin(value);
            }
            histo[colors] += 1;
        }
    }

    fclose(fi);

    //
    // On va afficher l'ensemble des données issues de l'histo
    //

    printf("+------+------------+--------+---------+\n");
    printf("| Idx  | #Occurence | Occ. %% | Cumul.%% |\n");
    printf("+------+------------+--------+---------+\n");

    double sum = 0.0;
    for(uint64_t i = 1; i < histo.size(); i += 1)
    {
        if( histo[i] != 0 )
        {
            double proba = 100.0 * (double)histo[i] / (double)n_minimizr;
            sum         += proba;
            printf("%6llu | %10llu | %6.3f | %7.3f |\n", i, histo[i], proba, sum);
        }
    }

    printf("+------+------------+--------+---------+\n");

    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms_double = t2 - t1;
    double ms_time = ms_double.count();
    if( ms_time > 1000.0 )
        std::cout << "Elapsed time : " << (ms_time/1000.0) << "s\n";
    else
        std::cout << "Elapsed time : " << ms_time << "ms\n";
#endif
    return 0;
}