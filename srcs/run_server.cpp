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
#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include "../includes/Utils.hpp"

void	handleListeningFd(int epfd, int fd, std::map<int, Client*>& clients, std::map<int, Server*>& servers_fd)
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

	clients[client_fd] = new Client(client_fd, servers_fd.find(fd)->second);
}

void	handleClientFd(int epfd, int fd, std::map<int, Client*>& clients)
{
	char	buf[30];
	Client* clientPtr = clients[fd];

	ssize_t received = recv(clientPtr->getFD(), buf, sizeof(buf) - 1, 0);
	if (received == -1)
	{
		std::cout << "Recv error: " << strerror(errno) << std::endl;
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
		delete clientPtr;
		clients.erase(fd);
		close(fd);
	}
	else if (received == 0)
	{
		std::cout << "Connection closed" << std::endl;
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
		delete clientPtr;
		clients.erase(fd);
		close(fd);
	}
	else
	{
		buf[received] = '\0';
		clientPtr->appendData(buf, received);
		//step6: if request complete
		if (clientPtr->getReqComplete())
		{
			// std::cout << "this will handle full request" << std::endl;
			// std::cout << "method: " << clientPtr->getRequest()->getMethod() << std::endl;
			clientPtr->handleCompleteRequest();
			struct epoll_event ev;
			ev.events = EPOLLOUT;
			ev.data.fd = fd;
			if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
				throw_exception("epoll_ctl: ", strerror(errno));
		}
	}
}


void	handleClientResponse(int fd, int epfd, Client* c)
{
	(void) epfd;
	(void) c;

	std::string msg = "KKKKKKKKKKKKKKKLLLLLLLLLLLLLLLLL";
	char body_size[50];
	std::string body = "<html><body><h1>Hello from Webserv!<br>" + msg + "</h1></body></html>";
	sprintf(body_size, "%d", (int)body.size());
	std::string response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: " + std::string(body_size) + "\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		+ body;


	send(fd, response.c_str(), response.size(), 0);
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
		throw_exception("epoll_ctl: ", strerror(errno));
	return ;

	// int n = send(fd, c->getHeaders().c_str(), c->getHeaders().size(), 0);
	// if (n == -1)
	// 	throw_exception("send: ", strerror(errno));
	// else
	// {
	// 	c.writeBuffer.erase(0, n);
	// 	if (c.writeBuffer.empty())
	// 	{
	// 		struct epoll_event ev;
	// 		ev.events = EPOLLIN;
	// 		ev.data.fd = fd;

	// 		if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
	// 			throw_exception("epoll_ctl: ", strerror(errno));
	// 		c.readBuffer.clear();
	// 		return ;
	// 	}
	// }
}

void	run_server(int epfd, std::map<int, Server*>& servers_fd)
{
	struct epoll_event events[64];
	std::map<int, Client*> clients;

	while (true)
	{
		int nfds = epoll_wait(epfd, events, 64, -1);
		if (nfds == -1)
			throw_exception("epoll_wait: ", strerror(errno));

		for (int i = 0; i < nfds; i++)
		{
			int fd = events[i].data.fd;
			if (listening_fd(servers_fd, fd))
				handleListeningFd(epfd, fd, clients, servers_fd);
			else if (events[i].events & EPOLLIN)
				handleClientFd(epfd, fd, clients);
			else if (events[i].events & EPOLLOUT)
				handleClientResponse(fd, epfd, clients[fd]);
		}
	}
}