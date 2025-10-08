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

void	handleListeningClient(int epfd, int fd, std::map<int, Client*>& clients, std::vector<Server>& servers)
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
	
	Server* srv = NULL;
	for (size_t i = 0; i < servers.size(); i++)
	{
		if (servers[i].haslistenFD(fd))
		{
			srv = &servers[i];
			break;
		}
	}
	if (!srv)
	{
		std::cout << "ERROR: No server found for fd " << fd << std::endl;
        close(client_fd);
        return;
	}
	
	//TODO: must ensure which server the client interact with.
	clients[client_fd] = new Client(client_fd, srv);
}

void	handleClientRequest(int epfd, int fd, std::map<int, Client*>& clients)
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
		if (clientPtr->getReqComplete())
		{
			std::cout << "this will handle full request" << std::endl;
			if (!clientPtr->getRequestError())
				clientPtr->handleCompleteRequest();
			struct epoll_event ev;
			ev.events = EPOLLOUT;
			ev.data.fd = fd;
			if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
				throw_exception("epoll_ctl: ", strerror(errno));
		}
	}
}

void	run_server(int epfd, std::vector<int>& fd_vect, std::vector<Server>& servers)
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
			if (listening_fd(fd_vect, fd))
				handleListeningClient(epfd, fd, clients, servers);
			else if (events[i].events & EPOLLIN)
				handleClientRequest(epfd, fd, clients);
			else if (events[i].events & EPOLLOUT)
			{
				Response currentResponse = clients[fd]->getResponse();
				std::string res = currentResponse.build();
				ssize_t sent = send(fd, res.c_str(), res.length(), 0);
				if ((std::size_t)sent == res.size())
				{
					std::cout << "Response has been sent" << std::endl;
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					clients.erase(fd);
					close(fd);
				}
				if (sent == -1)
				{
					std::cout << "Sent Error: " << strerror(errno) << std::endl;
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					clients.erase(fd);
					close(fd);
				}
			}
		}
	}
}