#pragma once
#include <stdexcept>
#include <string>

class Error
{
private:
    std::string log_file_path;
    bool iscritical=false;
    bool path_locked = false;
public:
    void set_path(const std::string& lg_name);
    std::string error_log(std::string e_str);
    //std::string regular_log(std::string reg_str);
    std::string regular_log(std::string reg_str, std::string id_cl = "N/A", int bytes=0);
    void critic_set();
};


class log_error: public std::runtime_error
{
public:
    log_error(const std::string& s) : std::runtime_error(s) {}
    log_error(const char * s) : std::runtime_error(s) {}
};

class auth_error: public std::invalid_argument
{
public:
    auth_error(const std::string& s) : std::invalid_argument(s) {}
    auth_error(const char * s) : std::invalid_argument(s) {}
};
class option_error: public std::runtime_error
{
public:
    option_error(const std::string& s) : std::runtime_error(s) {}
    option_error(const char * s) : std::runtime_error(s) {}
};
