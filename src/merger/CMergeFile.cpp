#include "CMergeFile.hpp"

CMergeFile::CMergeFile()
{
    name        = "none";
    numb_colors = 0;
    real_colors = 0;
}

CMergeFile::CMergeFile(const std::string n, const int64_t nc, const int64_t rc)
{
    name        = n;
    numb_colors = nc;
    real_colors = rc;
}

CMergeFile::CMergeFile(const std::string n, const CMergeFile& cm1, const CMergeFile& cm2)
{
    name        = n;
    numb_colors = cm1.numb_colors + cm2.numb_colors;
    real_colors = cm1.real_colors + cm2.real_colors;
}