#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include "Location.hpp"


class Server
{
    private:
    std::vector<std::pair<std::string, int> >	listens;
	std::vector<Location>				    	locations;
    size_t										client_max_body_size;
    
    public:
    Server();
    ~Server();
    void    push_listen(std::pair<std::string, int>);
    void    push_location(Location);
    void    init_server(int epfd, std::vector<int>& fd_vect);
};

#endif