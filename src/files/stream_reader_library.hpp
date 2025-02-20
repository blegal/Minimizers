#pragma once
#include "stream_reader.hpp"

class stream_reader_library
{
public:
    static stream_reader*  allocate(const std::string& file);
};
