#pragma once
#include <string>
#include <iomanip>
#include <map>
#include <typeinfo>
#include <system_error>

//UNIX & POSIX
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <thread>
#include <memory.h>
#include "snd_rcv.h"
#include "auth.h"
#include "server_error.h"

class Connection
{
private:
    std::atomic<int> thread_count{0};
    int sock;
    std::unique_ptr <sockaddr_in> serv_addr;
    std::unique_ptr <sockaddr_in> client_addr;
    int queue_len=5;
    uint32_t serv_v;
public:
    void connect(Send_recv& clc, Auth& ath, std::shared_ptr<Error> er);
    void set_thread(int work_sock, Send_recv& clc, Auth& ath, std::shared_ptr<Error> er);
    void disconnect();
    void wait();
    Connection(unsigned short port, uint32_t v);
    ~Connection()
    {
        close(sock);
    }
};
