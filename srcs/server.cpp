#include "../includes/server.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>

Server::Server(const ServerConfig& config) : _config(config) {
    _epoll_fd = -1;
}

Server::~Server() {
    for (size_t i = 0; i < _listen_fds.size(); i++)
        close(_listen_fds[i]);
    if (_epoll_fd != -1)
        close(_epoll_fd);
}

int Server::createNonBlockingSocket(int domain, int type, int protocol) {
    // Add SOCK_NONBLOCK to type
    int sockfd = socket(domain, type | SOCK_NONBLOCK, protocol);
    if (sockfd < 0) {
        perror("socket failed");
    }
    return sockfd;
}

void Server::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool Server::isListeningFd(int fd) {
    for (size_t i = 0; i < _listen_fds.size(); i++)
        if (_listen_fds[i] == fd)
            return true;
    return false;
}

void Server::init() {
    _epoll_fd = epoll_create(1);
    if (_epoll_fd == -1)
        throw std::runtime_error("epoll_create failed");

    for (size_t i = 0; i < _config.listens.size(); i++) {
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd == -1)
            throw std::runtime_error("socket failed");

        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(_config.listens[i].second);
        addr.sin_addr.s_addr = inet_addr(_config.listens[i].first.c_str());

        if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) == -1)
            throw std::runtime_error("bind failed: " + std::string(strerror(errno)));

        if (listen(listen_fd, SOMAXCONN) == -1)
            throw std::runtime_error("listen failed");
            
        setNonBlocking(listen_fd);
        // listen_fd = createNonBlockingSocket(AF_INET, SOCK_STREAM, 0);

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = listen_fd;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
            throw std::runtime_error("epoll_ctl failed");

        _listen_fds.push_back(listen_fd);
        std::cout << "Listening on " << _config.listens[i].first
                  << ":" << _config.listens[i].second << "\n";
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
            throw std::runtime_error("epoll_ctl failed");
    }
}

void Server::handleNewConnection(int listen_fd) {
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd == -1) return;
    setNonBlocking(client_fd);
    // client_fd = createNonBlockingSocket(AF_INET, SOCK_STREAM, 0);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = client_fd;
    epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

    std::cout << "New client connected, fd=" << client_fd << "\n";
}

void Server::handleClientRequest(int client_fd) {
    char buffer[4096];
    int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        close(client_fd);
        std::cout << "Client disconnected fd=" << client_fd << "\n";
        return;
    }

    buffer[bytes] = '\0';
    std::cout << "Received request:\n" << buffer << "\n";

    // Always return the index file
    std::string body = "<html><body><h1>Hello from Webserv!</h1></body></html>";
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Content-Type: text/html\r\n"
        "\r\n" +
        body;

    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
}

void Server::run() {
    struct epoll_event events[64];

    while (true) {
        int nfds = epoll_wait(_epoll_fd, events, 64, -1);
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            if (isListeningFd(fd)) {
                handleNewConnection(fd);
            } else if (events[i].events & EPOLLIN) {
                handleClientRequest(fd);
            }
        }
    }
}
