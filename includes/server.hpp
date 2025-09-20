#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <string>
#include <map>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include "../ConfigStructs.hpp"

class Server {
private:
    ServerConfig _config;
    std::vector<int> _listen_fds;
    int _epoll_fd;

public:
    Server(const ServerConfig& config);
    ~Server();

    void init();
    void run();

private:
    void setNonBlocking(int fd);
    int createNonBlockingSocket(int domain, int type, int protocol);
    bool isListeningFd(int fd);
    void handleNewConnection(int listen_fd);
    void handleClientRequest(int client_fd);
};

#endif