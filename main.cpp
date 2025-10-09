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

// Maps CGI pipe fd -> client fd
std::map<int, int> cgi_clients;

// Store CGI output buffers
std::map<int, std::string> cgi_buffers;

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
    int x, y;
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
			x = bind(listen_fd, (const sockaddr*)&addr, sizeof(addr));
            if (x == -1)
				throw_exception("bind: ", strerror(errno));
            std::cout << "Bind return is ==> " << x << std::endl;
			y = listen(listen_fd, SOMAXCONN);
            if (y == -1)
				throw_exception("listen: ", strerror(errno));
            std::cout << "Listen return is ==> " << y << std::endl;
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

void cgi_function(int epfd, int client_fd, const std::string &script, const std::string &input)
{
    int in_pipe[2], out_pipe[2];
    if (pipe(in_pipe) == -1 || pipe(out_pipe) == -1)
        throw std::runtime_error("pipe failed");

    pid_t pid = fork();
    if (pid == -1)
        throw std::runtime_error("fork failed");
	else if (pid == 0)
	{
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        close(in_pipe[1]);
        close(out_pipe[0]);

        char *args[] = { (char *)script.c_str(), NULL };
        char *envp[] = { NULL };
        execve(script.c_str(), args, envp);

        perror("execve");
        exit(1);
	}
	else
	{
        close(in_pipe[0]);
        close(out_pipe[1]);

        // Write POST body if needed
        if (!input.empty())
            write(in_pipe[1], input.c_str(), input.size());
        close(in_pipe[1]);
		
		// Register out_pipe[0] in epoll
        fcntl(out_pipe[0], F_SETFL, O_NONBLOCK);
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = out_pipe[0];
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, out_pipe[0], &ev) == -1)
            throw std::runtime_error("epoll_ctl add cgi failed");

        // Track mapping
        cgi_clients[out_pipe[0]] = client_fd;
        cgi_buffers[out_pipe[0]] = "";
	}
}


// Track partial HTTP requests per client
std::map<int, std::string> request_buffers;

void run_server(int epfd, std::vector<int>& fd_vect)
{
    struct epoll_event events[64];
    char buf[4096];

    // Track partial request bodies for POST
    while (true)
    {
        int nfds = epoll_wait(epfd, events, 64, -1);
        if (nfds == -1)
            throw_exception("epoll_wait: ", strerror(errno));

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;

            // -------------------- [ACCEPT NEW CLIENT] --------------------
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

                continue;
            }

            // -------------------- [HANDLE CGI OUTPUT] --------------------
            if (cgi_clients.find(fd) != cgi_clients.end())
            {
                char buffer[1024];
                int n = read(fd, buffer, sizeof(buffer));
                if (n > 0)
                {
                    cgi_buffers[fd].append(buffer, n);
                }
                else if (n == 0)
                {
                    // CGI finished
                    int client_fd = cgi_clients[fd];
                    std::string body = cgi_buffers[fd];
                    std::ostringstream oss;
                    oss << body.size();
                    std::string response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Length: " + oss.str() + "\r\n"
                        "Content-Type: text/html\r\n\r\n" +
                        body;

                    send(client_fd, response.c_str(), response.size(), 0);
                    close(client_fd);
                    close(fd);

                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);

                    cgi_clients.erase(fd);
                    cgi_buffers.erase(fd);
                }
                else
                {
                    perror("read cgi");
                    close(fd);
                }
                continue;
            }

            // -------------------- [READ CLIENT REQUEST] --------------------
            if (events[i].events & EPOLLIN)
            {
                int n = recv(fd, buf, sizeof(buf) - 1, 0);
                if (n <= 0)
                {
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    continue;
                }

                buf[n] = '\0';
                request_buffers[fd] += buf;

                // Detect end of HTTP headers
                if (request_buffers[fd].find("\r\n\r\n") == std::string::npos)
                    continue; // not full yet

                std::istringstream req(request_buffers[fd]);
                std::string method, path, version;
                req >> method >> path >> version;
                if (method == "GET")
                {
                    if (path == "/") path = "/index.html";
                    std::string full_path = "www" + path;

                    std::ifstream file(full_path.c_str());
                    if (!file.is_open())
                    {
                        std::string response =
                            "HTTP/1.1 404 Not Found\r\n"
                            "Content-Length: 0\r\n"
                            "Connection: close\r\n\r\n";
                        send(fd, response.c_str(), response.size(), 0);
                        request_buffers.erase(fd);
                        close(fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                        continue;
                    }

                    std::stringstream ss;
                    ss << file.rdbuf();
                    std::string body = ss.str();
                    std::ostringstream len;
                    len << body.size();

                    std::string response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Length: " + len.str() + "\r\n"
                        "Content-Type: text/html\r\n"
                        "Connection: close\r\n\r\n" +
                        body;

                    send(fd, response.c_str(), response.size(), 0);
                    request_buffers.erase(fd);
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                }
                else if (method == "POST")
                {
                    // For simplicity: find Content-Length and body
                    size_t pos = request_buffers[fd].find("Content-Length:");
                    size_t len = 0;
                    if (pos != std::string::npos)
                    {
                        std::istringstream line(request_buffers[fd].substr(pos + 15));
                        line >> len;
                    }

                    size_t body_pos = request_buffers[fd].find("\r\n\r\n");
                    std::string body = request_buffers[fd].substr(body_pos + 4);

                    if (body.size() < len)
                        continue; // body incomplete

                    // Run CGI for .py file (example)
                    cgi_function(epfd, fd, "./cgi-bin/script.py", body);
                    request_buffers.erase(fd);
                }
                else if (method == "DELETE")
                {
                    std::string full_path = "www" + path;
                    if (remove(full_path.c_str()) == 0)
                    {
                        std::string response =
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Length: 0\r\n"
                            "Connection: close\r\n\r\n";
                        send(fd, response.c_str(), response.size(), 0);
                    }
                    else
                    {
                        std::string response =
                            "HTTP/1.1 404 Not Found\r\n"
                            "Content-Length: 0\r\n"
                            "Connection: close\r\n\r\n";
                        send(fd, response.c_str(), response.size(), 0);
                        request_buffers.erase(fd);
                    }
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                }
                else
                {
                    std::string response =
                        "HTTP/1.1 405 Method Not Allowed\r\n"
                        "Content-Length: 0\r\n"
                        "Connection: close\r\n\r\n";
                    send(fd, response.c_str(), response.size(), 0);
                    request_buffers.erase(fd);
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                }
            }

            // 4) Writable sockets (static responses, etc.)
            else if (events[i].events & EPOLLOUT)
            {
                std::string response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Length: 13\r\n"
                    "Connection: close\r\n\r\n"
                    "Hello, world!";

                send(fd, response.c_str(), response.size(), 0);
                close(fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
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
