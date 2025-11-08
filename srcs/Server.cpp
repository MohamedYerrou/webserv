#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include "../includes/Client.hpp"
#include "../includes/Utils.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
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

		server_fd.insert(std::make_pair(listen_fd, this));

		sockaddr_in addr;

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(listens[i].second);
		addr.sin_addr.s_addr = ip_toBinary(listens[i].first);

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
		return;
	
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
		return;
	
	CGIstdOut[pipe_fd] = CGIctx;
	
	setNonBlocking(pipe_fd);
	
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
	ev.data.fd = pipe_fd;
	
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_fd, &ev) == -1)
		throw_exception("epoll_ctl: ", strerror(errno));
}

void Server::cleanupCGIPipe(int epfd, int fd, Server* server, bool is_stdin)
{
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
	if (is_stdin)
		server->CGIstdIn.erase(fd);
	else
		server->CGIstdOut.erase(fd);
}

void Server::handleCGIStdinEvent(int epfd, int fd, uint32_t event_flags, Server* server)
{
	if (event_flags & (EPOLLERR | EPOLLHUP))
	{
		cleanupCGIPipe(epfd, fd, server, true);
		return;
	}
	
	if (event_flags & EPOLLOUT)
	{
		Client* cgiClient = server->CGIstdIn[fd].client;
		const std::string& body = cgiClient->getRequest()->getBody();
		size_t written_so_far = server->CGIstdIn[fd].bytes_written;
		
		if (written_so_far < body.length())
		{
			ssize_t written = write(fd, body.c_str() + written_so_far, body.length() - written_so_far);
			
			if (written > 0)
			{
				server->CGIstdIn[fd].bytes_written += written;
				
				if (server->CGIstdIn[fd].bytes_written >= body.length())
					cleanupCGIPipe(epfd, fd, server, true);
			}
			else if (written == 0)
				cleanupCGIPipe(epfd, fd, server, true);
			else
				cleanupCGIPipe(epfd, fd, server, true);
		}
		else
			cleanupCGIPipe(epfd, fd, server, true);
	}
}

void Server::handleCGIStdoutEvent(int epfd, int fd, uint32_t event_flags, Server* server, std::map<int, Client*>& clients)
{
	std::map<int, CGIContext>::iterator it = server->CGIstdOut.find(fd);
	
	if (clients.find(it->second.clientfd) == clients.end())
	{
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		server->CGIstdOut.erase(it);
		return;
	}
	
	Client* cgiClient = clients[it->second.clientfd];

	if (event_flags & (EPOLLERR | EPOLLHUP))
	{
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
	else if (event_flags & EPOLLIN)
	{
		char buf[4096];
		ssize_t bytesRead = read(fd, buf, sizeof(buf));
		
		if (bytesRead > 0)
			cgiClient->getCGIHandler()->appendResponse(buf, bytesRead);
		else if (bytesRead == 0)
		{
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
		else
		{
			cgiClient->getCGIHandler()->setErrorCode(502);
			cgiClient->getCGIHandler()->setError(true);
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
}

bool Server::handleCGIEvent(int epfd, int fd, uint32_t event_flags, std::map<int, Server*>& servers_fd, std::map<int, Client*>& clients)
{
	for (std::map<int, Server*>::iterator sit = servers_fd.begin(); sit != servers_fd.end(); ++sit)
	{
		Server* server = sit->second;
		
		if (server->CGIstdIn.find(fd) != server->CGIstdIn.end())
		{
			handleCGIStdinEvent(epfd, fd, event_flags, server);
			return true;
		}
		
		if (server->CGIstdOut.find(fd) != server->CGIstdOut.end())
		{
			handleCGIStdoutEvent(epfd, fd, event_flags, server, clients);
			return true;
		}
	}
	return false;
}

void	Server::session_expiration()
{
	std::map<std::string, Session>::iterator it;

	for (it = sessions.begin(); it != sessions.end(); )
	{
		if (time(NULL) - it->second.last_access > 4)
		{
			sessions.erase(it++);
		}
		else
			it++;
	}
	
}