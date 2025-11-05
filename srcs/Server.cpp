#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include "../includes/Utils.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <cerrno>
#include <utility>

Server::Server()
{
	client_max_body_size = 1894340421;
}

Server::~Server()
{
}

std::map<std::string, Session>&	Server::getSessions()
{
	return sessions;
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

void    Server::push_location(Location location, bool& inLocation)
{
    locations.push_back(location);
	inLocation = false;
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

void	Server::addCgiIn(CGIContext CGIctx, int epfd)
{
	int pipe_fd = CGIctx.pipe_to_cgi;
		
	if (CGIstdIn.find(pipe_fd) != CGIstdIn.end())
		throw_exception("addCgiIn: ", "CGI input pipe already registered");
	
	CGIstdIn[pipe_fd] = CGIctx;
	
	setNonBlocking(pipe_fd);
	
	struct epoll_event ev;
	ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
	ev.data.fd = pipe_fd;
	
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_fd, &ev) == -1)
		throw_exception("epoll_ctl: ", strerror(errno));
}


void	Server::addCgiOut(CGIContext CGIctx, int epfd)
{
	int pipe_fd = CGIctx.pipe_from_cgi;
		
	if (CGIstdOut.find(pipe_fd) != CGIstdOut.end())
		throw_exception("addCgiOut: ", "CGI output pipe already registered");
	
	CGIstdOut[pipe_fd] = CGIctx;
	
	setNonBlocking(pipe_fd);
	
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
	ev.data.fd = pipe_fd;
	
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_fd, &ev) == -1)
		throw_exception("epoll_ctl: ", strerror(errno));
}

bool Server::handleCGIEvent(int epfd, int fd, uint32_t event_flags, std::map<int, Server*>& servers_fd, std::map<int, Client*>& clients)
{
	for (std::map<int, Server*>::iterator sit = servers_fd.begin(); sit != servers_fd.end(); ++sit)
	{
		Server* server = sit->second;
		
		// Handle CGI stdin (writing to CGI)
		if (server->CGIstdIn.find(fd) != server->CGIstdIn.end())
		{
			if (event_flags & (EPOLLERR | EPOLLHUP))
			{
				epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
				close(fd);
				server->CGIstdIn.erase(fd);
			}
			else if (event_flags & EPOLLOUT)
			{
				Client* cgiClient = server->CGIstdIn[fd].client;
				const std::string& body = cgiClient->getRequest()->getBody();
				size_t written_so_far = server->CGIstdIn[fd].bytes_written;
				
				if (written_so_far < body.length())
				{
					// ONE write per epoll cycle
					ssize_t written = write(fd, body.c_str() + written_so_far, body.length() - written_so_far);
					
					if (written > 0)
					{
						server->CGIstdIn[fd].bytes_written += written;
						
						// Check if all data written
						if (server->CGIstdIn[fd].bytes_written >= body.length())
						{
							epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
							close(fd);
							server->CGIstdIn.erase(fd);
						}
					}
					else
					{
						// write failed or returned 0/negative - close pipe
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						close(fd);
						server->CGIstdIn.erase(fd);
					}
				}
				else
				{
					// All data already written
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					close(fd);
					server->CGIstdIn.erase(fd);
				}
			}
			
			return true;
		}
		
		// Handle CGI stdout (reading from CGI)
		if (server->CGIstdOut.find(fd) != server->CGIstdOut.end())
		{
			std::map<int, CGIContext>::iterator it = server->CGIstdOut.find(fd);
			
			// Check if client still exists
			if (clients.find(it->second.clientfd) == clients.end())
			{
				epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
				close(fd);
				server->CGIstdOut.erase(it);
				return true;
			}
			
			Client* cgiClient = clients[it->second.clientfd];

			if (event_flags & (EPOLLERR | EPOLLHUP))
			{
				// Pipe closed - check CGI process exit status
				int status;
				pid_t cgi_pid = cgiClient->getCGIHandler()->getPid();
				pid_t result = waitpid(cgi_pid, &status, WNOHANG);
				
				if (result > 0)
				{
					// Process exited - check status
					if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
					{
						// Non-zero exit status indicates error
						cgiClient->getCGIHandler()->setErrorCode(500);
					}
					else if (WIFSIGNALED(status))
					{
						// Process was killed by a signal
						cgiClient->getCGIHandler()->setErrorCode(500);
					}
				}
				
				cgiClient->getCGIHandler()->setComplete(true);
				epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
				close(fd);
				server->CGIstdOut.erase(it);
				
				// Switch client to EPOLLOUT mode
				struct epoll_event ev;
				ev.events = EPOLLOUT;
				ev.data.fd = cgiClient->getFD();
				epoll_ctl(epfd, EPOLL_CTL_MOD, cgiClient->getFD(), &ev);
			}
			else if (event_flags & EPOLLIN)
			{
				char buf[4096];
				ssize_t bytesRead = read(fd, buf, sizeof(buf));
				
				if (bytesRead > 0)
				{
					cgiClient->getCGIHandler()->appendResponse(buf, bytesRead);
				}
				else
				{
					// Check process exit status
					int status;
					pid_t cgi_pid = cgiClient->getCGIHandler()->getPid();
					pid_t result = waitpid(cgi_pid, &status, WNOHANG);
					
					if (result > 0)
					{
						if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
							cgiClient->getCGIHandler()->setErrorCode(500);
						else if (WIFSIGNALED(status))
							cgiClient->getCGIHandler()->setErrorCode(500);
					}
					
					cgiClient->getCGIHandler()->setComplete(true);
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					close(fd);
					server->CGIstdOut.erase(it);
					
					struct epoll_event ev;
					ev.events = EPOLLOUT;
					ev.data.fd = cgiClient->getFD();
					epoll_ctl(epfd, EPOLL_CTL_MOD, cgiClient->getFD(), &ev);
				}
			}

			return true;
		}
	}
	
	return false;
}
