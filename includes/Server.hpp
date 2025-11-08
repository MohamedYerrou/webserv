#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <stdint.h>
#include <stdint.h>
#include "Location.hpp"
#include "Session.hpp"

class Client;
typedef struct s_CGIContext 
{
	int childpid;
	int clientfd;
	std::string body;
	size_t bytes_written;
	std::string output_buffer;
	bool is_stdin_closed;
	bool is_stdout_closed;
	int pipe_to_cgi;
	int pipe_from_cgi;
	bool is_error;
	Client* client;
} CGIContext;

class Server
{
	private:
	std::vector<std::pair<std::string, int> >	listens;
	std::vector<Location>						locations;
	std::map<int , CGIContext> 					CGIstdIn;
	std::map<int , CGIContext> 					CGIstdOut;
	std::map<std::string, Session>				sessions;
	std::vector<int>							listen_fd;
	size_t										client_max_body_size;


		
	public:
	Server();
	~Server();
	void										push_listen(std::string);
	void										push_location(Location location, bool& inLocation);
	void										init_server(int epfd, std::map<int, Server*>& fd_vect);
	const std::vector<Location>&				getLocations() const;
	
	void										setMaxBodySize(std::string size);
	size_t										getMaxBodySize();
	void										addCgiIn(CGIContext CGIctx, int fd);
	void										addCgiOut(CGIContext CGIctx, int fd);
	static bool									handleCGIEvent(int epfd, int fd, uint32_t event_flags, std::map<int, Server*>& servers_fd, std::map<int, Client*>& clients);
	static void									handleCGIStdinEvent(int epfd, int fd, uint32_t event_flags, Server* server);
	static void									handleCGIStdoutEvent(int epfd, int fd, uint32_t event_flags, Server* server, std::map<int, Client*>& clients);
	static void									cleanupCGIPipe(int epfd, int fd, Server* server, bool is_stdin);
	static void									cleanupTimedOutCGI(int epfd, Client* client);
	std::map<std::string, Session>&				getSessions();
	void										session_expiration();
};

#endif