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
#include <csignal>
#include "Server.hpp"
#include "Client.hpp"
#include "Utils.hpp"
#include "HtmlMacros.hpp"

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

void	handleClientRequest(int epfd, int fd, std::map<int, Client*>& clients, std::map<int, Server*>& servers_fd)
{
	char	buf[3000];
	Client* clientPtr = clients[fd];
	(void)servers_fd; // Used for CGI context

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
		if (clientPtr->getReqComplete())
		{
			if (!clientPtr->getRequestError())
				clientPtr->handleCompleteRequest();
			clientPtr->setLastActivityTime(time(NULL));
			clientPtr->setIsRequesting(false);

			if (clientPtr->getCGIHandler() && clientPtr->getCGIHandler()->isStarted()
				&& !clientPtr->getCGIHandler()->isComplete())
			{
				Server* server = clientPtr->getServer();
				
				CGIContext ctx;
				ctx.childpid = clientPtr->getCGIHandler()->getPid();
				ctx.clientfd = fd;
				ctx.body = clientPtr->getRequest()->getBody();
				ctx.bytes_written = 0;
				ctx.output_buffer = "";
				ctx.is_stdin_closed = false;
				ctx.is_stdout_closed = false;
				ctx.pipe_to_cgi = clientPtr->getCGIHandler()->getStdinFd();
				ctx.pipe_from_cgi = clientPtr->getCGIHandler()->getStdoutFd();
				ctx.is_error = false;
				ctx.client = clientPtr;
				
				server->addCgiIn(ctx, epfd);
				server->addCgiOut(ctx, epfd);
			}
			else
			{
				struct epoll_event ev;
				ev.events = EPOLLOUT;
				ev.data.fd = fd;
				if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
					throw_exception("epoll_ctl: ", strerror(errno));
			}
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
		// std::cout << "Building res: " << res << std::endl;
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

void	handleClientCGIResponse(int epfd, int fd, std::map<int, Client*>& clients)
{
	Client* client = clients[fd];
	
	if (!client->getCGIHandler())
	{
		handleClientResponse(epfd, fd, clients);
		return;
	}
	
	CGIHandler* cgi = client->getCGIHandler();
	if (!cgi->isComplete())
		return;
	
	std::string response;
	if (cgi->hasError())
	{
		std::cerr << "hello hello " << std::endl;
		int error_code = cgi->getErrorCode();
		if (error_code == 0)
			error_code = 500;
		
		std::string error_body = CGI_ERROR_BODY(error_code);
		if (error_code == 500)
			response = HTTP_ERROR_RESPONSE("500 Internal Server Error", error_body);
		else if (error_code == 502)
			response = HTTP_ERROR_RESPONSE("502 Bad Gateway", error_body);
		else if (error_code == 504)
			response = HTTP_ERROR_RESPONSE("504 Gateway Timeout", error_body);
		else
			response = HTTP_ERROR_RESPONSE("500 Internal Server Error", error_body);
	}
	else
	{
		std::string body = cgi->getBuffer();
		
		if (body.size() == 1 && body[0] == 'E')
		{
			std::string error_body = HTML_ERROR_502;
			response = HTTP_ERROR_RESPONSE("502 Bad Gateway", error_body);
		}
		else if (body.find("HTTP/") == 0)
			response = body;
		else if (body.find("Status:") == 0 || body.find("Content-Type:") == 0)
		{
			size_t separator_pos = body.find("\r\n\r\n");
			if (separator_pos == std::string::npos)
				separator_pos = body.find("\n\n");
			
			if (separator_pos == std::string::npos)
			{
				std::string error_body = HTML_ERROR_502;
				response = HTTP_ERROR_RESPONSE("502 Bad Gateway", error_body);
			}
			else
			{
				std::cout << "body is ==> : " << body << std::endl; 
				response = "HTTP/1.1 200 OK\r\n" + body;
			}
		}
		else
		{
			std::string error_body = HTML_ERROR_502; 
			response = HTTP_ERROR_RESPONSE("502 Bad Gateway", error_body);
		}
	}
	
	ssize_t sent = send(fd, response.c_str(), response.length(), 0);
	if (sent == -1)
		std::cout << "CGI Sent Error: " << strerror(errno) << std::endl;
	client->cleanupCGI();
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	delete client;
	clients.erase(fd);
	close(fd);
}

void	handleTimeOut(int epfd, std::map<int, Client*>& clients)
{
	std::map<int, Client*>::iterator it;
	for (it = clients.begin(); it != clients.end(); )
	{
		Client* client = it->second;
		if (client && (client->isTimedOut() || (client->getIsCGI() && client->isCgiTimedOut())))
		{
			if (client->getIsCGI() && client->isCgiTimedOut() && client->getCGIHandler())
			{
				client->getCGIHandler()->setErrorCode(504);
				client->getCGIHandler()->setComplete(true);
				struct epoll_event ev;
				ev.events = EPOLLOUT;
				ev.data.fd = it->first;
				if (epoll_ctl(epfd, EPOLL_CTL_MOD, it->first, &ev) == -1)
				{
					epoll_ctl(epfd, EPOLL_CTL_DEL, it->first, NULL);
					close(it->first);
					delete client;
					clients.erase(it++);
				}
				else
					++it;
			}
			else
			{
				std::string error_body = HTML_ERROR_408;
				std::string str = HTTP_ERROR_RESPONSE("408 Request Timeout", error_body);
				
				epoll_ctl(epfd, EPOLL_CTL_DEL, it->first, NULL);
				close(it->first);
				delete client;
				clients.erase(it++);
			}
		}
		else
			it++;
	}
}

void run_server(int epfd, std::map<int, Server*>& servers_fd)
{
    struct epoll_event events[64];
    std::map<int, Client*> clients;

    while (true)
    {
        int nfds = epoll_wait(epfd, events, 64, 1000);
        if (nfds == -1)
            throw_exception("epoll_wait: ", strerror(errno));
		handleTimeOut(epfd, clients);
        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;
            uint32_t event_flags = events[i].events;

            if (listening_fd(servers_fd, fd))
            {
				handleListeningClient(epfd, fd, clients, servers_fd);
				continue;
			}
			if (Server::handleCGIEvent(epfd, fd, event_flags, servers_fd, clients))
				continue;
			if (clients.find(fd) == clients.end())
			{
				epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
				close(fd);
				continue;
			}
			
			Client* client = clients[fd];
            if (event_flags & EPOLLIN)
				handleClientRequest(epfd, fd, clients, servers_fd);
            else if (event_flags & EPOLLOUT)
            {
                if (client->getIsCGI())
                    handleClientCGIResponse(epfd, fd, clients);
                else
                    handleClientResponse(epfd, fd, clients);
            }
        }
    }
}
