#include "../includes/server.hpp"

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

Server::Server(const ServerConfig& conf) : server_fd(-1), epoll_fd(-1), config(conf) {}

Server::~Server() {
    if(server_fd != -1) close(server_fd);
    if(epoll_fd != -1) close(epoll_fd);
    for(int fd : clients) close(fd);
}

void Server::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::init() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) throw std::runtime_error("socket failed");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(config.port);

    if(bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind failed");

    if(listen(server_fd, 10) < 0)
        throw std::runtime_error("listen failed");

    setNonBlocking(server_fd);

    epoll_fd = epoll_create(10);
    if(epoll_fd < 0)
        throw std::runtime_error("epoll_create failed");

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    std::cout << "Server listening on port " << config.port << std::endl;
}

void Server::run() {
    epoll_event events[MAX_EVENTS];

    while(true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if(n < 0) continue;

        for(int i = 0; i < n; i++) {
            if(events[i].data.fd == server_fd)
                acceptClients();
            else
                handleClient(events[i].data.fd);
        }
    }
}

void Server::acceptClients() {
    while(true) {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int cfd = accept(server_fd, (sockaddr*)&client_addr, &len);
        if(cfd < 0) break;

        setNonBlocking(cfd);

        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = cfd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ev);

        clients.push_back(cfd);
        std::cout << "Client connected: " << cfd << std::endl;
    }
}


void Server::handleClient(int fd) {
    char buffer[BUFFER_SIZE];
    ssize_t r = recv(fd, buffer, sizeof(buffer)-1, 0);

    if(r <= 0) { closeClient(fd); return; }
    buffer[r] = '\0';
    std::cout << "Request:\n" << buffer;

    // Serve index file from config.root
    std::string path = config.root + "/" + config.index;
    std::ifstream file(path.c_str());
    std::string body;
    
    if(file.is_open()) {
        body.assign((std::istreambuf_iterator<char>(file)),
                    (std::istreambuf_iterator<char>()));
    } else {
        body = "File not found";
    }

    std::string resp = "HTTP/1.1 200 OK\r\n";
    resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    resp += "Content-Type: text/html\r\n\r\n";
    resp += body;

    send(fd, resp.c_str(), resp.size(), 0);
    closeClient(fd);
}

void Server::closeClient(int fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    clients.erase(std::remove(clients.begin(), clients.end(), fd), clients.end());
    std::cout << "Client disconnected: " << fd << std::endl;
}