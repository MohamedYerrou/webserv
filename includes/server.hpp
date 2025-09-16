#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <netinet/in.h>

struct ServerConfig {
    int port;
    std::string server_name;
    std::string root;
    std::string index;
};

class Server {
private:
    int server_fd;
    int epoll_fd;
    sockaddr_in addr;
    ServerConfig config;
    std::vector<int> clients;

public:
    Server(const ServerConfig& conf);
    ~Server();

    void init();
    void run();

private:
    void setNonBlocking(int fd);
    void acceptClients();
    void handleClient(int fd);
    void closeClient(int fd);
};

#endif
