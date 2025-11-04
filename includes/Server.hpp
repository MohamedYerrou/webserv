#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include "Location.hpp"
#include "session.hpp"


class Server
{
	private:
	std::vector<std::pair<std::string, int> >	listens;
	std::vector<Location>						locations;
	size_t										client_max_body_size;
	std::vector<int>							listen_fd;
	std::map<std::string, session>				sessions;
		
	public:
	Server();
	~Server();
	void							push_listen(std::string);
	void							push_location(Location location, bool& inLocation);
	void							init_server(int epfd, std::map<int, Server*>& fd_vect);
	const std::vector<Location>&	getLocations() const;
	void							setMaxBodySize(std::string size);
	size_t							getMaxBodySize();
	std::map<std::string, session>&	getSessions();

};

#endif