#include <fstream>
#include <sstream>
#include <utility> 
#include <vector>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "Utils.hpp"
#include "Server.hpp"
#include "Client.hpp"

void	handleListeningFd(int fd, int epfd, std::map<int, Client>& Clients)
{
	int client_fd = accept(fd, NULL, NULL);
	if (client_fd == -1)
		throw_exception("accept: ", strerror(errno));
	setNonBlocking(client_fd);

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = client_fd;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
		throw_exception("epoll_ctl: ", strerror(errno));
	Clients.insert(std::make_pair(client_fd, Client(client_fd)));
}

std::string	generateResponse(int fd, Client& c)
{
	(void)fd;
	std::cout << c.readBuffer << std::endl;
	char body_size[1000];
	std::string body = "<html><body><h1>Hello from Webserv!<br>" + c.readBuffer + "</h1></body></html>";
	sprintf(body_size, "%d", (int)body.size());
	std::string response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: " + std::string(body_size) + "\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		+ body;
	return response;
}

void	handleClientFd(int fd, int epfd, std::map<int, Client>& Clients)
{
	char	buf[10];

	int n = recv(fd, buf, sizeof(buf) - 1, 0);
	if (n == 0)
	{
		if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
			throw_exception("epoll_ctl: ", strerror(errno));
		close(fd);
		Clients.erase(fd);
		return ;
	}
	else if (n == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
			throw_exception("epoll_ctl: ", strerror(errno));
		close(fd);
		Clients.erase(fd);
	}
	else if (n > 0)
	{
		Clients[fd].readBuffer.append(buf, n);
		if (Clients[fd].readBuffer.find("\r\n\r\n") == std::string::npos)
		{
			
			return ;
		}
		else
		{
			Clients[fd].requestComplete = true;
			Clients[fd].writeBuffer = generateResponse(fd, Clients[fd]);
			struct epoll_event ev;
			ev.events = EPOLLOUT;
			ev.data.fd = fd;

			if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
				throw_exception("epoll_ctl: ", strerror(errno));
			std::cout << "kkkkkkkkkkkkkkkkkkkkk\n";
		}
	}
}

void	handleClientResponse(int fd, int epfd, Client& c)
{
	int n = send(fd, c.writeBuffer.c_str(), c.writeBuffer.size(), 0);
	if (n == -1)
		throw_exception("send: ", strerror(errno));
	else
	{
		c.writeBuffer.erase(0, n);
		if (c.writeBuffer.empty())
		{
			struct epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.fd = fd;

			if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
				throw_exception("epoll_ctl: ", strerror(errno));
			return ;
		}
	}
}

void	run_server(int epfd, std::vector<int>& fd_vect)
{
	struct epoll_event				events[64];
	std::map<int , Client>			clients;    


	while (true)
	{
		int nfds = epoll_wait(epfd, events, 64, -1);
		if (nfds == -1)
			throw_exception("epoll_wait: ", strerror(errno));
		for (int i = 0; i < nfds; i++)
		{
			int fd = events[i].data.fd;
			if (listening_fd(fd_vect, fd))
				handleListeningFd(fd, epfd, clients);
			else if (events[i].events & EPOLLIN)
				handleClientFd(fd, epfd, clients);
			else if (events[i].events & EPOLLOUT)
				handleClientResponse(fd, epfd, clients[fd]);
		}
	}
}