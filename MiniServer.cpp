#include "MiniServer.hpp"
#include "Request.hpp"

MiniServer::MiniServer()
    : server_fd(-1), client_fd(-1)
{
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    client_len = sizeof(client_addr);
}

MiniServer::~MiniServer()
{
}

int MiniServer::setup()
{
    int op = 1;
    server_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (server_fd == -1)
        throw std::runtime_error("Socket Error: " + std::string(strerror(errno)));
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) == -1)
    {
        close(server_fd);
        throw std::runtime_error("socket option error: " + std::string(strerror(errno)));
    }
    std::cout << "Created server socket fd: " << server_fd << std::endl;
    if (bind(server_fd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        close(server_fd);
        throw std::runtime_error("Bind error: " + std::string(strerror(errno)));
    }
    if (listen(server_fd, BACKLOG) == -1)
    {
        close(server_fd);
        throw std::runtime_error("Listen error: " + std::string(strerror(errno)));
    }
    std::cout << "Listenning on fd: " << server_fd << std::endl;
    return server_fd;
}

void    MiniServer::accept_connection()
{
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1)
    {
        close(server_fd);
        throw std::runtime_error("Accept error: " + std::string(strerror(errno)));
    }
    std::cout << "Accepted a new client fd: " << client_fd << std::endl;
}

void    MiniServer::process_message()
{
    char buffer[BUFFER_SIZE];
    std::string raw_request;
    while (true)
    {
        ssize_t bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes == -1)
        {
            throw std::runtime_error("Recv error: " + std::string(strerror(errno)));
        }
        if (bytes == 0)
        {
            std::cout << "the client " << client_fd << " closed connection" << std::endl;
            close(client_fd);
            break ;
        }
        buffer[bytes] = '\0';
        raw_request.append(buffer);
        if (raw_request.find("\r\n\r\n") != std::string::npos)
            break ;
    }
    Request req;
    req.parseRequest(raw_request);
    parsedRequest(req);
    std::cout << "Close client connection" << std::endl;
    close(client_fd);
    std::cout << "Close server connection" << std::endl;
    close(server_fd);
}