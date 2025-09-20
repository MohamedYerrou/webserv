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
#include "ConfigStructs.hpp"
#include "includes/server.hpp"


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
			port = std::stoi(tok.substr(tok.find(':') + 1));
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

			code = std::stoi(tokens[++i]);
			path = tokens[++i];
			currServer.errors.insert({code, path});
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
			currLocation.cgi.insert({tokens[++i], tokens[++i]});
		else if (tok == "return" && inLocation)
		{
			std::string	path;
			int			status;
			
			status = std::stoi(tokens[++i]);
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

// void	start_server(std::vector<ServerConfig>& servers)
// {
// 	for (size_t i = 0; i < servers.size(); i++)
// 	{
// 		for (size_t j = 0; j < servers[i].listens.size(); j++)
// 		{
// 			int fd = socket(AF_INET, SOCK_STREAM, 0);
// 			sockaddr_in addr;
// 			addr.sin_family = AF_INET;
// 			addr.sin_port = htons(servers[i].listens[j].second);
// 			addr.sin_addr.s_addr = inet_addr(servers[i].listens[j].first.c_str());
// 			bind(fd, (const sockaddr*)&addr, sizeof(addr));
// 			listen(fd, SOMAXCONN);
			
// 		}
		
// 	}
	
// }

bool listening_fd(std::vector<int>& vect, int fd)
{
	for (size_t i = 0; i < vect.size(); i++)
	{
		if (fd == vect[i])
			return true;
	}
	return false;
}

// int main()
// {
//     std::fstream				file("file.txt");
//     std::string					line;
// 	std::string					word;
// 	std::stringstream			ss;
// 	std::vector<std::string>	tokens;
// 	std::vector<ServerConfig>	servers;
// 	int	i = {};

//     while (std::getline(file, line))
// 	{
// 		std::stringstream	ss(line);
// 		if (line.front() == '#')
// 			continue;
// 		while (ss >> word)
// 		{
// 			if (word.front() == '#')
// 				break;;
// 			if (word.back() == ';')
// 			{
// 				word.pop_back();
// 				tokens.push_back(word);
// 				tokens.push_back(";");
// 			}
// 			else if (word.back() != ';')
// 				tokens.push_back(word);
// 		}
// 	}
	
// 	servers = parser(tokens);
// 	int epfd = epoll_create(0);
// 	std::vector<int> fd_vect;
// 	for (size_t i = 0; i < servers.size(); i++)
// 	{
// 		for (size_t j = 0; j < servers[i].listens.size(); j++)
// 		{
// 			int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
// 			fd_vect.push_back(listen_fd);
// 			sockaddr_in addr;
// 			addr.sin_family = AF_INET;
// 			addr.sin_port = htons(servers[i].listens[j].second);
// 			addr.sin_addr.s_addr = inet_addr(servers[i].listens[j].first.c_str());
// 			bind(listen_fd, (const sockaddr*)&addr, sizeof(addr));
// 			listen(listen_fd, SOMAXCONN);
// 			struct epoll_event ev;
// 			ev.events = EPOLLIN;
// 			ev.data.fd = listen_fd;
// 			epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);
// 		}
		
// 	}

// 	struct epoll_event events[64];
// 	while (true)
// 	{
// 		int nfds = epoll_wait(epfd, events, 64, -1);
// 		for (int i = 0; i < nfds; i++)
// 		{
// 			int fd = events[i].data.fd;
// 			if (listening_fd(fd_vect, fd))
// 			{
// 				int client_fd = accept(fd, NULL, NULL);
// 				fcntl(client_fd, F_SETFL, O_NONBLOCK);
// 				struct epoll_event ev;
// 				ev.events = EPOLLIN;
// 				ev.data.fd = client_fd;
// 				epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
// 			}
// 			else if (events[i].events & EPOLLIN)
// 			{

// 			}
// 		}
// 	}
// 	// for (size_t i = 0; i < config.size(); i++)
// 	// {
// 	// 	config[i].print();
// 	// }
	
//     return 0;
// }

int main() {
    // Load config file
    std::fstream file("file.txt");
    std::string line, word;
    std::vector<std::string> tokens;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        while (ss >> word) {
            if (word[0] == '#') break;
            if (word.back() == ';') {
                word.pop_back();
                tokens.push_back(word);
                tokens.push_back(";");
            } else {
                tokens.push_back(word);
            }
        }
    }

    std::vector<ServerConfig> servers = parser(tokens);

    if (servers.empty()) {
        std::cerr << "No servers defined in config\n";
        return 1;
    }

    // Just run the first server for now
    Server server(servers[0]);
    server.init();
    server.run();

    return 0;
}
