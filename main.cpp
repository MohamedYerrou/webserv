#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <utility> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "ConfigStructs.hpp"

void	tokenizer(std::vector<std::string>&	tokens)
{
	std::fstream				file("file.txt");
	std::string					line;
	std::string					word;
	std::stringstream			ss;

	while (std::getline(file, line))
	{
		std::stringstream	ss(line);
		if (!line.empty() && line[0] == '#')
			continue;
		while (ss >> word)
		{
			if (!line.empty() && word[0] == '#')
				break;;
			if (!word.empty() && word[word.size() - 1] == ';')
			{
				word.erase(word.size() - 1);
				tokens.push_back(word);
				tokens.push_back(";");
			}
			else if (!word.empty() && word[word.size() - 1] != ';')
				tokens.push_back(word);
		}
	}
}

std::vector<ServerConfig> parser(std::vector<std::string>& tokens)
{
	std::vector<ServerConfig>	config;
	LocationConfig				currLocation;
	ServerConfig				currServer;
	std::string					tok;
	bool						inLocation = false;
	bool						inServer = false;


	for (size_t i = 0; i < tokens.size(); i++)
	{
		tok = tokens[i];
		if (tok == "server")
		{
			currServer = ServerConfig();
			inServer = true;
			i++;
		}
		else if (tok == "listen" && inServer)
		{
			int				port;
			std::string		ip;
			std::pair<std::string, int> p;


			tok = tokens[++i];
			ip = tok.substr(0, tok.find(':'));
			port = atoi(tok.substr(tok.find(':') + 1).c_str());
			p = make_pair(ip, port);
			currServer.listens.push_back(p);
			i++;
		}
		else if (tok == "root" && inServer && !inLocation)
			currServer.root = tokens[++i];
		else if (tok == "index" && inServer && !inLocation)
			currServer.index = tokens[++i];
		else if (tok == "error_page" && inServer && !inLocation)
		{
			int			code;
			std::string	path;

			code = atoi(tokens[++i].c_str());
			path = tokens[++i];
			currServer.errors.insert(make_pair(code, path));
		}
		else if (tok == "location" && inServer && !inLocation)
		{
			inLocation = true;
			currLocation = LocationConfig();
			currLocation.path = tokens[++i];
		}
		else if (tok == "}" && inLocation)
		{
			currServer.locations.push_back(currLocation);
			inLocation = false;
		}
		else if (tok == "root" && inLocation)
			currLocation.root = tokens[++i];
		else if (tok == "index" && inLocation)
			currLocation.index = tokens[++i];
		else if (tok == "upload_store" && inLocation)
			currLocation.upload_store = tokens[++i];
		else if (tok == "methods" && inLocation)
		{
			i++;
			while (tokens[i] == "GET" || tokens[i] == "POST" || tokens[i] == "DELETE")
				currLocation.methods.push_back(tokens[i++]);
		}
		else if (tok == "cgi" && inLocation)
		{
			std::string	ext = tokens[++i];
			std::string path = tokens[++i];
			currLocation.cgi.insert(make_pair(ext, path));
		}
		else if (tok == "return" && inLocation)
		{
			std::string	path;
			int			status;
			
			status = atoi(tokens[++i].c_str());
			path = tokens[++i];
			currLocation.http_redirection = make_pair(status, path);
		}
		else if (tok == "}" && inServer && !inLocation)
		{
			config.push_back(currServer);
			inServer = false;
		}
	}

	return config;
}

bool listening_fd(std::vector<int>& vect, int fd)
{
	for (size_t i = 0; i < vect.size(); i++)
	{
		if (fd == vect[i])
			return true;
	}
	return false;
}

void	throw_exception(std::string function, std::string err)
{
	throw MyException(function + err);
}

void	init_server(std::vector<ServerConfig>&	servers, int epfd, std::vector<int>& fd_vect)
{
	for (size_t i = 0; i < servers.size(); i++)
	{
		for (size_t j = 0; j < servers[i].listens.size(); j++)
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
			addr.sin_port = htons(servers[i].listens[j].second);
			addr.sin_addr.s_addr = inet_addr(servers[i].listens[j].first.c_str());
			if (bind(listen_fd, (const sockaddr*)&addr, sizeof(addr)) == -1)
				throw_exception("bind: ", strerror(errno));
			if (listen(listen_fd, SOMAXCONN) == -1)
				throw_exception("listen: ", strerror(errno));
			std::cout << "Listening on: " << servers[i].listens[j].first << ":" <<
			servers[i].listens[j].second << std::endl;
			struct epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.fd = listen_fd;
			if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
				throw_exception("epoll_ctl: ", strerror(errno));
		}
	}
}

void	run_server(int epfd, std::vector<int>& fd_vect)
{
	struct epoll_event events[64];
	char	buf[1024];

	while (true)
	{
		int nfds = epoll_wait(epfd, events, 64, -1);
		if (nfds == -1)
			throw_exception("epoll_wait: ", strerror(errno));

		for (int i = 0; i < nfds; i++)
		{
			int fd = events[i].data.fd;
			if (listening_fd(fd_vect, fd))
			{
				int client_fd = accept(fd, NULL, NULL);
				if (client_fd == -1)
					throw_exception("accept: ", strerror(errno));
				fcntl(client_fd, F_SETFL, O_NONBLOCK);
				struct epoll_event ev;
				ev.events = EPOLLIN;
				ev.data.fd = client_fd;
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
					throw_exception("epoll_ctl: ", strerror(errno));
			}
			else if (events[i].events & EPOLLIN)
			{
				int n = recv(fd, buf, sizeof(buf) - 1, 0);
				if (n == -1)
				{
					perror("recv: ");
					close(fd);
					if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
						throw_exception("epoll_ctl: ", strerror(errno));
				}
				else
				{
					buf[n] = '\0';
					std::cout << buf << std::endl;
					std::istringstream request(buf);
					std::string method, path, version;
					request >> method >> path >> version;

					// if (method == "GET")
					// {
					// 	// GET FUNCTION
					// }
					// else if (method == "POST")
					// {
					// 	// POST FUNCTION
					// }
					// else if (method == "DELETE")
					// {
					// 	// DELETE FUNCTION
					// }
					// else
					// {
						std::string response =
						"HTTP/1.1 405 Method Not Allowed\r\n"
                		"Content-Length: 0\r\n"
                		"Connection: close\r\n\r\n";
						send(fd, response.c_str(), response.size(), 0);
						close(fd);
					// }
				}
			}
		}
	}
}



int main()
{
	std::vector<std::string>	tokens;
	std::vector<ServerConfig>	servers;
	std::vector<int> fd_vect;

	try
	{
		tokenizer(tokens);
		servers = parser(tokens);
		int epfd = epoll_create(1);
		init_server(servers, epfd, fd_vect);
		run_server(epfd, fd_vect);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		std::cout << "hhhh" << std::endl;
		exit(1);
	}
	
	// for (size_t i = 0; i < config.size(); i++)
	// {
	// 	config[i].print();
	// }
    return 0;
}
