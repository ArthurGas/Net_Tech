#pragma once
// C++
#include <iostream>
#include <memory>
#include <random>
#include <map>
#include <fstream>
#include <stdexcept>

// POSIX
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

//Crypto++
#include <cryptopp/cryptlib.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include "server_error.h"
//вспомогательная структура для создания словаря
struct User {
    std::string password;
    bool is_admin;
};

class Auth
{
private:
    int work_sock;
    std::map<std::string, User> base_cont;
    bool id_check(std::string id, std::map<std::string, User> base_c);
    bool pw_check(std::string pw_from_cl, std::string hashed_pw);
    bool status=0;
    std::string client_id;
public:
    std::map<std::string, User> base_read(std::string file_name);
    void set_base(std::map<std::string, User> base_c);
    Auth(): work_sock(-1) {}
    void operator()(int sock, std::shared_ptr<Error> er);
    void auth(std::shared_ptr<Error> er);
    std::string string_recv(std::shared_ptr<Error> er);
    std::string salt_gen();
    std::string pw_hash(std::string salt, std::string password);
    bool get_status() const
    {
        return status;
    }
    std::string get_id() const
    {
        return client_id;
    }
};
