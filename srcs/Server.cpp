#include "Server.hpp"
#include <sys/socket.h>
#include "Utils.hpp"
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <cerrno>


Server::Server(): client_max_body_size(1024)
{
}

Server::~Server()
{
}

void    Server::push_listen(std::pair<std::string, int> pair)
{
    listens.push_back(pair);
}

void    Server::push_location(Location location)
{
    locations.push_back(location);
}

void    Server::init_server(int epfd, std::vector<int>& fd_vect)
{
    for (size_t i = 0; i < listens.size(); i++)
		{
			int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
			if (listen_fd == -1)
				throw_exception("socket: ", strerror(errno));

			int opt = 1;
			if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
				throw_exception("setsockopt: ", strerror(errno));

			fd_vect.push_back(listen_fd);

			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(listens[i].second);
			addr.sin_addr.s_addr = inet_addr(listens[i].first.c_str());

			if (bind(listen_fd, (const sockaddr*)&addr, sizeof(addr)) == -1)
				throw_exception("bind: ", strerror(errno));

			if (listen(listen_fd, SOMAXCONN) == -1)
				throw_exception("listen: ", strerror(errno));

			setNonBlocking(listen_fd);

			std::cout << "Listening on: " << listens[i].first << ":" <<
			listens[i].second << std::endl;

			struct epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.fd = listen_fd;

			if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
				throw_exception("epoll_ctl: ", strerror(errno));
		}
}