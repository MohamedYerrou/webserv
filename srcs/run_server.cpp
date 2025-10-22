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
#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include "../includes/Utils.hpp"

void	handleListeningClient(int epfd, int fd, std::map<int, Client*>& clients, std::map<int, Server*>& servers_fd)
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

void	handleClientResponse(int epfd, int fd, std::map<int, Client*>& clients)
{
	Client* client = clients[fd];
	if (!client->getSentAll())
	{
		client->handleFile();
		Response& currentResponse = client->getResponse();
		std::string res = currentResponse.build();
		ssize_t sent = send(fd, res.c_str(), res.length(), 0);
		if (sent == -1)
		{
			std::cout << "Sent Error: " << strerror(errno) << std::endl;
			epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
			delete client;
			clients.erase(fd);
			close(fd);
		}
	}
	else
	{
		std::cout << "Response has been sent" << std::endl;
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
		delete client;
		clients.erase(fd);
		close(fd);
	}
}

void run_server(int epfd, std::map<int, Server*>& servers_fd)
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

            // New connection
            if (listening_fd(servers_fd, fd))
            {
                handleListeningClient(epfd, fd, clients, servers_fd);
                continue;
            }

            Client* client = clients[fd];

            // Readable socket
            if (events[i].events & EPOLLIN)
            {
                char buf[4096];
                ssize_t received = recv(fd, buf, sizeof(buf) - 1, 0);

                if (received <= 0)
                {
                    // Connection closed or error
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    delete client;
                    clients.erase(fd);
                    close(fd);
                    continue;
                }

                buf[received] = '\0';
                client->appendData(buf, received);

                if (client->getReqComplete())
                {
                    if (!client->getRequestError())
                        client->handleCompleteRequest();

                    // Switch to EPOLLOUT to send response or handle CGI
                    struct epoll_event ev;
                    ev.events = EPOLLOUT;
                    ev.data.fd = fd;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
                }
            }

            // Writable socket
            if (events[i].events & EPOLLOUT)
            {
                std::cout << "*********11111111111**********" << std::endl;
                if (client->getIsCGI())
                {
                    std::cout << "*********2222222222**********" << std::endl;
                    client->handleCGI();
                    if (client->getSentAll())
                    {
                        std::cout << "*********33333333333**********" << std::endl;
						client->cleanupCGI();
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                        delete client;
                        clients.erase(fd);
                        close(fd);
                    }
                }
                else
                {
                    std::cout << "*********55555555555**********" << std::endl;
                    handleClientResponse(epfd, fd, clients);
                }
            }
        }
    }
}
