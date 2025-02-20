#pragma once
#include "stream_writer.hpp"

class stream_writer_library
{
public:
    static stream_writer*  allocate(const std::string& file);
};
