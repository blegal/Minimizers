#pragma once
#include "file_reader.hpp"

class file_reader_library
{
public:
    static file_reader*  allocate(const std::string& file);
};
