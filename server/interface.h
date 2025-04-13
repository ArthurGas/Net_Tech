#pragma once
#include <string>
#include <iostream>
#include <ctime>
#include <boost/program_options.hpp>
#include <fstream>
#include "server_error.h"

namespace po = boost::program_options;

class Interface
{
private:
    uint32_t port;
    std::string base_file;
    std::string log_file;
    uint32_t version;
    void operator()(int sock);
public:
    Interface(): port(0), base_file(" ")
    {}
    bool set_options(int argscount, char *argvectors[], std::shared_ptr<Error> e);
    std::string get_basefile() const;
    uint32_t get_port() const;
    std::string get_logfile() const;
    uint32_t get_version() const;
};
