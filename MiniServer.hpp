#ifndef MINISERVER_HPP
#define MINISERVER_HPP

#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <string>
#include <stdexcept>
#include <unistd.h>

#define PORT 4242
#define BACKLOG 10
#define BUFFER_SIZE 1024

class MiniServer
{
    private:
        int server_fd;
        int client_fd;
        struct sockaddr_in sa;
        struct sockaddr_in client_addr;
        socklen_t client_len;
    public:
        MiniServer();
        ~MiniServer();
        int setup();
        void    accept_connection();
        void    process_message();
};

#endif