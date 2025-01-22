//
// Created by legal on 21/01/2025.
//

#ifndef COLOR_STATS_TOTO_HPP
#define COLOR_STATS_TOTO_HPP
#include <iostream>
#include <zlib.h>
#include <stdio.h>
#include <cassert>

int inf(FILE* fp)
{
    gzFile* gzf = gzdopen(fileno(fp), "r");
    assert(::gztell(gzf) == 0);
    std::cout << "pos: " << ::gztell(gzf) << std::endl;
    ::gzseek(gzf, 18L, SEEK_SET);
    char buf[768] = {0};
    ::gzread(gzf, buf, sizeof(buf)); // use a custom size as needed
    std::cout << buf << std::endl; // Print file contents from 18th char onward
    ::gzclose(gzf);
    return 0;
}

int main() {
    const char* in_filename = "infile.gz";
    FILE* fp = fopen(in_filename, "r");
    inf(fp);
    return 0;
}
#endif //COLOR_STATS_TOTO_HPP
