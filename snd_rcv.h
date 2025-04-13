#pragma once
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <errno.h>
#include <float.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory>
#include <cstdio>
#include <fmt/core.h>
#include <fstream>
#include <system_error>
#include "interface.h"
#include "server_error.h"
#include <regex>
#include <mutex>

class Send_recv
{
private:
    std::string file_type = ".txt";
    uint32_t count;
    uint32_t size;
    int work_sock;
    uint32_t serv_v;
    bool stat;
    std::string lgfile_path;
    std::string id_clc;
public:
    static std::mutex log_file_mutex; // Объявление
    Send_recv(uint32_t vers, std::string log_path): work_sock(-1)
    {
        serv_v = vers;
        lgfile_path = log_path;
    };
    bool calc(std::shared_ptr<Error> er);
    bool operator()(int sock, bool status, std::string cl_id, std::shared_ptr<Error> er);
    std::string filename_recv(int sock, std::shared_ptr<Error> er);
    void file_recv(int sock, const std::string& ofile_name, std::shared_ptr<Error> er);
    bool send_log(int sock, const std::string& lg_f, const std::string& date_st);
    size_t find_line_number(const std::string& lg_f, const std::string& date_st, size_t local_line_num);
    void replace_line_in_file(const std::string& lg_f, size_t line_number, const std::string& edited_line);
    void delete_lg_file(const std::string& lg_f);
};
