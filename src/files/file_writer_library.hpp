#pragma once
#include "file_writer.hpp"

class file_writer_library
{
public:
    static file_writer*  allocate(const std::string& file);
};
