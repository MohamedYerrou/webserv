#include "../includes/Server.hpp"
#include "../includes/Server.hpp"
#include <sys/socket.h>
#include "../includes/Utils.hpp"
#include "../includes/Utils.hpp"
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <cerrno>
#include <utility>
#include <utility>
#include <stdlib.h>

Server::Server()
{
	client_max_body_size = 1024;

}

Server::~Server()
{
}

//getters

const std::vector<Location>& Server::getLocations() const
{
	return locations;
}

void    Server::push_listen(std::string tok)
{
	int							port;
	std::string					ip;
	std::pair<std::string, int> pair;

	ip = tok.substr(0, tok.find(':'));
	port = atoi(tok.substr(tok.find(':') + 1).c_str());
	pair = make_pair(ip, port);

    listens.push_back(pair);
}

void    Server::push_location(Location location)
{
    locations.push_back(location);
}

void	Server::setMaxBodySize(std::string size)
{
	std::stringstream ss(size);

	if (size.empty())
		throw_exception("Empty max body size", "");
	else
	{
		for (size_t i = 0; i < size.size(); i++)
		{
			if (!isdigit(size[i]))
				throw_exception("Invalid max body size", "");
		}
	}
	ss >> client_max_body_size;
	std::cout << client_max_body_size << std::endl;
}

size_t	Server::getMaxBodySize()
{
	return client_max_body_size;
}

std::map<std::string, session>&	Server::getSessions()
{
	return sessions;
}


void    Server::init_server(int epfd, std::map<int, Server*>& server_fd)
{
    for (size_t i = 0; i < listens.size(); i++)
	{
		int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (listen_fd == -1)
			throw_exception("socket: ", strerror(errno));

		int opt = 1;
		if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			throw_exception("setsockopt: ", strerror(errno));

		// server_fd.push_back(listen_fd);
		server_fd.insert(std::make_pair(listen_fd, this));

		sockaddr_in addr;

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		// std::cout << listens[i].second << std::endl;
		addr.sin_port = htons(listens[i].second);
		// std::cout << addr.sin_port << std::endl;
		addr.sin_addr.s_addr = inet_addr(listens[i].first.c_str());
		// std::cout << listens[i].first << std::endl;

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